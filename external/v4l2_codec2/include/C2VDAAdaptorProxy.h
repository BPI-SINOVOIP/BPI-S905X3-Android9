// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_C2_VDA_ADAPTOR_PROXY_H
#define ANDROID_C2_VDA_ADAPTOR_PROXY_H

#include <memory>

#include <VideoDecodeAcceleratorAdaptor.h>

#include <video_decode_accelerator.h>

#include <arc/Future.h>
#include <mojo/public/cpp/bindings/binding.h>

#include <components/arc/common/video.mojom.h>
#include <components/arc/common/video_decode_accelerator.mojom.h>

namespace arc {
class MojoProcessSupport;
}  // namespace arc

namespace android {
namespace arc {
class C2VDAAdaptorProxy : public VideoDecodeAcceleratorAdaptor,
                          public ::arc::mojom::VideoDecodeClient {
public:
    C2VDAAdaptorProxy();
    explicit C2VDAAdaptorProxy(::arc::MojoProcessSupport* MojomProcessSupport);
    ~C2VDAAdaptorProxy() override;

    // Establishes ipc channel for video acceleration. Returns true if channel
    // connected successfully.
    // This must be called before all other methods.
    bool establishChannel();

    // Implementation of the VideoDecodeAcceleratorAdaptor interface.
    Result initialize(media::VideoCodecProfile profile, bool secureMode,
                      VideoDecodeAcceleratorAdaptor::Client* client) override;
    void decode(int32_t bitstreamId, int handleFd, off_t offset, uint32_t size) override;
    void assignPictureBuffers(uint32_t numOutputBuffers) override;
    void importBufferForPicture(int32_t pictureBufferId, HalPixelFormat format, int handleFd,
                                const std::vector<VideoFramePlane>& planes) override;
    void reusePictureBuffer(int32_t pictureBufferId) override;
    void flush() override;
    void reset() override;
    void destroy() override;

    // ::arc::mojom::VideoDecodeClient implementations.
    void ProvidePictureBuffers(::arc::mojom::PictureBufferFormatPtr format) override;
    void PictureReady(::arc::mojom::PicturePtr picture) override;
    void NotifyEndOfBitstreamBuffer(int32_t bitstream_id) override;
    void NotifyError(::arc::mojom::VideoDecodeAccelerator::Result error) override;

    // The following functions are called as callbacks.
    void NotifyResetDone(::arc::mojom::VideoDecodeAccelerator::Result result);
    void NotifyFlushDone(::arc::mojom::VideoDecodeAccelerator::Result result);

    static media::VideoDecodeAccelerator::SupportedProfiles GetSupportedProfiles(
            uint32_t inputFormatFourcc);
    static HalPixelFormat ResolveBufferFormat(bool crcb, bool semiplanar);

private:
    void onConnectionError(const std::string& pipeName);
    void establishChannelOnMojoThread(std::shared_ptr<::arc::Future<bool>> future);
    void onVersionReady(std::shared_ptr<::arc::Future<bool>> future, uint32_t version);

    // Closes ipc channel for video acceleration.
    // This must be called before deleting this object.
    void closeChannelOnMojoThread();

    // mojo thread corresponding part of C2VDAAdaptorProxy implementations.
    void initializeOnMojoThread(const media::VideoCodecProfile profile, const bool mSecureMode,
                                const ::arc::mojom::VideoDecodeAccelerator::InitializeCallback& cb);
    void decodeOnMojoThread(int32_t bitstreamId, int ashmemFd, off_t offset, uint32_t bytesUsed);
    void assignPictureBuffersOnMojoThread(uint32_t numOutputBuffers);

    void importBufferForPictureOnMojoThread(int32_t pictureBufferId, HalPixelFormat format,
                                            int handleFd,
                                            const std::vector<VideoFramePlane>& planes);
    void reusePictureBufferOnMojoThread(int32_t pictureBufferId);
    void flushOnMojoThread();
    void resetOnMojoThread();

    VideoDecodeAcceleratorAdaptor::Client* mClient;

    // Task runner for mojom functions.
    const scoped_refptr<base::SingleThreadTaskRunner> mMojoTaskRunner;

    // |mVDAPtr| and |mBinding| should only be called on |mMojoTaskRunner| after bound.
    ::arc::mojom::VideoDecodeAcceleratorPtr mVDAPtr;
    mojo::Binding<::arc::mojom::VideoDecodeClient> mBinding;

    // Used to cancel the wait on arc::Future.
    sp<::arc::CancellationRelay> mRelay;

    DISALLOW_COPY_AND_ASSIGN(C2VDAAdaptorProxy);
};
}  // namespace arc
}  // namespace android

#endif  // ANDROID_C2_VDA_ADAPTOR_PROXY_H
