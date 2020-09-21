// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_C2_VDA_ADAPTOR_H
#define ANDROID_C2_VDA_ADAPTOR_H

#include <VideoDecodeAcceleratorAdaptor.h>

#include <video_decode_accelerator.h>

#include <base/macros.h>

namespace android {

// This class translates adaptor API to media::VideoDecodeAccelerator API to make communication
// between Codec 2.0 VDA component and VDA.
class C2VDAAdaptor : public VideoDecodeAcceleratorAdaptor,
                     public media::VideoDecodeAccelerator::Client {
public:
    C2VDAAdaptor();
    ~C2VDAAdaptor() override;

    // Implementation of the VideoDecodeAcceleratorAdaptor interface.
    Result initialize(media::VideoCodecProfile profile, bool secureMode,
                      VideoDecodeAcceleratorAdaptor::Client* client) override;
    void decode(int32_t bitstreamId, int handleFd, off_t offset, uint32_t bytesUsed) override;
    void assignPictureBuffers(uint32_t numOutputBuffers) override;
    void importBufferForPicture(int32_t pictureBufferId, HalPixelFormat format, int handleFd,
                                const std::vector<VideoFramePlane>& planes) override;
    void reusePictureBuffer(int32_t pictureBufferId) override;
    void flush() override;
    void reset() override;
    void destroy() override;

    static media::VideoDecodeAccelerator::SupportedProfiles GetSupportedProfiles(
            uint32_t inputFormatFourcc);

    static HalPixelFormat ResolveBufferFormat(bool crcb, bool semiplanar);

    // Implementation of the media::VideoDecodeAccelerator::Client interface.
    void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                               media::VideoPixelFormat output_format,
                               const media::Size& dimensions) override;
    void DismissPictureBuffer(int32_t picture_buffer_id) override;
    void PictureReady(const media::Picture& picture) override;
    void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer_id) override;
    void NotifyFlushDone() override;
    void NotifyResetDone() override;
    void NotifyError(media::VideoDecodeAccelerator::Error error) override;

private:
    std::unique_ptr<media::VideoDecodeAccelerator> mVDA;
    VideoDecodeAcceleratorAdaptor::Client* mClient;

    // The number of allocated output buffers. This is obtained from assignPictureBuffers call from
    // client, and used to check validity of picture id in importBufferForPicture and
    // reusePictureBuffer.
    uint32_t mNumOutputBuffers;
    // The picture size for creating picture buffers. This is obtained while VDA calls
    // ProvidePictureBuffers.
    media::Size mPictureSize;

    DISALLOW_COPY_AND_ASSIGN(C2VDAAdaptor);
};

}  // namespace android

#endif  // ANDROID_C2_VDA_ADAPTOR_H
