#define LOG_NDEBUG  0
#define LOG_NNDEBUG 0

#define LOG_TAG "MIPISensor"

#if defined(LOG_NNDEBUG) && LOG_NNDEBUG == 0
#define ALOGVV ALOGV
#else
#define ALOGVV(...) ((void)0)
#endif

#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL | ATRACE_TAG_ALWAYS)
#include <utils/Log.h>
#include <utils/Trace.h>
#include <cutils/properties.h>
#include <android/log.h>

#include "../EmulatedFakeCamera3.h"
#include "Sensor.h"
#include "MIPISensor.h"
#include "ispaaalib.h"

#if ANDROID_PLATFORM_SDK_VERSION >= 24
#if ANDROID_PLATFORM_SDK_VERSION >= 29
#include <amlogic/am_gralloc_usage.h>
#else
#include <gralloc_usage_ext.h>
#endif
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 28
#include <amlogic/am_gralloc_ext.h>
#endif

#define ARRAY_SIZE(x) (sizeof((x))/sizeof(((x)[0])))

namespace android {

const usb_frmsize_discrete_t kUsbAvailablePictureSize[] = {
        {4128, 3096},
        {3264, 2448},
        {2592, 1944},
        {2592, 1936},
        {2560, 1920},
        {2688, 1520},
        {2048, 1536},
        {1600, 1200},
        {1920, 1088},
        {1920, 1080},
        {1440, 1080},
        {1280, 960},
        {1280, 720},
        {1024, 768},
        {960, 720},
        {720, 480},
        {640, 480},
        {352, 288},
        {320, 240},
};

extern bool IsUsbAvailablePictureSize(const usb_frmsize_discrete_t AvailablePictureSize[], uint32_t width, uint32_t height);

MIPISensor::MIPISensor() {
    mCameraVirtualDevice = nullptr;
    mVinfo = NULL;
    ALOGD("create MIPISensor");
}

MIPISensor::~MIPISensor() {
    ALOGD("delete MIPISensor");
     if (mVinfo) {
        delete(mVinfo);
        mVinfo = NULL;
    }
    if (mCameraUtil) {
        delete mCameraUtil;
        mCameraUtil = NULL;
    }
}

status_t MIPISensor::streamOff(void) {
    ALOGV("%s: E", __FUNCTION__);
    return mVinfo->stop_capturing();
}

int MIPISensor::SensorInit(int idx) {
    ALOGV("%s: E", __FUNCTION__);
    int ret = 0;
    if (mVinfo == NULL)
        mVinfo =  new CVideoInfo();
    ret = camera_open(idx);
    if (ret < 0) {
        ALOGE("Unable to open sensor %d, errno=%d\n", mVinfo->idx, ret);
        return ret;
    }
    InitVideoInfo(idx);
    mVinfo->camera_init();
    if (strstr((const char *)mVinfo->cap.card, "front"))
        mSensorFace = SENSOR_FACE_FRONT;
    else if (strstr((const char *)mVinfo->cap.card, "back"))
        mSensorFace = SENSOR_FACE_BACK;
    else
        mSensorFace = SENSOR_FACE_NONE;

    mSensorType = SENSOR_MIPI;
    return ret;
}

status_t MIPISensor::startUp(int idx) {
    ALOGV("%s: E", __FUNCTION__);
    int res;
    mCapturedBuffers = NULL;
    res = run("EmulatedFakeCamera3::Sensor",ANDROID_PRIORITY_URGENT_DISPLAY);

    if (res != OK) {
       ALOGE("Unable to start up sensor capture thread: %d", res);
    }
    res = SensorInit(idx);
    if (!mCameraUtil)
        mCameraUtil = new CameraUtil();
    return res;
}

int MIPISensor::camera_open(int idx) {
    int ret = 0;
    char dev_name[128];
    //sprintf(dev_name, "/dev/video%d",idx);
    if (mCameraVirtualDevice == nullptr)
        mCameraVirtualDevice = CameraVirtualDevice::getInstance();
    mMIPIDevicefd[0] = mCameraVirtualDevice->openVirtualDevice(idx);
    if (mMIPIDevicefd[0] < 0) {
        ALOGE("open %s failed, %s\n", dev_name, strerror(errno));
        ret = -ENOTTY;
    }
    ALOGD("open %s ok !", dev_name);
    mVinfo->fd = mMIPIDevicefd[0];
#ifdef ISP_ENABLE
    ALOGD( "enable isp lib");
    isp_lib_enable();
#endif
    return ret;
}

void MIPISensor::camera_close(void) {
    ALOGV("%s: E", __FUNCTION__);
    if (mMIPIDevicefd[0] < 0)
        return;
    if (mCameraVirtualDevice == nullptr)
        mCameraVirtualDevice = CameraVirtualDevice::getInstance();

    mCameraVirtualDevice->releaseVirtualDevice(mVinfo->idx,mMIPIDevicefd[0]);
    mMIPIDevicefd[0] = -1;
#ifdef ISP_ENABLE
    ALOGD( "disable isp lib");
    isp_lib_disable();
#endif
}

void MIPISensor::InitVideoInfo(int idx) {
    if (mVinfo) {
        mVinfo->fd = mMIPIDevicefd[0];
        mVinfo->idx = idx;
    }
}

status_t MIPISensor::shutDown() {
    ALOGV("%s: E", __FUNCTION__);
    int res;
    mTimeOutCount = 0;
    res = requestExitAndWait();
    if (res != OK) {
        ALOGE("Unable to shut down sensor capture thread: %d", res);
    }
    if (mVinfo != NULL)
        mVinfo->stop_capturing();

    camera_close();

    if (mImage_buffer) {
        delete [] mImage_buffer;
        mImage_buffer = NULL;
    }
    mSensorWorkFlag = false;
    ALOGD("%s: Exit", __FUNCTION__);
    return res;
}

uint32_t MIPISensor::getStreamUsage(int stream_type){
    ATRACE_CALL();
    uint32_t usage = Sensor::getStreamUsage(stream_type);
    usage = (GRALLOC_USAGE_HW_TEXTURE
            | GRALLOC_USAGE_HW_RENDER
            | GRALLOC_USAGE_SW_READ_MASK
            | GRALLOC_USAGE_SW_WRITE_MASK
            );

#if ANDROID_PLATFORM_SDK_VERSION >= 28
        usage = am_gralloc_get_omx_osd_producer_usage();
#else
        usage = GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_AML_DMA_BUFFER;
#endif

    ALOGV("%s: usage=0x%x", __FUNCTION__,usage);
    return usage;
}

void MIPISensor::captureRGB(uint8_t *img, uint32_t gain, uint32_t stride){
    uint8_t *src = NULL;
    int ret = 0, rotate = 0;
    uint32_t width = 0, height = 0;
    int dqTryNum = 3;

    rotate = getPictureRotate();
    width = mVinfo->picture.format.fmt.pix.width;
    height = mVinfo->picture.format.fmt.pix.height;

    mVinfo->stop_capturing();
    ret = mVinfo->start_picture(rotate);
    if (ret < 0)
    {
        ALOGD("start picture failed!");
        return;
    }
    while (1)
    {
        if (mFlushFlag) {
            break;
        }

        if (mExitSensorThread) {
            break;
        }

        src = (uint8_t *)mVinfo->get_picture();
        if (NULL == src) {
            usleep(10000);
            continue;
        }
        if ((NULL != src) && ((mVinfo->picture.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) ||
            (mVinfo->picture.format.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24))) {

            while (dqTryNum > 0) {
                if (NULL != src) {
                   mVinfo->putback_picture_frame();
                }
                usleep(10000);
                dqTryNum --;
                src = (uint8_t *)mVinfo->get_picture();
                while (src == NULL) {
                    usleep(10000);
                    src = (uint8_t *)mVinfo->get_picture();
                }
            }
        }

        if (NULL != src) {
            mSensorWorkFlag = true;
            if (mVinfo->picture.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
                if (mVinfo->picture.buf.length == mVinfo->picture.buf.bytesused) {
                    mCameraUtil->yuyv422_to_rgb24(src,img,width,height);
                    break;
                } else {
                    mVinfo->putback_picture_frame();
                    usleep(5000);
                }
            } else if (mVinfo->picture.format.fmt.pix.pixelformat == V4L2_PIX_FMT_RGB24) {
                if (mVinfo->picture.buf.length == width * height * 3) {
                    memcpy(img, src, mVinfo->picture.buf.length);
                } else {
                    mCameraUtil->rgb24_memcpy(img, src, width, height);
                }
                break;
            } else if (mVinfo->picture.format.fmt.pix.pixelformat == V4L2_PIX_FMT_NV21) {
                memcpy(img, src, mVinfo->picture.buf.length);
                break;
            }
        }
    }
    ALOGVV("get picture success !");
    mVinfo->stop_picture();

}

void MIPISensor::captureNV21(StreamBuffer b, uint32_t gain){
    ATRACE_CALL();
    uint8_t *src;
    //ALOGVV("MIPI NV21 sensor image captured");

    if (mKernelBuffer) {
        src = mKernelBuffer;
        if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_NV21) {
            uint32_t width = mVinfo->preview.format.fmt.pix.width;
            uint32_t height = mVinfo->preview.format.fmt.pix.height;
            if ((width == b.width) && (height == b.height)) {
                memcpy(b.img, src, b.stride * b.height * 3/2);
            } else {
                mCameraUtil->ReSizeNV21(src, b.img, b.width, b.height, b.stride,width,height);
            }
        } else if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            uint32_t width = mVinfo->preview.format.fmt.pix.width;
            uint32_t height = mVinfo->preview.format.fmt.pix.height;

            if ((width == b.width) && (height == b.height)) {
                memcpy(b.img, src, b.stride * b.height * 3/2);
            } else {
                mCameraUtil->ReSizeNV21(src, b.img, b.width, b.height, b.stride,width,height);
            }
        } else {
            ALOGE("Unable known sensor format: %d", mVinfo->preview.format.fmt.pix.pixelformat);
        }
        return ;
    }
    while (1) {
        if (mFlushFlag) {
            break;
        }

        if (mExitSensorThread) {
            break;
        }

        src = (uint8_t *)mVinfo->get_frame();
        if (NULL == src) {
            ALOGVV("get frame NULL, sleep 5ms");
            usleep(5000);
            continue;
        }
        mTimeOutCount = 0;
        if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_NV21) {
            if (mVinfo->preview.buf.length == b.width * b.height * 3/2) {
                memcpy(b.img, src, mVinfo->preview.buf.length);
            } else {
                mCameraUtil->nv21_memcpy_align32 (b.img, src, b.width, b.height);
            }
            mKernelBuffer = b.img;
        } else if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            uint32_t width = mVinfo->preview.format.fmt.pix.width;
            uint32_t height = mVinfo->preview.format.fmt.pix.height;
            memset(mImage_buffer, 0 , width * height * 3/2);
            mCameraUtil->YUYVToNV21(src, mImage_buffer, width, height);
            if ((width == b.width) && (height == b.height)) {
                memcpy(b.img, mImage_buffer, b.width * b.height * 3/2);
                mKernelBuffer = b.img;
            } else {
                if ((b.height % 2) != 0) {
                    DBG_LOGB("%d , b.height = %d", __LINE__, b.height);
                    b.height = b.height - 1;
                }
                mCameraUtil->ReSizeNV21(mImage_buffer, b.img, b.width, b.height, b.stride,width,height);
                mKernelBuffer = mImage_buffer;
            }
        }
        mSensorWorkFlag = true;
        mVinfo->putback_frame();
        break;
    }
}

void MIPISensor::captureYV12(StreamBuffer b, uint32_t gain){
    uint8_t *src;
    if (mKernelBuffer) {
        src = mKernelBuffer;
        if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU420) {
            int width = mVinfo->preview.format.fmt.pix.width;
            int height = mVinfo->preview.format.fmt.pix.height;
            mCameraUtil->ScaleYV12(src,width,height,b.img,b.width,b.height);
        } else if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            int width = mVinfo->preview.format.fmt.pix.width;
            int height = mVinfo->preview.format.fmt.pix.height;
            mCameraUtil->YUYVScaleYV12(src,width,height,b.img,b.width,b.height);
        } else {
            ALOGE("Unable known sensor format: %d",
                mVinfo->preview.format.fmt.pix.pixelformat);
        }
        return ;
    }
    while (1) {
        if (mFlushFlag) {
            break;
        }
        if (mExitSensorThread) {
            break;
        }
        src = (uint8_t *)mVinfo->get_frame();

        if (NULL == src) {
            ALOGVV("get frame NULL, sleep 5ms");
            usleep(5000);
            mTimeOutCount++;
            if (mTimeOutCount > 600) {
                force_reset_sensor();
            }
            continue;
        }
        mTimeOutCount = 0;
        if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YVU420) {
            if (mVinfo->preview.buf.length == b.width * b.height * 3/2) {
                memcpy(b.img, src, mVinfo->preview.buf.length);
            } else {
                mCameraUtil->yv12_memcpy_align32 (b.img, src, b.width, b.height);
            }
            mKernelBuffer = b.img;
        } else if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            int width = mVinfo->preview.format.fmt.pix.width;
            int height = mVinfo->preview.format.fmt.pix.height;
            mCameraUtil->YUYVToYV12(src, b.img, width, height);
            mKernelBuffer = b.img;
        } else {
            ALOGE("Unable known sensor format: %d",
                mVinfo->preview.format.fmt.pix.pixelformat);
        }
        mSensorWorkFlag = true;
        mVinfo->putback_frame();
        break;
    }
    ALOGVV("YV12 sensor image captured");
}
void MIPISensor::captureYUYV(uint8_t *img, uint32_t gain, uint32_t stride){
    uint8_t *src;
    if (mKernelBuffer) {
        src = mKernelBuffer;
        if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            //TODO YUYV scale
            //memcpy(img, src, vinfo->preview.buf.length);

        } else
            ALOGE("Unable known sensor format: %d", mVinfo->preview.format.fmt.pix.pixelformat);

        return ;
    }

    while (1) {
        if (mFlushFlag) {
            break;
        }
        if (mExitSensorThread) {
            break;
        }
        src = (uint8_t *)mVinfo->get_frame();
        if (NULL == src) {
            ALOGVV("get frame NULL, sleep 5ms");
            usleep(5000);
            mTimeOutCount++;
            if (mTimeOutCount > 600) {
                force_reset_sensor();
            }
            continue;
        }
        mTimeOutCount = 0;
        if (mVinfo->preview.format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
            memcpy(img, src, mVinfo->preview.buf.length);
            mKernelBuffer = src;
        } else {
            ALOGE("Unable known sensor format: %d", mVinfo->preview.format.fmt.pix.pixelformat);
        }
        mSensorWorkFlag = true;
        mVinfo->putback_frame();
        break;
    }
    ALOGVV("YUYV sensor image captured");
}

status_t MIPISensor::getOutputFormat(void) {
    uint32_t ret = 0;
     ret = mVinfo->EnumerateFormat(V4L2_PIX_FMT_NV21);
    if (ret)
        return ret;

    ret = mVinfo->EnumerateFormat(V4L2_PIX_FMT_YUYV);
    if (ret)
        return ret;
    ALOGE("Unable to find a supported sensor format!");
    return BAD_VALUE;
}

status_t MIPISensor::setOutputFormat(int width, int height, int pixelformat, bool isjpeg) {
    int res;
    mFramecount = 0;
    mCurFps = 0;
    gettimeofday(&mTimeStart, NULL);

    if (isjpeg) {
        mVinfo->picture.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVinfo->picture.format.fmt.pix.width = width;
        mVinfo->picture.format.fmt.pix.height = height;
        mVinfo->picture.format.fmt.pix.pixelformat = pixelformat;
    } else {
        mVinfo->preview.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVinfo->preview.format.fmt.pix.width = width;
        mVinfo->preview.format.fmt.pix.height = height;
        mVinfo->preview.format.fmt.pix.pixelformat = pixelformat;
        res = mVinfo->setBuffersFormat();
        if (res < 0) {
            ALOGE("set buffer failed\n");
            return res;
            }
    }
    if (NULL == mImage_buffer) {
        mPre_width = mVinfo->preview.format.fmt.pix.width;
        mPre_height = mVinfo->preview.format.fmt.pix.height;
        DBG_LOGB("setOutputFormat :: pre_width = %d, pre_height = %d \n" , mPre_width , mPre_height);
        mImage_buffer = new uint8_t[mPre_width * mPre_height * 3 / 2];
        if (mImage_buffer == NULL) {
            ALOGE("first time allocate mTemp_buffer failed !");
            return -1;
            }
        }
    if ((mPre_width != mVinfo->preview.format.fmt.pix.width)
        && (mPre_height != mVinfo->preview.format.fmt.pix.height)) {
            if (mImage_buffer) {
                delete [] mImage_buffer;
                mImage_buffer = NULL;
            }
            mPre_width = mVinfo->preview.format.fmt.pix.width;
            mPre_height = mVinfo->preview.format.fmt.pix.height;
            mImage_buffer = new uint8_t[mPre_width * mPre_height * 3 / 2];
            if (mImage_buffer == NULL) {
                ALOGE("allocate mTemp_buffer failed !");
                return -1;
            }
        }
        return OK;
}

int MIPISensor::halFormatToSensorFormat(uint32_t pixelfmt) {
    uint32_t ret = 0;
    uint32_t fmt = 0;
    switch (pixelfmt) {
        case HAL_PIXEL_FORMAT_YV12:
            fmt = V4L2_PIX_FMT_YVU420;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            fmt = V4L2_PIX_FMT_NV21;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            fmt = V4L2_PIX_FMT_YUYV;
            break;
        default:
            fmt = V4L2_PIX_FMT_NV21;
            break;
        break;
    }
    ret = mVinfo->EnumerateFormat(fmt);
    if (ret)
        return ret;

    ret = mVinfo->EnumerateFormat(V4L2_PIX_FMT_YUYV);
    if (ret)
        return ret;

    ALOGE("%s, Unable to find a supported sensor format!", __FUNCTION__);
    return BAD_VALUE;
}

status_t MIPISensor::IoctlStateProbe(void) {
    if (mVinfo->IsSupportRotation()) {
            msupportrotate = true;
            DBG_LOGA("camera support capture rotate");
            mIoctlSupport |= IOCTL_MASK_ROTATE;
    }
    return mIoctlSupport;
}

status_t MIPISensor::streamOn() {
    return mVinfo->start_capturing();
}

bool MIPISensor::isStreaming() {
    return mVinfo->isStreaming;
}

bool MIPISensor::isNeedRestart(uint32_t width, uint32_t height, uint32_t pixelformat) {
    if ((mVinfo->preview.format.fmt.pix.width != width)
        ||(mVinfo->preview.format.fmt.pix.height != height)) {
        return true;
    }
    return false;
}

int MIPISensor::getStreamConfigurations(uint32_t picSizes[], const int32_t kAvailableFormats[], int size) {
      int res;
    int i, j, k, START;
    int count = 0;
    //int pixelfmt;
    struct v4l2_frmsizeenum frmsize;
    char property[PROPERTY_VALUE_MAX];
    unsigned int support_w,support_h;

    support_w = 10000;
    support_h = 10000;
    memset(property, 0, sizeof(property));
    if (property_get("ro.media.camera_preview.maxsize", property, NULL) > 0) {
        CAMHAL_LOGDB("support Max Preview Size :%s",property);
        if (sscanf(property,"%dx%d",&support_w,&support_h) != 2) {
            support_w = 10000;
            support_h = 10000;
        }
    }

    memset(&frmsize,0,sizeof(frmsize));
    frmsize.pixel_format = getOutputFormat();

    START = 0;
    for (i = 0; ; i++) {
        frmsize.index = i;
        res = ioctl(mVinfo->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
        if (res < 0) {
            DBG_LOGB("index=%d, break\n", i);
            break;
        }

        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) { //only support this type

            if (0 != (frmsize.discrete.width%16))
                continue;

            if ((frmsize.discrete.width * frmsize.discrete.height) > (support_w * support_h))
                continue;
            if (count >= size)
                break;

            picSizes[count+0] = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
            picSizes[count+1] = frmsize.discrete.width;
            picSizes[count+2] = frmsize.discrete.height;
            picSizes[count+3] = ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;

            DBG_LOGB("get output width=%d, height=%d, format=%d\n",
                frmsize.discrete.width, frmsize.discrete.height, frmsize.pixel_format);
            if (0 == i) {
                count += 4;
                continue;
            }

            for (k = count; k > START; k -= 4) {
                if (frmsize.discrete.width * frmsize.discrete.height >
                        picSizes[k - 3] * picSizes[k - 2]) {
                    picSizes[k + 1] = picSizes[k - 3];
                    picSizes[k + 2] = picSizes[k - 2];

                } else {
                    break;
                }
            }
            picSizes[k + 1] = frmsize.discrete.width;
            picSizes[k + 2] = frmsize.discrete.height;

            count += 4;
        }
    }

    START = count;
    for (i = 0; ; i++) {
        frmsize.index = i;
        res = ioctl(mVinfo->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
        if (res < 0) {
            DBG_LOGB("index=%d, break\n", i);
            break;
        }

        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) { //only support this type

            if (0 != (frmsize.discrete.width%16))
                continue;

            if ((frmsize.discrete.width * frmsize.discrete.height) > (support_w * support_h))
                continue;
            if (count >= size)
                break;

            picSizes[count+0] = HAL_PIXEL_FORMAT_YCbCr_420_888;
            picSizes[count+1] = frmsize.discrete.width;
            picSizes[count+2] = frmsize.discrete.height;
            picSizes[count+3] = ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;

            DBG_LOGB("get output width=%d, height=%d, format =\
                HAL_PIXEL_FORMAT_YCbCr_420_888\n", frmsize.discrete.width,
                                                    frmsize.discrete.height);
            if (0 == i) {
                count += 4;
                continue;
            }

            for (k = count; k > START; k -= 4) {
                if (frmsize.discrete.width * frmsize.discrete.height >
                        picSizes[k - 3] * picSizes[k - 2]) {
                    picSizes[k + 1] = picSizes[k - 3];
                    picSizes[k + 2] = picSizes[k - 2];

                } else {
                    break;
                }
            }
            picSizes[k + 1] = frmsize.discrete.width;
            picSizes[k + 2] = frmsize.discrete.height;

            count += 4;
        }
    }

    uint32_t jpgSrcfmt[] = {
        V4L2_PIX_FMT_RGB24,
        V4L2_PIX_FMT_YUYV,
    };

    START = count;
    for (j = 0; j<(int)(sizeof(jpgSrcfmt)/sizeof(jpgSrcfmt[0])); j++) {
        memset(&frmsize,0,sizeof(frmsize));
        frmsize.pixel_format = jpgSrcfmt[j];

        for (i = 0; ; i++) {
            frmsize.index = i;
            res = ioctl(mVinfo->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
            if (res < 0) {
                DBG_LOGB("index=%d, break\n", i);
                break;
            }

            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) { //only support this type

                if (0 != (frmsize.discrete.width%16))
                    continue;

                //if((frmsize.discrete.width > support_w) && (frmsize.discrete.height >support_h))
                //    continue;

                if (count >= size)
                    break;

                if (frmsize.pixel_format == V4L2_PIX_FMT_YUYV) {
                    if (!IsUsbAvailablePictureSize(kUsbAvailablePictureSize, frmsize.discrete.width, frmsize.discrete.height))
                        continue;
                }

                picSizes[count+0] = HAL_PIXEL_FORMAT_BLOB;
                picSizes[count+1] = frmsize.discrete.width;
                picSizes[count+2] = frmsize.discrete.height;
                picSizes[count+3] = ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;

                if (0 == i) {
                    count += 4;
                    continue;
                }

                //TODO insert in descend order
                for (k = count; k > START; k -= 4) {
                    if (frmsize.discrete.width * frmsize.discrete.height >
                            picSizes[k - 3] * picSizes[k - 2]) {
                        picSizes[k + 1] = picSizes[k - 3];
                        picSizes[k + 2] = picSizes[k - 2];

                    } else {
                        break;
                    }
                }

                picSizes[k + 1] = frmsize.discrete.width;
                picSizes[k + 2] = frmsize.discrete.height;

                count += 4;
            }
        }

        if (frmsize.index > 0)
            break;
    }

    if (frmsize.index == 0)
        CAMHAL_LOGDA("no support pixel fmt for jpeg");

    return count;
}

int MIPISensor::getStreamConfigurationDurations(uint32_t picSizes[], int64_t duration[], int size, bool flag) {
    int ret=0; int framerate=0; int temp_rate=0;
    struct v4l2_frmivalenum fival;
    int i,j=0;
    int count = 0;
    int tmp_size = size;
    memset(duration, 0 ,sizeof(int64_t) * size);
    int pixelfmt_tbl[] = {
        V4L2_PIX_FMT_YVU420,
        V4L2_PIX_FMT_NV21,
        V4L2_PIX_FMT_RGB24,
        V4L2_PIX_FMT_YUYV,
    };

    for ( i = 0; i < (int) ARRAY_SIZE(pixelfmt_tbl); i++)
    {
        /* we got all duration for each resolution for prev format*/
        if (count >= tmp_size)
            break;

        for ( ; size > 0; size-=4)
        {
            memset(&fival, 0, sizeof(fival));

            for (fival.index = 0;;fival.index++)
            {
                fival.pixel_format = pixelfmt_tbl[i];
                fival.width = picSizes[size-3];
                fival.height = picSizes[size-2];
                if ((ret = ioctl(mVinfo->fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0) {
                    if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                        temp_rate = fival.discrete.denominator/fival.discrete.numerator;
                        if (framerate < temp_rate)
                            framerate = temp_rate;
                        duration[count+0] = (int64_t)(picSizes[size-4]);
                        duration[count+1] = (int64_t)(picSizes[size-3]);
                        duration[count+2] = (int64_t)(picSizes[size-2]);
                        duration[count+3] = (int64_t)((1.0/framerate) * 1000000000);
                        j++;
                    } else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
                        temp_rate = fival.discrete.denominator/fival.discrete.numerator;
                        if (framerate < temp_rate)
                            framerate = temp_rate;
                        duration[count+0] = (int64_t)picSizes[size-4];
                        duration[count+1] = (int64_t)picSizes[size-3];
                        duration[count+2] = (int64_t)picSizes[size-2];
                        duration[count+3] = (int64_t)((1.0/framerate) * 1000000000);
                        j++;
                    } else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
                        temp_rate = fival.discrete.denominator/fival.discrete.numerator;
                        if (framerate < temp_rate)
                            framerate = temp_rate;
                        duration[count+0] = (int64_t)picSizes[size-4];
                        duration[count+1] = (int64_t)picSizes[size-3];
                        duration[count+2] = (int64_t)picSizes[size-2];
                        duration[count+3] = (int64_t)((1.0/framerate) * 1000000000);
                        j++;
                    }
                } else {
                    if (j > 0) {
                        if (count >= tmp_size)
                            break;
                        duration[count+0] = (int64_t)(picSizes[size-4]);
                        duration[count+1] = (int64_t)(picSizes[size-3]);
                        duration[count+2] = (int64_t)(picSizes[size-2]);
                        if (framerate == 5) {
                            if ((!flag) && ((duration[count+0] == HAL_PIXEL_FORMAT_YCbCr_420_888)
                                || (duration[count+0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)))
                                duration[count+3] = 0;
                            else
                                duration[count+3] = (int64_t)200000000L;
                        } else if (framerate == 10) {
                            if ((!flag) && ((duration[count+0] == HAL_PIXEL_FORMAT_YCbCr_420_888)
                                || (duration[count+0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)))
                                duration[count+3] = 0;
                            else
                                duration[count+3] = (int64_t)100000000L;
                        } else if (framerate == 15) {
                            if ((!flag) && ((duration[count+0] == HAL_PIXEL_FORMAT_YCbCr_420_888)
                                || (duration[count+0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)))
                                duration[count+3] = 0;
                            else
                                duration[count+3] = (int64_t)66666666L;
                        } else if (framerate == 30) {
                            if ((!flag) && ((duration[count+0] == HAL_PIXEL_FORMAT_YCbCr_420_888)
                                || (duration[count+0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)))
                                duration[count+3] = 0;
                            else
                                duration[count+3] = (int64_t)33333333L;
                        } else if (framerate == 60) {
                            if ((!flag) && ((duration[count+0] == HAL_PIXEL_FORMAT_YCbCr_420_888)
                                || (duration[count+0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)))
                                duration[count+3] = 0;
                            else {
                                if (mSensorType == SENSOR_USB)
                                    duration[count+3] = (int64_t)33333333L;
                                else
                                    duration[count+3] = (int64_t)16666666L;
                            }
                        } else {
                            if ((!flag) && ((duration[count+0] == HAL_PIXEL_FORMAT_YCbCr_420_888)
                                || (duration[count+0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)))
                                duration[count+3] = 0;
                            else
                                duration[count+3] = (int64_t)66666666L;
                        }
                        count += 4;
                        break;
                    } else {
                        break;
                    }
                }
            }
            framerate=0;
            j=0;
        }
        size = tmp_size;
    }

    return count;
}

int64_t MIPISensor::getMinFrameDuration() {
    int64_t tmpDuration =  66666666L; // 1/15 s
    int64_t frameDuration =  66666666L; // 1/15 s
    struct v4l2_frmivalenum fival;
    int i,j;

    uint32_t pixelfmt_tbl[]={
        V4L2_PIX_FMT_YUYV,
        V4L2_PIX_FMT_NV21,
    };
    struct v4l2_frmsize_discrete resolution_tbl[]={
        {1920, 1080},
        {1280, 960},
        {640, 480},
        {352, 288},
        {320, 240},
    };

    for (i = 0; i < (int)ARRAY_SIZE(pixelfmt_tbl); i++) {
        for (j = 0; j < (int) ARRAY_SIZE(resolution_tbl); j++) {
            memset(&fival, 0, sizeof(fival));
            fival.index = 0;
            fival.pixel_format = pixelfmt_tbl[i];
            fival.width = resolution_tbl[j].width;
            fival.height = resolution_tbl[j].height;

            while (ioctl(mVinfo->fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival) == 0) {
                if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
                    tmpDuration =
                        fival.discrete.numerator * 1000000000L / fival.discrete.denominator;

                    if (frameDuration > tmpDuration)
                        frameDuration = tmpDuration;
                } else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
                    frameDuration =
                        fival.stepwise.max.numerator * 1000000000L / fival.stepwise.max.denominator;
                    break;
                } else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
                    frameDuration =
                        fival.stepwise.max.numerator * 1000000000L / fival.stepwise.max.denominator;
                    break;
                }
                fival.index++;
            }
        }

        if (fival.index > 0) {
            break;
        }
    }

    //CAMHAL_LOGDB("enum frameDuration=%lld\n", frameDuration);
    return frameDuration;
}

int MIPISensor::getPictureSizes(int32_t picSizes[], int size, bool preview) {
    int res;
    int i;
    int count = 0;
    struct v4l2_frmsizeenum frmsize;
    char property[PROPERTY_VALUE_MAX];
    unsigned int support_w,support_h;
    int preview_fmt;

    support_w = 10000;
    support_h = 10000;
    memset(property, 0, sizeof(property));
    if (property_get("ro.media.camera_preview.maxsize", property, NULL) > 0) {
        CAMHAL_LOGDB("support Max Preview Size :%s",property);
        if (sscanf(property,"%dx%d",&support_w,&support_h) !=2 ) {
            support_w = 10000;
            support_h = 10000;
        }
    }
    memset(&frmsize,0,sizeof(frmsize));
    preview_fmt = V4L2_PIX_FMT_NV21;//getOutputFormat();

    if (preview_fmt == V4L2_PIX_FMT_NV21) {
        if (preview == true)
            frmsize.pixel_format = V4L2_PIX_FMT_NV21;
        else
            frmsize.pixel_format = V4L2_PIX_FMT_RGB24;
    } else if (preview_fmt == V4L2_PIX_FMT_YVU420) {
        if (preview == true)
            frmsize.pixel_format = V4L2_PIX_FMT_YVU420;
        else
            frmsize.pixel_format = V4L2_PIX_FMT_RGB24;
    } else if (preview_fmt == V4L2_PIX_FMT_YUYV)
        frmsize.pixel_format = V4L2_PIX_FMT_YUYV;

    for (i = 0; ; i++) {
        frmsize.index = i;
        res = ioctl(mVinfo->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize);
        if (res < 0) {
            DBG_LOGB("index=%d, break\n", i);
            break;
        }


        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) { //only support this type

            if (0 != (frmsize.discrete.width%16))
                continue;

            if ((frmsize.discrete.width > support_w) && (frmsize.discrete.height > support_h))
                continue;

            if (count >= size)
                break;

            picSizes[count] = frmsize.discrete.width;
            picSizes[count+1]  =  frmsize.discrete.height;

            if (0 == i) {
                count += 2;
                continue;
            }

            //TODO insert in descend order
            if (picSizes[count + 0] * picSizes[count + 1] > picSizes[count - 1] * picSizes[count - 2]) {
                picSizes[count + 0] = picSizes[count - 2];
                picSizes[count + 1] = picSizes[count - 1];

                picSizes[count - 2] = frmsize.discrete.width;
                picSizes[count - 1] = frmsize.discrete.height;
            }

            count += 2;
        }
    }

    return count;
}

status_t MIPISensor::force_reset_sensor() {
    DBG_LOGA("force_reset_sensor");
    status_t ret;
    mTimeOutCount = 0;
    ret = streamOff();
    ret = mVinfo->setBuffersFormat();
    ret = streamOn();
    DBG_LOGB("%s , ret = %d", __FUNCTION__, ret);
    return ret;
}

int MIPISensor::captureNewImage() {
    bool isjpeg = false;
    uint32_t gain = mGainFactor;
    mKernelBuffer = NULL;

    // Might be adding more buffers, so size isn't constant
    ALOGVV("%s:buffer size=%d\n",__FUNCTION__,mNextCapturedBuffers->size());
    for (size_t i = 0; i < mNextCapturedBuffers->size(); i++) {
        const StreamBuffer &b = (*mNextCapturedBuffers)[i];
        ALOGVV("Sensor capturing buffer %d: stream %d,"
                " %d x %d, format %x, stride %d, buf %p, img %p",
                i, b.streamId, b.width, b.height, b.format, b.stride,
                b.buffer, b.img);
        switch (b.format) {
#if PLATFORM_SDK_VERSION <= 22
            case HAL_PIXEL_FORMAT_RAW_SENSOR:
                captureRaw(b.img, gain, b.stride);
                break;
#endif
            case HAL_PIXEL_FORMAT_RGB_888:
                captureRGB(b.img, gain, b.stride);
                break;
            case HAL_PIXEL_FORMAT_RGBA_8888:
                captureRGBA(b.img, gain, b.stride);
                break;
            case HAL_PIXEL_FORMAT_BLOB:
                // Add auxillary buffer of the right size
                // Assumes only one BLOB (JPEG) buffer in
                // mNextCapturedBuffers
                StreamBuffer bAux;
                int orientation;
                orientation = getPictureRotate();
                ALOGD("bAux orientation=%d",orientation);
                uint32_t pixelfmt;
                if ((b.width == mVinfo->preview.format.fmt.pix.width &&
                b.height == mVinfo->preview.format.fmt.pix.height) && (orientation == 0)) {

                pixelfmt = getOutputFormat();
                if (pixelfmt == V4L2_PIX_FMT_YVU420) {
                    pixelfmt = HAL_PIXEL_FORMAT_YV12;
                } else if (pixelfmt == V4L2_PIX_FMT_NV21) {
                    pixelfmt = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                } else if (pixelfmt == V4L2_PIX_FMT_YUYV) {
                    pixelfmt = HAL_PIXEL_FORMAT_YCbCr_422_I;
                } else {
                    pixelfmt = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                }
                } else {
                    isjpeg = true;
                    pixelfmt = HAL_PIXEL_FORMAT_RGB_888;
                }

                if (!msupportrotate) {
                    bAux.streamId = 0;
                    bAux.width = b.width;
                    bAux.height = b.height;
                    bAux.format = pixelfmt;
                    bAux.stride = b.width;
                    bAux.buffer = NULL;
                } else {
                    if ((orientation == 90) || (orientation == 270)) {
                        bAux.streamId = 0;
                        bAux.width = b.height;
                        bAux.height = b.width;
                        bAux.format = pixelfmt;
                        bAux.stride = b.height;
                        bAux.buffer = NULL;
                    } else {
                        bAux.streamId = 0;
                        bAux.width = b.width;
                        bAux.height = b.height;
                        bAux.format = pixelfmt;
                        bAux.stride = b.width;
                        bAux.buffer = NULL;
                    }
                }
                // TODO: Reuse these
                bAux.img = new uint8_t[b.width * b.height * 3];
                mNextCapturedBuffers->push_back(bAux);
                break;
            case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            case HAL_PIXEL_FORMAT_YCbCr_420_888:
                captureNV21(b, gain);
                break;
            case HAL_PIXEL_FORMAT_YV12:
                captureYV12(b, gain);
                break;
            case HAL_PIXEL_FORMAT_YCbCr_422_I:
                captureYUYV(b.img, gain, b.stride);
                break;
            default:
                ALOGE("%s: Unknown format %x, no output", __FUNCTION__,
                        b.format);
                break;
        }
    }
    return 0;
}


int MIPISensor::getZoom(int *zoomMin, int *zoomMax, int *zoomStep) {
    int ret = 0;
    struct v4l2_queryctrl qc;
    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_ZOOM_ABSOLUTE;
    ret = ioctl (mVinfo->fd, VIDIOC_QUERYCTRL, &qc);
    if ((qc.flags == V4L2_CTRL_FLAG_DISABLED) || ( ret < 0)
        || (qc.type != V4L2_CTRL_TYPE_INTEGER)) {
        ret = -1;
        *zoomMin = 0;
        *zoomMax = 0;
        *zoomStep = 1;
        CAMHAL_LOGDB("%s: Can't get zoom level!\n", __FUNCTION__);
    } else {
        if ((qc.step != 0) && (qc.minimum != 0) &&
            ((qc.minimum/qc.step) > (qc.maximum/qc.minimum))) {
                DBG_LOGA("adjust zoom step. \n");
                qc.step = (qc.minimum * qc.step);
            }
        *zoomMin = qc.minimum;
        *zoomMax = qc.maximum;
        *zoomStep = qc.step;
        DBG_LOGB("zoomMin:%dzoomMax:%dzoomStep:%d\n", *zoomMin, *zoomMax, *zoomStep);
    }
    return ret ;
}

int MIPISensor::setZoom(int zoomValue) {
    int ret = 0;
    struct v4l2_control ctl;
    memset( &ctl, 0, sizeof(ctl));
    ctl.value = zoomValue;
    ctl.id = V4L2_CID_ZOOM_ABSOLUTE;
    ret = ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl);
    if (ret < 0) {
        ALOGE("%s: Set zoom level failed!\n", __FUNCTION__);
        }
    return ret ;
}
status_t MIPISensor::setEffect(uint8_t effect) {
     int ret = 0;
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_COLORFX;
    switch (effect) {
        case ANDROID_CONTROL_EFFECT_MODE_OFF:
        ctl.value= CAM_EFFECT_ENC_NORMAL;
        break;
        case ANDROID_CONTROL_EFFECT_MODE_NEGATIVE:
        ctl.value= CAM_EFFECT_ENC_COLORINV;
        break;
        case ANDROID_CONTROL_EFFECT_MODE_SEPIA:
        ctl.value= CAM_EFFECT_ENC_SEPIA;
        break;
        default:
        ALOGE("%s: Doesn't support effect mode %d",
        __FUNCTION__, effect);
        return BAD_VALUE;
    }
    DBG_LOGB("set effect mode:%d", effect);
    ret = ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl);
    if (ret < 0)
        CAMHAL_LOGDB("Set effect fail: %s. ret=%d", strerror(errno),ret);
    return ret ;
}

int MIPISensor::getExposure(int *maxExp, int *minExp, int *def, camera_metadata_rational *step) {
       struct v4l2_queryctrl qc;
       int ret=0;
       int level = 0;
       int middle = 0;

       memset( &qc, 0, sizeof(qc));

           DBG_LOGA("getExposure\n");
       qc.id = V4L2_CID_EXPOSURE;
       ret = ioctl(mVinfo->fd, VIDIOC_QUERYCTRL, &qc);
       if (ret < 0) {
           CAMHAL_LOGDB("QUERYCTRL failed, errno=%d\n", errno);
           *minExp = -4;
           *maxExp = 4;
           *def = 0;
           step->numerator = 1;
           step->denominator = 1;
           return ret;
       }

       if (0 < qc.step)
           level = ( qc.maximum - qc.minimum + 1 )/qc.step;

       if ((level > MAX_LEVEL_FOR_EXPOSURE)
         || (level < MIN_LEVEL_FOR_EXPOSURE)) {
           *minExp = -4;
           *maxExp = 4;
           *def = 0;
           step->numerator = 1;
           step->denominator = 1;
           DBG_LOGB("not in[min,max], min=%d, max=%d, def=%d\n",
                                           *minExp, *maxExp, *def);
           return true;
       }

       middle = (qc.minimum+qc.maximum)/2;
       *minExp = qc.minimum - middle;
       *maxExp = qc.maximum - middle;
       *def = qc.default_value - middle;
       step->numerator = 1;
       step->denominator = 2;//qc.step;
           DBG_LOGB("min=%d, max=%d, step=%d\n", qc.minimum, qc.maximum, qc.step);
       return ret;
}

status_t MIPISensor::setExposure(int expCmp) {
    int ret = 0;
    struct v4l2_control ctl;
    struct v4l2_queryctrl qc;

    if (mEV == expCmp) {
        return 0;
    } else {
        mEV = expCmp;
    }
    memset(&ctl, 0, sizeof(ctl));
    memset(&qc, 0, sizeof(qc));

    qc.id = V4L2_CID_EXPOSURE;

    ret = ioctl(mVinfo->fd, VIDIOC_QUERYCTRL, &qc);
    if (ret < 0) {
        CAMHAL_LOGDB("AMLOGIC CAMERA get Exposure fail: %s. ret=%d", strerror(errno),ret);
    }

    ctl.id = V4L2_CID_EXPOSURE;
    ctl.value = expCmp + (qc.maximum - qc.minimum) / 2;

    ret = ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl);
    if (ret < 0) {
        CAMHAL_LOGDB("AMLOGIC CAMERA Set Exposure fail: %s. ret=%d", strerror(errno),ret);
    }
        DBG_LOGB("setExposure value%d mEVmin%d mEVmax%d\n",ctl.value, qc.minimum, qc.maximum);
    return ret ;
}

int MIPISensor::getAntiBanding(uint8_t *antiBanding, uint8_t maxCont) {
    struct v4l2_queryctrl qc;
    struct v4l2_querymenu qm;
    int ret;
    int mode_count = -1;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_POWER_LINE_FREQUENCY;
    ret = ioctl (mVinfo->fd, VIDIOC_QUERYCTRL, &qc);
    if ( (ret<0) || (qc.flags == V4L2_CTRL_FLAG_DISABLED)) {
        DBG_LOGB("camera handle %d can't support this ctrl",mVinfo->fd);
    } else if ( qc.type != V4L2_CTRL_TYPE_INTEGER) {
        DBG_LOGB("this ctrl of camera handle %d can't support menu type",mVinfo->fd);
    } else {
        memset(&qm, 0, sizeof(qm));

        int index = 0;
        mode_count = 1;
        antiBanding[0] = ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF;

        for (index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            if (mode_count >= maxCont)
                break;

            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_POWER_LINE_FREQUENCY;
            qm.index = index;
            if (ioctl (mVinfo->fd, VIDIOC_QUERYMENU, &qm) < 0) {
                continue;
            } else {
                if (strcmp((char*)qm.name,"50hz") == 0) {
                    antiBanding[mode_count] = ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"60hz") == 0) {
                    antiBanding[mode_count] = ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"auto") == 0) {
                    antiBanding[mode_count] = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
                    mode_count++;
                }

            }
        }
    }

    return mode_count;
}
status_t MIPISensor::setAntiBanding(uint8_t antiBanding) {
    int ret = 0;
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_POWER_LINE_FREQUENCY;

    switch (antiBanding) {
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF:
        ctl.value= CAM_ANTIBANDING_OFF;
        break;
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ:
        ctl.value= CAM_ANTIBANDING_50HZ;
        break;
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ:
        ctl.value= CAM_ANTIBANDING_60HZ;
        break;
    case ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO:
        ctl.value= CAM_ANTIBANDING_AUTO;
        break;
    default:
            ALOGE("%s: Doesn't support ANTIBANDING mode %d",
                    __FUNCTION__, antiBanding);
            return BAD_VALUE;
    }

    DBG_LOGB("anti banding mode:%d", antiBanding);
    ret = ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl);
    if ( ret < 0) {
        CAMHAL_LOGDA("failed to set anti banding mode!\n");
        return BAD_VALUE;
    }
    return ret;
}

status_t MIPISensor::setFocuasArea(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    int ret = 0;
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_FOCUS_ABSOLUTE;
    ctl.value = ((x0 + x1) / 2 + 1000) << 16;
    ctl.value |= ((y0 + y1) / 2 + 1000) & 0xffff;

    ret = ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl);
    return ret;
}

int MIPISensor::getAutoFocus(uint8_t *afMode, uint8_t maxCount) {
    struct v4l2_queryctrl qc;
    struct v4l2_querymenu qm;
    int ret;
    int mode_count = -1;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_FOCUS_AUTO;
    ret = ioctl (mVinfo->fd, VIDIOC_QUERYCTRL, &qc);
    if ( (ret<0) || (qc.flags == V4L2_CTRL_FLAG_DISABLED)) {
        DBG_LOGB("camera handle %d can't support this ctrl",mVinfo->fd);
    } else if ( qc.type != V4L2_CTRL_TYPE_MENU) {
        DBG_LOGB("this ctrl of camera handle %d can't support menu type",mVinfo->fd);
    } else {
        memset(&qm, 0, sizeof(qm));

        int index = 0;
        mode_count = 1;
        afMode[0] = ANDROID_CONTROL_AF_MODE_OFF;

        for (index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            if (mode_count >= maxCount)
                break;

            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_FOCUS_AUTO;
            qm.index = index;
            if (ioctl (mVinfo->fd, VIDIOC_QUERYMENU, &qm) < 0) {
                continue;
            } else {
                if (strcmp((char*)qm.name,"auto") == 0) {
                    afMode[mode_count] = ANDROID_CONTROL_AF_MODE_AUTO;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"continuous-video") == 0) {
                    afMode[mode_count] = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"continuous-picture") == 0) {
                    afMode[mode_count] = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
                    mode_count++;
                }

            }
        }
    }

    return mode_count;
}
status_t MIPISensor::setAutoFocuas(uint8_t afMode) {
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_FOCUS_AUTO;

    switch (afMode) {
        case ANDROID_CONTROL_AF_MODE_AUTO:
            ctl.value = CAM_FOCUS_MODE_AUTO;
            break;
        case ANDROID_CONTROL_AF_MODE_MACRO:
            ctl.value = CAM_FOCUS_MODE_MACRO;
            break;
        case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
            ctl.value = CAM_FOCUS_MODE_CONTI_VID;
            break;
        case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
            ctl.value = CAM_FOCUS_MODE_CONTI_PIC;
            break;
        default:
            ALOGE("%s: Emulator doesn't support AF mode %d",
                    __FUNCTION__, afMode);
            return BAD_VALUE;
    }

    if (ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl) < 0) {
        CAMHAL_LOGDA("failed to set camera focuas mode!\n");
        return BAD_VALUE;
    }

    return OK;
}
int MIPISensor::getAWB(uint8_t *awbMode, uint8_t maxCount) {
    struct v4l2_queryctrl qc;
    struct v4l2_querymenu qm;
    int ret;
    int mode_count = -1;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_DO_WHITE_BALANCE;
    ret = ioctl (mVinfo->fd, VIDIOC_QUERYCTRL, &qc);
    if ( (ret<0) || (qc.flags == V4L2_CTRL_FLAG_DISABLED)) {
        DBG_LOGB("camera handle %d can't support this ctrl",mVinfo->fd);
    } else if ( qc.type != V4L2_CTRL_TYPE_MENU) {
        DBG_LOGB("this ctrl of camera handle %d can't support menu type",mVinfo->fd);
    } else {
        memset(&qm, 0, sizeof(qm));

        int index = 0;
        mode_count = 1;
        awbMode[0] = ANDROID_CONTROL_AWB_MODE_OFF;

        for (index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            if (mode_count >= maxCount)
                break;

            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_DO_WHITE_BALANCE;
            qm.index = index;
            if (ioctl (mVinfo->fd, VIDIOC_QUERYMENU, &qm) < 0) {
                continue;
            } else {
                if (strcmp((char*)qm.name,"auto") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_AUTO;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"daylight") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_DAYLIGHT;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"incandescent") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_INCANDESCENT;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"fluorescent") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_FLUORESCENT;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"warm-fluorescent") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"cloudy-daylight") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"twilight") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_TWILIGHT;
                    mode_count++;
                } else if (strcmp((char*)qm.name,"shade") == 0) {
                    awbMode[mode_count] = ANDROID_CONTROL_AWB_MODE_SHADE;
                    mode_count++;
                }

            }
        }
    }

    return mode_count;
}
status_t MIPISensor::setAWB(uint8_t awbMode) {
     int ret = 0;
    struct v4l2_control ctl;
    ctl.id = V4L2_CID_DO_WHITE_BALANCE;

    switch (awbMode) {
        case ANDROID_CONTROL_AWB_MODE_AUTO:
            ctl.value = CAM_WB_AUTO;
            break;
        case ANDROID_CONTROL_AWB_MODE_INCANDESCENT:
            ctl.value = CAM_WB_INCANDESCENCE;
            break;
        case ANDROID_CONTROL_AWB_MODE_FLUORESCENT:
            ctl.value = CAM_WB_FLUORESCENT;
            break;
        case ANDROID_CONTROL_AWB_MODE_DAYLIGHT:
            ctl.value = CAM_WB_DAYLIGHT;
            break;
        case ANDROID_CONTROL_AWB_MODE_SHADE:
            ctl.value = CAM_WB_SHADE;
            break;
        default:
            ALOGE("%s: Emulator doesn't support AWB mode %d",
                    __FUNCTION__, awbMode);
            return BAD_VALUE;
    }
    ret = ioctl(mVinfo->fd, VIDIOC_S_CTRL, &ctl);
    return ret;
}
void MIPISensor::setSensorListener(SensorListener *listener) {
    Sensor::setSensorListener(listener);
}

}
