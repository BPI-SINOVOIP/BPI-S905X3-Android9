/*
 * Copyright 2018 The Android Open Source Project
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

#include <dvr/dvr_api.h>
#include <gui/BufferHubProducer.h>
#include <inttypes.h>
#include <log/log.h>
#include <system/window.h>

namespace android {

using namespace dvr;

/* static */
sp<BufferHubProducer> BufferHubProducer::Create(const std::shared_ptr<ProducerQueue>& queue) {
    sp<BufferHubProducer> producer = new BufferHubProducer;
    producer->queue_ = queue;
    return producer;
}

/* static */
sp<BufferHubProducer> BufferHubProducer::Create(ProducerQueueParcelable parcelable) {
    if (!parcelable.IsValid()) {
        ALOGE("BufferHubProducer::Create: Invalid producer parcelable.");
        return nullptr;
    }

    sp<BufferHubProducer> producer = new BufferHubProducer;
    producer->queue_ = ProducerQueue::Import(parcelable.TakeChannelHandle());
    return producer;
}

status_t BufferHubProducer::requestBuffer(int slot, sp<GraphicBuffer>* buf) {
    ALOGV("requestBuffer: slot=%d", slot);

    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_api_ == kNoConnectedApi) {
        ALOGE("requestBuffer: BufferHubProducer has no connected producer");
        return NO_INIT;
    }

    if (slot < 0 || slot >= max_buffer_count_) {
        ALOGE("requestBuffer: slot index %d out of range [0, %d)", slot, max_buffer_count_);
        return BAD_VALUE;
    } else if (!buffers_[slot].mBufferState.isDequeued()) {
        ALOGE("requestBuffer: slot %d is not owned by the producer (state = %s)", slot,
              buffers_[slot].mBufferState.string());
        return BAD_VALUE;
    } else if (buffers_[slot].mGraphicBuffer != nullptr) {
        ALOGE("requestBuffer: slot %d is not empty.", slot);
        return BAD_VALUE;
    } else if (buffers_[slot].mBufferProducer == nullptr) {
        ALOGE("requestBuffer: slot %d is not dequeued.", slot);
        return BAD_VALUE;
    }

    const auto& buffer_producer = buffers_[slot].mBufferProducer;
    sp<GraphicBuffer> graphic_buffer = buffer_producer->buffer()->buffer();

    buffers_[slot].mGraphicBuffer = graphic_buffer;
    buffers_[slot].mRequestBufferCalled = true;

    *buf = graphic_buffer;
    return NO_ERROR;
}

status_t BufferHubProducer::setMaxDequeuedBufferCount(int max_dequeued_buffers) {
    ALOGV("setMaxDequeuedBufferCount: max_dequeued_buffers=%d", max_dequeued_buffers);

    std::unique_lock<std::mutex> lock(mutex_);

    if (max_dequeued_buffers <= 0 ||
        max_dequeued_buffers >
                int(BufferHubQueue::kMaxQueueCapacity - kDefaultUndequeuedBuffers)) {
        ALOGE("setMaxDequeuedBufferCount: %d out of range (0, %zu]", max_dequeued_buffers,
              BufferHubQueue::kMaxQueueCapacity);
        return BAD_VALUE;
    }

    // The new dequeued_buffers count should not be violated by the number
    // of currently dequeued buffers.
    int dequeued_count = 0;
    for (const auto& buf : buffers_) {
        if (buf.mBufferState.isDequeued()) {
            dequeued_count++;
        }
    }
    if (dequeued_count > max_dequeued_buffers) {
        ALOGE("setMaxDequeuedBufferCount: the requested dequeued_buffers"
              "count (%d) exceeds the current dequeued buffer count (%d)",
              max_dequeued_buffers, dequeued_count);
        return BAD_VALUE;
    }

    max_dequeued_buffer_count_ = max_dequeued_buffers;
    return NO_ERROR;
}

status_t BufferHubProducer::setAsyncMode(bool async) {
    if (async) {
        // TODO(b/36724099) BufferHubQueue's consumer end always acquires the buffer
        // automatically and behaves differently from IGraphicBufferConsumer. Thus,
        // android::BufferQueue's async mode (a.k.a. allocating an additional buffer
        // to prevent dequeueBuffer from being blocking) technically does not apply
        // here.
        //
        // In Daydream, non-blocking producer side dequeue is guaranteed by careful
        // buffer consumer implementations. In another word, BufferHubQueue based
        // dequeueBuffer should never block whether setAsyncMode(true) is set or
        // not.
        //
        // See: IGraphicBufferProducer::setAsyncMode and
        // BufferQueueProducer::setAsyncMode for more about original implementation.
        ALOGW("BufferHubProducer::setAsyncMode: BufferHubQueue should always be "
              "asynchronous. This call makes no effact.");
        return NO_ERROR;
    }
    return NO_ERROR;
}

status_t BufferHubProducer::dequeueBuffer(int* out_slot, sp<Fence>* out_fence, uint32_t width,
                                          uint32_t height, PixelFormat format, uint64_t usage,
                                          uint64_t* /*outBufferAge*/,
                                          FrameEventHistoryDelta* /* out_timestamps */) {
    ALOGV("dequeueBuffer: w=%u, h=%u, format=%d, usage=%" PRIu64, width, height, format, usage);

    status_t ret;
    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_api_ == kNoConnectedApi) {
        ALOGE("dequeueBuffer: BufferQueue has no connected producer");
        return NO_INIT;
    }

    const uint32_t kLayerCount = 1;
    if (int32_t(queue_->capacity()) < max_dequeued_buffer_count_ + kDefaultUndequeuedBuffers) {
        // Lazy allocation. When the capacity of |queue_| has not reached
        // |max_dequeued_buffer_count_|, allocate new buffer.
        // TODO(jwcai) To save memory, the really reasonable thing to do is to go
        // over existing slots and find first existing one to dequeue.
        ret = AllocateBuffer(width, height, kLayerCount, format, usage);
        if (ret < 0) return ret;
    }

    size_t slot = 0;
    std::shared_ptr<BufferProducer> buffer_producer;

    for (size_t retry = 0; retry < BufferHubQueue::kMaxQueueCapacity; retry++) {
        LocalHandle fence;
        auto buffer_status = queue_->Dequeue(dequeue_timeout_ms_, &slot, &fence);
        if (!buffer_status) return NO_MEMORY;

        buffer_producer = buffer_status.take();
        if (!buffer_producer) return NO_MEMORY;

        if (width == buffer_producer->width() && height == buffer_producer->height() &&
            uint32_t(format) == buffer_producer->format()) {
            // The producer queue returns a buffer producer matches the request.
            break;
        }

        // Needs reallocation.
        // TODO(jwcai) Consider use VLOG instead if we find this log is not useful.
        ALOGI("dequeueBuffer: requested buffer (w=%u, h=%u, format=%u) is different "
              "from the buffer returned at slot: %zu (w=%u, h=%u, format=%u). Need "
              "re-allocattion.",
              width, height, format, slot, buffer_producer->width(), buffer_producer->height(),
              buffer_producer->format());
        // Mark the slot as reallocating, so that later we can set
        // BUFFER_NEEDS_REALLOCATION when the buffer actually get dequeued.
        buffers_[slot].mIsReallocating = true;

        // Remove the old buffer once the allocation before allocating its
        // replacement.
        RemoveBuffer(slot);

        // Allocate a new producer buffer with new buffer configs. Note that if
        // there are already multiple buffers in the queue, the next one returned
        // from |queue_->Dequeue| may not be the new buffer we just reallocated.
        // Retry up to BufferHubQueue::kMaxQueueCapacity times.
        ret = AllocateBuffer(width, height, kLayerCount, format, usage);
        if (ret < 0) return ret;
    }

    // With the BufferHub backed solution. Buffer slot returned from
    // |queue_->Dequeue| is guaranteed to avaiable for producer's use.
    // It's either in free state (if the buffer has never been used before) or
    // in queued state (if the buffer has been dequeued and queued back to
    // BufferHubQueue).
    LOG_ALWAYS_FATAL_IF((!buffers_[slot].mBufferState.isFree() &&
                         !buffers_[slot].mBufferState.isQueued()),
                        "dequeueBuffer: slot %zu is not free or queued, actual state: %s.", slot,
                        buffers_[slot].mBufferState.string());

    buffers_[slot].mBufferState.freeQueued();
    buffers_[slot].mBufferState.dequeue();
    ALOGV("dequeueBuffer: slot=%zu", slot);

    // TODO(jwcai) Handle fence properly. |BufferHub| has full fence support, we
    // just need to exopose that through |BufferHubQueue| once we need fence.
    *out_fence = Fence::NO_FENCE;
    *out_slot = int(slot);
    ret = NO_ERROR;

    if (buffers_[slot].mIsReallocating) {
        ret |= BUFFER_NEEDS_REALLOCATION;
        buffers_[slot].mIsReallocating = false;
    }

    return ret;
}

status_t BufferHubProducer::detachBuffer(int /* slot */) {
    ALOGE("BufferHubProducer::detachBuffer not implemented.");
    return INVALID_OPERATION;
}

status_t BufferHubProducer::detachNextBuffer(sp<GraphicBuffer>* /* out_buffer */,
                                             sp<Fence>* /* out_fence */) {
    ALOGE("BufferHubProducer::detachNextBuffer not implemented.");
    return INVALID_OPERATION;
}

status_t BufferHubProducer::attachBuffer(int* /* out_slot */,
                                         const sp<GraphicBuffer>& /* buffer */) {
    // With this BufferHub backed implementation, we assume (for now) all buffers
    // are allocated and owned by the BufferHub. Thus the attempt of transfering
    // ownership of a buffer to the buffer queue is intentionally unsupported.
    LOG_ALWAYS_FATAL("BufferHubProducer::attachBuffer not supported.");
    return INVALID_OPERATION;
}

status_t BufferHubProducer::queueBuffer(int slot, const QueueBufferInput& input,
                                        QueueBufferOutput* output) {
    ALOGV("queueBuffer: slot %d", slot);

    if (output == nullptr) {
        return BAD_VALUE;
    }

    int64_t timestamp;
    bool is_auto_timestamp;
    android_dataspace dataspace;
    Rect crop(Rect::EMPTY_RECT);
    int scaling_mode;
    uint32_t transform;
    sp<Fence> fence;

    input.deflate(&timestamp, &is_auto_timestamp, &dataspace, &crop, &scaling_mode, &transform,
                  &fence);

    // Check input scaling mode is valid.
    switch (scaling_mode) {
        case NATIVE_WINDOW_SCALING_MODE_FREEZE:
        case NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW:
        case NATIVE_WINDOW_SCALING_MODE_SCALE_CROP:
        case NATIVE_WINDOW_SCALING_MODE_NO_SCALE_CROP:
            break;
        default:
            ALOGE("queueBuffer: unknown scaling mode %d", scaling_mode);
            return BAD_VALUE;
    }

    // Check input fence is valid.
    if (fence == nullptr) {
        ALOGE("queueBuffer: fence is NULL");
        return BAD_VALUE;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_api_ == kNoConnectedApi) {
        ALOGE("queueBuffer: BufferQueue has no connected producer");
        return NO_INIT;
    }

    if (slot < 0 || slot >= max_buffer_count_) {
        ALOGE("queueBuffer: slot index %d out of range [0, %d)", slot, max_buffer_count_);
        return BAD_VALUE;
    } else if (!buffers_[slot].mBufferState.isDequeued()) {
        ALOGE("queueBuffer: slot %d is not owned by the producer (state = %s)", slot,
              buffers_[slot].mBufferState.string());
        return BAD_VALUE;
    } else if ((!buffers_[slot].mRequestBufferCalled || buffers_[slot].mGraphicBuffer == nullptr)) {
        ALOGE("queueBuffer: slot %d is not requested (mRequestBufferCalled=%d, "
              "mGraphicBuffer=%p)",
              slot, buffers_[slot].mRequestBufferCalled, buffers_[slot].mGraphicBuffer.get());
        return BAD_VALUE;
    }

    // Post the buffer producer with timestamp in the metadata.
    const auto& buffer_producer = buffers_[slot].mBufferProducer;

    // Check input crop is not out of boundary of current buffer.
    Rect buffer_rect(buffer_producer->width(), buffer_producer->height());
    Rect cropped_rect(Rect::EMPTY_RECT);
    crop.intersect(buffer_rect, &cropped_rect);
    if (cropped_rect != crop) {
        ALOGE("queueBuffer: slot %d has out-of-boundary crop.", slot);
        return BAD_VALUE;
    }

    LocalHandle fence_fd(fence->isValid() ? fence->dup() : -1);

    DvrNativeBufferMetadata meta_data;
    meta_data.timestamp = timestamp;
    meta_data.is_auto_timestamp = int32_t(is_auto_timestamp);
    meta_data.dataspace = int32_t(dataspace);
    meta_data.crop_left = crop.left;
    meta_data.crop_top = crop.top;
    meta_data.crop_right = crop.right;
    meta_data.crop_bottom = crop.bottom;
    meta_data.scaling_mode = int32_t(scaling_mode);
    meta_data.transform = int32_t(transform);

    buffer_producer->PostAsync(&meta_data, fence_fd);
    buffers_[slot].mBufferState.queue();

    output->width = buffer_producer->width();
    output->height = buffer_producer->height();
    output->transformHint = 0; // default value, we don't use it yet.

    // |numPendingBuffers| counts of the number of buffers that has been enqueued
    // by the producer but not yet acquired by the consumer. Due to the nature
    // of BufferHubQueue design, this is hard to trace from the producer's client
    // side, but it's safe to assume it's zero.
    output->numPendingBuffers = 0;

    // Note that we are not setting nextFrameNumber here as it seems to be only
    // used by surface flinger. See more at b/22802885, ag/791760.
    output->nextFrameNumber = 0;

    return NO_ERROR;
}

status_t BufferHubProducer::cancelBuffer(int slot, const sp<Fence>& fence) {
    ALOGV(__FUNCTION__);

    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_api_ == kNoConnectedApi) {
        ALOGE("cancelBuffer: BufferQueue has no connected producer");
        return NO_INIT;
    }

    if (slot < 0 || slot >= max_buffer_count_) {
        ALOGE("cancelBuffer: slot index %d out of range [0, %d)", slot, max_buffer_count_);
        return BAD_VALUE;
    } else if (!buffers_[slot].mBufferState.isDequeued()) {
        ALOGE("cancelBuffer: slot %d is not owned by the producer (state = %s)", slot,
              buffers_[slot].mBufferState.string());
        return BAD_VALUE;
    } else if (fence == nullptr) {
        ALOGE("cancelBuffer: fence is NULL");
        return BAD_VALUE;
    }

    auto buffer_producer = buffers_[slot].mBufferProducer;
    queue_->Enqueue(buffer_producer, size_t(slot), 0ULL);
    buffers_[slot].mBufferState.cancel();
    buffers_[slot].mFence = fence;
    ALOGV("cancelBuffer: slot %d", slot);

    return NO_ERROR;
}

status_t BufferHubProducer::query(int what, int* out_value) {
    ALOGV(__FUNCTION__);

    std::unique_lock<std::mutex> lock(mutex_);

    if (out_value == nullptr) {
        ALOGE("query: out_value was NULL");
        return BAD_VALUE;
    }

    int value = 0;
    switch (what) {
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            // TODO(b/36187402) This should be the maximum number of buffers that this
            // producer queue's consumer can acquire. Set to be at least one. Need to
            // find a way to set from the consumer side.
            value = kDefaultUndequeuedBuffers;
            break;
        case NATIVE_WINDOW_BUFFER_AGE:
            value = 0;
            break;
        case NATIVE_WINDOW_WIDTH:
            value = int32_t(queue_->default_width());
            break;
        case NATIVE_WINDOW_HEIGHT:
            value = int32_t(queue_->default_height());
            break;
        case NATIVE_WINDOW_FORMAT:
            value = int32_t(queue_->default_format());
            break;
        case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
            // BufferHubQueue is always operating in async mode, thus semantically
            // consumer can never be running behind. See BufferQueueCore.cpp core
            // for more information about the original meaning of this flag.
            value = 0;
            break;
        case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
            // TODO(jwcai) This is currently not implement as we don't need
            // IGraphicBufferConsumer parity.
            value = 0;
            break;
        case NATIVE_WINDOW_DEFAULT_DATASPACE:
            // TODO(jwcai) Return the default value android::BufferQueue is using as
            // there is no way dvr::ConsumerQueue can set it.
            value = 0; // HAL_DATASPACE_UNKNOWN
            break;
        case NATIVE_WINDOW_STICKY_TRANSFORM:
            // TODO(jwcai) Return the default value android::BufferQueue is using as
            // there is no way dvr::ConsumerQueue can set it.
            value = 0;
            break;
        case NATIVE_WINDOW_CONSUMER_IS_PROTECTED:
            // In Daydream's implementation, the consumer end (i.e. VR Compostior)
            // knows how to handle protected buffers.
            value = 1;
            break;
        default:
            return BAD_VALUE;
    }

    ALOGV("query: key=%d, v=%d", what, value);
    *out_value = value;
    return NO_ERROR;
}

status_t BufferHubProducer::connect(const sp<IProducerListener>& /* listener */, int api,
                                    bool /* producer_controlled_by_app */,
                                    QueueBufferOutput* output) {
    // Consumer interaction are actually handled by buffer hub, and we need
    // to maintain consumer operations here. We only need to perform basic input
    // parameter checks here.
    ALOGV(__FUNCTION__);

    if (output == nullptr) {
        return BAD_VALUE;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_api_ != kNoConnectedApi) {
        return BAD_VALUE;
    }

    if (!queue_->is_connected()) {
        ALOGE("BufferHubProducer::connect: This BufferHubProducer is not "
              "connected to bufferhud. Has it been taken out as a parcelable?");
        return BAD_VALUE;
    }

    switch (api) {
        case NATIVE_WINDOW_API_EGL:
        case NATIVE_WINDOW_API_CPU:
        case NATIVE_WINDOW_API_MEDIA:
        case NATIVE_WINDOW_API_CAMERA:
            connected_api_ = api;

            output->width = queue_->default_width();
            output->height = queue_->default_height();

            // default values, we don't use them yet.
            output->transformHint = 0;
            output->numPendingBuffers = 0;
            output->nextFrameNumber = 0;
            output->bufferReplaced = false;

            break;
        default:
            ALOGE("BufferHubProducer::connect: unknow API %d", api);
            return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t BufferHubProducer::disconnect(int api, DisconnectMode /*mode*/) {
    // Consumer interaction are actually handled by buffer hub, and we need
    // to maintain consumer operations here.  We only need to perform basic input
    // parameter checks here.
    ALOGV(__FUNCTION__);

    std::unique_lock<std::mutex> lock(mutex_);

    if (kNoConnectedApi == connected_api_) {
        return NO_INIT;
    } else if (api != connected_api_) {
        return BAD_VALUE;
    }

    FreeAllBuffers();
    connected_api_ = kNoConnectedApi;
    return NO_ERROR;
}

status_t BufferHubProducer::setSidebandStream(const sp<NativeHandle>& stream) {
    if (stream != nullptr) {
        // TODO(jwcai) Investigate how is is used, maybe use BufferHubBuffer's
        // metadata.
        ALOGE("SidebandStream is not currently supported.");
        return INVALID_OPERATION;
    }
    return NO_ERROR;
}

void BufferHubProducer::allocateBuffers(uint32_t /* width */, uint32_t /* height */,
                                        PixelFormat /* format */, uint64_t /* usage */) {
    // TODO(jwcai) |allocateBuffers| aims to preallocate up to the maximum number
    // of buffers permitted by the current BufferQueue configuration (aka
    // |max_buffer_count_|).
    ALOGE("BufferHubProducer::allocateBuffers not implemented.");
}

status_t BufferHubProducer::allowAllocation(bool /* allow */) {
    ALOGE("BufferHubProducer::allowAllocation not implemented.");
    return INVALID_OPERATION;
}

status_t BufferHubProducer::setGenerationNumber(uint32_t generation_number) {
    ALOGV(__FUNCTION__);

    std::unique_lock<std::mutex> lock(mutex_);
    generation_number_ = generation_number;
    return NO_ERROR;
}

String8 BufferHubProducer::getConsumerName() const {
    // BufferHub based implementation could have one to many producer/consumer
    // relationship, thus |getConsumerName| from the producer side does not
    // make any sense.
    ALOGE("BufferHubProducer::getConsumerName not supported.");
    return String8("BufferHubQueue::DummyConsumer");
}

status_t BufferHubProducer::setSharedBufferMode(bool shared_buffer_mode) {
    if (shared_buffer_mode) {
        ALOGE("BufferHubProducer::setSharedBufferMode(true) is not supported.");
        // TODO(b/36373181) Front buffer mode for buffer hub queue as ANativeWindow.
        return INVALID_OPERATION;
    }
    // Setting to default should just work as a no-op.
    return NO_ERROR;
}

status_t BufferHubProducer::setAutoRefresh(bool auto_refresh) {
    if (auto_refresh) {
        ALOGE("BufferHubProducer::setAutoRefresh(true) is not supported.");
        return INVALID_OPERATION;
    }
    // Setting to default should just work as a no-op.
    return NO_ERROR;
}

status_t BufferHubProducer::setDequeueTimeout(nsecs_t timeout) {
    ALOGV(__FUNCTION__);

    std::unique_lock<std::mutex> lock(mutex_);
    dequeue_timeout_ms_ = static_cast<int>(timeout / (1000 * 1000));
    return NO_ERROR;
}

status_t BufferHubProducer::getLastQueuedBuffer(sp<GraphicBuffer>* /* out_buffer */,
                                                sp<Fence>* /* out_fence */,
                                                float /*out_transform_matrix*/[16]) {
    ALOGE("BufferHubProducer::getLastQueuedBuffer not implemented.");
    return INVALID_OPERATION;
}

void BufferHubProducer::getFrameTimestamps(FrameEventHistoryDelta* /*outDelta*/) {
    ALOGE("BufferHubProducer::getFrameTimestamps not implemented.");
}

status_t BufferHubProducer::getUniqueId(uint64_t* out_id) const {
    ALOGV(__FUNCTION__);

    *out_id = unique_id_;
    return NO_ERROR;
}

status_t BufferHubProducer::getConsumerUsage(uint64_t* out_usage) const {
    ALOGV(__FUNCTION__);

    // same value as returned by querying NATIVE_WINDOW_CONSUMER_USAGE_BITS
    *out_usage = 0;
    return NO_ERROR;
}

status_t BufferHubProducer::TakeAsParcelable(ProducerQueueParcelable* out_parcelable) {
    if (!out_parcelable || out_parcelable->IsValid()) return BAD_VALUE;

    if (connected_api_ != kNoConnectedApi) {
        ALOGE("BufferHubProducer::TakeAsParcelable: BufferHubProducer has "
              "connected client. Must disconnect first.");
        return BAD_VALUE;
    }

    if (!queue_->is_connected()) {
        ALOGE("BufferHubProducer::TakeAsParcelable: This BufferHubProducer "
              "is not connected to bufferhud. Has it been taken out as a "
              "parcelable?");
        return BAD_VALUE;
    }

    auto status = queue_->TakeAsParcelable();
    if (!status) {
        ALOGE("BufferHubProducer::TakeAsParcelable: Failed to take out "
              "ProducuerQueueParcelable from the producer queue, error: %s.",
              status.GetErrorMessage().c_str());
        return BAD_VALUE;
    }

    *out_parcelable = status.take();
    return NO_ERROR;
}

status_t BufferHubProducer::AllocateBuffer(uint32_t width, uint32_t height, uint32_t layer_count,
                                           PixelFormat format, uint64_t usage) {
    auto status = queue_->AllocateBuffer(width, height, layer_count, uint32_t(format), usage);
    if (!status) {
        ALOGE("BufferHubProducer::AllocateBuffer: Failed to allocate buffer: %s",
              status.GetErrorMessage().c_str());
        return NO_MEMORY;
    }

    size_t slot = status.get();
    auto buffer_producer = queue_->GetBuffer(slot);

    LOG_ALWAYS_FATAL_IF(buffer_producer == nullptr, "Failed to get buffer producer at slot: %zu",
                        slot);

    buffers_[slot].mBufferProducer = buffer_producer;

    return NO_ERROR;
}

status_t BufferHubProducer::RemoveBuffer(size_t slot) {
    auto status = queue_->RemoveBuffer(slot);
    if (!status) {
        ALOGE("BufferHubProducer::RemoveBuffer: Failed to remove buffer: %s",
              status.GetErrorMessage().c_str());
        return INVALID_OPERATION;
    }

    // Reset in memory objects related the the buffer.
    buffers_[slot].mBufferProducer = nullptr;
    buffers_[slot].mGraphicBuffer = nullptr;
    buffers_[slot].mBufferState.detachProducer();
    return NO_ERROR;
}

status_t BufferHubProducer::FreeAllBuffers() {
    for (size_t slot = 0; slot < BufferHubQueue::kMaxQueueCapacity; slot++) {
        // Reset in memory objects related the the buffer.
        buffers_[slot].mGraphicBuffer = nullptr;
        buffers_[slot].mBufferState.reset();
        buffers_[slot].mRequestBufferCalled = false;
        buffers_[slot].mBufferProducer = nullptr;
        buffers_[slot].mFence = Fence::NO_FENCE;
    }

    auto status = queue_->FreeAllBuffers();
    if (!status) {
        ALOGE("BufferHubProducer::FreeAllBuffers: Failed to free all buffers on "
              "the queue: %s",
              status.GetErrorMessage().c_str());
    }

    if (queue_->capacity() != 0 || queue_->count() != 0) {
        LOG_ALWAYS_FATAL("BufferHubProducer::FreeAllBuffers: Not all buffers are freed.");
    }

    return NO_ERROR;
}

status_t BufferHubProducer::exportToParcel(Parcel* parcel) {
    status_t res = TakeAsParcelable(&pending_producer_parcelable_);
    if (res != NO_ERROR) return res;

    if (!pending_producer_parcelable_.IsValid()) {
        ALOGE("BufferHubProducer::exportToParcel: Invalid parcelable object.");
        return BAD_VALUE;
    }

    res = parcel->writeUint32(USE_BUFFER_HUB);
    if (res != NO_ERROR) {
        ALOGE("BufferHubProducer::exportToParcel: Cannot write magic, res=%d.", res);
        return res;
    }

    return pending_producer_parcelable_.writeToParcel(parcel);
}

IBinder* BufferHubProducer::onAsBinder() {
    ALOGE("BufferHubProducer::onAsBinder: BufferHubProducer should never be used as an Binder "
          "object.");
    return nullptr;
}

} // namespace android
