/*
 * Copyright (C) 2013 The Android Open Source Project
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

/*
 * Contains implementation of a class EmulatedFakeCamera3 that encapsulates
 * functionality of an advanced fake camera.
 */

#include <inttypes.h>

#define LOG_NDEBUG 0
//#define LOG_NNDEBUG 0
#define LOG_TAG "EmulatedCamera_FakeCamera3"
#include <android/log.h>

#include "EmulatedFakeCamera3.h"
#include "EmulatedCameraFactory.h"
#include <ui/Fence.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <sys/types.h>

#include <cutils/properties.h>
#include "fake-pipeline2/Sensor.h"
#include "fake-pipeline2/JpegCompressor.h"
#include <cmath>
#include <gralloc_priv.h>
#include <binder/IPCThreadState.h>

#if defined(LOG_NNDEBUG) && LOG_NNDEBUG == 0
#define ALOGVV ALOGV
#else
#define ALOGVV(...) ((void)0)
#endif

namespace android {

/**
 * Constants for camera capabilities
 */

const int64_t USEC = 1000LL;
const int64_t MSEC = USEC * 1000LL;
//const int64_t SEC = MSEC * 1000LL;


const int32_t EmulatedFakeCamera3::kAvailableFormats[] = {
        //HAL_PIXEL_FORMAT_RAW_SENSOR,
        HAL_PIXEL_FORMAT_BLOB,
        //HAL_PIXEL_FORMAT_RGBA_8888,
        HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
        // These are handled by YCbCr_420_888
        HAL_PIXEL_FORMAT_YV12,
        HAL_PIXEL_FORMAT_YCrCb_420_SP,
        //HAL_PIXEL_FORMAT_YCbCr_422_I,
        HAL_PIXEL_FORMAT_YCbCr_420_888
};

const uint32_t EmulatedFakeCamera3::kAvailableRawSizes[2] = {
    640, 480
    //    Sensor::kResolution[0], Sensor::kResolution[1]
};

const uint64_t EmulatedFakeCamera3::kAvailableRawMinDurations[1] = {
    (const uint64_t)Sensor::kFrameDurationRange[0]
};

const uint32_t EmulatedFakeCamera3::kAvailableProcessedSizesBack[6] = {
    640, 480, 320, 240,// 1280, 720
    //    Sensor::kResolution[0], Sensor::kResolution[1]
};

const uint32_t EmulatedFakeCamera3::kAvailableProcessedSizesFront[4] = {
    640, 480, 320, 240
    //    Sensor::kResolution[0], Sensor::kResolution[1]
};

const uint64_t EmulatedFakeCamera3::kAvailableProcessedMinDurations[1] = {
    (const uint64_t)Sensor::kFrameDurationRange[0]
};

const uint32_t EmulatedFakeCamera3::kAvailableJpegSizesBack[2] = {
    1280,720 
    //    Sensor::kResolution[0], Sensor::kResolution[1]
};

const uint32_t EmulatedFakeCamera3::kAvailableJpegSizesFront[2] = {
    640, 480
    //    Sensor::kResolution[0], Sensor::kResolution[1]
};


const uint64_t EmulatedFakeCamera3::kAvailableJpegMinDurations[1] = {
    (const uint64_t)Sensor::kFrameDurationRange[0]
};

/**
 * 3A constants
 */

// Default exposure and gain targets for different scenarios
const nsecs_t EmulatedFakeCamera3::kNormalExposureTime       = 10 * MSEC;
const nsecs_t EmulatedFakeCamera3::kFacePriorityExposureTime = 30 * MSEC;
const int     EmulatedFakeCamera3::kNormalSensitivity        = 100;
const int     EmulatedFakeCamera3::kFacePrioritySensitivity  = 400;
const float   EmulatedFakeCamera3::kExposureTrackRate        = 0.1;
const int     EmulatedFakeCamera3::kPrecaptureMinFrames      = 10;
const int     EmulatedFakeCamera3::kStableAeMaxFrames        = 100;
const float   EmulatedFakeCamera3::kExposureWanderMin        = -2;
const float   EmulatedFakeCamera3::kExposureWanderMax        = 1;

/**
 * Camera device lifecycle methods
 */
static const ssize_t kMinJpegBufferSize = 256 * 1024 + sizeof(camera3_jpeg_blob);
jpegsize EmulatedFakeCamera3::getMaxJpegResolution(uint32_t picSizes[],int count) {
    uint32_t maxJpegWidth = 0, maxJpegHeight = 0;
    jpegsize maxJpegResolution;
    for (int i=0; i < count; i+= 4) {
        uint32_t width = picSizes[i+1];
        uint32_t height = picSizes[i+2];
        if (picSizes[i+0] == HAL_PIXEL_FORMAT_BLOB &&
        (width * height > maxJpegWidth * maxJpegHeight)) {
            maxJpegWidth = width;
            maxJpegHeight = height;
        }
    }
    maxJpegResolution.width = maxJpegWidth;
    maxJpegResolution.height = maxJpegHeight;
    return maxJpegResolution;
}
ssize_t EmulatedFakeCamera3::getJpegBufferSize(int width, int height) {
    if (maxJpegResolution.width == 0) {
        return BAD_VALUE;
    }
    ssize_t maxJpegBufferSize = JpegCompressor::kMaxJpegSize;

#if PLATFORM_SDK_VERSION <= 22
    // Calculate final jpeg buffer size for the given resolution.
    float scaleFactor = ((float) (width * height)) /
            (maxJpegResolution.width * maxJpegResolution.height);
    ssize_t jpegBufferSize = scaleFactor * maxJpegBufferSize;
    // Bound the buffer size to [MIN_JPEG_BUFFER_SIZE, maxJpegBufferSize].
    if (jpegBufferSize > maxJpegBufferSize) {
        jpegBufferSize = maxJpegBufferSize;
    } else if (jpegBufferSize < kMinJpegBufferSize) {
        jpegBufferSize = kMinJpegBufferSize;
    }
#else
    assert(kMinJpegBufferSize < maxJpegBufferSize);
    // Calculate final jpeg buffer size for the given resolution.
    float scaleFactor = ((float) (width * height)) /
        (maxJpegResolution.width * maxJpegResolution.height);
    ssize_t jpegBufferSize = scaleFactor * (maxJpegBufferSize - kMinJpegBufferSize) +
        kMinJpegBufferSize;
    if (jpegBufferSize > maxJpegBufferSize)
        jpegBufferSize = maxJpegBufferSize;
#endif

    return jpegBufferSize;
}

EmulatedFakeCamera3::EmulatedFakeCamera3(int cameraId, struct hw_module_t* module) :
        EmulatedCamera3(cameraId, module) {
    ALOGI("Constructing emulated fake camera 3 cameraID:%d", mCameraID);

    for (size_t i = 0; i < CAMERA3_TEMPLATE_COUNT; i++) {
        mDefaultTemplates[i] = NULL;
    }

    /**
     * Front cameras = limited mode
     * Back cameras = full mode
     */
    //TODO limited or full mode, read this from camera driver
    //mFullMode = facingBack;
    mCameraStatus = CAMERA_INIT;
    mSupportCap = 0;
    mSupportRotate = 0;
    mFullMode = 0;
    mFlushTag = false;
    mPlugged = false;

}

EmulatedFakeCamera3::~EmulatedFakeCamera3() {
    for (size_t i = 0; i < CAMERA3_TEMPLATE_COUNT; i++) {
        if (mDefaultTemplates[i] != NULL) {
            free_camera_metadata(mDefaultTemplates[i]);
        }
    }

    if (mCameraInfo != NULL) {
        CAMHAL_LOGIA("free mCameraInfo");
        free_camera_metadata(mCameraInfo);
        mCameraInfo = NULL;
    }
}

status_t EmulatedFakeCamera3::Initialize() {
    DBG_LOGB("mCameraID=%d,mStatus=%d,ddd\n", mCameraID, mStatus);
    status_t res;

#ifdef HAVE_VERSION_INFO
    CAMHAL_LOGIB("\n--------------------------------\n"
                  "author:aml.sh multi-media team\n"
                  "branch name:   %s\n"
                  "git version:   %s \n"
                  "last changed:  %s\n"
                  "build-time:    %s\n"
                  "build-name:    %s\n"
                  "uncommitted-file-num:%d\n"
                  "ssh user@%s, cd %s\n"
                  "hostname %s\n"
                  "--------------------------------\n",
                  CAMHAL_BRANCH_NAME,
                  CAMHAL_GIT_VERSION,
                  CAMHAL_LAST_CHANGED,
                  CAMHAL_BUILD_TIME,
                  CAMHAL_BUILD_NAME,
                  CAMHAL_GIT_UNCOMMIT_FILE_NUM,
                  CAMHAL_IP, CAMHAL_PATH, CAMHAL_HOSTNAME
                  );
#endif


    if (mStatus != STATUS_ERROR) {
        ALOGE("%s: Already initialized!", __FUNCTION__);
        return INVALID_OPERATION;
    }

    res = constructStaticInfo();
    if (res != OK) {
        ALOGE("%s: Unable to allocate static info: %s (%d)",
                __FUNCTION__, strerror(-res), res);
        return res;
    }

    return EmulatedCamera3::Initialize();
}

status_t EmulatedFakeCamera3::connectCamera(hw_device_t** device) {
    ALOGV("%s: E", __FUNCTION__);
    DBG_LOGB("%s, ddd", __FUNCTION__);
    Mutex::Autolock l(mLock);
    status_t res;
    DBG_LOGB("%s , mStatus = %d" , __FUNCTION__, mStatus);

    if ((mStatus != STATUS_CLOSED) || !mPlugged) {
        ALOGE("%s: Can't connect in state %d, mPlugged=%d",
                __FUNCTION__, mStatus, mPlugged);
        return INVALID_OPERATION;
    }

    mSensor = new Sensor();
    mSensor->setSensorListener(this);

    res = mSensor->startUp(mCameraID);
    DBG_LOGB("mSensor startUp, mCameraID=%d\n", mCameraID);
    if (res != NO_ERROR) return res;

    mSupportCap = mSensor->IoctlStateProbe();
    if (mSupportCap & IOCTL_MASK_ROTATE) {
        mSupportRotate = true;
    }

    mReadoutThread = new ReadoutThread(this);
    mJpegCompressor = new JpegCompressor();

    res = mReadoutThread->setJpegCompressorListener(this);
    if (res != NO_ERROR) {
        return res;
    }
    res = mReadoutThread->startJpegCompressor(this);
    if (res != NO_ERROR) {
        return res;
    }

    res = mReadoutThread->run("EmuCam3::readoutThread");
    if (res != NO_ERROR) return res;

    // Initialize fake 3A

    mControlMode  = ANDROID_CONTROL_MODE_AUTO;
    mFacePriority = false;
    mAeMode       = ANDROID_CONTROL_AE_MODE_ON;
    mAfMode       = ANDROID_CONTROL_AF_MODE_AUTO;
    mAwbMode      = ANDROID_CONTROL_AWB_MODE_AUTO;
    mAeState      = ANDROID_CONTROL_AE_STATE_CONVERGED;//ANDROID_CONTROL_AE_STATE_INACTIVE;
    mAfState      = ANDROID_CONTROL_AF_STATE_INACTIVE;
    mAwbState     = ANDROID_CONTROL_AWB_STATE_INACTIVE;
    mAfTriggerId  = 0;
    mAeCurrentExposureTime = kNormalExposureTime;
    mAeCurrentSensitivity  = kNormalSensitivity;

    return EmulatedCamera3::connectCamera(device);
}

status_t EmulatedFakeCamera3::plugCamera() {
    {
        Mutex::Autolock l(mLock);

        if (!mPlugged) {
            CAMHAL_LOGIB("%s: Plugged back in", __FUNCTION__);
            mPlugged = true;
        }
    }

    return NO_ERROR;
}

status_t EmulatedFakeCamera3::unplugCamera() {
    {
        Mutex::Autolock l(mLock);

        if (mPlugged) {
            CAMHAL_LOGIB("%s: Unplugged camera", __FUNCTION__);
            mPlugged = false;
        }
    }
    return true;
}

camera_device_status_t EmulatedFakeCamera3::getHotplugStatus() {
    Mutex::Autolock l(mLock);
    return mPlugged ?
        CAMERA_DEVICE_STATUS_PRESENT :
        CAMERA_DEVICE_STATUS_NOT_PRESENT;
}

bool EmulatedFakeCamera3::getCameraStatus()
{
    CAMHAL_LOGVB("%s, mCameraStatus = %d",__FUNCTION__,mCameraStatus);
    bool ret = false;
    if (mStatus == STATUS_CLOSED) {
        ret = true;
    } else {
        ret = false;
    }
    return ret;
}

status_t EmulatedFakeCamera3::closeCamera() {
    DBG_LOGB("%s, %d\n", __FUNCTION__, __LINE__);
    status_t res;
    {
        Mutex::Autolock l(mLock);
        if (mStatus == STATUS_CLOSED) return OK;
    }

    CAMHAL_LOGDB("%s, %d\n", __FUNCTION__, __LINE__);
    mReadoutThread->sendFlushSingnal();
    mSensor->sendExitSingalToSensor();
    res = mSensor->shutDown();
    if (res != NO_ERROR) {
        ALOGE("%s: Unable to shut down sensor: %d", __FUNCTION__, res);
        return res;
    }
    mSensor.clear();
    CAMHAL_LOGDB("%s, %d\n", __FUNCTION__, __LINE__);

    {
        Mutex::Autolock l(mLock);
        res = mReadoutThread->shutdownJpegCompressor(this);
        if (res != OK) {
            ALOGE("%s: Unable to shut down JpegCompressor: %d", __FUNCTION__, res);
            return res;
        }
        mReadoutThread->sendExitReadoutThreadSignal();
        mReadoutThread->requestExit();
    }
    CAMHAL_LOGDB("%s, %d\n", __FUNCTION__, __LINE__);

    mReadoutThread->join();
    DBG_LOGA("Sucess exit ReadOutThread");
    {
        Mutex::Autolock l(mLock);
        // Clear out private stream information
        for (StreamIterator s = mStreams.begin(); s != mStreams.end(); s++) {
            PrivateStreamInfo *privStream =
                    static_cast<PrivateStreamInfo*>((*s)->priv);
            delete privStream;
            (*s)->priv = NULL;
        }
        mStreams.clear();
        mReadoutThread.clear();
    }
    CAMHAL_LOGDB("%s, %d\n", __FUNCTION__, __LINE__);
    return EmulatedCamera3::closeCamera();
}

status_t EmulatedFakeCamera3::getCameraInfo(struct camera_info *info) {
    char property[PROPERTY_VALUE_MAX];
    info->facing = mFacingBack ? CAMERA_FACING_BACK : CAMERA_FACING_FRONT;
    if (mSensorType == SENSOR_USB) {
        if (mFacingBack) {
            property_get("hw.camera.orientation.back", property, "0");
        } else {
            property_get("hw.camera.orientation.front", property, "0");
        }
        int32_t orientation = atoi(property);
        property_get("hw.camera.usb.orientation_offset", property, "0");
        orientation += atoi(property);
        orientation %= 360;
        info->orientation = orientation ;
    } else {
        if (mFacingBack) {
            property_get("hw.camera.orientation.back", property, "270");
        } else {
            property_get("hw.camera.orientation.front", property, "90");
        }
        info->orientation = atoi(property);
    }
    return EmulatedCamera3::getCameraInfo(info);
}

/**
 * Camera3 interface methods
 */

void EmulatedFakeCamera3::getValidJpegSize(uint32_t picSizes[], uint32_t availablejpegsize[], int count) {
    int i,j,k;
    bool valid = true;
    for (i=0,j=0; i < count; i+= 4) {
        for (k= 0; k<=j ;k+=2) {
            if ((availablejpegsize[k]*availablejpegsize[k+1]) == (picSizes[i+1]*picSizes[i+2])) {

                valid = false;
            }
        }
        if (valid) {
            availablejpegsize[j] = picSizes[i+1];
            availablejpegsize[j+1] = picSizes[i+2];
            j+=2;
        }
        valid = true;
    }
}

status_t EmulatedFakeCamera3::checkValidJpegSize(uint32_t width, uint32_t height) {

    int validsizecount = 0;
    uint32_t count = sizeof(mAvailableJpegSize)/sizeof(mAvailableJpegSize[0]);
    for (uint32_t f = 0; f < count; f+=2) {
        if (mAvailableJpegSize[f] != 0) {
            if ((mAvailableJpegSize[f] == width)&&(mAvailableJpegSize[f+1] == height)) {
                validsizecount++;
            }
        } else {
            break;
        }
    }
    if (validsizecount == 0)
        return BAD_VALUE;
    return OK;
}

status_t EmulatedFakeCamera3::configureStreams(
        camera3_stream_configuration *streamList) {
    Mutex::Autolock l(mLock);
    uint32_t width, height, pixelfmt;
    bool isRestart = false;
    mFlushTag = false;
    DBG_LOGB("%s: %d streams", __FUNCTION__, streamList->num_streams);

    if (mStatus != STATUS_OPEN && mStatus != STATUS_READY) {
        ALOGE("%s: Cannot configure streams in state %d",
                __FUNCTION__, mStatus);
        return NO_INIT;
    }

    /**
     * Sanity-check input list.
     */
    if (streamList == NULL) {
        ALOGE("%s: NULL stream configuration", __FUNCTION__);
        return BAD_VALUE;
    }

    if (streamList->streams == NULL) {
        ALOGE("%s: NULL stream list", __FUNCTION__);
        return BAD_VALUE;
    }

    if (streamList->num_streams < 1) {
        ALOGE("%s: Bad number of streams requested: %d", __FUNCTION__,
                streamList->num_streams);
        return BAD_VALUE;
    }

    camera3_stream_t *inputStream = NULL;
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];

        if (newStream == NULL) {
            ALOGE("%s: Stream index %zu was NULL",
                  __FUNCTION__, i);
            return BAD_VALUE;
        }

        if (newStream->max_buffers <= 0) {
            isRestart = true;//mSensor->isNeedRestart(newStream->width, newStream->height, newStream->format);
            DBG_LOGB("format=%x, w*h=%dx%d, stream_type=%d, max_buffers=%d, isRestart=%d\n",
                    newStream->format, newStream->width, newStream->height,
                    newStream->stream_type, newStream->max_buffers,
                    isRestart);
        }

        if ((newStream->width == 0) || (newStream->width == UINT32_MAX) ||
            (newStream->height == 0) || (newStream->height == UINT32_MAX)) {
            ALOGE("invalid width or height. \n");
            return -EINVAL;
        }

        if (newStream->rotation == INT32_MAX) {
            ALOGE("invalid StreamRotation. \n");
            return -EINVAL;
        }

        ALOGV("%s: Stream %p (id %zu), type %d, usage 0x%x, format 0x%x",
                __FUNCTION__, newStream, i, newStream->stream_type,
                newStream->usage,
                newStream->format);

        if (newStream->stream_type == CAMERA3_STREAM_INPUT ||
            newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
            if (inputStream != NULL) {

                ALOGE("%s: Multiple input streams requested!", __FUNCTION__);
                return BAD_VALUE;
            }
            inputStream = newStream;
        }

        bool validFormat = false;
        for (size_t f = 0;
             f < sizeof(kAvailableFormats)/sizeof(kAvailableFormats[0]);
             f++) {
            if (newStream->format == kAvailableFormats[f]) {
                validFormat = true;
                //HAL_PIXEL_FORMAT_YCrCb_420_SP,
                if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == newStream->format)
                    newStream->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;

                break;
            }
            DBG_LOGB("stream_type=%d\n", newStream->stream_type);
        }
        if (!validFormat) {
            ALOGE("%s: Unsupported stream format 0x%x requested",
                    __FUNCTION__, newStream->format);
            return -EINVAL;
        }

        status_t ret = checkValidJpegSize(newStream->width, newStream->height);
        if (ret != OK) {
            ALOGE("Invalid Jpeg Size. \n");
            return BAD_VALUE;
        }

    }
    mInputStream = inputStream;
    width = 0;
    height = 0;
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        DBG_LOGB("find propert width and height, format=%x, w*h=%dx%d, stream_type=%d, max_buffers=%d\n",
                newStream->format, newStream->width, newStream->height, newStream->stream_type, newStream->max_buffers);
        if ((HAL_PIXEL_FORMAT_BLOB != newStream->format) &&
            (CAMERA3_STREAM_OUTPUT == newStream->stream_type)) {

            if (width < newStream->width)
                    width = newStream->width;

            if (height < newStream->height)
                    height = newStream->height;

            pixelfmt = (uint32_t)newStream->format;
            if (HAL_PIXEL_FORMAT_YCbCr_420_888 == pixelfmt)
                pixelfmt =  HAL_PIXEL_FORMAT_YCrCb_420_SP;
        }

    }

    //TODO modify this ugly code
    if (isRestart) {
        isRestart = mSensor->isNeedRestart(width, height, pixelfmt);
    }

    if (isRestart) {
        mSensor->streamOff();
        pixelfmt = mSensor->halFormatToSensorFormat(pixelfmt);
        mSensor->setOutputFormat(width, height, pixelfmt, 0);
        mSensor->streamOn();
        DBG_LOGB("width=%d, height=%d, pixelfmt=%.4s\n",
                        width, height, (char*)&pixelfmt);
    }

    /**
     * Initially mark all existing streams as not alive
     */
    for (StreamIterator s = mStreams.begin(); s != mStreams.end(); ++s) {
        PrivateStreamInfo *privStream =
                static_cast<PrivateStreamInfo*>((*s)->priv);
        privStream->alive = false;
    }

    /**
     * Find new streams and mark still-alive ones
     */
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        if (newStream->priv == NULL) {
            // New stream, construct info
            PrivateStreamInfo *privStream = new PrivateStreamInfo();
            privStream->alive = true;
            privStream->registered = false;

            DBG_LOGB("stream_type=%d\n", newStream->stream_type);
            newStream->max_buffers = kMaxBufferCount;
            newStream->priv = privStream;
            mStreams.push_back(newStream);
        } else {
            // Existing stream, mark as still alive.
            PrivateStreamInfo *privStream =
                    static_cast<PrivateStreamInfo*>(newStream->priv);
            CAMHAL_LOGDA("Existing stream ?");
            privStream->alive = true;
        }
        // Always update usage and max buffers
        /*for cts CameraDeviceTest -> testPrepare*/
        newStream->max_buffers = kMaxBufferCount;
        newStream->usage = mSensor->getStreamUsage(newStream->stream_type);
        DBG_LOGB("%d, newStream=%p, stream_type=%d, usage=%x, priv=%p, w*h=%dx%d\n",
                (int)i, newStream, newStream->stream_type, newStream->usage, newStream->priv, newStream->width, newStream->height);
    }

    /**
     * Reap the dead streams
     */
    for (StreamIterator s = mStreams.begin(); s != mStreams.end();) {
        PrivateStreamInfo *privStream =
                static_cast<PrivateStreamInfo*>((*s)->priv);
        if (!privStream->alive) {
            DBG_LOGA("delete not alive streams");
            (*s)->priv = NULL;
            delete privStream;
            s = mStreams.erase(s);
        } else {
            ++s;
        }
    }

    /**
     * Can't reuse settings across configure call
     */
    mPrevSettings.clear();

    return OK;
}

status_t EmulatedFakeCamera3::registerStreamBuffers(
        const camera3_stream_buffer_set *bufferSet) {
    DBG_LOGB("%s: E", __FUNCTION__);
    Mutex::Autolock l(mLock);

    /**
     * Sanity checks
     */
    DBG_LOGA("==========sanity checks\n");

    // OK: register streams at any time during configure
    // (but only once per stream)
    if (mStatus != STATUS_READY && mStatus != STATUS_ACTIVE) {
        ALOGE("%s: Cannot register buffers in state %d",
                __FUNCTION__, mStatus);
        return NO_INIT;
    }

    if (bufferSet == NULL) {
        ALOGE("%s: NULL buffer set!", __FUNCTION__);
        return BAD_VALUE;
    }

    StreamIterator s = mStreams.begin();
    for (; s != mStreams.end(); ++s) {
        if (bufferSet->stream == *s) break;
    }
    if (s == mStreams.end()) {
        ALOGE("%s: Trying to register buffers for a non-configured stream!",
                __FUNCTION__);
        return BAD_VALUE;
    }

    /**
     * Register the buffers. This doesn't mean anything to the emulator besides
     * marking them off as registered.
     */

    PrivateStreamInfo *privStream =
            static_cast<PrivateStreamInfo*>((*s)->priv);

#if 0
    if (privStream->registered) {
        ALOGE("%s: Illegal to register buffer more than once", __FUNCTION__);
        return BAD_VALUE;
    }
#endif

    privStream->registered = true;

    return OK;
}

const camera_metadata_t* EmulatedFakeCamera3::constructDefaultRequestSettings(
        int type) {
    DBG_LOGB("%s: E", __FUNCTION__);
    Mutex::Autolock l(mLock);

    if (type < 0 || type >= CAMERA3_TEMPLATE_COUNT) {
        ALOGE("%s: Unknown request settings template: %d",
                __FUNCTION__, type);
        return NULL;
    }

    /**
     * Cache is not just an optimization - pointer returned has to live at
     * least as long as the camera device instance does.
     */
    if (mDefaultTemplates[type] != NULL) {
        return mDefaultTemplates[type];
    }

    CameraMetadata settings;

    /** android.request */
    static const uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    settings.update(ANDROID_REQUEST_TYPE, &requestType, 1);

    static const uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
    settings.update(ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);

    static const int32_t id = 0;
    settings.update(ANDROID_REQUEST_ID, &id, 1);

    static const int32_t frameCount = 0;
    settings.update(ANDROID_REQUEST_FRAME_COUNT, &frameCount, 1);

    /** android.lens */

    static const float focusDistance = 0;
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);

    static const float aperture = 2.8f;
    settings.update(ANDROID_LENS_APERTURE, &aperture, 1);

//    static const float focalLength = 5.0f;
	static const float focalLength = 3.299999952316284f;
    settings.update(ANDROID_LENS_FOCAL_LENGTH, &focalLength, 1);

    static const float filterDensity = 0;
    settings.update(ANDROID_LENS_FILTER_DENSITY, &filterDensity, 1);

    static const uint8_t opticalStabilizationMode =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
            &opticalStabilizationMode, 1);

    // FOCUS_RANGE set only in frame

    /** android.sensor */

    static const int32_t testAvailablePattern = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    settings.update(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES, &testAvailablePattern, 1);
    static const int32_t testPattern = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    settings.update(ANDROID_SENSOR_TEST_PATTERN_MODE, &testPattern, 1);
    static const int64_t exposureTime = 10 * MSEC;
    settings.update(ANDROID_SENSOR_EXPOSURE_TIME, &exposureTime, 1);

    int64_t frameDuration =  mSensor->getMinFrameDuration();
    if (mSensorType == SENSOR_USB) {
        if (frameDuration < (int64_t)33333333L)
            frameDuration = (int64_t)33333333L;
    }
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);

    static const int32_t sensitivity = 100;
    settings.update(ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);

    static const int64_t rollingShutterSkew = 0;
    settings.update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, &rollingShutterSkew, 1);
    // TIMESTAMP set only in frame

    /** android.flash */

	static const uint8_t flashstate = ANDROID_FLASH_STATE_UNAVAILABLE;
	settings.update(ANDROID_FLASH_STATE, &flashstate, 1);

    static const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    settings.update(ANDROID_FLASH_MODE, &flashMode, 1);

    static const uint8_t flashPower = 10;
    settings.update(ANDROID_FLASH_FIRING_POWER, &flashPower, 1);

    static const int64_t firingTime = 0;
    settings.update(ANDROID_FLASH_FIRING_TIME, &firingTime, 1);

    /** Processing block modes */
    uint8_t hotPixelMode = 0;
    uint8_t demosaicMode = 0;
    uint8_t noiseMode = 0;
    uint8_t shadingMode = 0;
    uint8_t colorMode = 0;
    uint8_t tonemapMode = 0;
    uint8_t edgeMode = 0;
    switch (type) {

      case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
      case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        noiseMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
        // fall-through
      case CAMERA3_TEMPLATE_STILL_CAPTURE:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        demosaicMode = ANDROID_DEMOSAIC_MODE_HIGH_QUALITY;
        shadingMode = ANDROID_SHADING_MODE_HIGH_QUALITY;
        colorMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_HIGH_QUALITY;
        edgeMode = ANDROID_EDGE_MODE_HIGH_QUALITY;
        break;
      case CAMERA3_TEMPLATE_PREVIEW:
        // fall-through
      case CAMERA3_TEMPLATE_VIDEO_RECORD:
        // fall-through
      case CAMERA3_TEMPLATE_MANUAL:
        // fall-through
      default:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
        demosaicMode = ANDROID_DEMOSAIC_MODE_FAST;
        noiseMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorMode = ANDROID_COLOR_CORRECTION_MODE_FAST;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    }
    settings.update(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
    settings.update(ANDROID_DEMOSAIC_MODE, &demosaicMode, 1);
    settings.update(ANDROID_NOISE_REDUCTION_MODE, &noiseMode, 1);
    settings.update(ANDROID_SHADING_MODE, &shadingMode, 1);
    settings.update(ANDROID_COLOR_CORRECTION_MODE, &colorMode, 1);
    settings.update(ANDROID_TONEMAP_MODE, &tonemapMode, 1);
    settings.update(ANDROID_EDGE_MODE, &edgeMode, 1);

    /** android.noise */
    static const uint8_t noiseStrength = 5;
    settings.update(ANDROID_NOISE_REDUCTION_STRENGTH, &noiseStrength, 1);
    static uint8_t availableNBModes[] = {
        ANDROID_NOISE_REDUCTION_MODE_OFF,
        ANDROID_NOISE_REDUCTION_MODE_FAST,
        ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY,
    };
    settings.update(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
            availableNBModes, sizeof(availableNBModes)/sizeof(availableNBModes));


    /** android.color */
#if PLATFORM_SDK_VERSION >= 23
    static const camera_metadata_rational colorTransform[9] = {
        {1, 1}, {0, 1}, {0, 1},
        {0, 1}, {1, 1}, {0, 1},
        {0, 1}, {0, 1}, {1, 1}
    };
    settings.update(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);
#else
    static const float colorTransform[9] = {
        1.0f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };
    settings.update(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);
#endif
    /** android.tonemap */
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };
    settings.update(ANDROID_TONEMAP_CURVE_RED, tonemapCurve, 4);
    settings.update(ANDROID_TONEMAP_CURVE_GREEN, tonemapCurve, 4);
    settings.update(ANDROID_TONEMAP_CURVE_BLUE, tonemapCurve, 4);

    /** android.edge */
    static const uint8_t edgeStrength = 5;
    settings.update(ANDROID_EDGE_STRENGTH, &edgeStrength, 1);

    /** android.scaler */
    static const uint8_t croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    settings.update(ANDROID_SCALER_CROPPING_TYPE, &croppingType, 1);

    static const int32_t cropRegion[] = {
        0, 0, (int32_t)Sensor::kResolution[0], (int32_t)Sensor::kResolution[1],
    };
    settings.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);

    /** android.jpeg */
    static const uint8_t jpegQuality = 80;
    settings.update(ANDROID_JPEG_QUALITY, &jpegQuality, 1);

    static const int32_t thumbnailSize[2] = {
        320, 240
    };
    settings.update(ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);

    static const uint8_t thumbnailQuality = 80;
    settings.update(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbnailQuality, 1);

    static const double gpsCoordinates[3] = {
        0, 0, 0
    };
    settings.update(ANDROID_JPEG_GPS_COORDINATES, gpsCoordinates, 3); //default 2 value

    static const uint8_t gpsProcessingMethod[32] = "None";
    settings.update(ANDROID_JPEG_GPS_PROCESSING_METHOD, gpsProcessingMethod, 32);

    static const int64_t gpsTimestamp = 0;
    settings.update(ANDROID_JPEG_GPS_TIMESTAMP, &gpsTimestamp, 1);

    static const int32_t jpegOrientation = 0;
    settings.update(ANDROID_JPEG_ORIENTATION, &jpegOrientation, 1);

    /** android.stats */

    static const uint8_t faceDetectMode =
        ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    settings.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);

    static const uint8_t histogramMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    settings.update(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);

    static const uint8_t sharpnessMapMode =
        ANDROID_STATISTICS_SHARPNESS_MAP_MODE_OFF;
    settings.update(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);

    static const uint8_t hotPixelMapMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    settings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE,&hotPixelMapMode, 1);
    static const uint8_t sceneFlicker = ANDROID_STATISTICS_SCENE_FLICKER_NONE;
    settings.update(ANDROID_STATISTICS_SCENE_FLICKER,&sceneFlicker, 1);
    static const uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,&lensShadingMapMode, 1);
    // faceRectangles, faceScores, faceLandmarks, faceIds, histogram,
    // sharpnessMap only in frames

    /** android.control */

    uint8_t controlIntent = 0;
    uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO; //default value
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    switch (type) {
      case CAMERA3_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        break;
      case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        break;
      case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        break;
      case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        break;
      case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        break;
      case CAMERA3_TEMPLATE_MANUAL:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_MANUAL;
        controlMode = ANDROID_CONTROL_MODE_OFF;
        aeMode = ANDROID_CONTROL_AE_MODE_OFF;
        awbMode = ANDROID_CONTROL_AWB_MODE_OFF;
        break;
      default:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }
    settings.update(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);
    settings.update(ANDROID_CONTROL_MODE, &controlMode, 1);

    static const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    settings.update(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    static const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY;
    settings.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    settings.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    static const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

    static const uint8_t aePrecaptureTrigger =
        ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePrecaptureTrigger, 1);

    static const int32_t mAfTriggerId = 0;
    settings.update(ANDROID_CONTROL_AF_TRIGGER_ID,&mAfTriggerId, 1);
    static const uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);

    //static const int32_t controlRegions[5] = {
    //    0, 0, (int32_t)Sensor::kResolution[0], (int32_t)Sensor::kResolution[1],
    //    1000
    //};
//    settings.update(ANDROID_CONTROL_AE_REGIONS, controlRegions, 5);

    static const int32_t aeExpCompensation = 0;
    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExpCompensation, 1);

    static const int32_t aeTargetFpsRange[2] = {
        30, 30
    };
    settings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, aeTargetFpsRange, 2);

    static const uint8_t aeAntibandingMode =
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    settings.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &aeAntibandingMode, 1);

    settings.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    static const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

//    settings.update(ANDROID_CONTROL_AWB_REGIONS, controlRegions, 5);

    uint8_t afMode = 0;
    switch (type) {
      case CAMERA3_TEMPLATE_PREVIEW:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        break;
      case CAMERA3_TEMPLATE_STILL_CAPTURE:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        break;
      case CAMERA3_TEMPLATE_VIDEO_RECORD:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        //afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        //afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        break;
      case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        //afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        break;
      case CAMERA3_TEMPLATE_MANUAL:
        afMode = ANDROID_CONTROL_AF_MODE_OFF;
        break;
      default:
        afMode = ANDROID_CONTROL_AF_MODE_AUTO;
        break;
    }
    settings.update(ANDROID_CONTROL_AF_MODE, &afMode, 1);

	static const uint8_t afstate = ANDROID_CONTROL_AF_STATE_INACTIVE;
	settings.update(ANDROID_CONTROL_AF_STATE,&afstate,1);

//    settings.update(ANDROID_CONTROL_AF_REGIONS, controlRegions, 5);

    static const uint8_t aestate = ANDROID_CONTROL_AE_STATE_CONVERGED;
    settings.update(ANDROID_CONTROL_AE_STATE,&aestate,1);
    static const uint8_t awbstate = ANDROID_CONTROL_AWB_STATE_INACTIVE;
    settings.update(ANDROID_CONTROL_AWB_STATE,&awbstate,1);
    static const uint8_t vstabMode =
        ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstabMode, 1);

    // aeState, awbState, afState only in frame

    uint8_t aberrationMode = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;
    settings.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
        &aberrationMode, 1);

    mDefaultTemplates[type] = settings.release();

    return mDefaultTemplates[type];
}

status_t EmulatedFakeCamera3::processCaptureRequest(
        camera3_capture_request *request) {
    status_t res;
    nsecs_t  exposureTime;
    //nsecs_t  frameDuration;
    uint32_t sensitivity;
    uint32_t frameNumber;
    bool mHaveThumbnail = false;
    CameraMetadata settings;
    Buffers *sensorBuffers = NULL;
    HalBufferVector *buffers = NULL;

    if (mFlushTag) {
       DBG_LOGA("already flush, but still send Capture Request .\n");
    }

    {
      Mutex::Autolock l(mLock);

    /** Validation */

      if (mStatus < STATUS_READY) {
         ALOGE("%s: Can't submit capture requests in state %d", __FUNCTION__,
                           mStatus);
         return INVALID_OPERATION;
      }

      if (request == NULL) {
         ALOGE("%s: NULL request!", __FUNCTION__);
         return BAD_VALUE;
      }

      frameNumber = request->frame_number;

      if (request->settings == NULL && mPrevSettings.isEmpty()) {
         ALOGE("%s: Request %d: NULL settings for first request after"
                  "configureStreams()", __FUNCTION__, frameNumber);
         return BAD_VALUE;
      }

      if (request->input_buffer != NULL &&
                  request->input_buffer->stream != mInputStream) {
         DBG_LOGB("%s: Request %d: Input buffer not from input stream!",
                  __FUNCTION__, frameNumber);
         DBG_LOGB("%s: Bad stream %p, expected: %p",
                  __FUNCTION__, request->input_buffer->stream,
                  mInputStream);
         DBG_LOGB("%s: Bad stream type %d, expected stream type %d",
                  __FUNCTION__, request->input_buffer->stream->stream_type,
                  mInputStream ? mInputStream->stream_type : -1);

         return BAD_VALUE;
      }

      if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
         ALOGE("%s: Request %d: No output buffers provided!",
                  __FUNCTION__, frameNumber);
         return BAD_VALUE;
      }

    // Validate all buffers, starting with input buffer if it's given

      ssize_t idx;
      const camera3_stream_buffer_t *b;
      if (request->input_buffer != NULL) {
         idx = -1;
         b = request->input_buffer;
      } else {
         idx = 0;
         b = request->output_buffers;
      }
      do {
         PrivateStreamInfo *priv =
               static_cast<PrivateStreamInfo*>(b->stream->priv);
         if (priv == NULL) {
               ALOGE("%s: Request %d: Buffer %zu: Unconfigured stream!",
               __FUNCTION__, frameNumber, idx);
               return BAD_VALUE;
         }
#if 0
      if (!priv->alive || !priv->registered) {
         ALOGE("%s: Request %d: Buffer %zu: Unregistered or dead stream! alive=%d, registered=%d\n",
               __FUNCTION__, frameNumber, idx,
               priv->alive, priv->registered);
         //return BAD_VALUE;
      }
#endif
         if (b->status != CAMERA3_BUFFER_STATUS_OK) {
             ALOGE("%s: Request %d: Buffer %zu: Status not OK!",
                   __FUNCTION__, frameNumber, idx);
             return BAD_VALUE;
         }
         if (b->release_fence != -1) {
            ALOGE("%s: Request %d: Buffer %zu: Has a release fence!",
                  __FUNCTION__, frameNumber, idx);
            return BAD_VALUE;
         }
         if (b->buffer == NULL) {
            ALOGE("%s: Request %d: Buffer %zu: NULL buffer handle!",
                 __FUNCTION__, frameNumber, idx);
            return BAD_VALUE;
         }
         idx++;
         b = &(request->output_buffers[idx]);
      } while (idx < (ssize_t)request->num_output_buffers);

      // TODO: Validate settings parameters

      /**
      * Start processing this request
   */
      mStatus = STATUS_ACTIVE;

      camera_metadata_entry e;

      if (request->settings == NULL) {
         settings.acquire(mPrevSettings);
      } else {
         settings = request->settings;

         uint8_t  antiBanding = 0;
         uint8_t effectMode = 0;
         int exposureCmp = 0;
         int32_t previewFpsRange[2];

         e = settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
         if (e.count == 0) {
              ALOGE("%s: get ANDROID_CONTROL_AE_TARGET_FPS_RANGE failed!", __FUNCTION__);
              return BAD_VALUE;
         } else {
              previewFpsRange[0] = e.data.i32[0];
              previewFpsRange[1] = e.data.i32[1];
              mFrameDuration =  1000000000 / previewFpsRange[1];
              ALOGI("set ANDROID_CONTROL_AE_TARGET_FPS_RANGE :%d,%d", previewFpsRange[0], previewFpsRange[1]);
         }

         e = settings.find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
         if (e.count == 0) {
              ALOGE("%s: No antibanding entry!", __FUNCTION__);
              return BAD_VALUE;
         }
         antiBanding = e.data.u8[0];
         mSensor->setAntiBanding(antiBanding);

         e = settings.find(ANDROID_CONTROL_EFFECT_MODE);
         if (e.count == 0) {
              ALOGE("%s: No antibanding entry!", __FUNCTION__);
              return BAD_VALUE;
         }
         effectMode = e.data.u8[0];
         mSensor->setEffect(effectMode);

         e = settings.find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
         if (e.count == 0) {
              ALOGE("%s: No exposure entry!", __FUNCTION__);
              //return BAD_VALUE;
         } else {
              exposureCmp = e.data.i32[0];
              DBG_LOGB("set expsore compensaton %d\n", exposureCmp);
              mSensor->setExposure(exposureCmp);
         }

         int32_t cropRegion[4];
         int32_t cropWidth;
         int32_t outputWidth = request->output_buffers[0].stream->width;

         e = settings.find(ANDROID_SCALER_CROP_REGION);
         if (e.count == 0) {
              ALOGE("%s: No corp region entry!", __FUNCTION__);
              //return BAD_VALUE;
         } else {
              cropRegion[0] = e.data.i32[0];
              cropRegion[1] = e.data.i32[1];
              cropWidth = cropRegion[2] = e.data.i32[2];
              cropRegion[3] = e.data.i32[3];
              for (int i = mZoomMin; i <= mZoomMax; i += mZoomStep) {
                   //if ( (float) i / mZoomMin >= (float) outputWidth / cropWidth) {
                   if ( i * cropWidth >= outputWidth * mZoomMin ) {
                         mSensor->setZoom(i);
                         break;
                   }
              }
              DBG_LOGB("cropRegion:%d, %d, %d, %d\n", cropRegion[0], cropRegion[1],cropRegion[2],cropRegion[3]);
         }
      }

      res = process3A(settings);
      if (res != OK) {
              ALOGVV("%s: process3A failed!", __FUNCTION__);
              //return res;
      }

      // TODO: Handle reprocessing

      /**
      * Get ready for sensor config
    */

      bool     needJpeg = false;
      ssize_t jpegbuffersize;
      uint32_t jpegpixelfmt;

      exposureTime = settings.find(ANDROID_SENSOR_EXPOSURE_TIME).data.i64[0];
      //frameDuration = settings.find(ANDROID_SENSOR_FRAME_DURATION).data.i64[0];
      sensitivity = settings.find(ANDROID_SENSOR_SENSITIVITY).data.i32[0];

      sensorBuffers = new Buffers();
      buffers = new HalBufferVector();

      sensorBuffers->setCapacity(request->num_output_buffers);
      buffers->setCapacity(request->num_output_buffers);

      // Process all the buffers we got for output, constructing internal buffer
      // structures for them, and lock them for writing.
      for (size_t i = 0; i < request->num_output_buffers; i++) {
              const camera3_stream_buffer &srcBuf = request->output_buffers[i];
              const private_handle_t *privBuffer =
                      (const private_handle_t*)(*srcBuf.buffer);
              StreamBuffer destBuf;
              destBuf.streamId = kGenericStreamId;
              destBuf.width    = srcBuf.stream->width;
              destBuf.height   = srcBuf.stream->height;
              destBuf.format   = privBuffer->format; // Use real private format
              destBuf.stride   = privBuffer->stride; //srcBuf.stream->width; // TODO: query from gralloc
              destBuf.buffer   = srcBuf.buffer;
              destBuf.share_fd = privBuffer->share_fd;

              if (destBuf.format == HAL_PIXEL_FORMAT_BLOB) {
                     needJpeg = true;
                     memset(&info,0,sizeof(struct ExifInfo));
                     info.orientation = settings.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
                     jpegpixelfmt = mSensor->getOutputFormat();
                     if (!mSupportRotate) {
                            info.mainwidth = srcBuf.stream->width;
                            info.mainheight = srcBuf.stream->height;
                     } else {
                            if ((info.orientation == 90)  || (info.orientation == 270)) {
                                info.mainwidth = srcBuf.stream->height;
                                info.mainheight = srcBuf.stream->width;
                            } else {
                                info.mainwidth = srcBuf.stream->width;
                                info.mainheight = srcBuf.stream->height;
                            }
                     }
                     if ((jpegpixelfmt == V4L2_PIX_FMT_MJPEG) || (jpegpixelfmt == V4L2_PIX_FMT_YUYV)) {
                            mSensor->setOutputFormat(info.mainwidth,info.mainheight,jpegpixelfmt,1);
                     } else {
                            mSensor->setOutputFormat(info.mainwidth,info.mainheight,V4L2_PIX_FMT_RGB24,1);
                     }
              }

              // Wait on fence
              sp<Fence> bufferAcquireFence = new Fence(srcBuf.acquire_fence);
              res = bufferAcquireFence->wait(kFenceTimeoutMs);
              if (res == TIMED_OUT) {
                     ALOGE("%s: Request %d: Buffer %zu: Fence timed out after %d ms",
                                __FUNCTION__, frameNumber, i, kFenceTimeoutMs);
              }
              if (res == OK) {
                     // Lock buffer for writing
                     const Rect rect(destBuf.width, destBuf.height);
                     if (srcBuf.stream->format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
                           if (privBuffer->format == HAL_PIXEL_FORMAT_YCbCr_420_888/*HAL_PIXEL_FORMAT_YCrCb_420_SP*/) {
                                  android_ycbcr ycbcr = android_ycbcr();
                                  res = GraphicBufferMapper::get().lockYCbCr(
                                      *(destBuf.buffer),
                                      GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK,
                                      rect,
                                      &ycbcr);
                                  // This is only valid because we know that emulator's
                                  // YCbCr_420_888 is really contiguous NV21 under the hood
                                  destBuf.img = static_cast<uint8_t*>(ycbcr.y);
                           } else {
                                  ALOGE("Unexpected private format for flexible YUV: 0x%x",
                                             privBuffer->format);
                                  res = INVALID_OPERATION;
                           }
                     } else {
                           res = GraphicBufferMapper::get().lock(*(destBuf.buffer),
                                 GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK,
                                 rect,
                                 (void**)&(destBuf.img));
                     }
                     if (res != OK) {
                           ALOGE("%s: Request %d: Buffer %zu: Unable to lock buffer",
                                  __FUNCTION__, frameNumber, i);
                     }
              }

              if (res != OK) {
                     // Either waiting or locking failed. Unlock locked buffers and bail
                     // out.
                     for (size_t j = 0; j < i; j++) {
                            GraphicBufferMapper::get().unlock(
                                     *(request->output_buffers[i].buffer));
                     }
                     ALOGE("line:%d, format for this usage: %d x %d, usage %x, format=%x, returned\n",
                              __LINE__, destBuf.width, destBuf.height, privBuffer->usage, privBuffer->format);
                     return NO_INIT;
              }
              sensorBuffers->push_back(destBuf);
              buffers->push_back(srcBuf);
      }

      if (needJpeg) {
              if (!mSupportRotate) {
                   info.thumbwidth = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[0];
                   info.thumbheight = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[1];
              } else {
                   if ((info.orientation == 90) || (info.orientation == 270)) {
                          info.thumbwidth = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[1];
                          info.thumbheight = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[0];
                   } else {
                          info.thumbwidth = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[0];
                          info.thumbheight = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[1];
                   }
              }
              if (settings.exists(ANDROID_JPEG_GPS_COORDINATES)) {
                   info.latitude = settings.find(ANDROID_JPEG_GPS_COORDINATES).data.d[0];
                   info.longitude = settings.find(ANDROID_JPEG_GPS_COORDINATES).data.d[1];
                   info.altitude = settings.find(ANDROID_JPEG_GPS_COORDINATES).data.d[2];
                   info.has_latitude = true;
                   info.has_longitude = true;
                   info.has_altitude = true;
              } else {
                   info.has_latitude = false;
                   info.has_longitude = false;
                   info.has_altitude = false;
              }
              if (settings.exists(ANDROID_JPEG_GPS_PROCESSING_METHOD)) {
                   uint8_t * gpsString = settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).data.u8;
                   memcpy(info.gpsProcessingMethod, gpsString , sizeof(info.gpsProcessingMethod)-1);
                   info.has_gpsProcessingMethod = true;
              } else {
                   info.has_gpsProcessingMethod = false;
              }
              if (settings.exists(ANDROID_JPEG_GPS_TIMESTAMP)) {
                   info.gpsTimestamp = settings.find(ANDROID_JPEG_GPS_TIMESTAMP).data.i64[0];
                   info.has_gpsTimestamp = true;
              } else {
                   info.has_gpsTimestamp = false;
              }
              if (settings.exists(ANDROID_LENS_FOCAL_LENGTH)) {
                   info.focallen = settings.find(ANDROID_LENS_FOCAL_LENGTH).data.f[0];
                   info.has_focallen = true;
              } else {
                   info.has_focallen = false;
              }
              jpegbuffersize = getJpegBufferSize(info.mainwidth,info.mainheight);

              mJpegCompressor->SetMaxJpegBufferSize(jpegbuffersize);
              mJpegCompressor->SetExifInfo(info);
              mSensor->setPictureRotate(info.orientation);
              if ((info.thumbwidth > 0) && (info.thumbheight > 0)) {
                   mHaveThumbnail = true;
              }
              DBG_LOGB("%s::thumbnailSize_width=%d,thumbnailSize_height=%d,mainsize_width=%d,mainsize_height=%d,jpegOrientation=%d",__FUNCTION__,
                                      info.thumbwidth,info.thumbheight,info.mainwidth,info.mainheight,info.orientation);
      }
      /**
      * Wait for JPEG compressor to not be busy, if needed
   */
#if 0
      if (needJpeg) {
              bool ready = mJpegCompressor->waitForDone(kFenceTimeoutMs);
              if (!ready) {
                     ALOGE("%s: Timeout waiting for JPEG compression to complete!",
                                __FUNCTION__);
                     return NO_INIT;
              }
      }
#else
      while (needJpeg) {
              bool ready = mJpegCompressor->waitForDone(kFenceTimeoutMs);
              if (ready) {
                     break;
              }
      }
#endif
    }
      /**
      * Wait until the in-flight queue has room
    */
      res = mReadoutThread->waitForReadout();
      if (res != OK) {
              ALOGE("%s: Timeout waiting for previous requests to complete!",
                            __FUNCTION__);
              return NO_INIT;
      }

      /**
      * Wait until sensor's ready. This waits for lengthy amounts of time with
      * mLock held, but the interface spec is that no other calls may by done to
      * the HAL by the framework while process_capture_request is happening.
   */
    {
        Mutex::Autolock l(mLock);
        int syncTimeoutCount = 0;
        while (!mSensor->waitForVSync(kSyncWaitTimeout)) {
              if (mStatus == STATUS_ERROR) {
                    return NO_INIT;
              }
              if (syncTimeoutCount == kMaxSyncTimeoutCount) {
                    ALOGE("%s: Request %d: Sensor sync timed out after %" PRId64 " ms",
                            __FUNCTION__, frameNumber,
                            kSyncWaitTimeout * kMaxSyncTimeoutCount / 1000000);
                    return NO_INIT;
              }
              syncTimeoutCount++;
        }

        /**
        * Configure sensor and queue up the request to the readout thread
     */
        mSensor->setExposureTime(exposureTime);
        //mSensor->setFrameDuration(frameDuration);
        mSensor->setFrameDuration(mFrameDuration);
        mSensor->setSensitivity(sensitivity);
        mSensor->setDestinationBuffers(sensorBuffers);
        mSensor->setFrameNumber(request->frame_number);

        ReadoutThread::Request r;
        r.frameNumber = request->frame_number;
        r.settings = settings;
        r.sensorBuffers = sensorBuffers;
        r.buffers = buffers;
        r.havethumbnail = mHaveThumbnail;

        mReadoutThread->queueCaptureRequest(r);
        ALOGVV("%s: Queued frame %d", __FUNCTION__, request->frame_number);

        // Cache the settings for next time
        mPrevSettings.acquire(settings);
    }
    CAMHAL_LOGVB("%s ,  X" , __FUNCTION__);
    return OK;
}

/** Debug methods */

void EmulatedFakeCamera3::dump(int fd) {

    String8 result;
    uint32_t count = sizeof(mAvailableJpegSize)/sizeof(mAvailableJpegSize[0]);
    result = String8::format("%s, valid resolution\n", __FILE__);

    for (uint32_t f = 0; f < count; f+=2) {
        if (mAvailableJpegSize[f] == 0)
            break;
        result.appendFormat("width: %d , height =%d\n",
            mAvailableJpegSize[f], mAvailableJpegSize[f+1]);
    }
    result.appendFormat("\nmZoomMin: %d , mZoomMax =%d, mZoomStep=%d\n",
                            mZoomMin, mZoomMax, mZoomStep);

    if (mZoomStep <= 0) {
        result.appendFormat("!!!!!!!!!camera apk may have no picture out\n");
    }

    write(fd, result.string(), result.size());

    if (mSensor.get() != NULL) {
        mSensor->dump(fd);
    }

}
//flush all request
//TODO returned buffers every request held immediately with
//CAMERA3_BUFFER_STATUS_ERROR flag.
int EmulatedFakeCamera3::flush_all_requests() {
    DBG_LOGA("flush all request");
    mFlushTag = true;
    mReadoutThread->flushAllRequest(true);
    mReadoutThread->setFlushFlag(false);
    mSensor->setFlushFlag(false);
    return 0;
}
/** Tag query methods */
const char* EmulatedFakeCamera3::getVendorSectionName(uint32_t tag) {
    return NULL;
}

const char* EmulatedFakeCamera3::getVendorTagName(uint32_t tag) {
    return NULL;
}

int EmulatedFakeCamera3::getVendorTagType(uint32_t tag) {
    return 0;
}

/**
 * Private methods
 */

camera_metadata_ro_entry_t EmulatedFakeCamera3::staticInfo(const CameraMetadata *info, uint32_t tag,
        size_t minCount, size_t maxCount, bool required) const {

    camera_metadata_ro_entry_t entry = info->find(tag);

    if (CC_UNLIKELY( entry.count == 0 ) && required) {
        const char* tagSection = get_camera_metadata_section_name(tag);
        if (tagSection == NULL) tagSection = "<unknown>";
        const char* tagName = get_camera_metadata_tag_name(tag);
        if (tagName == NULL) tagName = "<unknown>";

        ALOGE("Error finding static metadata entry '%s.%s' (%x)",
                tagSection, tagName, tag);
    } else if (CC_UNLIKELY(
            (minCount != 0 && entry.count < minCount) ||
            (maxCount != 0 && entry.count > maxCount) ) ) {
        const char* tagSection = get_camera_metadata_section_name(tag);
        if (tagSection == NULL) tagSection = "<unknown>";
        const char* tagName = get_camera_metadata_tag_name(tag);
        if (tagName == NULL) tagName = "<unknown>";
        ALOGE("Malformed static metadata entry '%s.%s' (%x):"
                "Expected between %zu and %zu values, but got %zu values",
                tagSection, tagName, tag, minCount, maxCount, entry.count);
    }

    return entry;
}

//this is only for debug
void EmulatedFakeCamera3::getStreamConfigurationp(CameraMetadata *info) {
    const int STREAM_CONFIGURATION_SIZE = 4;
    const int STREAM_FORMAT_OFFSET = 0;
    const int STREAM_WIDTH_OFFSET = 1;
    const int STREAM_HEIGHT_OFFSET = 2;
    const int STREAM_IS_INPUT_OFFSET = 3;

    camera_metadata_ro_entry_t availableStreamConfigs =
                staticInfo(info, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    CAMHAL_LOGDB(" stream, availableStreamConfigs.count=%zu\n", availableStreamConfigs.count);

    for (size_t i=0; i < availableStreamConfigs.count; i+= STREAM_CONFIGURATION_SIZE) {
        int32_t format = availableStreamConfigs.data.i32[i + STREAM_FORMAT_OFFSET];
        int32_t width = availableStreamConfigs.data.i32[i + STREAM_WIDTH_OFFSET];
        int32_t height = availableStreamConfigs.data.i32[i + STREAM_HEIGHT_OFFSET];
        int32_t isInput = availableStreamConfigs.data.i32[i + STREAM_IS_INPUT_OFFSET];
        CAMHAL_LOGDB("f=%x, w*h=%dx%d, du=%d\n", format, width, height, isInput);
    }

}

//this is only for debug
void EmulatedFakeCamera3::getStreamConfigurationDurations(CameraMetadata *info) {
    const int STREAM_CONFIGURATION_SIZE = 4;
    //const int STREAM_FORMAT_OFFSET = 0;
    //const int STREAM_WIDTH_OFFSET = 1;
    //const int STREAM_HEIGHT_OFFSET = 2;
    //const int STREAM_IS_INPUT_OFFSET = 3;

    camera_metadata_ro_entry_t availableStreamConfigs =
                staticInfo(info, ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS);
    CAMHAL_LOGDB("availableStreamConfigs.count=%zu\n", availableStreamConfigs.count);

    for (size_t i=0; i < availableStreamConfigs.count; i+= STREAM_CONFIGURATION_SIZE) {
        //availableStreamConfigs.data.i64[i + STREAM_FORMAT_OFFSET];
        //availableStreamConfigs.data.i64[i + STREAM_WIDTH_OFFSET];
        //availableStreamConfigs.data.i64[i + STREAM_HEIGHT_OFFSET];
        //availableStreamConfigs.data.i64[i + STREAM_IS_INPUT_OFFSET];
        //CAMHAL_LOGDB("f=%lx, w*h=%ldx%ld, du=%ld\n", format, width, height, isInput);
    }
}

void EmulatedFakeCamera3::updateCameraMetaData(CameraMetadata *info) {

}

status_t EmulatedFakeCamera3::constructStaticInfo() {

    status_t ret = OK;
    CameraMetadata info;
    uint32_t picSizes[64 * 8];
    int64_t* duration = NULL;
    int count, duration_count, availablejpegsize;
    uint8_t maxCount = 10;
    char property[PROPERTY_VALUE_MAX];
    unsigned int supportrotate;
    availablejpegsize = ARRAY_SIZE(mAvailableJpegSize);
    memset(mAvailableJpegSize,0,(sizeof(uint32_t))*availablejpegsize);
    sp<Sensor> s = new Sensor();
    ret = s->startUp(mCameraID);
    if (ret != OK) {
        DBG_LOGA("sensor start up failed");
        return ret;
    }

    mSensorType = s->getSensorType();

    if ( mSensorType == SENSOR_USB) {
        char property[PROPERTY_VALUE_MAX];
        property_get("ro.media.camera_usb.faceback", property, "false");
        if (strstr(property, "true"))
            mFacingBack = 1;
        else
            mFacingBack = 0;
        ALOGI("Setting usb camera cameraID:%d to back camera:%s\n",
                     mCameraID, property);
    } else {
        if (s->mSensorFace == SENSOR_FACE_FRONT) {
            mFacingBack = 0;
        } else if (s->mSensorFace == SENSOR_FACE_BACK) {
            mFacingBack = 1;
        } else if (s->mSensorFace == SENSOR_FACE_NONE) {
            if (gEmulatedCameraFactory.getEmulatedCameraNum() == 1) {
                mFacingBack = 1;
            } else if ( mCameraID == 0) {
                mFacingBack = 1;
            } else {
                mFacingBack = 0;
            }
        }

        ALOGI("Setting on board camera cameraID:%d to back camera:%d[0 false, 1 true]\n",
                     mCameraID, mFacingBack);
    }

    mSupportCap = s->IoctlStateProbe();
    if (mSupportCap & IOCTL_MASK_ROTATE) {
        supportrotate = true;
    } else {
        supportrotate = false;
    }
    // android.lens

    // 5 cm min focus distance for back camera, infinity (fixed focus) for front
    // TODO read this ioctl from camera driver
    DBG_LOGB("mCameraID=%d,mCameraInfo=%p\n", mCameraID, mCameraInfo);
    const float minFocusDistance = 0.0;
    info.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
            &minFocusDistance, 1);

    // 5 m hyperfocal distance for back camera, infinity (fixed focus) for front
    //const float hyperFocalDistance = mFacingBack ? 1.0/5.0 : 0.0;
    info.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
            &minFocusDistance, 1);

    static const float focalLength = 3.30f; // mm
    info.update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
            &focalLength, 1);
    static const float aperture = 2.8f;
    info.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
            &aperture, 1);
    static const float filterDensity = 0;
    info.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
            &filterDensity, 1);
    static const uint8_t availableOpticalStabilization =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    info.update(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
            &availableOpticalStabilization, 1);

    static const int32_t lensShadingMapSize[] = {1, 1};
    info.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE, lensShadingMapSize,
            sizeof(lensShadingMapSize)/sizeof(int32_t));

    /*lens facing related camera feature*/
    /*camera feature setting in /device/amlogic/xxx/xxx.mk files*/
    uint8_t lensFacing = mFacingBack ?
            ANDROID_LENS_FACING_BACK : ANDROID_LENS_FACING_FRONT;
   /*in cdd , usb camera is external facing*/
   if ( mSensorType == SENSOR_USB )
          lensFacing = ANDROID_LENS_FACING_EXTERNAL;

    info.update(ANDROID_LENS_FACING, &lensFacing, 1);

    float lensPosition[3];
    if (mFacingBack) {
        // Back-facing camera is center-top on device
        lensPosition[0] = 0;
        lensPosition[1] = 20;
        lensPosition[2] = -5;
    } else {
        // Front-facing camera is center-right on device
        lensPosition[0] = 20;
        lensPosition[1] = 20;
        lensPosition[2] = 0;
    }
#if PLATFORM_SDK_VERSION <= 22
    info.update(ANDROID_LENS_POSITION, lensPosition, sizeof(lensPosition)/
            sizeof(float));
#endif
    static const uint8_t lensCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    info.update(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,&lensCalibration,1);

    // android.sensor

    static const int32_t testAvailablePattern = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    info.update(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES, &testAvailablePattern, 1);
    static const int32_t testPattern = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    info.update(ANDROID_SENSOR_TEST_PATTERN_MODE, &testPattern, 1);
    info.update(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
            Sensor::kExposureTimeRange, 2);

    info.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            &Sensor::kFrameDurationRange[1], 1);

    info.update(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
            Sensor::kSensitivityRange,
            sizeof(Sensor::kSensitivityRange)
            /sizeof(int32_t));

    info.update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
            &Sensor::kColorFilterArrangement, 1);

    static const float sensorPhysicalSize[2] = {3.20f, 2.40f}; // mm
    info.update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            sensorPhysicalSize, 2);

    info.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
            (int32_t*)&Sensor::kMaxRawValue, 1);

    static const int32_t blackLevelPattern[4] = {
            (int32_t)Sensor::kBlackLevel, (int32_t)Sensor::kBlackLevel,
            (int32_t)Sensor::kBlackLevel, (int32_t)Sensor::kBlackLevel
    };
    info.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            blackLevelPattern, sizeof(blackLevelPattern)/sizeof(int32_t));

    static const uint8_t timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN;
    info.update(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE, &timestampSource, 1);
    if (mSensorType == SENSOR_USB) {
        if (mFacingBack) {
            property_get("hw.camera.orientation.back", property, "0");
        } else {
            property_get("hw.camera.orientation.front", property, "0");
        }
        int32_t orientation = atoi(property);
        property_get("hw.camera.usb.orientation_offset", property, "0");
        orientation += atoi(property);
        orientation %= 360;
        info.update(ANDROID_SENSOR_ORIENTATION, &orientation, 1);
    } else {
        if (mFacingBack) {
            property_get("hw.camera.orientation.back", property, "0");
            const int32_t orientation = atoi(property);
            info.update(ANDROID_SENSOR_ORIENTATION, &orientation, 1);
        } else {
            property_get("hw.camera.orientation.front", property, "90");
            const int32_t orientation = atoi(property);
            info.update(ANDROID_SENSOR_ORIENTATION, &orientation, 1);
        }
    }

    static const int64_t rollingShutterSkew = 0;
    info.update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, &rollingShutterSkew, 1);

    //TODO: sensor color calibration fields

    // android.flash
    static const uint8_t flashAvailable = 0;
    info.update(ANDROID_FLASH_INFO_AVAILABLE, &flashAvailable, 1);

    static const uint8_t flashstate = ANDROID_FLASH_STATE_UNAVAILABLE;
    info.update(ANDROID_FLASH_STATE, &flashstate, 1);

    static const int64_t flashChargeDuration = 0;
    info.update(ANDROID_FLASH_INFO_CHARGE_DURATION, &flashChargeDuration, 1);

    /** android.noise */
    static const uint8_t availableNBModes = ANDROID_NOISE_REDUCTION_MODE_OFF;
    info.update(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES, &availableNBModes, 1);

    // android.tonemap
    static const uint8_t availabletonemapModes[] = {
        ANDROID_TONEMAP_MODE_FAST,
        ANDROID_TONEMAP_MODE_HIGH_QUALITY
    };
    info.update(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES, availabletonemapModes,
        sizeof(availabletonemapModes)/sizeof(availabletonemapModes[0]));

    static const int32_t tonemapCurvePoints = 128;
    info.update(ANDROID_TONEMAP_MAX_CURVE_POINTS, &tonemapCurvePoints, 1);

    // android.scaler

    static const uint8_t croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    info.update(ANDROID_SCALER_CROPPING_TYPE, &croppingType, 1);

    info.update(ANDROID_SCALER_AVAILABLE_FORMATS,
            kAvailableFormats,
            sizeof(kAvailableFormats)/sizeof(int32_t));

    info.update(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
            (int64_t*)kAvailableRawMinDurations,
            sizeof(kAvailableRawMinDurations)/sizeof(uint64_t));

    //for version 3.2 ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
    count = sizeof(picSizes)/sizeof(picSizes[0]);
    count = s->getStreamConfigurations(picSizes, kAvailableFormats, count);

    info.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
           (int32_t*)picSizes, count);

    if (count < availablejpegsize) {
        availablejpegsize = count;
    }
    getValidJpegSize(picSizes,mAvailableJpegSize,availablejpegsize);

    maxJpegResolution = getMaxJpegResolution(picSizes,count);
    int32_t full_size[4];
    if (mFacingBack) {
        full_size[0] = 0;
        full_size[1] = 0;
        full_size[2] = maxJpegResolution.width;
        full_size[3] = maxJpegResolution.height;
    } else {
        full_size[0] = 0;
        full_size[1] = 0;
        full_size[2] = maxJpegResolution.width;
        full_size[3] = maxJpegResolution.height;
    }
    /*activeArray.width <= pixelArraySize.Width && activeArray.height<= pixelArraySize.Height*/
    info.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
            (int32_t*)full_size,
            sizeof(full_size)/sizeof(full_size[0]));
    info.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
            (int32_t*)(&full_size[2]), 2);

    duration = new int64_t[count];
    if (duration == NULL) {
        DBG_LOGA("allocate memory for duration failed");
        return NO_MEMORY;
    } else {
        memset(duration,0,sizeof(int64_t)*count);
    }
    duration_count = s->getStreamConfigurationDurations(picSizes, duration, count, true);
    info.update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
            duration, duration_count);

    memset(duration,0,sizeof(int64_t)*count);
    duration_count = s->getStreamConfigurationDurations(picSizes, duration, count, false);
    info.update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
            duration, duration_count);

    info.update(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS,
            (int64_t*)kAvailableProcessedMinDurations,
            sizeof(kAvailableProcessedMinDurations)/sizeof(uint64_t));

    info.update(ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
            (int64_t*)kAvailableJpegMinDurations,
            sizeof(kAvailableJpegMinDurations)/sizeof(uint64_t));


    // android.jpeg

    static const int32_t jpegThumbnailSizes[] = {
            0, 0,
            128, 72,
            160, 120,
            320, 240
     };
    info.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
            jpegThumbnailSizes, sizeof(jpegThumbnailSizes)/sizeof(int32_t));

    static const int32_t jpegMaxSize = JpegCompressor::kMaxJpegSize;
    info.update(ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);

    // android.stats

    static const uint8_t availableFaceDetectModes[] = {
        ANDROID_STATISTICS_FACE_DETECT_MODE_OFF,
        ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE,
        ANDROID_STATISTICS_FACE_DETECT_MODE_FULL
    };

    info.update(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
            availableFaceDetectModes,
            sizeof(availableFaceDetectModes));

    static const int32_t maxFaceCount = 8;
    info.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
            &maxFaceCount, 1);

    static const int32_t histogramSize = 64;
    info.update(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
            &histogramSize, 1);

    static const int32_t maxHistogramCount = 1000;
    info.update(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &maxHistogramCount, 1);

    static const int32_t sharpnessMapSize[2] = {64, 64};
    info.update(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sharpnessMapSize, sizeof(sharpnessMapSize)/sizeof(int32_t));

    static const int32_t maxSharpnessMapValue = 1000;
    info.update(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &maxSharpnessMapValue, 1);
    static const uint8_t hotPixelMapMode = ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF;
    info.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE,&hotPixelMapMode, 1);

    static const uint8_t sceneFlicker = ANDROID_STATISTICS_SCENE_FLICKER_NONE;
    info.update(ANDROID_STATISTICS_SCENE_FLICKER,&sceneFlicker, 1);
    static const uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    info.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE,&lensShadingMapMode, 1);
    // android.control

	static const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY;
    info.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    static const uint8_t availableSceneModes[] = {
      //      ANDROID_CONTROL_SCENE_MODE_DISABLED,
			ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY
    };
    info.update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
            availableSceneModes, sizeof(availableSceneModes));

    static const uint8_t availableEffects[] = {
            ANDROID_CONTROL_EFFECT_MODE_OFF
    };
    info.update(ANDROID_CONTROL_AVAILABLE_EFFECTS,
            availableEffects, sizeof(availableEffects));

    static const int32_t max3aRegions[] = {/*AE*/ 0,/*AWB*/ 0,/*AF*/ 0};
    info.update(ANDROID_CONTROL_MAX_REGIONS,
            max3aRegions, sizeof(max3aRegions)/sizeof(max3aRegions[0]));

    static const uint8_t availableAeModes[] = {
            ANDROID_CONTROL_AE_MODE_OFF,
            ANDROID_CONTROL_AE_MODE_ON
    };
    info.update(ANDROID_CONTROL_AE_AVAILABLE_MODES,
            availableAeModes, sizeof(availableAeModes));


    static const int32_t availableTargetFpsRanges[] = {
            5, 15, 15, 15, 5, 20, 20, 20, 5, 25, 25, 25, 5, 30, 30, 30,
    };
    info.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            availableTargetFpsRanges,
            sizeof(availableTargetFpsRanges)/sizeof(int32_t));

    uint8_t awbModes[maxCount];
    count = s->getAWB(awbModes, maxCount);
    if (count < 0) {
        static const uint8_t availableAwbModes[] = {
                ANDROID_CONTROL_AWB_MODE_OFF,
                ANDROID_CONTROL_AWB_MODE_AUTO,
                ANDROID_CONTROL_AWB_MODE_INCANDESCENT,
                ANDROID_CONTROL_AWB_MODE_FLUORESCENT,
                ANDROID_CONTROL_AWB_MODE_DAYLIGHT,
                ANDROID_CONTROL_AWB_MODE_SHADE
        };
        info.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                availableAwbModes, sizeof(availableAwbModes));
    } else {
        DBG_LOGB("getAWB %d ",count);
        info.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                awbModes, count);
    }

	static const uint8_t afstate = ANDROID_CONTROL_AF_STATE_INACTIVE;
	info.update(ANDROID_CONTROL_AF_STATE,&afstate,1);

    static const uint8_t availableAfModesFront[] = {
            ANDROID_CONTROL_AF_MODE_OFF
    };

    if (mFacingBack) {
        uint8_t afMode[maxCount];
        count = s->getAutoFocus(afMode, maxCount);
        if (count < 0) {
            static const uint8_t availableAfModesBack[] = {
                    ANDROID_CONTROL_AF_MODE_OFF,
                    //ANDROID_CONTROL_AF_MODE_AUTO,
                    //ANDROID_CONTROL_AF_MODE_MACRO,
                    //ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO,
                    //ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE,
            };

            info.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                        availableAfModesBack, sizeof(availableAfModesBack));
        } else {
            info.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                        afMode, count);
        }
    } else {
        info.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                    availableAfModesFront, sizeof(availableAfModesFront));
    }

    uint8_t antiBanding[maxCount];
    count = s->getAntiBanding(antiBanding, maxCount);
    if (count < 0) {
        static const uint8_t availableAntibanding[] = {
                ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,
                ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO,
        };
        info.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                availableAntibanding, sizeof(availableAntibanding));
    } else {
        info.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                antiBanding, count);
    }

    camera_metadata_rational step;
    int maxExp, minExp, def;
    ret = s->getExposure(&maxExp, &minExp, &def, &step);
    if (ret < 0) {
        static const int32_t aeExpCompensation = 0;
        info.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExpCompensation, 1);

        static const camera_metadata_rational exposureCompensationStep = {
                1, 3
        };
        info.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
                &exposureCompensationStep, 1);

        int32_t exposureCompensationRange[] = {-6, 6};
        info.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
                exposureCompensationRange,
                sizeof(exposureCompensationRange)/sizeof(int32_t));
    } else {
        DBG_LOGB("exposure compensation support:(%d, %d)\n", minExp, maxExp);
        int32_t exposureCompensationRange[] = {minExp, maxExp};
        info.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
                exposureCompensationRange,
                sizeof(exposureCompensationRange)/sizeof(int32_t));
        info.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
                &step, 1);
        info.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &def, 1);
    }

    ret = s->getZoom(&mZoomMin, &mZoomMax, &mZoomStep);
    if (ret < 0) {
        float maxZoom = 1.0;
        info.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
            &maxZoom, 1);
    } else {
        if (mZoomMin != 0) {
            float maxZoom = mZoomMax / mZoomMin;
            info.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
                &maxZoom, 1);
        } else {
            float maxZoom = 1.0;
            info.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
                &maxZoom, 1);
        }
    }

    static const uint8_t availableVstabModes[] = {
            ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF
    };
    info.update(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
            availableVstabModes, sizeof(availableVstabModes));

    static const uint8_t aestate = ANDROID_CONTROL_AE_STATE_CONVERGED;
    info.update(ANDROID_CONTROL_AE_STATE,&aestate,1);
    static const uint8_t awbstate = ANDROID_CONTROL_AWB_STATE_INACTIVE;
    info.update(ANDROID_CONTROL_AWB_STATE,&awbstate,1);
    // android.info
    const uint8_t supportedHardwareLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;
        //mFullMode ? ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL :
        //            ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    info.update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
                &supportedHardwareLevel,
                /*count*/1);

    int32_t android_sync_max_latency = ANDROID_SYNC_MAX_LATENCY_UNKNOWN;
    info.update(ANDROID_SYNC_MAX_LATENCY, &android_sync_max_latency, 1);

    uint8_t len[] = {1};
    info.update(ANDROID_REQUEST_PIPELINE_DEPTH, (uint8_t *)len, 1);

    /*for cts BurstCaptureTest ->testYuvBurst */
    uint8_t maxlen[] = {kMaxBufferCount};
    info.update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH, (uint8_t *)maxlen, 1);
    uint8_t cap[] = {
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
    };
    info.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
            (uint8_t *)cap, sizeof(cap)/sizeof(cap[0]));


    int32_t partialResultCount = 1;
    info.update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT,&partialResultCount,1);
    int32_t maxNumOutputStreams[3] = {0,2,1};
    info.update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,maxNumOutputStreams,3);
    uint8_t aberrationMode[] = {ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF};
    info.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE,
            aberrationMode, 1);
    info.update(ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
            aberrationMode, 1);

    getAvailableChKeys(&info, supportedHardwareLevel);

    if (mCameraInfo != NULL) {
        CAMHAL_LOGDA("mCameraInfo is not null, mem leak?");
    }
    mCameraInfo = info.release();
    DBG_LOGB("mCameraID=%d,mCameraInfo=%p\n", mCameraID, mCameraInfo);

    if (duration != NULL) {
        delete [] duration;
    }

    s->shutDown();
    s.clear();
    mPlugged = true;

    return OK;
}

status_t EmulatedFakeCamera3::process3A(CameraMetadata &settings) {
    /**
     * Extract top-level 3A controls
     */
    status_t res;

    //bool facePriority = false;

    camera_metadata_entry e;

    e = settings.find(ANDROID_CONTROL_MODE);
    if (e.count == 0) {
        ALOGE("%s: No control mode entry!", __FUNCTION__);
        return BAD_VALUE;
    }
    uint8_t controlMode = e.data.u8[0];

    e = settings.find(ANDROID_CONTROL_SCENE_MODE);
    if (e.count == 0) {
        ALOGE("%s: No scene mode entry!", __FUNCTION__);
        return BAD_VALUE;
    }
    uint8_t sceneMode = e.data.u8[0];

    if (controlMode == ANDROID_CONTROL_MODE_OFF) {
        mAeState  = ANDROID_CONTROL_AE_STATE_INACTIVE;
        mAfState  = ANDROID_CONTROL_AF_STATE_INACTIVE;
        mAwbState = ANDROID_CONTROL_AWB_STATE_INACTIVE;
        update3A(settings);
        return OK;
    } else if (controlMode == ANDROID_CONTROL_MODE_USE_SCENE_MODE) {
        switch(sceneMode) {
            case ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY:
                mFacePriority = true;
                break;
            default:
                ALOGE("%s: Emulator doesn't support scene mode %d",
                        __FUNCTION__, sceneMode);
                return BAD_VALUE;
        }
    } else {
        mFacePriority = false;
    }

    // controlMode == AUTO or sceneMode = FACE_PRIORITY
    // Process individual 3A controls

    res = doFakeAE(settings);
    if (res != OK) return res;

    res = doFakeAF(settings);
    if (res != OK) return res;

    res = doFakeAWB(settings);
    if (res != OK) return res;

    update3A(settings);
    return OK;
}

status_t EmulatedFakeCamera3::doFakeAE(CameraMetadata &settings) {
    camera_metadata_entry e;

    e = settings.find(ANDROID_CONTROL_AE_MODE);
    if (e.count == 0) {
        ALOGE("%s: No AE mode entry!", __FUNCTION__);
        return BAD_VALUE;
    }
    uint8_t aeMode = e.data.u8[0];

    switch (aeMode) {
        case ANDROID_CONTROL_AE_MODE_OFF:
            // AE is OFF
            mAeState = ANDROID_CONTROL_AE_STATE_INACTIVE;
            return OK;
        case ANDROID_CONTROL_AE_MODE_ON:
            // OK for AUTO modes
            break;
        default:
            ALOGVV("%s: Emulator doesn't support AE mode %d",
                    __FUNCTION__, aeMode);
            return BAD_VALUE;
    }

    e = settings.find(ANDROID_CONTROL_AE_LOCK);
    if (e.count == 0) {
        ALOGE("%s: No AE lock entry!", __FUNCTION__);
        return BAD_VALUE;
    }
    bool aeLocked = (e.data.u8[0] == ANDROID_CONTROL_AE_LOCK_ON);

    e = settings.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
    bool precaptureTrigger = false;
    if (e.count != 0) {
        precaptureTrigger =
                (e.data.u8[0] == ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START);
    }

    if (precaptureTrigger) {
        ALOGVV("%s: Pre capture trigger = %d", __FUNCTION__, precaptureTrigger);
    } else if (e.count > 0) {
        ALOGVV("%s: Pre capture trigger was present? %zu",
              __FUNCTION__,
              e.count);
    }

    if (precaptureTrigger || mAeState == ANDROID_CONTROL_AE_STATE_PRECAPTURE) {
        // Run precapture sequence
        if (mAeState != ANDROID_CONTROL_AE_STATE_PRECAPTURE) {
            mAeCounter = 0;
        }

        if (mFacePriority) {
            mAeTargetExposureTime = kFacePriorityExposureTime;
        } else {
            mAeTargetExposureTime = kNormalExposureTime;
        }

        if (mAeCounter > kPrecaptureMinFrames &&
                (mAeTargetExposureTime - mAeCurrentExposureTime) <
                mAeTargetExposureTime / 10) {
            // Done with precapture
            mAeCounter = 0;
            mAeState = aeLocked ? ANDROID_CONTROL_AE_STATE_LOCKED :
                    ANDROID_CONTROL_AE_STATE_CONVERGED;
        } else {
            // Converge some more
            mAeCurrentExposureTime +=
                    (mAeTargetExposureTime - mAeCurrentExposureTime) *
                    kExposureTrackRate;
            mAeCounter++;
            mAeState = ANDROID_CONTROL_AE_STATE_PRECAPTURE;
        }

    } else if (!aeLocked) {
        // Run standard occasional AE scan
        switch (mAeState) {
            case ANDROID_CONTROL_AE_STATE_CONVERGED:
            case ANDROID_CONTROL_AE_STATE_INACTIVE:
                mAeCounter++;
                if (mAeCounter > kStableAeMaxFrames) {
                    mAeTargetExposureTime =
                            mFacePriority ? kFacePriorityExposureTime :
                            kNormalExposureTime;
                    float exposureStep = ((double)rand() / RAND_MAX) *
                            (kExposureWanderMax - kExposureWanderMin) +
                            kExposureWanderMin;
                    mAeTargetExposureTime *= std::pow(2, exposureStep);
                    mAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                }
                break;
            case ANDROID_CONTROL_AE_STATE_SEARCHING:
                mAeCurrentExposureTime +=
                        (mAeTargetExposureTime - mAeCurrentExposureTime) *
                        kExposureTrackRate;
                if (abs(mAeTargetExposureTime - mAeCurrentExposureTime) <
                        mAeTargetExposureTime / 10) {
                    // Close enough
                    mAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                    mAeCounter = 0;
                }
                break;
            case ANDROID_CONTROL_AE_STATE_LOCKED:
                mAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                mAeCounter = 0;
                break;
            default:
                ALOGE("%s: Emulator in unexpected AE state %d",
                        __FUNCTION__, mAeState);
                return INVALID_OPERATION;
        }
    } else {
        // AE is locked
        mAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
    }

    return OK;
}

status_t EmulatedFakeCamera3::doFakeAF(CameraMetadata &settings) {
    camera_metadata_entry e;

    e = settings.find(ANDROID_CONTROL_AF_MODE);
    if (e.count == 0) {
        ALOGE("%s: No AF mode entry!", __FUNCTION__);
        return BAD_VALUE;
    }
    uint8_t afMode = e.data.u8[0];

    e = settings.find(ANDROID_CONTROL_AF_TRIGGER);
    typedef camera_metadata_enum_android_control_af_trigger af_trigger_t;
    af_trigger_t afTrigger;
    // If we have an afTrigger, afTriggerId should be set too
    if (e.count != 0) {
        afTrigger = static_cast<af_trigger_t>(e.data.u8[0]);

        e = settings.find(ANDROID_CONTROL_AF_TRIGGER_ID);

        if (e.count == 0) {
            ALOGE("%s: When android.control.afTrigger is set "
                  " in the request, afTriggerId needs to be set as well",
                  __FUNCTION__);
            return BAD_VALUE;
        }

        mAfTriggerId = e.data.i32[0];

        ALOGVV("%s: AF trigger set to 0x%x", __FUNCTION__, afTrigger);
        ALOGVV("%s: AF trigger ID set to 0x%x", __FUNCTION__, mAfTriggerId);
        ALOGVV("%s: AF mode is 0x%x", __FUNCTION__, afMode);
    } else {
        afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    }
    if (!mFacingBack) {
        afMode = ANDROID_CONTROL_AF_MODE_OFF;
    }

    switch (afMode) {
        case ANDROID_CONTROL_AF_MODE_OFF:
            mAfState = ANDROID_CONTROL_AF_STATE_INACTIVE;
            return OK;
        case ANDROID_CONTROL_AF_MODE_AUTO:
        case ANDROID_CONTROL_AF_MODE_MACRO:
        case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
        case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
            if (!mFacingBack) {
                ALOGE("%s: Front camera doesn't support AF mode %d",
                        __FUNCTION__, afMode);
                return BAD_VALUE;
            }
            mSensor->setAutoFocuas(afMode);
            // OK, handle transitions lower on
            break;
        default:
            ALOGE("%s: Emulator doesn't support AF mode %d",
                    __FUNCTION__, afMode);
            return BAD_VALUE;
    }
#if 0
    e = settings.find(ANDROID_CONTROL_AF_REGIONS);
    if (e.count == 0) {
        ALOGE("%s:Get ANDROID_CONTROL_AF_REGIONS failed\n", __FUNCTION__);
        return BAD_VALUE;
    }
    int32_t x0 = e.data.i32[0];
    int32_t y0 = e.data.i32[1];
    int32_t x1 = e.data.i32[2];
    int32_t y1 = e.data.i32[3];
    mSensor->setFocuasArea(x0, y0, x1, y1);
    DBG_LOGB(" x0:%d, y0:%d,x1:%d,y1:%d,\n", x0, y0, x1, y1);
#endif


    bool afModeChanged = mAfMode != afMode;
    mAfMode = afMode;

    /**
     * Simulate AF triggers. Transition at most 1 state per frame.
     * - Focusing always succeeds (goes into locked, or PASSIVE_SCAN).
     */

    bool afTriggerStart = false;
    bool afTriggerCancel = false;
    switch (afTrigger) {
        case ANDROID_CONTROL_AF_TRIGGER_IDLE:
            break;
        case ANDROID_CONTROL_AF_TRIGGER_START:
            afTriggerStart = true;
            break;
        case ANDROID_CONTROL_AF_TRIGGER_CANCEL:
            afTriggerCancel = true;
            // Cancel trigger always transitions into INACTIVE
            mAfState = ANDROID_CONTROL_AF_STATE_INACTIVE;

            ALOGV("%s: AF State transition to STATE_INACTIVE", __FUNCTION__);

            // Stay in 'inactive' until at least next frame
            return OK;
        default:
            ALOGE("%s: Unknown af trigger value %d", __FUNCTION__, afTrigger);
            return BAD_VALUE;
    }

    // If we get down here, we're either in an autofocus mode
    //  or in a continuous focus mode (and no other modes)

    int oldAfState = mAfState;
    switch (mAfState) {
        case ANDROID_CONTROL_AF_STATE_INACTIVE:
            if (afTriggerStart) {
                switch (afMode) {
                    case ANDROID_CONTROL_AF_MODE_AUTO:
                        // fall-through
                    case ANDROID_CONTROL_AF_MODE_MACRO:
                        mAfState = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
                        break;
                    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
                        // fall-through
                    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
                        mAfState = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
                        break;
                }
            } else {
                // At least one frame stays in INACTIVE
                if (!afModeChanged) {
                    switch (afMode) {
                        case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
                            // fall-through
                        case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
                            mAfState = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
                            break;
                    }
                }
            }
            break;
        case ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN:
            /**
             * When the AF trigger is activated, the algorithm should finish
             * its PASSIVE_SCAN if active, and then transition into AF_FOCUSED
             * or AF_NOT_FOCUSED as appropriate
             */
            if (afTriggerStart) {
                // Randomly transition to focused or not focused
                if (rand() % 3) {
                    mAfState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
                } else {
                    mAfState = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
                }
            }
            /**
             * When the AF trigger is not involved, the AF algorithm should
             * start in INACTIVE state, and then transition into PASSIVE_SCAN
             * and PASSIVE_FOCUSED states
             */
            else if (!afTriggerCancel) {
               // Randomly transition to passive focus
                if (rand() % 3 == 0) {
                    mAfState = ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED;
                }
            }

            break;
        case ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED:
            if (afTriggerStart) {
                // Randomly transition to focused or not focused
                if (rand() % 3) {
                    mAfState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
                } else {
                    mAfState = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
                }
            }
            // TODO: initiate passive scan (PASSIVE_SCAN)
            break;
        case ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN:
            // Simulate AF sweep completing instantaneously

            // Randomly transition to focused or not focused
            if (rand() % 3) {
                mAfState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
            } else {
                mAfState = ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED;
            }
            break;
        case ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED:
            if (afTriggerStart) {
                switch (afMode) {
                    case ANDROID_CONTROL_AF_MODE_AUTO:
                        // fall-through
                    case ANDROID_CONTROL_AF_MODE_MACRO:
                        mAfState = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
                        break;
                    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
                        // fall-through
                    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
                        // continuous autofocus => trigger start has no effect
                        break;
                }
            }
            break;
        case ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
            if (afTriggerStart) {
                switch (afMode) {
                    case ANDROID_CONTROL_AF_MODE_AUTO:
                        // fall-through
                    case ANDROID_CONTROL_AF_MODE_MACRO:
                        mAfState = ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN;
                        break;
                    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
                        // fall-through
                    case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
                        // continuous autofocus => trigger start has no effect
                        break;
                }
            }
            break;
        default:
            ALOGE("%s: Bad af state %d", __FUNCTION__, mAfState);
    }

    {
        char afStateString[100] = {0,};
        camera_metadata_enum_snprint(ANDROID_CONTROL_AF_STATE,
                oldAfState,
                afStateString,
                sizeof(afStateString));

        char afNewStateString[100] = {0,};
        camera_metadata_enum_snprint(ANDROID_CONTROL_AF_STATE,
                mAfState,
                afNewStateString,
                sizeof(afNewStateString));
        ALOGVV("%s: AF state transitioned from %s to %s",
              __FUNCTION__, afStateString, afNewStateString);
    }


    return OK;
}

status_t EmulatedFakeCamera3::doFakeAWB(CameraMetadata &settings) {
    camera_metadata_entry e;

    e = settings.find(ANDROID_CONTROL_AWB_MODE);
    if (e.count == 0) {
        ALOGE("%s: No AWB mode entry!", __FUNCTION__);
        return BAD_VALUE;
    }
    uint8_t awbMode = e.data.u8[0];
    //DBG_LOGB(" awbMode%d\n", awbMode);

    // TODO: Add white balance simulation

    switch (awbMode) {
        case ANDROID_CONTROL_AWB_MODE_OFF:
            mAwbState = ANDROID_CONTROL_AWB_STATE_INACTIVE;
            return OK;
        case ANDROID_CONTROL_AWB_MODE_AUTO:
        case ANDROID_CONTROL_AWB_MODE_INCANDESCENT:
        case ANDROID_CONTROL_AWB_MODE_FLUORESCENT:
        case ANDROID_CONTROL_AWB_MODE_DAYLIGHT:
        case ANDROID_CONTROL_AWB_MODE_SHADE:
            mAwbState = ANDROID_CONTROL_AWB_STATE_CONVERGED; //add for cts
            if (mSensorType == SENSOR_USB)
                return OK;
            else
                return mSensor->setAWB(awbMode);
            break;
        default:
            ALOGE("%s: Emulator doesn't support AWB mode %d",
                    __FUNCTION__, awbMode);
            return BAD_VALUE;
    }

    return OK;
}


void EmulatedFakeCamera3::update3A(CameraMetadata &settings) {
    if (mAeState != ANDROID_CONTROL_AE_STATE_INACTIVE) {
        settings.update(ANDROID_SENSOR_EXPOSURE_TIME,
                &mAeCurrentExposureTime, 1);
        settings.update(ANDROID_SENSOR_SENSITIVITY,
                &mAeCurrentSensitivity, 1);
    }

    settings.update(ANDROID_CONTROL_AE_STATE,
            &mAeState, 1);
    settings.update(ANDROID_CONTROL_AF_STATE,
            &mAfState, 1);
    settings.update(ANDROID_CONTROL_AWB_STATE,
            &mAwbState, 1);
    /**
     * TODO: Trigger IDs need a think-through
     */
    settings.update(ANDROID_CONTROL_AF_TRIGGER_ID,
            &mAfTriggerId, 1);
}

void EmulatedFakeCamera3::signalReadoutIdle() {
    Mutex::Autolock l(mLock);
    CAMHAL_LOGVB("%s ,  E" , __FUNCTION__);
    // Need to chek isIdle again because waiting on mLock may have allowed
    // something to be placed in the in-flight queue.
    if (mStatus == STATUS_ACTIVE && mReadoutThread->isIdle()) {
        ALOGV("Now idle");
        mStatus = STATUS_READY;
    }
    CAMHAL_LOGVB("%s ,  X ,  mStatus = %d " , __FUNCTION__, mStatus);
}

void EmulatedFakeCamera3::onSensorEvent(uint32_t frameNumber, Event e,
        nsecs_t timestamp) {
    switch(e) {
        case Sensor::SensorListener::EXPOSURE_START: {
            ALOGVV("%s: Frame %d: Sensor started exposure at %lld",
                    __FUNCTION__, frameNumber, timestamp);
            // Trigger shutter notify to framework
            camera3_notify_msg_t msg;
            msg.type = CAMERA3_MSG_SHUTTER;
            msg.message.shutter.frame_number = frameNumber;
            msg.message.shutter.timestamp = timestamp;
            sendNotify(&msg);
            break;
        }
        case Sensor::SensorListener::ERROR_CAMERA_DEVICE: {
            camera3_notify_msg_t msg;
            msg.type = CAMERA3_MSG_ERROR;
            msg.message.error.frame_number = frameNumber;
            msg.message.error.error_stream = NULL;
            msg.message.error.error_code = 1;
            sendNotify(&msg);
            break;
        }
        default:
            ALOGW("%s: Unexpected sensor event %d at %" PRId64, __FUNCTION__,
                    e, timestamp);
            break;
    }
}

EmulatedFakeCamera3::ReadoutThread::ReadoutThread(EmulatedFakeCamera3 *parent) :
        mParent(parent), mJpegWaiting(false) {
    mExitReadoutThread = false;
    mFlushFlag = false;
}

EmulatedFakeCamera3::ReadoutThread::~ReadoutThread() {
    for (List<Request>::iterator i = mInFlightQueue.begin();
         i != mInFlightQueue.end(); i++) {
        delete i->buffers;
        delete i->sensorBuffers;
    }
}

status_t EmulatedFakeCamera3::ReadoutThread::flushAllRequest(bool flag) {
    status_t res;
    mFlushFlag = flag;
    Mutex::Autolock l(mLock);
    CAMHAL_LOGDB("count = %zu" , mInFlightQueue.size());
    if (mInFlightQueue.size() > 0) {
        mParent->mSensor->setFlushFlag(true);
        res = mFlush.waitRelative(mLock, kSyncWaitTimeout * 15);
        if (res != OK && res != TIMED_OUT) {
           ALOGE("%s: Error waiting for mFlush singnal : %d",
                __FUNCTION__, res);
           return INVALID_OPERATION;
        }
        DBG_LOGA("finish flush all request");
    }
    return 0;
}

void EmulatedFakeCamera3::ReadoutThread::sendFlushSingnal(void) {
    Mutex::Autolock l(mLock);
    mFlush.signal();
}

void EmulatedFakeCamera3::ReadoutThread::setFlushFlag(bool flag) {
    mFlushFlag = flag;
}

void EmulatedFakeCamera3::ReadoutThread::queueCaptureRequest(const Request &r) {
    Mutex::Autolock l(mLock);

    mInFlightQueue.push_back(r);
    mInFlightSignal.signal();
}

bool EmulatedFakeCamera3::ReadoutThread::isIdle() {
    Mutex::Autolock l(mLock);
    return mInFlightQueue.empty() && !mThreadActive;
}

status_t EmulatedFakeCamera3::ReadoutThread::waitForReadout() {
    status_t res;
    Mutex::Autolock l(mLock);
    CAMHAL_LOGVB("%s ,  E" , __FUNCTION__);
    int loopCount = 0;
    while (mInFlightQueue.size() >= kMaxQueueSize) {
        res = mInFlightSignal.waitRelative(mLock, kWaitPerLoop);
        if (res != OK && res != TIMED_OUT) {
            ALOGE("%s: Error waiting for in-flight queue to shrink",
                    __FUNCTION__);
            return INVALID_OPERATION;
        }
        if (loopCount == kMaxWaitLoops) {
            ALOGE("%s: Timed out waiting for in-flight queue to shrink",
                    __FUNCTION__);
            return TIMED_OUT;
        }
        loopCount++;
    }
    return OK;
}

status_t EmulatedFakeCamera3::ReadoutThread::setJpegCompressorListener(EmulatedFakeCamera3 *parent) {
    status_t res;
    res = mParent->mJpegCompressor->setlistener(this);
    if (res != NO_ERROR) {
        ALOGE("%s: set JpegCompressor Listner failed",__FUNCTION__);
    }
    return res;
}

status_t EmulatedFakeCamera3::ReadoutThread::startJpegCompressor(EmulatedFakeCamera3 *parent) {
    status_t res;
    res = mParent->mJpegCompressor->start();
    if (res != NO_ERROR) {
        ALOGE("%s: JpegCompressor start failed",__FUNCTION__);
    }
    return res;
}

status_t EmulatedFakeCamera3::ReadoutThread::shutdownJpegCompressor(EmulatedFakeCamera3 *parent) {
    status_t res;
    res = mParent->mJpegCompressor->cancel();
    if (res != OK) {
        ALOGE("%s: JpegCompressor cancel failed",__FUNCTION__);
    }
    return res;
}

void EmulatedFakeCamera3::ReadoutThread::sendExitReadoutThreadSignal(void) {
    mExitReadoutThread = true;
    mInFlightSignal.signal();
}

bool EmulatedFakeCamera3::ReadoutThread::threadLoop() {
    status_t res;
    ALOGVV("%s: ReadoutThread waiting for request", __FUNCTION__);

    // First wait for a request from the in-flight queue
    if (mExitReadoutThread) {
        return false;
    }

    {
        Mutex::Autolock l(mLock);
        if ((mInFlightQueue.size() == 0) && (mFlushFlag) &&
                 (mCurrentRequest.settings.isEmpty())) {
            mFlush.signal();
        }
    }

    if (mCurrentRequest.settings.isEmpty()) {
        Mutex::Autolock l(mLock);
        if (mInFlightQueue.empty()) {
            res = mInFlightSignal.waitRelative(mLock, kWaitPerLoop);
            if (res == TIMED_OUT) {
                ALOGVV("%s: ReadoutThread: Timed out waiting for request",
                        __FUNCTION__);
                return true;
            } else if (res != NO_ERROR) {
                ALOGE("%s: Error waiting for capture requests: %d",
                        __FUNCTION__, res);
                return false;
            }
        }

        if (mExitReadoutThread) {
            return false;
        }

        mCurrentRequest.frameNumber = mInFlightQueue.begin()->frameNumber;
        mCurrentRequest.settings.acquire(mInFlightQueue.begin()->settings);
        mCurrentRequest.buffers = mInFlightQueue.begin()->buffers;
        mCurrentRequest.sensorBuffers = mInFlightQueue.begin()->sensorBuffers;
        mCurrentRequest.havethumbnail = mInFlightQueue.begin()->havethumbnail;
        mInFlightQueue.erase(mInFlightQueue.begin());
        mInFlightSignal.signal();
        mThreadActive = true;
        ALOGVV("%s: Beginning readout of frame %d", __FUNCTION__,
                mCurrentRequest.frameNumber);
    }

    // Then wait for it to be delivered from the sensor
    ALOGVV("%s: ReadoutThread: Wait for frame to be delivered from sensor",
            __FUNCTION__);

    nsecs_t captureTime;
    status_t gotFrame =
            mParent->mSensor->waitForNewFrame(kWaitPerLoop, &captureTime);
    if (gotFrame == 0) {
        ALOGVV("%s: ReadoutThread: Timed out waiting for sensor frame",
                __FUNCTION__);
        return true;
    }

    if (gotFrame == -1) {
        DBG_LOGA("Sensor thread had exited , here should exit ReadoutThread Loop");
        return false;
    }

    bool workflag =
            mParent->mSensor->get_sensor_status();
    if (!workflag)
        return true;

    ALOGVV("Sensor done with readout for frame %d, captured at %lld ",
            mCurrentRequest.frameNumber, captureTime);

    // Check if we need to JPEG encode a buffer, and send it for async
    // compression if so. Otherwise prepare the buffer for return.
    bool needJpeg = false;
    HalBufferVector::iterator buf = mCurrentRequest.buffers->begin();
    while (buf != mCurrentRequest.buffers->end()) {
        bool goodBuffer = true;
        if ( buf->stream->format ==
                HAL_PIXEL_FORMAT_BLOB) {
            Mutex::Autolock jl(mJpegLock);
            needJpeg = true;
            CaptureRequest currentcapture;
            currentcapture.frameNumber = mCurrentRequest.frameNumber;
            currentcapture.sensorBuffers = mCurrentRequest.sensorBuffers;
            currentcapture.buf = buf;
            currentcapture.mNeedThumbnail = mCurrentRequest.havethumbnail;
            mParent->mJpegCompressor->queueRequest(currentcapture);
            //this sensorBuffers delete in the jpegcompress;
            mCurrentRequest.sensorBuffers = NULL;
            buf = mCurrentRequest.buffers->erase(buf);
            continue;
        }
        GraphicBufferMapper::get().unlock(*(buf->buffer));

        buf->status = goodBuffer ? CAMERA3_BUFFER_STATUS_OK :
                CAMERA3_BUFFER_STATUS_ERROR;
        buf->acquire_fence = -1;
        buf->release_fence = -1;

        ++buf;
    } // end while

    // Construct result for all completed buffers and results

    camera3_capture_result result;

    mCurrentRequest.settings.update(ANDROID_SENSOR_TIMESTAMP,
            &captureTime, 1);

    const uint8_t pipelineDepth = needJpeg ? kMaxBufferCount : kMaxBufferCount - 1;
    mCurrentRequest.settings.update(ANDROID_REQUEST_PIPELINE_DEPTH,
            &pipelineDepth, 1);

    memset(&result, 0, sizeof(result));
    result.frame_number = mCurrentRequest.frameNumber;
    result.result = mCurrentRequest.settings.getAndLock();
    result.num_output_buffers = mCurrentRequest.buffers->size();
    result.output_buffers = mCurrentRequest.buffers->array();
    result.partial_result = 1;

    // Go idle if queue is empty, before sending result

    bool signalIdle = false;
    {
        Mutex::Autolock l(mLock);
        if (mInFlightQueue.empty()) {
            mThreadActive = false;
            signalIdle = true;
        }
    }

    if (signalIdle) mParent->signalReadoutIdle();

    // Send it off to the framework
    ALOGVV("%s: ReadoutThread: Send result to framework",
            __FUNCTION__);
    mParent->sendCaptureResult(&result);

    // Clean up
    mCurrentRequest.settings.unlock(result.result);

    delete mCurrentRequest.buffers;
    mCurrentRequest.buffers = NULL;
    if (!needJpeg) {
        delete mCurrentRequest.sensorBuffers;
        mCurrentRequest.sensorBuffers = NULL;
    }
    mCurrentRequest.settings.clear();
    CAMHAL_LOGVB("%s ,  X  " , __FUNCTION__);
    return true;
}

void EmulatedFakeCamera3::ReadoutThread::onJpegDone(
        const StreamBuffer &jpegBuffer, bool success , CaptureRequest &r) {
    Mutex::Autolock jl(mJpegLock);
    GraphicBufferMapper::get().unlock(*(jpegBuffer.buffer));

    mJpegHalBuffer = *(r.buf);
    mJpegHalBuffer.status = success ?
            CAMERA3_BUFFER_STATUS_OK : CAMERA3_BUFFER_STATUS_ERROR;
    mJpegHalBuffer.acquire_fence = -1;
    mJpegHalBuffer.release_fence = -1;
    mJpegWaiting = false;

    camera3_capture_result result;
    result.frame_number = r.frameNumber;
    result.result = NULL;
    result.input_buffer = NULL;
    result.num_output_buffers = 1;
    result.output_buffers = &mJpegHalBuffer;
    result.partial_result = 1;

    if (!success) {
        ALOGE("%s: Compression failure, returning error state buffer to"
                " framework", __FUNCTION__);
    } else {
        DBG_LOGB("%s: Compression complete, returning buffer to framework",
                __FUNCTION__);
    }

    mParent->sendCaptureResult(&result);

}

void EmulatedFakeCamera3::ReadoutThread::onJpegInputDone(
        const StreamBuffer &inputBuffer) {
    // Should never get here, since the input buffer has to be returned
    // by end of processCaptureRequest
    ALOGE("%s: Unexpected input buffer from JPEG compressor!", __FUNCTION__);
}


}; // namespace android
