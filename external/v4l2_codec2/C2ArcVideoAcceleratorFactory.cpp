// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// #define LOG_NDEBUG 0
#define LOG_TAG "C2ArcVideoAcceleratorFactory"

#include <C2ArcVideoAcceleratorFactory.h>

#include <base/bind.h>
#include <binder/IServiceManager.h>
#include <mojo/edk/embedder/embedder.h>
#include <mojo/public/cpp/bindings/interface_request.h>
#include <mojo/public/cpp/system/handle.h>
#include <utils/Log.h>

namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(C2ArcVideoAcceleratorFactory)

bool C2ArcVideoAcceleratorFactory::createVideoDecodeAccelerator(
        ::arc::mojom::VideoDecodeAcceleratorRequest request) {
    if (!mRemoteFactory) {
        ALOGE("Factory is not ready");
        return false;
    }
    mRemoteFactory->CreateDecodeAccelerator(std::move(request));
    return true;
}

bool C2ArcVideoAcceleratorFactory::createVideoEncodeAccelerator(
        ::arc::mojom::VideoEncodeAcceleratorRequest request) {
    if (!mRemoteFactory) {
        ALOGE("Factory is not ready");
        return false;
    }
    mRemoteFactory->CreateEncodeAccelerator(std::move(request));
    return true;
}

bool C2ArcVideoAcceleratorFactory::createVideoProtectedBufferAllocator(
        ::arc::mojom::VideoProtectedBufferAllocatorRequest request) {
    if (!mRemoteFactory) {
        ALOGE("Factory is not ready");
        return false;
    }
    mRemoteFactory->CreateProtectedBufferAllocator(std::move(request));
    return true;
}

int32_t C2ArcVideoAcceleratorFactory::hostVersion() const {
    return mHostVersion;
}

C2ArcVideoAcceleratorFactory::C2ArcVideoAcceleratorFactory() : mHostVersion(0) {
    sp<IBinder> binder =
            defaultServiceManager()->getService(String16("android.os.IArcVideoBridge"));
    if (binder == nullptr) {
        ALOGE("Failed to find IArcVideoBridge service");
        return;
    }
    mArcVideoBridge = interface_cast<IArcVideoBridge>(binder);
    mHostVersion = mArcVideoBridge->hostVersion();
    if (mHostVersion < 4) {
        ALOGW("HostVersion(%d) is outdated", mHostVersion);
        return;
    }

    ALOGV("HostVersion: %d", mHostVersion);

    ::arc::MojoBootstrapResult bootstrapResult =
            mArcVideoBridge->bootstrapVideoAcceleratorFactory();
    if (!bootstrapResult.is_valid()) {
        ALOGE("bootstrapVideoAcceleratorFactory returns invalid result");
        return;
    }
    mojo::edk::ScopedPlatformHandle handle(
            mojo::edk::PlatformHandle(bootstrapResult.releaseFd().release()));
    ALOGV("SetParentPipeHandle(fd=%d)", handle.get().handle);
    mojo::edk::SetParentPipeHandle(std::move(handle));
    mojo::ScopedMessagePipeHandle server_pipe =
            mojo::edk::CreateChildMessagePipe(bootstrapResult.releaseToken());
    mRemoteFactory.Bind(mojo::InterfacePtrInfo<::arc::mojom::VideoAcceleratorFactory>(
            std::move(server_pipe), 7u));
}

}  // namespace android
