// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// #define LOG_NDEBUG 0
#define LOG_TAG "C2VDAAdaptorProxy"

#include <C2ArcVideoAcceleratorFactory.h>
#include <C2VDAAdaptorProxy.h>

#include <videodev2.h>

#include <arc/MojoProcessSupport.h>
#include <arc/MojoThread.h>
#include <base/bind.h>
#include <binder/IServiceManager.h>
#include <mojo/edk/embedder/embedder.h>
#include <mojo/public/cpp/system/handle.h>
#include <utils/Log.h>

namespace mojo {
template <>
struct TypeConverter<::arc::VideoFramePlane, android::VideoFramePlane> {
    static ::arc::VideoFramePlane Convert(const android::VideoFramePlane& plane) {
        return ::arc::VideoFramePlane{static_cast<int32_t>(plane.mOffset),
                                      static_cast<int32_t>(plane.mStride)};
    }
};
}  // namespace mojo

namespace android {
namespace arc {
constexpr SupportedPixelFormat kSupportedPixelFormats[] = {
        // {mCrcb, mSemiplanar, mPixelFormat}
        {false, true, HalPixelFormat::NV12},
        {true, false, HalPixelFormat::YV12},
        // Add more buffer formats when needed
};

C2VDAAdaptorProxy::C2VDAAdaptorProxy()
      : C2VDAAdaptorProxy(::arc::MojoProcessSupport::getLeakyInstance()) {}

C2VDAAdaptorProxy::C2VDAAdaptorProxy(::arc::MojoProcessSupport* mojoProcessSupport)
      : mClient(nullptr),
        mMojoTaskRunner(mojoProcessSupport->mojo_thread().getTaskRunner()),
        mBinding(this),
        mRelay(new ::arc::CancellationRelay()) {}

C2VDAAdaptorProxy::~C2VDAAdaptorProxy() {}

void C2VDAAdaptorProxy::onConnectionError(const std::string& pipeName) {
    ALOGE("onConnectionError (%s)", pipeName.c_str());
    mRelay->cancel();
    NotifyError(::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
}

bool C2VDAAdaptorProxy::establishChannel() {
    ALOGV("establishChannel");
    auto future = ::arc::Future<bool>::make_shared(mRelay);
    mMojoTaskRunner->PostTask(FROM_HERE,
                              base::Bind(&C2VDAAdaptorProxy::establishChannelOnMojoThread,
                                         base::Unretained(this), future));
    return future->wait() && future->get();
}

void C2VDAAdaptorProxy::establishChannelOnMojoThread(std::shared_ptr<::arc::Future<bool>> future) {
    C2ArcVideoAcceleratorFactory& factory = ::android::C2ArcVideoAcceleratorFactory::getInstance();

    if (!factory.createVideoDecodeAccelerator(mojo::MakeRequest(&mVDAPtr))) {
        future->set(false);
        return;
    }
    mVDAPtr.set_connection_error_handler(base::Bind(&C2VDAAdaptorProxy::onConnectionError,
                                                    base::Unretained(this),
                                                    std::string("mVDAPtr (vda pipe)")));
    mVDAPtr.QueryVersion(base::Bind(&C2VDAAdaptorProxy::onVersionReady, base::Unretained(this),
                                    std::move(future)));
}

void C2VDAAdaptorProxy::onVersionReady(std::shared_ptr<::arc::Future<bool>> future, uint32_t version) {
    ALOGI("VideoDecodeAccelerator ready (version=%d)", version);

    future->set(true);
}

void C2VDAAdaptorProxy::ProvidePictureBuffers(::arc::mojom::PictureBufferFormatPtr format) {
    ALOGV("ProvidePictureBuffers");
    mClient->providePictureBuffers(
            format->min_num_buffers,
            media::Size(format->coded_size.width(), format->coded_size.height()));
}
void C2VDAAdaptorProxy::PictureReady(::arc::mojom::PicturePtr picture) {
    ALOGV("PictureReady");
    const auto& rect = picture->crop_rect;
    mClient->pictureReady(picture->picture_buffer_id, picture->bitstream_id,
                          media::Rect(rect.x(), rect.y(), rect.right(), rect.bottom()));
}

static VideoDecodeAcceleratorAdaptor::Result convertErrorCode(
        ::arc::mojom::VideoDecodeAccelerator::Result error) {
    switch (error) {
    case ::arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE:
        return VideoDecodeAcceleratorAdaptor::ILLEGAL_STATE;
    case ::arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT:
        return VideoDecodeAcceleratorAdaptor::INVALID_ARGUMENT;
    case ::arc::mojom::VideoDecodeAccelerator::Result::UNREADABLE_INPUT:
        return VideoDecodeAcceleratorAdaptor::UNREADABLE_INPUT;
    case ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE:
        return VideoDecodeAcceleratorAdaptor::PLATFORM_FAILURE;
    case ::arc::mojom::VideoDecodeAccelerator::Result::INSUFFICIENT_RESOURCES:
        return VideoDecodeAcceleratorAdaptor::INSUFFICIENT_RESOURCES;

    default:
        ALOGE("Unknown error code: %d", static_cast<int>(error));
        return VideoDecodeAcceleratorAdaptor::PLATFORM_FAILURE;
    }
}

void C2VDAAdaptorProxy::NotifyError(::arc::mojom::VideoDecodeAccelerator::Result error) {
    ALOGE("NotifyError %d", static_cast<int>(error));
    mClient->notifyError(convertErrorCode(error));
}

void C2VDAAdaptorProxy::NotifyEndOfBitstreamBuffer(int32_t bitstream_id) {
    ALOGV("NotifyEndOfBitstreamBuffer");
    mClient->notifyEndOfBitstreamBuffer(bitstream_id);
}

void C2VDAAdaptorProxy::NotifyResetDone(::arc::mojom::VideoDecodeAccelerator::Result result) {
    ALOGV("NotifyResetDone");
    if (result != ::arc::mojom::VideoDecodeAccelerator::Result::SUCCESS) {
        ALOGE("Reset is done incorrectly.");
        NotifyError(result);
        return;
    }
    mClient->notifyResetDone();
}

void C2VDAAdaptorProxy::NotifyFlushDone(::arc::mojom::VideoDecodeAccelerator::Result result) {
    ALOGV("NotifyFlushDone");
    if (result == ::arc::mojom::VideoDecodeAccelerator::Result::CANCELLED) {
        // Flush is cancelled by a succeeding Reset(). A client expects this behavior.
        ALOGE("Flush is canceled.");
        return;
    }
    if (result != ::arc::mojom::VideoDecodeAccelerator::Result::SUCCESS) {
        ALOGE("Flush is done incorrectly.");
        NotifyError(result);
        return;
    }
    mClient->notifyFlushDone();
}

//static
media::VideoDecodeAccelerator::SupportedProfiles C2VDAAdaptorProxy::GetSupportedProfiles(
        uint32_t inputFormatFourcc) {
    media::VideoDecodeAccelerator::SupportedProfiles profiles(1);
    profiles[0].min_resolution = media::Size(16, 16);
    profiles[0].max_resolution = media::Size(4096, 4096);
    switch (inputFormatFourcc) {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H264_SLICE:
        profiles[0].profile = media::H264PROFILE_MAIN;
        break;
    case V4L2_PIX_FMT_VP8:
    case V4L2_PIX_FMT_VP8_FRAME:
        profiles[0].profile = media::VP8PROFILE_ANY;
        break;
    case V4L2_PIX_FMT_VP9:
    case V4L2_PIX_FMT_VP9_FRAME:
        profiles[0].profile = media::VP9PROFILE_PROFILE0;
        break;
    default:
        ALOGE("Unknown formatfourcc: %d", inputFormatFourcc);
        return {};
    }
    return profiles;
}

//static
HalPixelFormat C2VDAAdaptorProxy::ResolveBufferFormat(bool crcb, bool semiplanar) {
    auto value = std::find_if(std::begin(kSupportedPixelFormats), std::end(kSupportedPixelFormats),
                              [crcb, semiplanar](const struct SupportedPixelFormat& f) {
                                  return f.mCrcb == crcb && f.mSemiplanar == semiplanar;
                              });
    LOG_ALWAYS_FATAL_IF(value == std::end(kSupportedPixelFormats),
                        "Unsupported pixel format: (crcb=%d, semiplanar=%d)", crcb, semiplanar);
    return value->mPixelFormat;
}

VideoDecodeAcceleratorAdaptor::Result C2VDAAdaptorProxy::initialize(
        media::VideoCodecProfile profile, bool secureMode,
        VideoDecodeAcceleratorAdaptor::Client* client) {
    ALOGV("initialize(profile=%d, secureMode=%d)", static_cast<int>(profile),
          static_cast<int>(secureMode));
    DCHECK(client);
    DCHECK(!mClient);
    mClient = client;

    if (!establishChannel()) {
        ALOGE("establishChannel failed");
        return VideoDecodeAcceleratorAdaptor::PLATFORM_FAILURE;
    }

    auto future = ::arc::Future<::arc::mojom::VideoDecodeAccelerator::Result>::make_shared(mRelay);
    mMojoTaskRunner->PostTask(FROM_HERE, base::Bind(&C2VDAAdaptorProxy::initializeOnMojoThread,
                                                    base::Unretained(this), profile, secureMode,
                                                    ::arc::FutureCallback(future)));

    if (!future->wait()) {
        ALOGE("Connection lost");
        return VideoDecodeAcceleratorAdaptor::PLATFORM_FAILURE;
    }
    return static_cast<VideoDecodeAcceleratorAdaptor::Result>(future->get());
}

void C2VDAAdaptorProxy::initializeOnMojoThread(
        const media::VideoCodecProfile profile, const bool secureMode,
        const ::arc::mojom::VideoDecodeAccelerator::InitializeCallback& cb) {
    // base::Unretained is safe because we own |mBinding|.
    auto client = mBinding.CreateInterfacePtrAndBind();
    mBinding.set_connection_error_handler(base::Bind(&C2VDAAdaptorProxy::onConnectionError,
                                                     base::Unretained(this),
                                                     std::string("mBinding (client pipe)")));

    ::arc::mojom::VideoDecodeAcceleratorConfigPtr arcConfig =
            ::arc::mojom::VideoDecodeAcceleratorConfig::New();
    arcConfig->secure_mode = secureMode;
    arcConfig->profile = static_cast<::arc::mojom::VideoCodecProfile>(profile);
    mVDAPtr->Initialize(std::move(arcConfig), std::move(client), cb);
}

void C2VDAAdaptorProxy::decode(int32_t bitstreamId, int handleFd, off_t offset, uint32_t size) {
    ALOGV("decode");
    mMojoTaskRunner->PostTask(
            FROM_HERE, base::Bind(&C2VDAAdaptorProxy::decodeOnMojoThread, base::Unretained(this),
                                  bitstreamId, handleFd, offset, size));
}

void C2VDAAdaptorProxy::decodeOnMojoThread(int32_t bitstreamId, int handleFd, off_t offset,
                                           uint32_t size) {
    MojoHandle wrappedHandle;
    MojoResult wrapResult = mojo::edk::CreatePlatformHandleWrapper(
            mojo::edk::ScopedPlatformHandle(mojo::edk::PlatformHandle(handleFd)), &wrappedHandle);
    if (wrapResult != MOJO_RESULT_OK) {
        ALOGE("failed to wrap handle: %d", static_cast<int>(wrapResult));
        NotifyError(::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
        return;
    }
    auto bufferPtr = ::arc::mojom::BitstreamBuffer::New();
    bufferPtr->bitstream_id = bitstreamId;
    bufferPtr->handle_fd = mojo::ScopedHandle(mojo::Handle(wrappedHandle));
    bufferPtr->offset = offset;
    bufferPtr->bytes_used = size;
    mVDAPtr->Decode(std::move(bufferPtr));
}

void C2VDAAdaptorProxy::assignPictureBuffers(uint32_t numOutputBuffers) {
    ALOGV("assignPictureBuffers: %d", numOutputBuffers);
    mMojoTaskRunner->PostTask(FROM_HERE,
                              base::Bind(&C2VDAAdaptorProxy::assignPictureBuffersOnMojoThread,
                                         base::Unretained(this), numOutputBuffers));
}

void C2VDAAdaptorProxy::assignPictureBuffersOnMojoThread(uint32_t numOutputBuffers) {
    mVDAPtr->AssignPictureBuffers(numOutputBuffers);
}

void C2VDAAdaptorProxy::importBufferForPicture(int32_t pictureBufferId, HalPixelFormat format,
                                               int handleFd,
                                               const std::vector<VideoFramePlane>& planes) {
    ALOGV("importBufferForPicture");
    mMojoTaskRunner->PostTask(
            FROM_HERE,
            base::Bind(&C2VDAAdaptorProxy::importBufferForPictureOnMojoThread,
                       base::Unretained(this), pictureBufferId, format, handleFd, planes));
}

void C2VDAAdaptorProxy::importBufferForPictureOnMojoThread(
        int32_t pictureBufferId, HalPixelFormat format, int handleFd,
        const std::vector<VideoFramePlane>& planes) {
    MojoHandle wrappedHandle;
    MojoResult wrapResult = mojo::edk::CreatePlatformHandleWrapper(
            mojo::edk::ScopedPlatformHandle(mojo::edk::PlatformHandle(handleFd)), &wrappedHandle);
    if (wrapResult != MOJO_RESULT_OK) {
        ALOGE("failed to wrap handle: %d", static_cast<int>(wrapResult));
        NotifyError(::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
        return;
    }

    mVDAPtr->ImportBufferForPicture(pictureBufferId,
                                    static_cast<::arc::mojom::HalPixelFormat>(format),
                                    mojo::ScopedHandle(mojo::Handle(wrappedHandle)),
                                    mojo::ConvertTo<std::vector<::arc::VideoFramePlane>>(planes));
}

void C2VDAAdaptorProxy::reusePictureBuffer(int32_t pictureBufferId) {
    ALOGV("reusePictureBuffer: %d", pictureBufferId);
    mMojoTaskRunner->PostTask(FROM_HERE,
                              base::Bind(&C2VDAAdaptorProxy::reusePictureBufferOnMojoThread,
                                         base::Unretained(this), pictureBufferId));
}

void C2VDAAdaptorProxy::reusePictureBufferOnMojoThread(int32_t pictureBufferId) {
    mVDAPtr->ReusePictureBuffer(pictureBufferId);
}

void C2VDAAdaptorProxy::flush() {
    ALOGV("flush");
    mMojoTaskRunner->PostTask(
            FROM_HERE, base::Bind(&C2VDAAdaptorProxy::flushOnMojoThread, base::Unretained(this)));
}

void C2VDAAdaptorProxy::flushOnMojoThread() {
    mVDAPtr->Flush(base::Bind(&C2VDAAdaptorProxy::NotifyFlushDone, base::Unretained(this)));
}

void C2VDAAdaptorProxy::reset() {
    ALOGV("reset");
    mMojoTaskRunner->PostTask(
            FROM_HERE, base::Bind(&C2VDAAdaptorProxy::resetOnMojoThread, base::Unretained(this)));
}

void C2VDAAdaptorProxy::resetOnMojoThread() {
    mVDAPtr->Reset(base::Bind(&C2VDAAdaptorProxy::NotifyResetDone, base::Unretained(this)));
}

void C2VDAAdaptorProxy::destroy() {
    ALOGV("destroy");
    ::arc::Future<void> future;
    ::arc::PostTaskAndSetFutureWithResult(
            mMojoTaskRunner.get(), FROM_HERE,
            base::Bind(&C2VDAAdaptorProxy::closeChannelOnMojoThread, base::Unretained(this)),
            &future);
    future.get();
}

void C2VDAAdaptorProxy::closeChannelOnMojoThread() {
    if (mBinding.is_bound()) mBinding.Close();
    mVDAPtr.reset();
}

}  // namespace arc
}  // namespace android
