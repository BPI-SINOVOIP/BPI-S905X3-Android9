/*
 * Copyright (C) 2016-2018 The Android Open Source Project
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

#include "Camera3SharedOutputStream.h"

namespace android {

namespace camera3 {

const size_t Camera3SharedOutputStream::kMaxOutputs;

Camera3SharedOutputStream::Camera3SharedOutputStream(int id,
        const std::vector<sp<Surface>>& surfaces,
        uint32_t width, uint32_t height, int format,
        uint64_t consumerUsage, android_dataspace dataSpace,
        camera3_stream_rotation_t rotation,
        nsecs_t timestampOffset, const String8& physicalCameraId,
        int setId) :
        Camera3OutputStream(id, CAMERA3_STREAM_OUTPUT, width, height,
                            format, dataSpace, rotation, physicalCameraId,
                            consumerUsage, timestampOffset, setId) {
    size_t consumerCount = std::min(surfaces.size(), kMaxOutputs);
    if (surfaces.size() > consumerCount) {
        ALOGE("%s: Trying to add more consumers than the maximum ", __func__);
    }
    for (size_t i = 0; i < consumerCount; i++) {
        mSurfaces[i] = surfaces[i];
    }
}

Camera3SharedOutputStream::~Camera3SharedOutputStream() {
    disconnectLocked();
}

status_t Camera3SharedOutputStream::connectStreamSplitterLocked() {
    status_t res = OK;

    mStreamSplitter = new Camera3StreamSplitter();

    uint64_t usage;
    getEndpointUsage(&usage);

    std::unordered_map<size_t, sp<Surface>> initialSurfaces;
    for (size_t i = 0; i < kMaxOutputs; i++) {
        if (mSurfaces[i] != nullptr) {
            initialSurfaces.emplace(i, mSurfaces[i]);
        }
    }

    android::PixelFormat format = isFormatOverridden() ? getOriginalFormat() : getFormat();
    res = mStreamSplitter->connect(initialSurfaces, usage, mUsage, camera3_stream::max_buffers,
            getWidth(), getHeight(), format, &mConsumer);
    if (res != OK) {
        ALOGE("%s: Failed to connect to stream splitter: %s(%d)",
                __FUNCTION__, strerror(-res), res);
        return res;
    }

    return res;
}

status_t Camera3SharedOutputStream::notifyBufferReleased(ANativeWindowBuffer *anwBuffer) {
    Mutex::Autolock l(mLock);
    status_t res = OK;
    const sp<GraphicBuffer> buffer(static_cast<GraphicBuffer*>(anwBuffer));

    if (mStreamSplitter != nullptr) {
        res = mStreamSplitter->notifyBufferReleased(buffer);
    }

    return res;
}

bool Camera3SharedOutputStream::isConsumerConfigurationDeferred(size_t surface_id) const {
    Mutex::Autolock l(mLock);
    if (surface_id >= kMaxOutputs) {
        return true;
    }

    return (mSurfaces[surface_id] == nullptr);
}

status_t Camera3SharedOutputStream::setConsumers(const std::vector<sp<Surface>>& surfaces) {
    Mutex::Autolock l(mLock);
    if (surfaces.size() == 0) {
        ALOGE("%s: it's illegal to set zero consumer surfaces!", __FUNCTION__);
        return INVALID_OPERATION;
    }

    status_t ret = OK;
    for (auto& surface : surfaces) {
        if (surface == nullptr) {
            ALOGE("%s: it's illegal to set a null consumer surface!", __FUNCTION__);
            return INVALID_OPERATION;
        }

        ssize_t id = getNextSurfaceIdLocked();
        if (id < 0) {
            ALOGE("%s: No surface ids available!", __func__);
            return NO_MEMORY;
        }

        mSurfaces[id] = surface;

        // Only call addOutput if the splitter has been connected.
        if (mStreamSplitter != nullptr) {
            ret = mStreamSplitter->addOutput(id, surface);
            if (ret != OK) {
                ALOGE("%s: addOutput failed with error code %d", __FUNCTION__, ret);
                return ret;

            }
        }
    }
    return ret;
}

status_t Camera3SharedOutputStream::getBufferLocked(camera3_stream_buffer *buffer,
        const std::vector<size_t>& surface_ids) {
    ANativeWindowBuffer* anb;
    int fenceFd = -1;

    status_t res;
    res = getBufferLockedCommon(&anb, &fenceFd);
    if (res != OK) {
        return res;
    }

    // Attach the buffer to the splitter output queues. This could block if
    // the output queue doesn't have any empty slot. So unlock during the course
    // of attachBufferToOutputs.
    sp<Camera3StreamSplitter> splitter = mStreamSplitter;
    mLock.unlock();
    res = splitter->attachBufferToOutputs(anb, surface_ids);
    mLock.lock();
    if (res != OK) {
        ALOGE("%s: Stream %d: Cannot attach stream splitter buffer to outputs: %s (%d)",
                __FUNCTION__, mId, strerror(-res), res);
        // Only transition to STATE_ABANDONED from STATE_CONFIGURED. (If it is STATE_PREPARING,
        // let prepareNextBuffer handle the error.)
        if (res == NO_INIT && mState == STATE_CONFIGURED) {
            mState = STATE_ABANDONED;
        }

        return res;
    }

    /**
     * FenceFD now owned by HAL except in case of error,
     * in which case we reassign it to acquire_fence
     */
    handoutBufferLocked(*buffer, &(anb->handle), /*acquireFence*/fenceFd,
                        /*releaseFence*/-1, CAMERA3_BUFFER_STATUS_OK, /*output*/true);

    return OK;
}

status_t Camera3SharedOutputStream::queueBufferToConsumer(sp<ANativeWindow>& consumer,
            ANativeWindowBuffer* buffer, int anwReleaseFence) {
    status_t res = consumer->queueBuffer(consumer.get(), buffer, anwReleaseFence);

    // After queuing buffer to the internal consumer queue, check whether the buffer is
    // successfully queued to the output queues.
    if (res == OK) {
        res = mStreamSplitter->getOnFrameAvailableResult();
        if (res != OK) {
            ALOGE("%s: getOnFrameAvailable returns %d", __FUNCTION__, res);
        }
    } else {
        ALOGE("%s: queueBufer failed %d", __FUNCTION__, res);
    }

    return res;
}

status_t Camera3SharedOutputStream::configureQueueLocked() {
    status_t res;

    if ((res = Camera3IOStreamBase::configureQueueLocked()) != OK) {
        return res;
    }

    res = connectStreamSplitterLocked();
    if (res != OK) {
        ALOGE("Cannot connect to stream splitter: %s(%d)", strerror(-res), res);
        return res;
    }

    res = configureConsumerQueueLocked();
    if (res != OK) {
        ALOGE("Failed to configureConsumerQueueLocked: %s(%d)", strerror(-res), res);
        return res;
    }

    return OK;
}

status_t Camera3SharedOutputStream::disconnectLocked() {
    status_t res;
    res = Camera3OutputStream::disconnectLocked();

    if (mStreamSplitter != nullptr) {
        mStreamSplitter->disconnect();
    }

    return res;
}

status_t Camera3SharedOutputStream::getEndpointUsage(uint64_t *usage) const {

    status_t res = OK;
    uint64_t u = 0;

    if (mConsumer == nullptr) {
        // Called before shared buffer queue is constructed.
        *usage = getPresetConsumerUsage();

        for (size_t id = 0; id < kMaxOutputs; id++) {
            if (mSurfaces[id] != nullptr) {
                res = getEndpointUsageForSurface(&u, mSurfaces[id]);
                *usage |= u;
            }
        }
    } else {
        // Called after shared buffer queue is constructed.
        res = getEndpointUsageForSurface(&u, mConsumer);
        *usage |= u;
    }

    return res;
}

ssize_t Camera3SharedOutputStream::getNextSurfaceIdLocked() {
    ssize_t id = -1;
    for (size_t i = 0; i < kMaxOutputs; i++) {
        if (mSurfaces[i] == nullptr) {
            id = i;
            break;
        }
    }

    return id;
}

ssize_t Camera3SharedOutputStream::getSurfaceId(const sp<Surface> &surface) {
    Mutex::Autolock l(mLock);
    ssize_t id = -1;
    for (size_t i = 0; i < kMaxOutputs; i++) {
        if (mSurfaces[i] == surface) {
            id = i;
            break;
        }
    }

    return id;
}

status_t Camera3SharedOutputStream::revertPartialUpdateLocked(
        const KeyedVector<sp<Surface>, size_t> &removedSurfaces,
        const KeyedVector<sp<Surface>, size_t> &attachedSurfaces) {
    status_t ret = OK;

    for (size_t i = 0; i < attachedSurfaces.size(); i++) {
        size_t index = attachedSurfaces.valueAt(i);
        if (mStreamSplitter != nullptr) {
            ret = mStreamSplitter->removeOutput(index);
            if (ret != OK) {
                return UNKNOWN_ERROR;
            }
        }
        mSurfaces[index] = nullptr;
    }

    for (size_t i = 0; i < removedSurfaces.size(); i++) {
        size_t index = removedSurfaces.valueAt(i);
        if (mStreamSplitter != nullptr) {
            ret = mStreamSplitter->addOutput(index, removedSurfaces.keyAt(i));
            if (ret != OK) {
                return UNKNOWN_ERROR;
            }
        }
        mSurfaces[index] = removedSurfaces.keyAt(i);
    }

    return ret;
}

status_t Camera3SharedOutputStream::updateStream(const std::vector<sp<Surface>> &outputSurfaces,
        const std::vector<OutputStreamInfo> &outputInfo,
        const std::vector<size_t> &removedSurfaceIds,
        KeyedVector<sp<Surface>, size_t> *outputMap) {
    status_t ret = OK;
    Mutex::Autolock l(mLock);

    if ((outputMap == nullptr) || (outputInfo.size() != outputSurfaces.size()) ||
            (outputSurfaces.size() > kMaxOutputs)) {
        return BAD_VALUE;
    }

    uint64_t usage;
    getEndpointUsage(&usage);
    KeyedVector<sp<Surface>, size_t> removedSurfaces;
    //Check whether the new surfaces are compatible.
    for (const auto &infoIt : outputInfo) {
        bool imgReaderUsage = (infoIt.consumerUsage & GRALLOC_USAGE_SW_READ_OFTEN) ? true : false;
        bool sizeMismatch = ((static_cast<uint32_t>(infoIt.width) != getWidth()) ||
                                (static_cast<uint32_t> (infoIt.height) != getHeight())) ?
                                true : false;
        if ((imgReaderUsage && sizeMismatch) ||
                (infoIt.format != getOriginalFormat() && infoIt.format != getFormat()) ||
                (infoIt.dataSpace != getDataSpace() &&
                 infoIt.dataSpace != getOriginalDataSpace())) {
            ALOGE("%s: Shared surface parameters format: 0x%x dataSpace: 0x%x "
                    " don't match source stream format: 0x%x  dataSpace: 0x%x", __FUNCTION__,
                    infoIt.format, infoIt.dataSpace, getFormat(), getDataSpace());
            return BAD_VALUE;
        }
    }

    //First remove all absent outputs
    for (const auto &it : removedSurfaceIds) {
        if (mStreamSplitter != nullptr) {
            ret = mStreamSplitter->removeOutput(it);
            if (ret != OK) {
                ALOGE("%s: failed with error code %d", __FUNCTION__, ret);
                status_t res = revertPartialUpdateLocked(removedSurfaces, *outputMap);
                if (res != OK) {
                    return res;
                }
                return ret;

            }
        }
        mSurfaces[it] = nullptr;
        removedSurfaces.add(mSurfaces[it], it);
    }

    //Next add the new outputs
    for (const auto &it : outputSurfaces) {
        ssize_t surfaceId = getNextSurfaceIdLocked();
        if (surfaceId < 0) {
            ALOGE("%s: No more available output slots!", __FUNCTION__);
            status_t res = revertPartialUpdateLocked(removedSurfaces, *outputMap);
            if (res != OK) {
                return res;
            }
            return NO_MEMORY;
        }
        if (mStreamSplitter != nullptr) {
            ret = mStreamSplitter->addOutput(surfaceId, it);
            if (ret != OK) {
                ALOGE("%s: failed with error code %d", __FUNCTION__, ret);
                status_t res = revertPartialUpdateLocked(removedSurfaces, *outputMap);
                if (res != OK) {
                    return res;
                }
                return ret;
            }
        }
        mSurfaces[surfaceId] = it;
        outputMap->add(it, surfaceId);
    }

    return ret;
}

} // namespace camera3

} // namespace android
