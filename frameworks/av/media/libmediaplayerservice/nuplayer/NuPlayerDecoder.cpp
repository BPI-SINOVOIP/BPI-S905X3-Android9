/*
 * Copyright 2014 The Android Open Source Project
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
#define LOG_TAG "NuPlayerDecoder"
#include <utils/Log.h>
#include <inttypes.h>

#include <algorithm>

#include "NuPlayerCCDecoder.h"
#include "NuPlayerDecoder.h"
#include "NuPlayerDrm.h"
#include "NuPlayerRenderer.h"
#include "NuPlayerSource.h"

#include <cutils/properties.h>
#include <media/ICrypto.h>
#include <media/MediaBufferHolder.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/avc_utils.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/SurfaceUtils.h>
#include <gui/Surface.h>

#include "ATSParser.h"

namespace android {

static float kDisplayRefreshingRate = 60.f; // TODO: get this from the display

// The default total video frame rate of a stream when that info is not available from
// the source.
static float kDefaultVideoFrameRateTotal = 30.f;

static inline bool getAudioDeepBufferSetting() {
    return property_get_bool("media.stagefright.audio.deep", false /* default_value */);
}

NuPlayer::Decoder::Decoder(
        const sp<AMessage> &notify,
        const sp<Source> &source,
        pid_t pid,
        uid_t uid,
        const sp<Renderer> &renderer,
        const sp<Surface> &surface,
        const sp<CCDecoder> &ccDecoder)
    : DecoderBase(notify),
      mSurface(surface),
      mSource(source),
      mRenderer(renderer),
      mCCDecoder(ccDecoder),
      mPid(pid),
      mUid(uid),
      mSkipRenderingUntilMediaTimeUs(-1ll),
      mNumFramesTotal(0ll),
      mNumInputFramesDropped(0ll),
      mNumOutputFramesDropped(0ll),
      mVideoWidth(0),
      mVideoHeight(0),
      mIsAudio(true),
      mIsVideoAVC(false),
      mIsSecure(false),
      mIsEncrypted(false),
      mIsEncryptedObservedEarlier(false),
      mFormatChangePending(false),
      mTimeChangePending(false),
      mFrameRateTotal(kDefaultVideoFrameRateTotal),
      mPlaybackSpeed(1.0f),
      mNumVideoTemporalLayerTotal(1), // decode all layers
      mNumVideoTemporalLayerAllowed(1),
      mCurrentMaxVideoTemporalLayerId(0),
      mResumePending(false),
      mComponentName("decoder") {
    mCodecLooper = new ALooper;
    mCodecLooper->setName("NPDecoder-CL");
    mCodecLooper->start(false, false, ANDROID_PRIORITY_AUDIO);
    mVideoTemporalLayerAggregateFps[0] = mFrameRateTotal;
}

NuPlayer::Decoder::~Decoder() {
    // Need to stop looper first since mCodec could be accessed on the mDecoderLooper.
    stopLooper();
    if (mCodec != NULL) {
        mCodec->release();
    }
    releaseAndResetMediaBuffers();
}

sp<AMessage> NuPlayer::Decoder::getStats() const {
    mStats->setInt64("frames-total", mNumFramesTotal);
    mStats->setInt64("frames-dropped-input", mNumInputFramesDropped);
    mStats->setInt64("frames-dropped-output", mNumOutputFramesDropped);
    return mStats;
}

status_t NuPlayer::Decoder::setVideoSurface(const sp<Surface> &surface) {
    if (surface == NULL || ADebug::isExperimentEnabled("legacy-setsurface")) {
        return BAD_VALUE;
    }

    sp<AMessage> msg = new AMessage(kWhatSetVideoSurface, this);

    msg->setObject("surface", surface);
    sp<AMessage> response;
    status_t err = msg->postAndAwaitResponse(&response);
    if (err == OK && response != NULL) {
        CHECK(response->findInt32("err", &err));
    }
    return err;
}

void NuPlayer::Decoder::onMessageReceived(const sp<AMessage> &msg) {
    ALOGV("[%s] onMessage: %s", mComponentName.c_str(), msg->debugString().c_str());

    switch (msg->what()) {
        case kWhatCodecNotify:
        {
            int32_t cbID;
            CHECK(msg->findInt32("callbackID", &cbID));

            ALOGV("[%s] kWhatCodecNotify: cbID = %d, paused = %d",
                    mIsAudio ? "audio" : "video", cbID, mPaused);

            if (mPaused) {
                break;
            }

            switch (cbID) {
                case MediaCodec::CB_INPUT_AVAILABLE:
                {
                    int32_t index;
                    CHECK(msg->findInt32("index", &index));

                    handleAnInputBuffer(index);
                    break;
                }

                case MediaCodec::CB_OUTPUT_AVAILABLE:
                {
                    int32_t index;
                    size_t offset;
                    size_t size;
                    int64_t timeUs;
                    int32_t flags;

                    CHECK(msg->findInt32("index", &index));
                    CHECK(msg->findSize("offset", &offset));
                    CHECK(msg->findSize("size", &size));
                    CHECK(msg->findInt64("timeUs", &timeUs));
                    CHECK(msg->findInt32("flags", &flags));

                    handleAnOutputBuffer(index, offset, size, timeUs, flags);
                    break;
                }

                case MediaCodec::CB_OUTPUT_FORMAT_CHANGED:
                {
                    sp<AMessage> format;
                    CHECK(msg->findMessage("format", &format));

                    handleOutputFormatChange(format);
                    break;
                }

                case MediaCodec::CB_ERROR:
                {
                    status_t err;
                    CHECK(msg->findInt32("err", &err));
                    ALOGE("Decoder (%s) reported error : 0x%x",
                            mIsAudio ? "audio" : "video", err);

                    handleError(err);
                    break;
                }

                default:
                {
                    TRESPASS();
                    break;
                }
            }

            break;
        }

        case kWhatRenderBuffer:
        {
            if (!isStaleReply(msg)) {
                onRenderBuffer(msg);
            }
            break;
        }

        case kWhatAudioOutputFormatChanged:
        {
            if (!isStaleReply(msg)) {
                status_t err;
                if (msg->findInt32("err", &err) && err != OK) {
                    ALOGE("Renderer reported 0x%x when changing audio output format", err);
                    handleError(err);
                }
            }
            break;
        }

        case kWhatSetVideoSurface:
        {
            sp<AReplyToken> replyID;
            CHECK(msg->senderAwaitsResponse(&replyID));

            sp<RefBase> obj;
            CHECK(msg->findObject("surface", &obj));
            sp<Surface> surface = static_cast<Surface *>(obj.get()); // non-null
            int32_t err = INVALID_OPERATION;
            // NOTE: in practice mSurface is always non-null, but checking here for completeness
            if (mCodec != NULL && mSurface != NULL) {
                // TODO: once AwesomePlayer is removed, remove this automatic connecting
                // to the surface by MediaPlayerService.
                //
                // at this point MediaPlayerService::client has already connected to the
                // surface, which MediaCodec does not expect
                err = nativeWindowDisconnect(surface.get(), "kWhatSetVideoSurface(surface)");
                if (err == OK) {
                    err = mCodec->setSurface(surface);
                    ALOGI_IF(err, "codec setSurface returned: %d", err);
                    if (err == OK) {
                        // reconnect to the old surface as MPS::Client will expect to
                        // be able to disconnect from it.
                        (void)nativeWindowConnect(mSurface.get(), "kWhatSetVideoSurface(mSurface)");
                        mSurface = surface;
                    }
                }
                if (err != OK) {
                    // reconnect to the new surface on error as MPS::Client will expect to
                    // be able to disconnect from it.
                    (void)nativeWindowConnect(surface.get(), "kWhatSetVideoSurface(err)");
                }
            }

            sp<AMessage> response = new AMessage;
            response->setInt32("err", err);
            response->postReply(replyID);
            break;
        }

        case kWhatDrmReleaseCrypto:
        {
            ALOGV("kWhatDrmReleaseCrypto");
            onReleaseCrypto(msg);
            break;
        }

        default:
            DecoderBase::onMessageReceived(msg);
            break;
    }
}

void NuPlayer::Decoder::onConfigure(const sp<AMessage> &format) {
    CHECK(mCodec == NULL);

    mFormatChangePending = false;
    mTimeChangePending = false;

    ++mBufferGeneration;

    AString mime;
    CHECK(format->findString("mime", &mime));

    mIsAudio = !strncasecmp("audio/", mime.c_str(), 6);
    mIsVideoAVC = !strcasecmp(MEDIA_MIMETYPE_VIDEO_AVC, mime.c_str());

    mComponentName = mime;
    mComponentName.append(" decoder");
    ALOGV("[%s] onConfigure (surface=%p)", mComponentName.c_str(), mSurface.get());

    mCodec = MediaCodec::CreateByType(
            mCodecLooper, mime.c_str(), false /* encoder */, NULL /* err */, mPid, mUid);
    int32_t secure = 0;
    if (format->findInt32("secure", &secure) && secure != 0) {
        if (mCodec != NULL) {
            mCodec->getName(&mComponentName);
            mComponentName.append(".secure");
            mCodec->release();
            ALOGI("[%s] creating", mComponentName.c_str());
            mCodec = MediaCodec::CreateByComponentName(
                    mCodecLooper, mComponentName.c_str(), NULL /* err */, mPid, mUid);
        }
    }
    if (mCodec == NULL) {
        ALOGE("Failed to create %s%s decoder",
                (secure ? "secure " : ""), mime.c_str());
        handleError(UNKNOWN_ERROR);
        return;
    }
    mIsSecure = secure;

    mCodec->getName(&mComponentName);

    status_t err;
    if (mSurface != NULL) {
        // disconnect from surface as MediaCodec will reconnect
        err = nativeWindowDisconnect(mSurface.get(), "onConfigure");
        // We treat this as a warning, as this is a preparatory step.
        // Codec will try to connect to the surface, which is where
        // any error signaling will occur.
        ALOGW_IF(err != OK, "failed to disconnect from surface: %d", err);
    }

    // Modular DRM
    void *pCrypto;
    if (!format->findPointer("crypto", &pCrypto)) {
        pCrypto = NULL;
    }
    sp<ICrypto> crypto = (ICrypto*)pCrypto;
    // non-encrypted source won't have a crypto
    mIsEncrypted = (crypto != NULL);
    // configure is called once; still using OR in case the behavior changes.
    mIsEncryptedObservedEarlier = mIsEncryptedObservedEarlier || mIsEncrypted;
    ALOGV("onConfigure mCrypto: %p (%d)  mIsSecure: %d",
            crypto.get(), (crypto != NULL ? crypto->getStrongCount() : 0), mIsSecure);

    err = mCodec->configure(
            format, mSurface, crypto, 0 /* flags */);

    if (err != OK) {
        ALOGE("Failed to configure [%s] decoder (err=%d)", mComponentName.c_str(), err);
        mCodec->release();
        mCodec.clear();
        handleError(err);
        return;
    }
    rememberCodecSpecificData(format);

    // the following should work in configured state
    CHECK_EQ((status_t)OK, mCodec->getOutputFormat(&mOutputFormat));
    CHECK_EQ((status_t)OK, mCodec->getInputFormat(&mInputFormat));

    mStats->setString("mime", mime.c_str());
    mStats->setString("component-name", mComponentName.c_str());

    if (!mIsAudio) {
        int32_t width, height;
        if (mOutputFormat->findInt32("width", &width)
                && mOutputFormat->findInt32("height", &height)) {
            mStats->setInt32("width", width);
            mStats->setInt32("height", height);
        }
    }

    sp<AMessage> reply = new AMessage(kWhatCodecNotify, this);
    mCodec->setCallback(reply);

    err = mCodec->start();
    if (err != OK) {
        ALOGE("Failed to start [%s] decoder (err=%d)", mComponentName.c_str(), err);
        mCodec->release();
        mCodec.clear();
        handleError(err);
        return;
    }

    releaseAndResetMediaBuffers();

    mPaused = false;
    mResumePending = false;
}

void NuPlayer::Decoder::onSetParameters(const sp<AMessage> &params) {
    bool needAdjustLayers = false;
    float frameRateTotal;
    if (params->findFloat("frame-rate-total", &frameRateTotal)
            && mFrameRateTotal != frameRateTotal) {
        needAdjustLayers = true;
        mFrameRateTotal = frameRateTotal;
    }

    int32_t numVideoTemporalLayerTotal;
    if (params->findInt32("temporal-layer-count", &numVideoTemporalLayerTotal)
            && numVideoTemporalLayerTotal >= 0
            && numVideoTemporalLayerTotal <= kMaxNumVideoTemporalLayers
            && mNumVideoTemporalLayerTotal != numVideoTemporalLayerTotal) {
        needAdjustLayers = true;
        mNumVideoTemporalLayerTotal = std::max(numVideoTemporalLayerTotal, 1);
    }

    if (needAdjustLayers && mNumVideoTemporalLayerTotal > 1) {
        // TODO: For now, layer fps is calculated for some specific architectures.
        // But it really should be extracted from the stream.
        mVideoTemporalLayerAggregateFps[0] =
            mFrameRateTotal / (float)(1ll << (mNumVideoTemporalLayerTotal - 1));
        for (int32_t i = 1; i < mNumVideoTemporalLayerTotal; ++i) {
            mVideoTemporalLayerAggregateFps[i] =
                mFrameRateTotal / (float)(1ll << (mNumVideoTemporalLayerTotal - i))
                + mVideoTemporalLayerAggregateFps[i - 1];
        }
    }

    float playbackSpeed;
    if (params->findFloat("playback-speed", &playbackSpeed)
            && mPlaybackSpeed != playbackSpeed) {
        needAdjustLayers = true;
        mPlaybackSpeed = playbackSpeed;
    }

    if (needAdjustLayers) {
        float decodeFrameRate = mFrameRateTotal;
        // enable temporal layering optimization only if we know the layering depth
        if (mNumVideoTemporalLayerTotal > 1) {
            int32_t layerId;
            for (layerId = 0; layerId < mNumVideoTemporalLayerTotal - 1; ++layerId) {
                if (mVideoTemporalLayerAggregateFps[layerId] * mPlaybackSpeed
                        >= kDisplayRefreshingRate * 0.9) {
                    break;
                }
            }
            mNumVideoTemporalLayerAllowed = layerId + 1;
            decodeFrameRate = mVideoTemporalLayerAggregateFps[layerId];
        }
        ALOGV("onSetParameters: allowed layers=%d, decodeFps=%g",
                mNumVideoTemporalLayerAllowed, decodeFrameRate);

        if (mCodec == NULL) {
            ALOGW("onSetParameters called before codec is created.");
            return;
        }

        sp<AMessage> codecParams = new AMessage();
        codecParams->setFloat("operating-rate", decodeFrameRate * mPlaybackSpeed);
        mCodec->setParameters(codecParams);
    }
}

void NuPlayer::Decoder::onSetRenderer(const sp<Renderer> &renderer) {
    mRenderer = renderer;
}

void NuPlayer::Decoder::onResume(bool notifyComplete) {
    mPaused = false;

    if (notifyComplete) {
        mResumePending = true;
    }

    if (mCodec == NULL) {
        ALOGE("[%s] onResume without a valid codec", mComponentName.c_str());
        handleError(NO_INIT);
        return;
    }
    mCodec->start();
}

void NuPlayer::Decoder::doFlush(bool notifyComplete) {
    if (mCCDecoder != NULL) {
        mCCDecoder->flush();
    }

    if (mRenderer != NULL) {
        mRenderer->flush(mIsAudio, notifyComplete);
        mRenderer->signalTimeDiscontinuity();
    }

    status_t err = OK;
    if (mCodec != NULL) {
        err = mCodec->flush();
        mCSDsToSubmit = mCSDsForCurrentFormat; // copy operator
        ++mBufferGeneration;
    }

    if (err != OK) {
        ALOGE("failed to flush [%s] (err=%d)", mComponentName.c_str(), err);
        handleError(err);
        // finish with posting kWhatFlushCompleted.
        // we attempt to release the buffers even if flush fails.
    }
    releaseAndResetMediaBuffers();
    mPaused = true;
}


void NuPlayer::Decoder::onFlush() {
    doFlush(true);

    if (isDiscontinuityPending()) {
        // This could happen if the client starts seeking/shutdown
        // after we queued an EOS for discontinuities.
        // We can consider discontinuity handled.
        finishHandleDiscontinuity(false /* flushOnTimeChange */);
    }

    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", kWhatFlushCompleted);
    notify->post();
}

void NuPlayer::Decoder::onShutdown(bool notifyComplete) {
    status_t err = OK;

    // if there is a pending resume request, notify complete now
    notifyResumeCompleteIfNecessary();

    if (mCodec != NULL) {
        err = mCodec->release();
        mCodec = NULL;
        ++mBufferGeneration;

        if (mSurface != NULL) {
            // reconnect to surface as MediaCodec disconnected from it
            status_t error = nativeWindowConnect(mSurface.get(), "onShutdown");
            ALOGW_IF(error != NO_ERROR,
                    "[%s] failed to connect to native window, error=%d",
                    mComponentName.c_str(), error);
        }
        mComponentName = "decoder";
    }

    releaseAndResetMediaBuffers();

    if (err != OK) {
        ALOGE("failed to release [%s] (err=%d)", mComponentName.c_str(), err);
        handleError(err);
        // finish with posting kWhatShutdownCompleted.
    }

    if (notifyComplete) {
        sp<AMessage> notify = mNotify->dup();
        notify->setInt32("what", kWhatShutdownCompleted);
        notify->post();
        mPaused = true;
    }
}

/*
 * returns true if we should request more data
 */
bool NuPlayer::Decoder::doRequestBuffers() {
    if (isDiscontinuityPending()) {
        return false;
    }
    status_t err = OK;
    while (err == OK && !mDequeuedInputBuffers.empty()) {
        size_t bufferIx = *mDequeuedInputBuffers.begin();
        sp<AMessage> msg = new AMessage();
        msg->setSize("buffer-ix", bufferIx);
        err = fetchInputData(msg);
        if (err != OK && err != ERROR_END_OF_STREAM) {
            // if EOS, need to queue EOS buffer
            break;
        }
        mDequeuedInputBuffers.erase(mDequeuedInputBuffers.begin());

        if (!mPendingInputMessages.empty()
                || !onInputBufferFetched(msg)) {
            mPendingInputMessages.push_back(msg);
        }
    }

    return err == -EWOULDBLOCK
            && mSource->feedMoreTSData() == OK;
}

void NuPlayer::Decoder::handleError(int32_t err)
{
    // We cannot immediately release the codec due to buffers still outstanding
    // in the renderer.  We signal to the player the error so it can shutdown/release the
    // decoder after flushing and increment the generation to discard unnecessary messages.

    ++mBufferGeneration;

    sp<AMessage> notify = mNotify->dup();
    notify->setInt32("what", kWhatError);
    notify->setInt32("err", err);
    notify->post();
}

status_t NuPlayer::Decoder::releaseCrypto()
{
    ALOGV("releaseCrypto");

    sp<AMessage> msg = new AMessage(kWhatDrmReleaseCrypto, this);

    sp<AMessage> response;
    status_t status = msg->postAndAwaitResponse(&response);
    if (status == OK && response != NULL) {
        CHECK(response->findInt32("status", &status));
        ALOGV("releaseCrypto ret: %d ", status);
    } else {
        ALOGE("releaseCrypto err: %d", status);
    }

    return status;
}

void NuPlayer::Decoder::onReleaseCrypto(const sp<AMessage>& msg)
{
    status_t status = INVALID_OPERATION;
    if (mCodec != NULL) {
        status = mCodec->releaseCrypto();
    } else {
        // returning OK if the codec has been already released
        status = OK;
        ALOGE("onReleaseCrypto No mCodec. err: %d", status);
    }

    sp<AMessage> response = new AMessage;
    response->setInt32("status", status);
    // Clearing the state as it's tied to crypto. mIsEncryptedObservedEarlier is sticky though
    // and lasts for the lifetime of this codec. See its use in fetchInputData.
    mIsEncrypted = false;

    sp<AReplyToken> replyID;
    CHECK(msg->senderAwaitsResponse(&replyID));
    response->postReply(replyID);
}

bool NuPlayer::Decoder::handleAnInputBuffer(size_t index) {
    if (isDiscontinuityPending()) {
        return false;
    }

    if (mCodec == NULL) {
        ALOGE("[%s] handleAnInputBuffer without a valid codec", mComponentName.c_str());
        handleError(NO_INIT);
        return false;
    }

    sp<MediaCodecBuffer> buffer;
    mCodec->getInputBuffer(index, &buffer);

    if (buffer == NULL) {
        ALOGE("[%s] handleAnInputBuffer, failed to get input buffer", mComponentName.c_str());
        handleError(UNKNOWN_ERROR);
        return false;
    }

    if (index >= mInputBuffers.size()) {
        for (size_t i = mInputBuffers.size(); i <= index; ++i) {
            mInputBuffers.add();
            mMediaBuffers.add();
            mInputBufferIsDequeued.add();
            mMediaBuffers.editItemAt(i) = NULL;
            mInputBufferIsDequeued.editItemAt(i) = false;
        }
    }
    mInputBuffers.editItemAt(index) = buffer;

    //CHECK_LT(bufferIx, mInputBuffers.size());

    if (mMediaBuffers[index] != NULL) {
        mMediaBuffers[index]->release();
        mMediaBuffers.editItemAt(index) = NULL;
    }
    mInputBufferIsDequeued.editItemAt(index) = true;

    if (!mCSDsToSubmit.isEmpty()) {
        sp<AMessage> msg = new AMessage();
        msg->setSize("buffer-ix", index);

        sp<ABuffer> buffer = mCSDsToSubmit.itemAt(0);
        ALOGI("[%s] resubmitting CSD", mComponentName.c_str());
        msg->setBuffer("buffer", buffer);
        mCSDsToSubmit.removeAt(0);
        if (!onInputBufferFetched(msg)) {
            handleError(UNKNOWN_ERROR);
            return false;
        }
        return true;
    }

    while (!mPendingInputMessages.empty()) {
        sp<AMessage> msg = *mPendingInputMessages.begin();
        if (!onInputBufferFetched(msg)) {
            break;
        }
        mPendingInputMessages.erase(mPendingInputMessages.begin());
    }

    if (!mInputBufferIsDequeued.editItemAt(index)) {
        return true;
    }

    mDequeuedInputBuffers.push_back(index);

    onRequestInputBuffers();
    return true;
}

bool NuPlayer::Decoder::handleAnOutputBuffer(
        size_t index,
        size_t offset,
        size_t size,
        int64_t timeUs,
        int32_t flags) {
    if (mCodec == NULL) {
        ALOGE("[%s] handleAnOutputBuffer without a valid codec", mComponentName.c_str());
        handleError(NO_INIT);
        return false;
    }

//    CHECK_LT(bufferIx, mOutputBuffers.size());
    sp<MediaCodecBuffer> buffer;
    mCodec->getOutputBuffer(index, &buffer);

    if (buffer == NULL) {
        ALOGE("[%s] handleAnOutputBuffer, failed to get output buffer", mComponentName.c_str());
        handleError(UNKNOWN_ERROR);
        return false;
    }

    if (index >= mOutputBuffers.size()) {
        for (size_t i = mOutputBuffers.size(); i <= index; ++i) {
            mOutputBuffers.add();
        }
    }

    mOutputBuffers.editItemAt(index) = buffer;

    buffer->setRange(offset, size);
    buffer->meta()->clear();
    buffer->meta()->setInt64("timeUs", timeUs);

    bool eos = flags & MediaCodec::BUFFER_FLAG_EOS;
    // we do not expect CODECCONFIG or SYNCFRAME for decoder

    sp<AMessage> reply = new AMessage(kWhatRenderBuffer, this);
    reply->setSize("buffer-ix", index);
    reply->setInt32("generation", mBufferGeneration);
    reply->setSize("size", size);

    if (eos) {
        ALOGI("[%s] saw output EOS", mIsAudio ? "audio" : "video");

        buffer->meta()->setInt32("eos", true);
        reply->setInt32("eos", true);
    }

    mNumFramesTotal += !mIsAudio;

    if (mSkipRenderingUntilMediaTimeUs >= 0) {
        if (timeUs < mSkipRenderingUntilMediaTimeUs) {
            ALOGV("[%s] dropping buffer at time %lld as requested.",
                     mComponentName.c_str(), (long long)timeUs);

            reply->post();
            if (eos) {
                notifyResumeCompleteIfNecessary();
                if (mRenderer != NULL && !isDiscontinuityPending()) {
                    mRenderer->queueEOS(mIsAudio, ERROR_END_OF_STREAM);
                }
            }
            return true;
        }

        mSkipRenderingUntilMediaTimeUs = -1;
    }

    // wait until 1st frame comes out to signal resume complete
    notifyResumeCompleteIfNecessary();

    if (mRenderer != NULL) {
        // send the buffer to renderer.
        mRenderer->queueBuffer(mIsAudio, buffer, reply);
        if (eos && !isDiscontinuityPending()) {
            mRenderer->queueEOS(mIsAudio, ERROR_END_OF_STREAM);
        }
    }

    return true;
}

void NuPlayer::Decoder::handleOutputFormatChange(const sp<AMessage> &format) {
    if (!mIsAudio) {
        int32_t width, height;
        if (format->findInt32("width", &width)
                && format->findInt32("height", &height)) {
            mStats->setInt32("width", width);
            mStats->setInt32("height", height);
        }
        sp<AMessage> notify = mNotify->dup();
        notify->setInt32("what", kWhatVideoSizeChanged);
        notify->setMessage("format", format);
        notify->post();
    } else if (mRenderer != NULL) {
        uint32_t flags;
        int64_t durationUs;
        bool hasVideo = (mSource->getFormat(false /* audio */) != NULL);
        if (getAudioDeepBufferSetting() // override regardless of source duration
                || (mSource->getDuration(&durationUs) == OK
                        && durationUs > AUDIO_SINK_MIN_DEEP_BUFFER_DURATION_US)) {
            flags = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
        } else {
            flags = AUDIO_OUTPUT_FLAG_NONE;
        }

        sp<AMessage> reply = new AMessage(kWhatAudioOutputFormatChanged, this);
        reply->setInt32("generation", mBufferGeneration);
        mRenderer->changeAudioFormat(
                format, false /* offloadOnly */, hasVideo,
                flags, mSource->isStreaming(), reply);
    }
}

void NuPlayer::Decoder::releaseAndResetMediaBuffers() {
    for (size_t i = 0; i < mMediaBuffers.size(); i++) {
        if (mMediaBuffers[i] != NULL) {
            mMediaBuffers[i]->release();
            mMediaBuffers.editItemAt(i) = NULL;
        }
    }
    mMediaBuffers.resize(mInputBuffers.size());
    for (size_t i = 0; i < mMediaBuffers.size(); i++) {
        mMediaBuffers.editItemAt(i) = NULL;
    }
    mInputBufferIsDequeued.clear();
    mInputBufferIsDequeued.resize(mInputBuffers.size());
    for (size_t i = 0; i < mInputBufferIsDequeued.size(); i++) {
        mInputBufferIsDequeued.editItemAt(i) = false;
    }

    mPendingInputMessages.clear();
    mDequeuedInputBuffers.clear();
    mSkipRenderingUntilMediaTimeUs = -1;
}

void NuPlayer::Decoder::requestCodecNotification() {
    if (mCodec != NULL) {
        sp<AMessage> reply = new AMessage(kWhatCodecNotify, this);
        reply->setInt32("generation", mBufferGeneration);
        mCodec->requestActivityNotification(reply);
    }
}

bool NuPlayer::Decoder::isStaleReply(const sp<AMessage> &msg) {
    int32_t generation;
    CHECK(msg->findInt32("generation", &generation));
    return generation != mBufferGeneration;
}

status_t NuPlayer::Decoder::fetchInputData(sp<AMessage> &reply) {
    sp<ABuffer> accessUnit;
    bool dropAccessUnit = true;
    do {
        status_t err = mSource->dequeueAccessUnit(mIsAudio, &accessUnit);

        if (err == -EWOULDBLOCK) {
            return err;
        } else if (err != OK) {
            if (err == INFO_DISCONTINUITY) {
                int32_t type;
                CHECK(accessUnit->meta()->findInt32("discontinuity", &type));

                bool formatChange =
                    (mIsAudio &&
                     (type & ATSParser::DISCONTINUITY_AUDIO_FORMAT))
                    || (!mIsAudio &&
                            (type & ATSParser::DISCONTINUITY_VIDEO_FORMAT));

                bool timeChange = (type & ATSParser::DISCONTINUITY_TIME) != 0;

                ALOGI("%s discontinuity (format=%d, time=%d)",
                        mIsAudio ? "audio" : "video", formatChange, timeChange);

                bool seamlessFormatChange = false;
                sp<AMessage> newFormat = mSource->getFormat(mIsAudio);
                if (formatChange) {
                    seamlessFormatChange =
                        supportsSeamlessFormatChange(newFormat);
                    // treat seamless format change separately
                    formatChange = !seamlessFormatChange;
                }

                // For format or time change, return EOS to queue EOS input,
                // then wait for EOS on output.
                if (formatChange /* not seamless */) {
                    mFormatChangePending = true;
                    err = ERROR_END_OF_STREAM;
                } else if (timeChange) {
                    rememberCodecSpecificData(newFormat);
                    mTimeChangePending = true;
                    err = ERROR_END_OF_STREAM;
                } else if (seamlessFormatChange) {
                    // reuse existing decoder and don't flush
                    rememberCodecSpecificData(newFormat);
                    continue;
                } else {
                    // This stream is unaffected by the discontinuity
                    return -EWOULDBLOCK;
                }
            }

            // reply should only be returned without a buffer set
            // when there is an error (including EOS)
            CHECK(err != OK);

            reply->setInt32("err", err);
            return ERROR_END_OF_STREAM;
        }

        dropAccessUnit = false;
        if (!mIsAudio && !mIsEncrypted) {
            // Extra safeguard if higher-level behavior changes. Otherwise, not required now.
            // Preventing the buffer from being processed (and sent to codec) if this is a later
            // round of playback but this time without prepareDrm. Or if there is a race between
            // stop (which is not blocking) and releaseDrm allowing buffers being processed after
            // Crypto has been released (GenericSource currently prevents this race though).
            // Particularly doing this check before IsAVCReferenceFrame call to prevent parsing
            // of encrypted data.
            if (mIsEncryptedObservedEarlier) {
                ALOGE("fetchInputData: mismatched mIsEncrypted/mIsEncryptedObservedEarlier (0/1)");

                return INVALID_OPERATION;
            }

            int32_t layerId = 0;
            bool haveLayerId = accessUnit->meta()->findInt32("temporal-layer-id", &layerId);
            if (mRenderer->getVideoLateByUs() > 100000ll
                    && mIsVideoAVC
                    && !IsAVCReferenceFrame(accessUnit)) {
                dropAccessUnit = true;
            } else if (haveLayerId && mNumVideoTemporalLayerTotal > 1) {
                // Add only one layer each time.
                if (layerId > mCurrentMaxVideoTemporalLayerId + 1
                        || layerId >= mNumVideoTemporalLayerAllowed) {
                    dropAccessUnit = true;
                    ALOGV("dropping layer(%d), speed=%g, allowed layer count=%d, max layerId=%d",
                            layerId, mPlaybackSpeed, mNumVideoTemporalLayerAllowed,
                            mCurrentMaxVideoTemporalLayerId);
                } else if (layerId > mCurrentMaxVideoTemporalLayerId) {
                    mCurrentMaxVideoTemporalLayerId = layerId;
                } else if (layerId == 0 && mNumVideoTemporalLayerTotal > 1
                        && IsIDR(accessUnit->data(), accessUnit->size())) {
                    mCurrentMaxVideoTemporalLayerId = mNumVideoTemporalLayerTotal - 1;
                }
            }
            if (dropAccessUnit) {
                if (layerId <= mCurrentMaxVideoTemporalLayerId && layerId > 0) {
                    mCurrentMaxVideoTemporalLayerId = layerId - 1;
                }
                ++mNumInputFramesDropped;
            }
        }
    } while (dropAccessUnit);

    // ALOGV("returned a valid buffer of %s data", mIsAudio ? "mIsAudio" : "video");
#if 0
    int64_t mediaTimeUs;
    CHECK(accessUnit->meta()->findInt64("timeUs", &mediaTimeUs));
    ALOGV("[%s] feeding input buffer at media time %.3f",
         mIsAudio ? "audio" : "video",
         mediaTimeUs / 1E6);
#endif

    if (mCCDecoder != NULL) {
        mCCDecoder->decode(accessUnit);
    }

    reply->setBuffer("buffer", accessUnit);

    return OK;
}

bool NuPlayer::Decoder::onInputBufferFetched(const sp<AMessage> &msg) {
    if (mCodec == NULL) {
        ALOGE("[%s] onInputBufferFetched without a valid codec", mComponentName.c_str());
        handleError(NO_INIT);
        return false;
    }

    size_t bufferIx;
    CHECK(msg->findSize("buffer-ix", &bufferIx));
    CHECK_LT(bufferIx, mInputBuffers.size());
    sp<MediaCodecBuffer> codecBuffer = mInputBuffers[bufferIx];

    sp<ABuffer> buffer;
    bool hasBuffer = msg->findBuffer("buffer", &buffer);
    bool needsCopy = true;

    if (buffer == NULL /* includes !hasBuffer */) {
        int32_t streamErr = ERROR_END_OF_STREAM;
        CHECK(msg->findInt32("err", &streamErr) || !hasBuffer);

        CHECK(streamErr != OK);

        // attempt to queue EOS
        status_t err = mCodec->queueInputBuffer(
                bufferIx,
                0,
                0,
                0,
                MediaCodec::BUFFER_FLAG_EOS);
        if (err == OK) {
            mInputBufferIsDequeued.editItemAt(bufferIx) = false;
        } else if (streamErr == ERROR_END_OF_STREAM) {
            streamErr = err;
            // err will not be ERROR_END_OF_STREAM
        }

        if (streamErr != ERROR_END_OF_STREAM) {
            ALOGE("Stream error for [%s] (err=%d), EOS %s queued",
                    mComponentName.c_str(),
                    streamErr,
                    err == OK ? "successfully" : "unsuccessfully");
            handleError(streamErr);
        }
    } else {
        sp<AMessage> extra;
        if (buffer->meta()->findMessage("extra", &extra) && extra != NULL) {
            int64_t resumeAtMediaTimeUs;
            if (extra->findInt64(
                        "resume-at-mediaTimeUs", &resumeAtMediaTimeUs)) {
                ALOGI("[%s] suppressing rendering until %lld us",
                        mComponentName.c_str(), (long long)resumeAtMediaTimeUs);
                mSkipRenderingUntilMediaTimeUs = resumeAtMediaTimeUs;
            }
        }

        int64_t timeUs = 0;
        uint32_t flags = 0;
        CHECK(buffer->meta()->findInt64("timeUs", &timeUs));

        int32_t eos, csd;
        // we do not expect SYNCFRAME for decoder
        if (buffer->meta()->findInt32("eos", &eos) && eos) {
            flags |= MediaCodec::BUFFER_FLAG_EOS;
        } else if (buffer->meta()->findInt32("csd", &csd) && csd) {
            flags |= MediaCodec::BUFFER_FLAG_CODECCONFIG;
        }

        // Modular DRM
        MediaBufferBase *mediaBuf = NULL;
        NuPlayerDrm::CryptoInfo *cryptInfo = NULL;

        // copy into codec buffer
        if (needsCopy) {
            if (buffer->size() > codecBuffer->capacity()) {
                handleError(ERROR_BUFFER_TOO_SMALL);
                mDequeuedInputBuffers.push_back(bufferIx);
                return false;
            }

            if (buffer->data() != NULL) {
                codecBuffer->setRange(0, buffer->size());
                memcpy(codecBuffer->data(), buffer->data(), buffer->size());
            } else { // No buffer->data()
                //Modular DRM
                sp<RefBase> holder;
                if (buffer->meta()->findObject("mediaBufferHolder", &holder)) {
                    mediaBuf = (holder != nullptr) ?
                        static_cast<MediaBufferHolder*>(holder.get())->mediaBuffer() : nullptr;
                }
                if (mediaBuf != NULL) {
                    codecBuffer->setRange(0, mediaBuf->size());
                    memcpy(codecBuffer->data(), mediaBuf->data(), mediaBuf->size());

                    MetaDataBase &meta_data = mediaBuf->meta_data();
                    cryptInfo = NuPlayerDrm::getSampleCryptoInfo(meta_data);
                } else { // No mediaBuf
                    ALOGE("onInputBufferFetched: buffer->data()/mediaBuf are NULL for %p",
                            buffer.get());
                    handleError(UNKNOWN_ERROR);
                    return false;
                }
            } // buffer->data()
        } // needsCopy

        status_t err;
        AString errorDetailMsg;
        if (cryptInfo != NULL) {
            err = mCodec->queueSecureInputBuffer(
                    bufferIx,
                    codecBuffer->offset(),
                    cryptInfo->subSamples,
                    cryptInfo->numSubSamples,
                    cryptInfo->key,
                    cryptInfo->iv,
                    cryptInfo->mode,
                    cryptInfo->pattern,
                    timeUs,
                    flags,
                    &errorDetailMsg);
            // synchronous call so done with cryptInfo here
            free(cryptInfo);
        } else {
            err = mCodec->queueInputBuffer(
                    bufferIx,
                    codecBuffer->offset(),
                    codecBuffer->size(),
                    timeUs,
                    flags,
                    &errorDetailMsg);
        } // no cryptInfo

        if (err != OK) {
            ALOGE("onInputBufferFetched: queue%sInputBuffer failed for [%s] (err=%d, %s)",
                    (cryptInfo != NULL ? "Secure" : ""),
                    mComponentName.c_str(), err, errorDetailMsg.c_str());
            handleError(err);
        } else {
            mInputBufferIsDequeued.editItemAt(bufferIx) = false;
        }

    }   // buffer != NULL
    return true;
}

void NuPlayer::Decoder::onRenderBuffer(const sp<AMessage> &msg) {
    status_t err;
    int32_t render;
    size_t bufferIx;
    int32_t eos;
    size_t size;
    CHECK(msg->findSize("buffer-ix", &bufferIx));

    if (!mIsAudio) {
        int64_t timeUs;
        sp<MediaCodecBuffer> buffer = mOutputBuffers[bufferIx];
        buffer->meta()->findInt64("timeUs", &timeUs);

        if (mCCDecoder != NULL && mCCDecoder->isSelected()) {
            mCCDecoder->display(timeUs);
        }
    }

    if (mCodec == NULL) {
        err = NO_INIT;
    } else if (msg->findInt32("render", &render) && render) {
        int64_t timestampNs;
        CHECK(msg->findInt64("timestampNs", &timestampNs));
        err = mCodec->renderOutputBufferAndRelease(bufferIx, timestampNs);
    } else {
        if (!msg->findInt32("eos", &eos) || !eos ||
                !msg->findSize("size", &size) || size) {
            mNumOutputFramesDropped += !mIsAudio;
        }
        err = mCodec->releaseOutputBuffer(bufferIx);
    }
    if (err != OK) {
        ALOGE("failed to release output buffer for [%s] (err=%d)",
                mComponentName.c_str(), err);
        handleError(err);
    }
    if (msg->findInt32("eos", &eos) && eos
            && isDiscontinuityPending()) {
        finishHandleDiscontinuity(true /* flushOnTimeChange */);
    }
}

bool NuPlayer::Decoder::isDiscontinuityPending() const {
    return mFormatChangePending || mTimeChangePending;
}

void NuPlayer::Decoder::finishHandleDiscontinuity(bool flushOnTimeChange) {
    ALOGV("finishHandleDiscontinuity: format %d, time %d, flush %d",
            mFormatChangePending, mTimeChangePending, flushOnTimeChange);

    // If we have format change, pause and wait to be killed;
    // If we have time change only, flush and restart fetching.

    if (mFormatChangePending) {
        mPaused = true;
    } else if (mTimeChangePending) {
        if (flushOnTimeChange) {
            doFlush(false /* notifyComplete */);
            signalResume(false /* notifyComplete */);
        }
    }

    // Notify NuPlayer to either shutdown decoder, or rescan sources
    sp<AMessage> msg = mNotify->dup();
    msg->setInt32("what", kWhatInputDiscontinuity);
    msg->setInt32("formatChange", mFormatChangePending);
    msg->post();

    mFormatChangePending = false;
    mTimeChangePending = false;
}

bool NuPlayer::Decoder::supportsSeamlessAudioFormatChange(
        const sp<AMessage> &targetFormat) const {
    if (targetFormat == NULL) {
        return true;
    }

    AString mime;
    if (!targetFormat->findString("mime", &mime)) {
        return false;
    }

    if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_AAC)) {
        // field-by-field comparison
        const char * keys[] = { "channel-count", "sample-rate", "is-adts" };
        for (unsigned int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
            int32_t oldVal, newVal;
            if (!mInputFormat->findInt32(keys[i], &oldVal) ||
                    !targetFormat->findInt32(keys[i], &newVal) ||
                    oldVal != newVal) {
                return false;
            }
        }

        sp<ABuffer> oldBuf, newBuf;
        if (mInputFormat->findBuffer("csd-0", &oldBuf) &&
                targetFormat->findBuffer("csd-0", &newBuf)) {
            if (oldBuf->size() != newBuf->size()) {
                return false;
            }
            return !memcmp(oldBuf->data(), newBuf->data(), oldBuf->size());
        }
    }
    return false;
}

bool NuPlayer::Decoder::supportsSeamlessFormatChange(const sp<AMessage> &targetFormat) const {
    if (mInputFormat == NULL) {
        return false;
    }

    if (targetFormat == NULL) {
        return true;
    }

    AString oldMime, newMime;
    if (!mInputFormat->findString("mime", &oldMime)
            || !targetFormat->findString("mime", &newMime)
            || !(oldMime == newMime)) {
        return false;
    }

    bool audio = !strncasecmp(oldMime.c_str(), "audio/", strlen("audio/"));
    bool seamless;
    if (audio) {
        seamless = supportsSeamlessAudioFormatChange(targetFormat);
    } else {
        int32_t isAdaptive;
        seamless = (mCodec != NULL &&
                mInputFormat->findInt32("adaptive-playback", &isAdaptive) &&
                isAdaptive);
    }

    ALOGV("%s seamless support for %s", seamless ? "yes" : "no", oldMime.c_str());
    return seamless;
}

void NuPlayer::Decoder::rememberCodecSpecificData(const sp<AMessage> &format) {
    if (format == NULL) {
        return;
    }
    mCSDsForCurrentFormat.clear();
    for (int32_t i = 0; ; ++i) {
        AString tag = "csd-";
        tag.append(i);
        sp<ABuffer> buffer;
        if (!format->findBuffer(tag.c_str(), &buffer)) {
            break;
        }
        mCSDsForCurrentFormat.push(buffer);
    }
}

void NuPlayer::Decoder::notifyResumeCompleteIfNecessary() {
    if (mResumePending) {
        mResumePending = false;

        sp<AMessage> notify = mNotify->dup();
        notify->setInt32("what", kWhatResumeCompleted);
        notify->post();
    }
}

}  // namespace android

