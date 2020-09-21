/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "Gralloc2"

#include <hidl/ServiceManagement.h>
#include <hwbinder/IPCThreadState.h>
#include <ui/Gralloc2.h>

#include <inttypes.h>
#include <log/log.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"
#include <sync/sync.h>
#pragma clang diagnostic pop

namespace android {

namespace Gralloc2 {

namespace {

static constexpr Error kTransactionError = Error::NO_RESOURCES;

uint64_t getValid10UsageBits() {
    static const uint64_t valid10UsageBits = []() -> uint64_t {
        using hardware::graphics::common::V1_0::BufferUsage;
        uint64_t bits = 0;
        for (const auto bit : hardware::hidl_enum_iterator<BufferUsage>()) {
            bits = bits | bit;
        }
        // TODO(b/72323293, b/72703005): Remove these additional bits
        bits = bits | (1 << 10) | (1 << 13);

        return bits;
    }();
    return valid10UsageBits;
}

uint64_t getValid11UsageBits() {
    static const uint64_t valid11UsageBits = []() -> uint64_t {
        using hardware::graphics::common::V1_1::BufferUsage;
        uint64_t bits = 0;
        for (const auto bit : hardware::hidl_enum_iterator<BufferUsage>()) {
            bits = bits | bit;
        }
        return bits;
    }();
    return valid11UsageBits;
}

}  // anonymous namespace

void Mapper::preload() {
    android::hardware::preloadPassthroughService<hardware::graphics::mapper::V2_0::IMapper>();
}

Mapper::Mapper()
{
    mMapper = hardware::graphics::mapper::V2_0::IMapper::getService();
    if (mMapper == nullptr) {
        LOG_ALWAYS_FATAL("gralloc-mapper is missing");
    }
    if (mMapper->isRemote()) {
        LOG_ALWAYS_FATAL("gralloc-mapper must be in passthrough mode");
    }

    // IMapper 2.1 is optional
    mMapperV2_1 = IMapper::castFrom(mMapper);
}

Gralloc2::Error Mapper::validateBufferDescriptorInfo(
        const IMapper::BufferDescriptorInfo& descriptorInfo) const {
    uint64_t validUsageBits = getValid10UsageBits();
    if (mMapperV2_1 != nullptr) {
        validUsageBits = validUsageBits | getValid11UsageBits();
    }

    if (descriptorInfo.usage & ~validUsageBits) {
        ALOGE("buffer descriptor contains invalid usage bits 0x%" PRIx64,
              descriptorInfo.usage & ~validUsageBits);
        return Error::BAD_VALUE;
    }
    return Error::NONE;
}

Error Mapper::createDescriptor(
        const IMapper::BufferDescriptorInfo& descriptorInfo,
        BufferDescriptor* outDescriptor) const
{
    Error error = validateBufferDescriptorInfo(descriptorInfo);
    if (error != Error::NONE) {
        return error;
    }

    auto hidl_cb = [&](const auto& tmpError, const auto& tmpDescriptor)
                   {
                       error = tmpError;
                       if (error != Error::NONE) {
                           return;
                       }

                       *outDescriptor = tmpDescriptor;
                   };

    hardware::Return<void> ret;
    if (mMapperV2_1 != nullptr) {
        ret = mMapperV2_1->createDescriptor_2_1(descriptorInfo, hidl_cb);
    } else {
        const hardware::graphics::mapper::V2_0::IMapper::BufferDescriptorInfo info = {
            descriptorInfo.width,
            descriptorInfo.height,
            descriptorInfo.layerCount,
            static_cast<hardware::graphics::common::V1_0::PixelFormat>(descriptorInfo.format),
            descriptorInfo.usage,
        };
        ret = mMapper->createDescriptor(info, hidl_cb);
    }

    return (ret.isOk()) ? error : kTransactionError;
}

Error Mapper::importBuffer(const hardware::hidl_handle& rawHandle,
        buffer_handle_t* outBufferHandle) const
{
    Error error;
    auto ret = mMapper->importBuffer(rawHandle,
            [&](const auto& tmpError, const auto& tmpBuffer)
            {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outBufferHandle = static_cast<buffer_handle_t>(tmpBuffer);
            });

    return (ret.isOk()) ? error : kTransactionError;
}

void Mapper::freeBuffer(buffer_handle_t bufferHandle) const
{
    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    auto ret = mMapper->freeBuffer(buffer);

    auto error = (ret.isOk()) ? static_cast<Error>(ret) : kTransactionError;
    ALOGE_IF(error != Error::NONE, "freeBuffer(%p) failed with %d",
            buffer, error);
}

Error Mapper::validateBufferSize(buffer_handle_t bufferHandle,
        const IMapper::BufferDescriptorInfo& descriptorInfo,
        uint32_t stride) const
{
    if (mMapperV2_1 == nullptr) {
        return Error::NONE;
    }

    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    auto ret = mMapperV2_1->validateBufferSize(buffer, descriptorInfo, stride);

    return (ret.isOk()) ? static_cast<Error>(ret) : kTransactionError;
}

void Mapper::getTransportSize(buffer_handle_t bufferHandle,
        uint32_t* outNumFds, uint32_t* outNumInts) const
{
    *outNumFds = uint32_t(bufferHandle->numFds);
    *outNumInts = uint32_t(bufferHandle->numInts);

    if (mMapperV2_1 == nullptr) {
        return;
    }

    Error error;
    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    auto ret = mMapperV2_1->getTransportSize(buffer,
            [&](const auto& tmpError, const auto& tmpNumFds, const auto& tmpNumInts) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outNumFds = tmpNumFds;
                *outNumInts = tmpNumInts;
            });

    if (!ret.isOk()) {
        error = kTransactionError;
    }
    ALOGE_IF(error != Error::NONE, "getTransportSize(%p) failed with %d",
            buffer, error);
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, void** outData) const
{
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    // put acquireFence in a hidl_handle
    hardware::hidl_handle acquireFenceHandle;
    NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 1, 0);
    if (acquireFence >= 0) {
        auto h = native_handle_init(acquireFenceStorage, 1, 0);
        h->data[0] = acquireFence;
        acquireFenceHandle = h;
    }

    Error error;
    auto ret = mMapper->lock(buffer, usage, accessRegion, acquireFenceHandle,
            [&](const auto& tmpError, const auto& tmpData)
            {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outData = tmpData;
            });

    // we own acquireFence even on errors
    if (acquireFence >= 0) {
        close(acquireFence);
    }

    return (ret.isOk()) ? error : kTransactionError;
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, YCbCrLayout* outLayout) const
{
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    // put acquireFence in a hidl_handle
    hardware::hidl_handle acquireFenceHandle;
    NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 1, 0);
    if (acquireFence >= 0) {
        auto h = native_handle_init(acquireFenceStorage, 1, 0);
        h->data[0] = acquireFence;
        acquireFenceHandle = h;
    }

    Error error;
    auto ret = mMapper->lockYCbCr(buffer, usage, accessRegion,
            acquireFenceHandle,
            [&](const auto& tmpError, const auto& tmpLayout)
            {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outLayout = tmpLayout;
            });

    // we own acquireFence even on errors
    if (acquireFence >= 0) {
        close(acquireFence);
    }

    return (ret.isOk()) ? error : kTransactionError;
}

int Mapper::unlock(buffer_handle_t bufferHandle) const
{
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    int releaseFence = -1;
    Error error;
    auto ret = mMapper->unlock(buffer,
            [&](const auto& tmpError, const auto& tmpReleaseFence)
            {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                auto fenceHandle = tmpReleaseFence.getNativeHandle();
                if (fenceHandle && fenceHandle->numFds == 1) {
                    int fd = dup(fenceHandle->data[0]);
                    if (fd >= 0) {
                        releaseFence = fd;
                    } else {
                        ALOGD("failed to dup unlock release fence");
                        sync_wait(fenceHandle->data[0], -1);
                    }
                }
            });

    if (!ret.isOk()) {
        error = kTransactionError;
    }

    if (error != Error::NONE) {
        ALOGE("unlock(%p) failed with %d", buffer, error);
    }

    return releaseFence;
}

Allocator::Allocator(const Mapper& mapper)
    : mMapper(mapper)
{
    mAllocator = IAllocator::getService();
    if (mAllocator == nullptr) {
        LOG_ALWAYS_FATAL("gralloc-alloc is missing");
    }
}

std::string Allocator::dumpDebugInfo() const
{
    std::string debugInfo;

    mAllocator->dumpDebugInfo([&](const auto& tmpDebugInfo) {
        debugInfo = tmpDebugInfo.c_str();
    });

    return debugInfo;
}

Error Allocator::allocate(BufferDescriptor descriptor, uint32_t count,
        uint32_t* outStride, buffer_handle_t* outBufferHandles) const
{
    Error error;
    auto ret = mAllocator->allocate(descriptor, count,
            [&](const auto& tmpError, const auto& tmpStride,
                const auto& tmpBuffers) {
                error = tmpError;
                if (tmpError != Error::NONE) {
                    return;
                }

                // import buffers
                for (uint32_t i = 0; i < count; i++) {
                    error = mMapper.importBuffer(tmpBuffers[i],
                            &outBufferHandles[i]);
                    if (error != Error::NONE) {
                        for (uint32_t j = 0; j < i; j++) {
                            mMapper.freeBuffer(outBufferHandles[j]);
                            outBufferHandles[j] = nullptr;
                        }
                        return;
                    }
                }

                *outStride = tmpStride;
            });

    // make sure the kernel driver sees BC_FREE_BUFFER and closes the fds now
    hardware::IPCThreadState::self()->flushCommands();

    return (ret.isOk()) ? error : kTransactionError;
}

} // namespace Gralloc2

} // namespace android
