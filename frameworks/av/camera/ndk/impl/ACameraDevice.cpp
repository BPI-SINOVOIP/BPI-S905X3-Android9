/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ACameraDevice"

#include <vector>
#include <inttypes.h>
#include <android/hardware/ICameraService.h>
#include <camera2/SubmitInfo.h>
#include <gui/Surface.h>
#include "ACameraDevice.h"
#include "ACameraMetadata.h"
#include "ACaptureRequest.h"
#include "ACameraCaptureSession.h"

using namespace android;

namespace android {
// Static member definitions
const char* CameraDevice::kContextKey        = "Context";
const char* CameraDevice::kDeviceKey         = "Device";
const char* CameraDevice::kErrorCodeKey      = "ErrorCode";
const char* CameraDevice::kCallbackFpKey     = "Callback";
const char* CameraDevice::kSessionSpKey      = "SessionSp";
const char* CameraDevice::kCaptureRequestKey = "CaptureRequest";
const char* CameraDevice::kTimeStampKey      = "TimeStamp";
const char* CameraDevice::kCaptureResultKey  = "CaptureResult";
const char* CameraDevice::kCaptureFailureKey = "CaptureFailure";
const char* CameraDevice::kSequenceIdKey     = "SequenceId";
const char* CameraDevice::kFrameNumberKey    = "FrameNumber";
const char* CameraDevice::kAnwKey            = "Anw";

/**
 * CameraDevice Implementation
 */
CameraDevice::CameraDevice(
        const char* id,
        ACameraDevice_StateCallbacks* cb,
        std::unique_ptr<ACameraMetadata> chars,
        ACameraDevice* wrapper) :
        mCameraId(id),
        mAppCallbacks(*cb),
        mChars(std::move(chars)),
        mServiceCallback(new ServiceCallback(this)),
        mWrapper(wrapper),
        mInError(false),
        mError(ACAMERA_OK),
        mIdle(true),
        mCurrentSession(nullptr) {
    mClosing = false;
    // Setup looper thread to perfrom device callbacks to app
    mCbLooper = new ALooper;
    mCbLooper->setName("C2N-dev-looper");
    status_t err = mCbLooper->start(
            /*runOnCallingThread*/false,
            /*canCallJava*/       true,
            PRIORITY_DEFAULT);
    if (err != OK) {
        ALOGE("%s: Unable to start camera device callback looper: %s (%d)",
                __FUNCTION__, strerror(-err), err);
        setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_DEVICE);
    }
    mHandler = new CallbackHandler();
    mCbLooper->registerHandler(mHandler);

    const CameraMetadata& metadata = mChars->getInternalData();
    camera_metadata_ro_entry entry = metadata.find(ANDROID_REQUEST_PARTIAL_RESULT_COUNT);
    if (entry.count != 1) {
        ALOGW("%s: bad count %zu for partial result count", __FUNCTION__, entry.count);
        mPartialResultCount = 1;
    } else {
        mPartialResultCount = entry.data.i32[0];
    }

    entry = metadata.find(ANDROID_LENS_INFO_SHADING_MAP_SIZE);
    if (entry.count != 2) {
        ALOGW("%s: bad count %zu for shading map size", __FUNCTION__, entry.count);
        mShadingMapSize[0] = 0;
        mShadingMapSize[1] = 0;
    } else {
        mShadingMapSize[0] = entry.data.i32[0];
        mShadingMapSize[1] = entry.data.i32[1];
    }
}

// Device close implementaiton
CameraDevice::~CameraDevice() {
    sp<ACameraCaptureSession> session = mCurrentSession.promote();
    {
        Mutex::Autolock _l(mDeviceLock);
        if (!isClosed()) {
            disconnectLocked(session);
        }
        mCurrentSession = nullptr;
        if (mCbLooper != nullptr) {
            mCbLooper->unregisterHandler(mHandler->id());
            mCbLooper->stop();
        }
    }
    mCbLooper.clear();
    mHandler.clear();
}

void
CameraDevice::postSessionMsgAndCleanup(sp<AMessage>& msg) {
    msg->post();
    msg.clear();
    sp<AMessage> cleanupMsg = new AMessage(kWhatCleanUpSessions, mHandler);
    cleanupMsg->post();
}

// TODO: cached created request?
camera_status_t
CameraDevice::createCaptureRequest(
        ACameraDevice_request_template templateId,
        ACaptureRequest** request) const {
    Mutex::Autolock _l(mDeviceLock);
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        return ret;
    }
    if (mRemote == nullptr) {
        return ACAMERA_ERROR_CAMERA_DISCONNECTED;
    }
    CameraMetadata rawRequest;
    binder::Status remoteRet = mRemote->createDefaultRequest(templateId, &rawRequest);
    if (remoteRet.serviceSpecificErrorCode() ==
            hardware::ICameraService::ERROR_ILLEGAL_ARGUMENT) {
        ALOGW("Create capture request failed! template %d is not supported on this device",
            templateId);
        return ACAMERA_ERROR_INVALID_PARAMETER;
    } else if (!remoteRet.isOk()) {
        ALOGE("Create capture request failed: %s", remoteRet.toString8().string());
        return ACAMERA_ERROR_UNKNOWN;
    }
    ACaptureRequest* outReq = new ACaptureRequest();
    outReq->settings = new ACameraMetadata(rawRequest.release(), ACameraMetadata::ACM_REQUEST);
    outReq->targets  = new ACameraOutputTargets();
    *request = outReq;
    return ACAMERA_OK;
}

camera_status_t
CameraDevice::createCaptureSession(
        const ACaptureSessionOutputContainer*       outputs,
        const ACaptureRequest* sessionParameters,
        const ACameraCaptureSession_stateCallbacks* callbacks,
        /*out*/ACameraCaptureSession** session) {
    sp<ACameraCaptureSession> currentSession = mCurrentSession.promote();
    Mutex::Autolock _l(mDeviceLock);
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        return ret;
    }

    if (currentSession != nullptr) {
        currentSession->closeByDevice();
        stopRepeatingLocked();
    }

    // Create new session
    ret = configureStreamsLocked(outputs, sessionParameters);
    if (ret != ACAMERA_OK) {
        ALOGE("Fail to create new session. cannot configure streams");
        return ret;
    }

    ACameraCaptureSession* newSession = new ACameraCaptureSession(
            mNextSessionId++, outputs, callbacks, this);

    // set new session as current session
    newSession->incStrong((void *) ACameraDevice_createCaptureSession);
    mCurrentSession = newSession;
    mFlushing = false;
    *session = newSession;
    return ACAMERA_OK;
}

camera_status_t
CameraDevice::captureLocked(
        sp<ACameraCaptureSession> session,
        /*optional*/ACameraCaptureSession_captureCallbacks* cbs,
        int numRequests, ACaptureRequest** requests,
        /*optional*/int* captureSequenceId) {
    return submitRequestsLocked(
            session, cbs, numRequests, requests, captureSequenceId, /*isRepeating*/false);
}

camera_status_t
CameraDevice::setRepeatingRequestsLocked(
        sp<ACameraCaptureSession> session,
        /*optional*/ACameraCaptureSession_captureCallbacks* cbs,
        int numRequests, ACaptureRequest** requests,
        /*optional*/int* captureSequenceId) {
    return submitRequestsLocked(
            session, cbs, numRequests, requests, captureSequenceId, /*isRepeating*/true);
}

camera_status_t
CameraDevice::submitRequestsLocked(
        sp<ACameraCaptureSession> session,
        /*optional*/ACameraCaptureSession_captureCallbacks* cbs,
        int numRequests, ACaptureRequest** requests,
        /*optional*/int* captureSequenceId,
        bool isRepeating) {
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        ALOGE("Camera %s submit capture request failed! ret %d", getId(), ret);
        return ret;
    }

    // Form two vectors of capture request, one for internal tracking
    std::vector<hardware::camera2::CaptureRequest> requestList;
    Vector<sp<CaptureRequest> > requestsV;
    requestsV.setCapacity(numRequests);
    for (int i = 0; i < numRequests; i++) {
        sp<CaptureRequest> req;
        ret = allocateCaptureRequest(requests[i], req);
        if (ret != ACAMERA_OK) {
            ALOGE("Convert capture request to internal format failure! ret %d", ret);
            return ret;
        }
        if (req->mSurfaceList.empty()) {
            ALOGE("Capture request without output target cannot be submitted!");
            return ACAMERA_ERROR_INVALID_PARAMETER;
        }
        requestList.push_back(*(req.get()));
        requestsV.push_back(req);
    }

    if (isRepeating) {
        ret = stopRepeatingLocked();
        if (ret != ACAMERA_OK) {
            ALOGE("Camera %s stop repeating failed! ret %d", getId(), ret);
            return ret;
        }
    }

    binder::Status remoteRet;
    hardware::camera2::utils::SubmitInfo info;
    remoteRet = mRemote->submitRequestList(requestList, isRepeating, &info);
    int sequenceId = info.mRequestId;
    int64_t lastFrameNumber = info.mLastFrameNumber;
    if (sequenceId < 0) {
        ALOGE("Camera %s submit request remote failure: ret %d", getId(), sequenceId);
        return ACAMERA_ERROR_UNKNOWN;
    }

    CallbackHolder cbHolder(session, requestsV, isRepeating, cbs);
    mSequenceCallbackMap.insert(std::make_pair(sequenceId, cbHolder));

    if (isRepeating) {
        // stopRepeating above should have cleanup repeating sequence id
        if (mRepeatingSequenceId != REQUEST_ID_NONE) {
            setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_DEVICE);
            return ACAMERA_ERROR_CAMERA_DEVICE;
        }
        mRepeatingSequenceId = sequenceId;
    } else {
        mSequenceLastFrameNumberMap.insert(std::make_pair(sequenceId, lastFrameNumber));
    }

    if (mIdle) {
        sp<AMessage> msg = new AMessage(kWhatSessionStateCb, mHandler);
        msg->setPointer(kContextKey, session->mUserSessionCallback.context);
        msg->setObject(kSessionSpKey, session);
        msg->setPointer(kCallbackFpKey, (void*) session->mUserSessionCallback.onActive);
        postSessionMsgAndCleanup(msg);
    }
    mIdle = false;
    mBusySession = session;

    if (captureSequenceId) {
        *captureSequenceId = sequenceId;
    }
    return ACAMERA_OK;
}

camera_status_t CameraDevice::updateOutputConfigurationLocked(ACaptureSessionOutput *output) {
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        return ret;
    }

    if (output == nullptr) {
        return ACAMERA_ERROR_INVALID_PARAMETER;
    }

    if (!output->mIsShared) {
        ALOGE("Error output configuration is not shared");
        return ACAMERA_ERROR_INVALID_OPERATION;
    }

    int32_t streamId = -1;
    for (auto& kvPair : mConfiguredOutputs) {
        if (kvPair.second.first == output->mWindow) {
            streamId = kvPair.first;
            break;
        }
    }
    if (streamId < 0) {
        ALOGE("Error: Invalid output configuration");
        return ACAMERA_ERROR_INVALID_PARAMETER;
    }

    sp<IGraphicBufferProducer> iGBP(nullptr);
    ret = getIGBPfromAnw(output->mWindow, iGBP);
    if (ret != ACAMERA_OK) {
        ALOGE("Camera device %s failed to extract graphic producer from native window",
                getId());
        return ret;
    }

    OutputConfiguration outConfig(iGBP, output->mRotation, OutputConfiguration::INVALID_SET_ID,
            true);

    for (auto& anw : output->mSharedWindows) {
        ret = getIGBPfromAnw(anw, iGBP);
        if (ret != ACAMERA_OK) {
            ALOGE("Camera device %s failed to extract graphic producer from native window",
                    getId());
            return ret;
        }
        outConfig.addGraphicProducer(iGBP);
    }

    auto remoteRet = mRemote->updateOutputConfiguration(streamId, outConfig);
    if (!remoteRet.isOk()) {
        switch (remoteRet.serviceSpecificErrorCode()) {
            case hardware::ICameraService::ERROR_INVALID_OPERATION:
                ALOGE("Camera device %s invalid operation: %s", getId(),
                        remoteRet.toString8().string());
                return ACAMERA_ERROR_INVALID_OPERATION;
                break;
            case hardware::ICameraService::ERROR_ALREADY_EXISTS:
                ALOGE("Camera device %s output surface already exists: %s", getId(),
                        remoteRet.toString8().string());
                return ACAMERA_ERROR_INVALID_PARAMETER;
                break;
            case hardware::ICameraService::ERROR_ILLEGAL_ARGUMENT:
                ALOGE("Camera device %s invalid input argument: %s", getId(),
                        remoteRet.toString8().string());
                return ACAMERA_ERROR_INVALID_PARAMETER;
                break;
            default:
                ALOGE("Camera device %s failed to add shared output: %s", getId(),
                        remoteRet.toString8().string());
                return ACAMERA_ERROR_UNKNOWN;
        }
    }
    mConfiguredOutputs[streamId] = std::make_pair(output->mWindow, outConfig);

    return ACAMERA_OK;
}

camera_status_t
CameraDevice::allocateCaptureRequest(
        const ACaptureRequest* request, /*out*/sp<CaptureRequest>& outReq) {
    camera_status_t ret;
    sp<CaptureRequest> req(new CaptureRequest());
    req->mPhysicalCameraSettings.push_back({std::string(mCameraId.string()),
            request->settings->getInternalData()});
    req->mIsReprocess = false; // NDK does not support reprocessing yet
    req->mContext = request->context;
    req->mSurfaceConverted = true; // set to true, and fill in stream/surface idx to speed up IPC

    for (auto outputTarget : request->targets->mOutputs) {
        ANativeWindow* anw = outputTarget.mWindow;
        sp<Surface> surface;
        ret = getSurfaceFromANativeWindow(anw, surface);
        if (ret != ACAMERA_OK) {
            ALOGE("Bad output target in capture request! ret %d", ret);
            return ret;
        }
        req->mSurfaceList.push_back(surface);

        bool found = false;
        // lookup stream/surface ID
        for (const auto& kvPair : mConfiguredOutputs) {
            int streamId = kvPair.first;
            const OutputConfiguration& outConfig = kvPair.second.second;
            const auto& gbps = outConfig.getGraphicBufferProducers();
            for (int surfaceId = 0; surfaceId < (int) gbps.size(); surfaceId++) {
                if (gbps[surfaceId] == surface->getIGraphicBufferProducer()) {
                    found = true;
                    req->mStreamIdxList.push_back(streamId);
                    req->mSurfaceIdxList.push_back(surfaceId);
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (!found) {
            ALOGE("Unconfigured output target %p in capture request!", anw);
            return ret;
        }
    }

    outReq = req;
    return ACAMERA_OK;
}

ACaptureRequest*
CameraDevice::allocateACaptureRequest(sp<CaptureRequest>& req) {
    ACaptureRequest* pRequest = new ACaptureRequest();
    CameraMetadata clone = req->mPhysicalCameraSettings.begin()->settings;
    pRequest->settings = new ACameraMetadata(clone.release(), ACameraMetadata::ACM_REQUEST);
    pRequest->targets  = new ACameraOutputTargets();
    for (size_t i = 0; i < req->mSurfaceList.size(); i++) {
        ANativeWindow* anw = static_cast<ANativeWindow*>(req->mSurfaceList[i].get());
        ACameraOutputTarget outputTarget(anw);
        pRequest->targets->mOutputs.insert(outputTarget);
    }
    pRequest->context = req->mContext;
    return pRequest;
}

void
CameraDevice::freeACaptureRequest(ACaptureRequest* req) {
    if (req == nullptr) {
        return;
    }
    delete req->settings;
    delete req->targets;
    delete req;
}

void
CameraDevice::notifySessionEndOfLifeLocked(ACameraCaptureSession* session) {
    if (isClosed()) {
        // Device is closing already. do nothing
        return;
    }

    if (mCurrentSession != session) {
        // Session has been replaced by other seesion or device is closed
        return;
    }
    mCurrentSession = nullptr;

    // Should not happen
    if (!session->mIsClosed) {
        ALOGE("Error: unclosed session %p reaches end of life!", session);
        setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_DEVICE);
        return;
    }

    // No new session, unconfigure now
    camera_status_t ret = configureStreamsLocked(nullptr, nullptr);
    if (ret != ACAMERA_OK) {
        ALOGE("Unconfigure stream failed. Device might still be configured! ret %d", ret);
    }
}

void
CameraDevice::disconnectLocked(sp<ACameraCaptureSession>& session) {
    if (mClosing.exchange(true)) {
        // Already closing, just return
        ALOGW("Camera device %s is already closing.", getId());
        return;
    }

    if (mRemote != nullptr) {
        mRemote->disconnect();
    }
    mRemote = nullptr;

    if (session != nullptr) {
        session->closeByDevice();
    }
}

camera_status_t
CameraDevice::stopRepeatingLocked() {
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        ALOGE("Camera %s stop repeating failed! ret %d", getId(), ret);
        return ret;
    }
    if (mRepeatingSequenceId != REQUEST_ID_NONE) {
        int repeatingSequenceId = mRepeatingSequenceId;
        mRepeatingSequenceId = REQUEST_ID_NONE;

        int64_t lastFrameNumber;
        binder::Status remoteRet = mRemote->cancelRequest(repeatingSequenceId, &lastFrameNumber);
        if (remoteRet.serviceSpecificErrorCode() ==
                hardware::ICameraService::ERROR_ILLEGAL_ARGUMENT) {
            ALOGV("Repeating request is already stopped.");
            return ACAMERA_OK;
        } else if (!remoteRet.isOk()) {
            ALOGE("Stop repeating request fails in remote: %s", remoteRet.toString8().string());
            return ACAMERA_ERROR_UNKNOWN;
        }
        checkRepeatingSequenceCompleteLocked(repeatingSequenceId, lastFrameNumber);
    }
    return ACAMERA_OK;
}

camera_status_t
CameraDevice::flushLocked(ACameraCaptureSession* session) {
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        ALOGE("Camera %s abort captures failed! ret %d", getId(), ret);
        return ret;
    }

    // This should never happen because creating a new session will close
    // previous one and thus reject any API call from previous session.
    // But still good to check here in case something unexpected happen.
    if (mCurrentSession != session) {
        ALOGE("Camera %s session %p is not current active session!", getId(), session);
        return ACAMERA_ERROR_INVALID_OPERATION;
    }

    if (mFlushing) {
        ALOGW("Camera %s is already aborting captures", getId());
        return ACAMERA_OK;
    }

    mFlushing = true;

    // Send onActive callback to guarantee there is always active->ready transition
    sp<AMessage> msg = new AMessage(kWhatSessionStateCb, mHandler);
    msg->setPointer(kContextKey, session->mUserSessionCallback.context);
    msg->setObject(kSessionSpKey, session);
    msg->setPointer(kCallbackFpKey, (void*) session->mUserSessionCallback.onActive);
    postSessionMsgAndCleanup(msg);

    // If device is already idling, send callback and exit early
    if (mIdle) {
        sp<AMessage> msg = new AMessage(kWhatSessionStateCb, mHandler);
        msg->setPointer(kContextKey, session->mUserSessionCallback.context);
        msg->setObject(kSessionSpKey, session);
        msg->setPointer(kCallbackFpKey, (void*) session->mUserSessionCallback.onReady);
        postSessionMsgAndCleanup(msg);
        mFlushing = false;
        return ACAMERA_OK;
    }

    int64_t lastFrameNumber;
    binder::Status remoteRet = mRemote->flush(&lastFrameNumber);
    if (!remoteRet.isOk()) {
        ALOGE("Abort captures fails in remote: %s", remoteRet.toString8().string());
        return ACAMERA_ERROR_UNKNOWN;
    }
    if (mRepeatingSequenceId != REQUEST_ID_NONE) {
        checkRepeatingSequenceCompleteLocked(mRepeatingSequenceId, lastFrameNumber);
    }
    return ACAMERA_OK;
}

camera_status_t
CameraDevice::waitUntilIdleLocked() {
    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        ALOGE("Wait until camera %s idle failed! ret %d", getId(), ret);
        return ret;
    }

    if (mRepeatingSequenceId != REQUEST_ID_NONE) {
        ALOGE("Camera device %s won't go to idle when there is repeating request!", getId());
        return ACAMERA_ERROR_INVALID_OPERATION;
    }

    binder::Status remoteRet = mRemote->waitUntilIdle();
    if (!remoteRet.isOk()) {
        ALOGE("Camera device %s waitUntilIdle failed: %s", getId(), remoteRet.toString8().string());
        // TODO: define a function to convert status_t -> camera_status_t
        return ACAMERA_ERROR_UNKNOWN;
    }

    return ACAMERA_OK;
}

camera_status_t
CameraDevice::getIGBPfromAnw(
        ANativeWindow* anw,
        sp<IGraphicBufferProducer>& out) {
    sp<Surface> surface;
    camera_status_t ret = getSurfaceFromANativeWindow(anw, surface);
    if (ret != ACAMERA_OK) {
        return ret;
    }
    out = surface->getIGraphicBufferProducer();
    return ACAMERA_OK;
}

camera_status_t
CameraDevice::getSurfaceFromANativeWindow(
        ANativeWindow* anw, sp<Surface>& out) {
    if (anw == nullptr) {
        ALOGE("Error: output ANativeWindow is null");
        return ACAMERA_ERROR_INVALID_PARAMETER;
    }
    int value;
    int err = (*anw->query)(anw, NATIVE_WINDOW_CONCRETE_TYPE, &value);
    if (err != OK || value != NATIVE_WINDOW_SURFACE) {
        ALOGE("Error: ANativeWindow is not backed by Surface!");
        return ACAMERA_ERROR_INVALID_PARAMETER;
    }
    sp<Surface> surface(static_cast<Surface*>(anw));
    out = surface;
    return ACAMERA_OK;
}

camera_status_t
CameraDevice::configureStreamsLocked(const ACaptureSessionOutputContainer* outputs,
        const ACaptureRequest* sessionParameters) {
    ACaptureSessionOutputContainer emptyOutput;
    if (outputs == nullptr) {
        outputs = &emptyOutput;
    }

    camera_status_t ret = checkCameraClosedOrErrorLocked();
    if (ret != ACAMERA_OK) {
        return ret;
    }

    std::set<std::pair<ANativeWindow*, OutputConfiguration>> outputSet;
    for (auto outConfig : outputs->mOutputs) {
        ANativeWindow* anw = outConfig.mWindow;
        sp<IGraphicBufferProducer> iGBP(nullptr);
        ret = getIGBPfromAnw(anw, iGBP);
        if (ret != ACAMERA_OK) {
            return ret;
        }
        outputSet.insert(std::make_pair(
                anw, OutputConfiguration(iGBP, outConfig.mRotation,
                        OutputConfiguration::INVALID_SET_ID, outConfig.mIsShared)));
    }
    auto addSet = outputSet;
    std::vector<int> deleteList;

    // Determine which streams need to be created, which to be deleted
    for (auto& kvPair : mConfiguredOutputs) {
        int streamId = kvPair.first;
        auto& outputPair = kvPair.second;
        if (outputSet.count(outputPair) == 0) {
            deleteList.push_back(streamId); // Need to delete a no longer needed stream
        } else {
            addSet.erase(outputPair);        // No need to add already existing stream
        }
    }

    ret = stopRepeatingLocked();
    if (ret != ACAMERA_OK) {
        ALOGE("Camera device %s stop repeating failed, ret %d", getId(), ret);
        return ret;
    }

    ret = waitUntilIdleLocked();
    if (ret != ACAMERA_OK) {
        ALOGE("Camera device %s wait until idle failed, ret %d", getId(), ret);
        return ret;
    }

    // Send onReady to previous session
    // CurrentSession will be updated after configureStreamLocked, so here
    // mCurrentSession is the session to be replaced by a new session
    if (!mIdle && mCurrentSession != nullptr) {
        if (mBusySession != mCurrentSession) {
            ALOGE("Current session != busy session");
            setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_DEVICE);
            return ACAMERA_ERROR_CAMERA_DEVICE;
        }
        sp<AMessage> msg = new AMessage(kWhatSessionStateCb, mHandler);
        msg->setPointer(kContextKey, mBusySession->mUserSessionCallback.context);
        msg->setObject(kSessionSpKey, mBusySession);
        msg->setPointer(kCallbackFpKey, (void*) mBusySession->mUserSessionCallback.onReady);
        mBusySession.clear();
        postSessionMsgAndCleanup(msg);
    }
    mIdle = true;

    binder::Status remoteRet = mRemote->beginConfigure();
    if (!remoteRet.isOk()) {
        ALOGE("Camera device %s begin configure failed: %s", getId(), remoteRet.toString8().string());
        return ACAMERA_ERROR_UNKNOWN;
    }

    // delete to-be-deleted streams
    for (auto streamId : deleteList) {
        remoteRet = mRemote->deleteStream(streamId);
        if (!remoteRet.isOk()) {
            ALOGE("Camera device %s failed to remove stream %d: %s", getId(), streamId,
                    remoteRet.toString8().string());
            return ACAMERA_ERROR_UNKNOWN;
        }
        mConfiguredOutputs.erase(streamId);
    }

    // add new streams
    for (auto outputPair : addSet) {
        int streamId;
        remoteRet = mRemote->createStream(outputPair.second, &streamId);
        if (!remoteRet.isOk()) {
            ALOGE("Camera device %s failed to create stream: %s", getId(),
                    remoteRet.toString8().string());
            return ACAMERA_ERROR_UNKNOWN;
        }
        mConfiguredOutputs.insert(std::make_pair(streamId, outputPair));
    }

    CameraMetadata params;
    if ((sessionParameters != nullptr) && (sessionParameters->settings != nullptr)) {
        params.append(sessionParameters->settings->getInternalData());
    }
    remoteRet = mRemote->endConfigure(/*isConstrainedHighSpeed*/ false, params);
    if (remoteRet.serviceSpecificErrorCode() == hardware::ICameraService::ERROR_ILLEGAL_ARGUMENT) {
        ALOGE("Camera device %s cannnot support app output configuration: %s", getId(),
                remoteRet.toString8().string());
        return ACAMERA_ERROR_STREAM_CONFIGURE_FAIL;
    } else if (!remoteRet.isOk()) {
        ALOGE("Camera device %s end configure failed: %s", getId(), remoteRet.toString8().string());
        return ACAMERA_ERROR_UNKNOWN;
    }

    return ACAMERA_OK;
}

void
CameraDevice::setRemoteDevice(sp<hardware::camera2::ICameraDeviceUser> remote) {
    Mutex::Autolock _l(mDeviceLock);
    mRemote = remote;
}

camera_status_t
CameraDevice::checkCameraClosedOrErrorLocked() const {
    if (mRemote == nullptr) {
        ALOGE("%s: camera device already closed", __FUNCTION__);
        return ACAMERA_ERROR_CAMERA_DISCONNECTED;
    }
    if (mInError) {// triggered by onDeviceError
        ALOGE("%s: camera device has encountered a serious error", __FUNCTION__);
        return mError;
    }
    return ACAMERA_OK;
}

void
CameraDevice::setCameraDeviceErrorLocked(camera_status_t error) {
    mInError = true;
    mError = error;
    return;
}

void
CameraDevice::FrameNumberTracker::updateTracker(int64_t frameNumber, bool isError) {
    ALOGV("updateTracker frame %" PRId64 " isError %d", frameNumber, isError);
    if (isError) {
        mFutureErrorSet.insert(frameNumber);
    } else if (frameNumber <= mCompletedFrameNumber) {
        ALOGE("Frame number %" PRId64 " decreased! current fn %" PRId64,
                frameNumber, mCompletedFrameNumber);
        return;
    } else {
        if (frameNumber != mCompletedFrameNumber + 1) {
            ALOGE("Frame number out of order. Expect %" PRId64 " but get %" PRId64,
                    mCompletedFrameNumber + 1, frameNumber);
            // Do not assert as in java implementation
        }
        mCompletedFrameNumber = frameNumber;
    }
    update();
}

void
CameraDevice::FrameNumberTracker::update() {
    for (auto it = mFutureErrorSet.begin(); it != mFutureErrorSet.end();) {
        int64_t errorFrameNumber = *it;
        if (errorFrameNumber == mCompletedFrameNumber + 1) {
            mCompletedFrameNumber++;
            it = mFutureErrorSet.erase(it);
        } else if (errorFrameNumber <= mCompletedFrameNumber) {
            // This should not happen, but deal with it anyway
            ALOGE("Completd frame number passed through current frame number!");
            // erase the old error since it's no longer useful
            it = mFutureErrorSet.erase(it);
        } else {
            // Normal requests hasn't catched up error frames, just break
            break;
        }
    }
    ALOGV("Update complete frame %" PRId64, mCompletedFrameNumber);
}

void
CameraDevice::onCaptureErrorLocked(
        int32_t errorCode,
        const CaptureResultExtras& resultExtras) {
    int sequenceId = resultExtras.requestId;
    int64_t frameNumber = resultExtras.frameNumber;
    int32_t burstId = resultExtras.burstId;
    auto it = mSequenceCallbackMap.find(sequenceId);
    if (it == mSequenceCallbackMap.end()) {
        ALOGE("%s: Error: capture sequence index %d not found!",
                __FUNCTION__, sequenceId);
        setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_SERVICE);
        return;
    }

    CallbackHolder cbh = (*it).second;
    sp<ACameraCaptureSession> session = cbh.mSession;
    if ((size_t) burstId >= cbh.mRequests.size()) {
        ALOGE("%s: Error: request index %d out of bound (size %zu)",
                __FUNCTION__, burstId, cbh.mRequests.size());
        setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_SERVICE);
        return;
    }
    sp<CaptureRequest> request = cbh.mRequests[burstId];

    // Handle buffer error
    if (errorCode == hardware::camera2::ICameraDeviceCallbacks::ERROR_CAMERA_BUFFER) {
        int32_t streamId = resultExtras.errorStreamId;
        ACameraCaptureSession_captureCallback_bufferLost onBufferLost =
                cbh.mCallbacks.onCaptureBufferLost;
        auto outputPairIt = mConfiguredOutputs.find(streamId);
        if (outputPairIt == mConfiguredOutputs.end()) {
            ALOGE("%s: Error: stream id %d does not exist", __FUNCTION__, streamId);
            setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_SERVICE);
            return;
        }

        const auto& gbps = outputPairIt->second.second.getGraphicBufferProducers();
        for (const auto& outGbp : gbps) {
            for (auto surface : request->mSurfaceList) {
                if (surface->getIGraphicBufferProducer() == outGbp) {
                    ANativeWindow* anw = static_cast<ANativeWindow*>(surface.get());
                    ALOGV("Camera %s Lost output buffer for ANW %p frame %" PRId64,
                            getId(), anw, frameNumber);

                    sp<AMessage> msg = new AMessage(kWhatCaptureBufferLost, mHandler);
                    msg->setPointer(kContextKey, cbh.mCallbacks.context);
                    msg->setObject(kSessionSpKey, session);
                    msg->setPointer(kCallbackFpKey, (void*) onBufferLost);
                    msg->setObject(kCaptureRequestKey, request);
                    msg->setPointer(kAnwKey, (void*) anw);
                    msg->setInt64(kFrameNumberKey, frameNumber);
                    postSessionMsgAndCleanup(msg);
                }
            }
        }
    } else { // Handle other capture failures
        // Fire capture failure callback if there is one registered
        ACameraCaptureSession_captureCallback_failed onError = cbh.mCallbacks.onCaptureFailed;
        sp<CameraCaptureFailure> failure(new CameraCaptureFailure());
        failure->frameNumber = frameNumber;
        // TODO: refine this when implementing flush
        failure->reason      = CAPTURE_FAILURE_REASON_ERROR;
        failure->sequenceId  = sequenceId;
        failure->wasImageCaptured = (errorCode ==
                hardware::camera2::ICameraDeviceCallbacks::ERROR_CAMERA_RESULT);

        sp<AMessage> msg = new AMessage(kWhatCaptureFail, mHandler);
        msg->setPointer(kContextKey, cbh.mCallbacks.context);
        msg->setObject(kSessionSpKey, session);
        msg->setPointer(kCallbackFpKey, (void*) onError);
        msg->setObject(kCaptureRequestKey, request);
        msg->setObject(kCaptureFailureKey, failure);
        postSessionMsgAndCleanup(msg);

        // Update tracker
        mFrameNumberTracker.updateTracker(frameNumber, /*isError*/true);
        checkAndFireSequenceCompleteLocked();
    }
    return;
}

void CameraDevice::CallbackHandler::onMessageReceived(
        const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatOnDisconnected:
        case kWhatOnError:
        case kWhatSessionStateCb:
        case kWhatCaptureStart:
        case kWhatCaptureResult:
        case kWhatCaptureFail:
        case kWhatCaptureSeqEnd:
        case kWhatCaptureSeqAbort:
        case kWhatCaptureBufferLost:
            ALOGV("%s: Received msg %d", __FUNCTION__, msg->what());
            break;
        case kWhatCleanUpSessions:
            mCachedSessions.clear();
            return;
        default:
            ALOGE("%s:Error: unknown device callback %d", __FUNCTION__, msg->what());
            return;
    }
    // Check the common part of all message
    void* context;
    bool found = msg->findPointer(kContextKey, &context);
    if (!found) {
        ALOGE("%s: Cannot find callback context!", __FUNCTION__);
        return;
    }
    switch (msg->what()) {
        case kWhatOnDisconnected:
        {
            ACameraDevice* dev;
            found = msg->findPointer(kDeviceKey, (void**) &dev);
            if (!found || dev == nullptr) {
                ALOGE("%s: Cannot find device pointer!", __FUNCTION__);
                return;
            }
            ACameraDevice_StateCallback onDisconnected;
            found = msg->findPointer(kCallbackFpKey, (void**) &onDisconnected);
            if (!found) {
                ALOGE("%s: Cannot find onDisconnected!", __FUNCTION__);
                return;
            }
            if (onDisconnected == nullptr) {
                return;
            }
            (*onDisconnected)(context, dev);
            break;
        }
        case kWhatOnError:
        {
            ACameraDevice* dev;
            found = msg->findPointer(kDeviceKey, (void**) &dev);
            if (!found || dev == nullptr) {
                ALOGE("%s: Cannot find device pointer!", __FUNCTION__);
                return;
            }
            ACameraDevice_ErrorStateCallback onError;
            found = msg->findPointer(kCallbackFpKey, (void**) &onError);
            if (!found) {
                ALOGE("%s: Cannot find onError!", __FUNCTION__);
                return;
            }
            int errorCode;
            found = msg->findInt32(kErrorCodeKey, &errorCode);
            if (!found) {
                ALOGE("%s: Cannot find error code!", __FUNCTION__);
                return;
            }
            if (onError == nullptr) {
                return;
            }
            (*onError)(context, dev, errorCode);
            break;
        }
        case kWhatSessionStateCb:
        case kWhatCaptureStart:
        case kWhatCaptureResult:
        case kWhatCaptureFail:
        case kWhatCaptureSeqEnd:
        case kWhatCaptureSeqAbort:
        case kWhatCaptureBufferLost:
        {
            sp<RefBase> obj;
            found = msg->findObject(kSessionSpKey, &obj);
            if (!found || obj == nullptr) {
                ALOGE("%s: Cannot find session pointer!", __FUNCTION__);
                return;
            }
            sp<ACameraCaptureSession> session(static_cast<ACameraCaptureSession*>(obj.get()));
            mCachedSessions.push(session);
            sp<CaptureRequest> requestSp = nullptr;
            switch (msg->what()) {
                case kWhatCaptureStart:
                case kWhatCaptureResult:
                case kWhatCaptureFail:
                case kWhatCaptureBufferLost:
                    found = msg->findObject(kCaptureRequestKey, &obj);
                    if (!found) {
                        ALOGE("%s: Cannot find capture request!", __FUNCTION__);
                        return;
                    }
                    requestSp = static_cast<CaptureRequest*>(obj.get());
                    break;
            }

            switch (msg->what()) {
                case kWhatSessionStateCb:
                {
                    ACameraCaptureSession_stateCallback onState;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onState);
                    if (!found) {
                        ALOGE("%s: Cannot find state callback!", __FUNCTION__);
                        return;
                    }
                    if (onState == nullptr) {
                        return;
                    }
                    (*onState)(context, session.get());
                    break;
                }
                case kWhatCaptureStart:
                {
                    ACameraCaptureSession_captureCallback_start onStart;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onStart);
                    if (!found) {
                        ALOGE("%s: Cannot find capture start callback!", __FUNCTION__);
                        return;
                    }
                    if (onStart == nullptr) {
                        return;
                    }
                    int64_t timestamp;
                    found = msg->findInt64(kTimeStampKey, &timestamp);
                    if (!found) {
                        ALOGE("%s: Cannot find timestamp!", __FUNCTION__);
                        return;
                    }
                    ACaptureRequest* request = allocateACaptureRequest(requestSp);
                    (*onStart)(context, session.get(), request, timestamp);
                    freeACaptureRequest(request);
                    break;
                }
                case kWhatCaptureResult:
                {
                    ACameraCaptureSession_captureCallback_result onResult;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onResult);
                    if (!found) {
                        ALOGE("%s: Cannot find capture result callback!", __FUNCTION__);
                        return;
                    }
                    if (onResult == nullptr) {
                        return;
                    }

                    found = msg->findObject(kCaptureResultKey, &obj);
                    if (!found) {
                        ALOGE("%s: Cannot find capture result!", __FUNCTION__);
                        return;
                    }
                    sp<ACameraMetadata> result(static_cast<ACameraMetadata*>(obj.get()));
                    ACaptureRequest* request = allocateACaptureRequest(requestSp);
                    (*onResult)(context, session.get(), request, result.get());
                    freeACaptureRequest(request);
                    break;
                }
                case kWhatCaptureFail:
                {
                    ACameraCaptureSession_captureCallback_failed onFail;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onFail);
                    if (!found) {
                        ALOGE("%s: Cannot find capture fail callback!", __FUNCTION__);
                        return;
                    }
                    if (onFail == nullptr) {
                        return;
                    }

                    found = msg->findObject(kCaptureFailureKey, &obj);
                    if (!found) {
                        ALOGE("%s: Cannot find capture failure!", __FUNCTION__);
                        return;
                    }
                    sp<CameraCaptureFailure> failureSp(
                            static_cast<CameraCaptureFailure*>(obj.get()));
                    ACameraCaptureFailure* failure =
                            static_cast<ACameraCaptureFailure*>(failureSp.get());
                    ACaptureRequest* request = allocateACaptureRequest(requestSp);
                    (*onFail)(context, session.get(), request, failure);
                    freeACaptureRequest(request);
                    break;
                }
                case kWhatCaptureSeqEnd:
                {
                    ACameraCaptureSession_captureCallback_sequenceEnd onSeqEnd;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onSeqEnd);
                    if (!found) {
                        ALOGE("%s: Cannot find sequence end callback!", __FUNCTION__);
                        return;
                    }
                    if (onSeqEnd == nullptr) {
                        return;
                    }
                    int seqId;
                    found = msg->findInt32(kSequenceIdKey, &seqId);
                    if (!found) {
                        ALOGE("%s: Cannot find frame number!", __FUNCTION__);
                        return;
                    }
                    int64_t frameNumber;
                    found = msg->findInt64(kFrameNumberKey, &frameNumber);
                    if (!found) {
                        ALOGE("%s: Cannot find frame number!", __FUNCTION__);
                        return;
                    }
                    (*onSeqEnd)(context, session.get(), seqId, frameNumber);
                    break;
                }
                case kWhatCaptureSeqAbort:
                {
                    ACameraCaptureSession_captureCallback_sequenceAbort onSeqAbort;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onSeqAbort);
                    if (!found) {
                        ALOGE("%s: Cannot find sequence end callback!", __FUNCTION__);
                        return;
                    }
                    if (onSeqAbort == nullptr) {
                        return;
                    }
                    int seqId;
                    found = msg->findInt32(kSequenceIdKey, &seqId);
                    if (!found) {
                        ALOGE("%s: Cannot find frame number!", __FUNCTION__);
                        return;
                    }
                    (*onSeqAbort)(context, session.get(), seqId);
                    break;
                }
                case kWhatCaptureBufferLost:
                {
                    ACameraCaptureSession_captureCallback_bufferLost onBufferLost;
                    found = msg->findPointer(kCallbackFpKey, (void**) &onBufferLost);
                    if (!found) {
                        ALOGE("%s: Cannot find buffer lost callback!", __FUNCTION__);
                        return;
                    }
                    if (onBufferLost == nullptr) {
                        return;
                    }

                    ANativeWindow* anw;
                    found = msg->findPointer(kAnwKey, (void**) &anw);
                    if (!found) {
                        ALOGE("%s: Cannot find ANativeWindow!", __FUNCTION__);
                        return;
                    }

                    int64_t frameNumber;
                    found = msg->findInt64(kFrameNumberKey, &frameNumber);
                    if (!found) {
                        ALOGE("%s: Cannot find frame number!", __FUNCTION__);
                        return;
                    }

                    ACaptureRequest* request = allocateACaptureRequest(requestSp);
                    (*onBufferLost)(context, session.get(), request, anw, frameNumber);
                    freeACaptureRequest(request);
                    break;
                }
            }
            break;
        }
    }
}

CameraDevice::CallbackHolder::CallbackHolder(
    sp<ACameraCaptureSession>          session,
    const Vector<sp<CaptureRequest> >& requests,
    bool                               isRepeating,
    ACameraCaptureSession_captureCallbacks* cbs) :
    mSession(session), mRequests(requests),
    mIsRepeating(isRepeating), mCallbacks(fillCb(cbs)) {}

void
CameraDevice::checkRepeatingSequenceCompleteLocked(
    const int sequenceId, const int64_t lastFrameNumber) {
    ALOGV("Repeating seqId %d lastFrameNumer %" PRId64, sequenceId, lastFrameNumber);
    if (lastFrameNumber == NO_FRAMES_CAPTURED) {
        if (mSequenceCallbackMap.count(sequenceId) == 0) {
            ALOGW("No callback found for sequenceId %d", sequenceId);
            return;
        }
        // remove callback holder from callback map
        auto cbIt = mSequenceCallbackMap.find(sequenceId);
        CallbackHolder cbh = cbIt->second;
        mSequenceCallbackMap.erase(cbIt);
        // send seq aborted callback
        sp<AMessage> msg = new AMessage(kWhatCaptureSeqAbort, mHandler);
        msg->setPointer(kContextKey, cbh.mCallbacks.context);
        msg->setObject(kSessionSpKey, cbh.mSession);
        msg->setPointer(kCallbackFpKey, (void*) cbh.mCallbacks.onCaptureSequenceAborted);
        msg->setInt32(kSequenceIdKey, sequenceId);
        postSessionMsgAndCleanup(msg);
    } else {
        // Use mSequenceLastFrameNumberMap to track
        mSequenceLastFrameNumberMap.insert(std::make_pair(sequenceId, lastFrameNumber));

        // Last frame might have arrived. Check now
        checkAndFireSequenceCompleteLocked();
    }
}

void
CameraDevice::checkAndFireSequenceCompleteLocked() {
    int64_t completedFrameNumber = mFrameNumberTracker.getCompletedFrameNumber();
    //std::map<int, int64_t> mSequenceLastFrameNumberMap;
    auto it = mSequenceLastFrameNumberMap.begin();
    while (it != mSequenceLastFrameNumberMap.end()) {
        int sequenceId = it->first;
        int64_t lastFrameNumber = it->second;
        bool seqCompleted = false;
        bool hasCallback  = true;

        if (mRemote == nullptr) {
            ALOGW("Camera %s closed while checking sequence complete", getId());
            return;
        }

        // Check if there is callback for this sequence
        // This should not happen because we always register callback (with nullptr inside)
        if (mSequenceCallbackMap.count(sequenceId) == 0) {
            ALOGW("No callback found for sequenceId %d", sequenceId);
            hasCallback = false;
        }

        if (lastFrameNumber <= completedFrameNumber) {
            ALOGV("seq %d reached last frame %" PRId64 ", completed %" PRId64,
                    sequenceId, lastFrameNumber, completedFrameNumber);
            seqCompleted = true;
        }

        if (seqCompleted && hasCallback) {
            // remove callback holder from callback map
            auto cbIt = mSequenceCallbackMap.find(sequenceId);
            CallbackHolder cbh = cbIt->second;
            mSequenceCallbackMap.erase(cbIt);
            // send seq complete callback
            sp<AMessage> msg = new AMessage(kWhatCaptureSeqEnd, mHandler);
            msg->setPointer(kContextKey, cbh.mCallbacks.context);
            msg->setObject(kSessionSpKey, cbh.mSession);
            msg->setPointer(kCallbackFpKey, (void*) cbh.mCallbacks.onCaptureSequenceCompleted);
            msg->setInt32(kSequenceIdKey, sequenceId);
            msg->setInt64(kFrameNumberKey, lastFrameNumber);

            // Clear the session sp before we send out the message
            // This will guarantee the rare case where the message is processed
            // before cbh goes out of scope and causing we call the session
            // destructor while holding device lock
            cbh.mSession.clear();
            postSessionMsgAndCleanup(msg);
        }

        // No need to track sequence complete if there is no callback registered
        if (seqCompleted || !hasCallback) {
            it = mSequenceLastFrameNumberMap.erase(it);
        } else {
            ++it;
        }
    }
}

/**
  * Camera service callback implementation
  */
binder::Status
CameraDevice::ServiceCallback::onDeviceError(
        int32_t errorCode,
        const CaptureResultExtras& resultExtras) {
    ALOGD("Device error received, code %d, frame number %" PRId64 ", request ID %d, subseq ID %d",
            errorCode, resultExtras.frameNumber, resultExtras.requestId, resultExtras.burstId);
    binder::Status ret = binder::Status::ok();
    sp<CameraDevice> dev = mDevice.promote();
    if (dev == nullptr) {
        return ret; // device has been closed
    }

    sp<ACameraCaptureSession> session = dev->mCurrentSession.promote();
    Mutex::Autolock _l(dev->mDeviceLock);
    if (dev->mRemote == nullptr) {
        return ret; // device has been closed
    }
    switch (errorCode) {
        case ERROR_CAMERA_DISCONNECTED:
        {
            // Camera is disconnected, close the session and expect no more callbacks
            if (session != nullptr) {
                session->closeByDevice();
            }
            dev->mCurrentSession = nullptr;
            sp<AMessage> msg = new AMessage(kWhatOnDisconnected, dev->mHandler);
            msg->setPointer(kContextKey, dev->mAppCallbacks.context);
            msg->setPointer(kDeviceKey, (void*) dev->getWrapper());
            msg->setPointer(kCallbackFpKey, (void*) dev->mAppCallbacks.onDisconnected);
            msg->post();
            break;
        }
        default:
            ALOGE("Unknown error from camera device: %d", errorCode);
            // no break
        case ERROR_CAMERA_DEVICE:
        case ERROR_CAMERA_SERVICE:
        {
            switch (errorCode) {
                case ERROR_CAMERA_DEVICE:
                    dev->setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_DEVICE);
                    break;
                case ERROR_CAMERA_SERVICE:
                    dev->setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_SERVICE);
                    break;
                default:
                    dev->setCameraDeviceErrorLocked(ACAMERA_ERROR_UNKNOWN);
                    break;
            }
            sp<AMessage> msg = new AMessage(kWhatOnError, dev->mHandler);
            msg->setPointer(kContextKey, dev->mAppCallbacks.context);
            msg->setPointer(kDeviceKey, (void*) dev->getWrapper());
            msg->setPointer(kCallbackFpKey, (void*) dev->mAppCallbacks.onError);
            msg->setInt32(kErrorCodeKey, errorCode);
            msg->post();
            break;
        }
        case ERROR_CAMERA_REQUEST:
        case ERROR_CAMERA_RESULT:
        case ERROR_CAMERA_BUFFER:
            dev->onCaptureErrorLocked(errorCode, resultExtras);
            break;
    }
    return ret;
}

binder::Status
CameraDevice::ServiceCallback::onDeviceIdle() {
    ALOGV("Camera is now idle");
    binder::Status ret = binder::Status::ok();
    sp<CameraDevice> dev = mDevice.promote();
    if (dev == nullptr) {
        return ret; // device has been closed
    }

    Mutex::Autolock _l(dev->mDeviceLock);
    if (dev->isClosed() || dev->mRemote == nullptr) {
        return ret;
    }

    if (dev->mIdle) {
        // Already in idle state. Possibly other thread did waitUntilIdle
        return ret;
    }

    if (dev->mCurrentSession != nullptr) {
        ALOGE("onDeviceIdle sending state cb");
        if (dev->mBusySession != dev->mCurrentSession) {
            ALOGE("Current session != busy session");
            dev->setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_DEVICE);
            return ret;
        }

        sp<AMessage> msg = new AMessage(kWhatSessionStateCb, dev->mHandler);
        msg->setPointer(kContextKey, dev->mBusySession->mUserSessionCallback.context);
        msg->setObject(kSessionSpKey, dev->mBusySession);
        msg->setPointer(kCallbackFpKey, (void*) dev->mBusySession->mUserSessionCallback.onReady);
        // Make sure we clear the sp first so the session destructor can
        // only happen on handler thread (where we don't hold device/session lock)
        dev->mBusySession.clear();
        dev->postSessionMsgAndCleanup(msg);
    }
    dev->mIdle = true;
    dev->mFlushing = false;
    return ret;
}

binder::Status
CameraDevice::ServiceCallback::onCaptureStarted(
        const CaptureResultExtras& resultExtras,
        int64_t timestamp) {
    binder::Status ret = binder::Status::ok();

    sp<CameraDevice> dev = mDevice.promote();
    if (dev == nullptr) {
        return ret; // device has been closed
    }
    Mutex::Autolock _l(dev->mDeviceLock);
    if (dev->isClosed() || dev->mRemote == nullptr) {
        return ret;
    }

    int sequenceId = resultExtras.requestId;
    int32_t burstId = resultExtras.burstId;

    auto it = dev->mSequenceCallbackMap.find(sequenceId);
    if (it != dev->mSequenceCallbackMap.end()) {
        CallbackHolder cbh = (*it).second;
        ACameraCaptureSession_captureCallback_start onStart = cbh.mCallbacks.onCaptureStarted;
        sp<ACameraCaptureSession> session = cbh.mSession;
        if ((size_t) burstId >= cbh.mRequests.size()) {
            ALOGE("%s: Error: request index %d out of bound (size %zu)",
                    __FUNCTION__, burstId, cbh.mRequests.size());
            dev->setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_SERVICE);
        }
        sp<CaptureRequest> request = cbh.mRequests[burstId];
        sp<AMessage> msg = new AMessage(kWhatCaptureStart, dev->mHandler);
        msg->setPointer(kContextKey, cbh.mCallbacks.context);
        msg->setObject(kSessionSpKey, session);
        msg->setPointer(kCallbackFpKey, (void*) onStart);
        msg->setObject(kCaptureRequestKey, request);
        msg->setInt64(kTimeStampKey, timestamp);
        dev->postSessionMsgAndCleanup(msg);
    }
    return ret;
}

binder::Status
CameraDevice::ServiceCallback::onResultReceived(
        const CameraMetadata& metadata,
        const CaptureResultExtras& resultExtras,
        const std::vector<PhysicalCaptureResultInfo>& physicalResultInfos) {
    (void) physicalResultInfos;
    binder::Status ret = binder::Status::ok();

    sp<CameraDevice> dev = mDevice.promote();
    if (dev == nullptr) {
        return ret; // device has been closed
    }
    int sequenceId = resultExtras.requestId;
    int64_t frameNumber = resultExtras.frameNumber;
    int32_t burstId = resultExtras.burstId;
    bool    isPartialResult = (resultExtras.partialResultCount < dev->mPartialResultCount);

    if (!isPartialResult) {
        ALOGV("SeqId %d frame %" PRId64 " result arrive.", sequenceId, frameNumber);
    }

    Mutex::Autolock _l(dev->mDeviceLock);
    if (dev->mRemote == nullptr) {
        return ret; // device has been disconnected
    }

    if (dev->isClosed()) {
        if (!isPartialResult) {
            dev->mFrameNumberTracker.updateTracker(frameNumber, /*isError*/false);
        }
        // early return to avoid callback sent to closed devices
        return ret;
    }

    CameraMetadata metadataCopy = metadata;
    metadataCopy.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE, dev->mShadingMapSize, /*data_count*/2);
    metadataCopy.update(ANDROID_SYNC_FRAME_NUMBER, &frameNumber, /*data_count*/1);

    auto it = dev->mSequenceCallbackMap.find(sequenceId);
    if (it != dev->mSequenceCallbackMap.end()) {
        CallbackHolder cbh = (*it).second;
        ACameraCaptureSession_captureCallback_result onResult = isPartialResult ?
                cbh.mCallbacks.onCaptureProgressed :
                cbh.mCallbacks.onCaptureCompleted;
        sp<ACameraCaptureSession> session = cbh.mSession;
        if ((size_t) burstId >= cbh.mRequests.size()) {
            ALOGE("%s: Error: request index %d out of bound (size %zu)",
                    __FUNCTION__, burstId, cbh.mRequests.size());
            dev->setCameraDeviceErrorLocked(ACAMERA_ERROR_CAMERA_SERVICE);
        }
        sp<CaptureRequest> request = cbh.mRequests[burstId];
        sp<ACameraMetadata> result(new ACameraMetadata(
                metadataCopy.release(), ACameraMetadata::ACM_RESULT));

        sp<AMessage> msg = new AMessage(kWhatCaptureResult, dev->mHandler);
        msg->setPointer(kContextKey, cbh.mCallbacks.context);
        msg->setObject(kSessionSpKey, session);
        msg->setPointer(kCallbackFpKey, (void*) onResult);
        msg->setObject(kCaptureRequestKey, request);
        msg->setObject(kCaptureResultKey, result);
        dev->postSessionMsgAndCleanup(msg);
    }

    if (!isPartialResult) {
        dev->mFrameNumberTracker.updateTracker(frameNumber, /*isError*/false);
        dev->checkAndFireSequenceCompleteLocked();
    }

    return ret;
}

binder::Status
CameraDevice::ServiceCallback::onPrepared(int) {
    // Prepare not yet implemented in NDK
    return binder::Status::ok();
}

binder::Status
CameraDevice::ServiceCallback::onRequestQueueEmpty() {
    // onRequestQueueEmpty not yet implemented in NDK
    return binder::Status::ok();
}

binder::Status
CameraDevice::ServiceCallback::onRepeatingRequestError(
        int64_t lastFrameNumber, int32_t stoppedSequenceId) {
    binder::Status ret = binder::Status::ok();

    sp<CameraDevice> dev = mDevice.promote();
    if (dev == nullptr) {
        return ret; // device has been closed
    }

    Mutex::Autolock _l(dev->mDeviceLock);

    int repeatingSequenceId = dev->mRepeatingSequenceId;
    if (stoppedSequenceId == repeatingSequenceId) {
        dev->mRepeatingSequenceId = REQUEST_ID_NONE;
    }

    dev->checkRepeatingSequenceCompleteLocked(repeatingSequenceId, lastFrameNumber);

    return ret;
}


} // namespace android
