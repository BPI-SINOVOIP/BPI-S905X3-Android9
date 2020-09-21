// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_C2_ARC_VIDEO_ACCELERATOR_FACTORY_H
#define ANDROID_C2_ARC_VIDEO_ACCELERATOR_FACTORY_H

#include <media/arcvideobridge/IArcVideoBridge.h>
#include <utils/Singleton.h>

#include <components/arc/common/video.mojom.h>
#include <components/arc/common/video_decode_accelerator.mojom.h>
#include <components/arc/common/video_encode_accelerator.mojom.h>

namespace android {
// Helper class to create message pipe to the ArcVideoAccelerator.
// This class should only be used in the Mojo thread.
class C2ArcVideoAcceleratorFactory : public Singleton<C2ArcVideoAcceleratorFactory> {
public:
    bool createVideoDecodeAccelerator(::arc::mojom::VideoDecodeAcceleratorRequest request);
    bool createVideoEncodeAccelerator(::arc::mojom::VideoEncodeAcceleratorRequest request);
    bool createVideoProtectedBufferAllocator(
            ::arc::mojom::VideoProtectedBufferAllocatorRequest request);
    int32_t hostVersion() const;

private:
    C2ArcVideoAcceleratorFactory();

    uint32_t mHostVersion;
    sp<IArcVideoBridge> mArcVideoBridge;
    ::arc::mojom::VideoAcceleratorFactoryPtr mRemoteFactory;

    friend class Singleton<C2ArcVideoAcceleratorFactory>;
};
}  // namespace android

#endif  // ANDROID_C2_ARC_VIDEO_ACCELERATOR_FACTORY_H
