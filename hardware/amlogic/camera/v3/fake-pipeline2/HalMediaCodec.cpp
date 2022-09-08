//#define LOG_NDEBUG  0
//#define LOG_NNDEBUG 0

#define LOG_TAG "HalMediaCodec"

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

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <stagefright/MediaCodecConstants.h>
#include "HalMediaCodec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef GE2D_ENABLE
#include "fake-pipeline2/ge2d_stream.h"
#endif
#define MIN(_a,_b) ((_a) > (_b) ? (_b) : (_a))
#define TIMEOUT_US 5000
namespace android {
int HalMediaCodec::init(int width, int height,const char* name)
{
    char mime[40];
    if (strcmp(name,"h264") == 0)  {
        sprintf(mime,"%s","video/avc");
    }else if(strcmp(name,"mjpeg") == 0){
        //sprintf(mime,"%s","video/x-motion-jpeg");
        sprintf(mime,"%s","video/mjpeg");
    }
    mMediaCodec = AMediaCodec_createDecoderByType(mime);
    if (NULL == mMediaCodec)
    {
        ALOGD("%s: init mediacodec fail.",__FUNCTION__);
        return 0;
    }

    AMediaFormat *videoFormat = AMediaFormat_new();
    AMediaFormat_setString(videoFormat, "mime", mime);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT,
            COLOR_FormatYUV420SemiPlanar);
    media_status_t status = AMediaCodec_configure(mMediaCodec, videoFormat, NULL, NULL, 0);
    if (status != AMEDIA_OK)
    {
        AMediaCodec_delete(mMediaCodec);
        mMediaCodec = NULL;
        return 0;
    }

    mWidth = width;
    mHeight = height;
    mOriFrameSize = width * height;
    mFrameSize = mOriFrameSize;

    status = AMediaCodec_start(mMediaCodec);
    if (status != AMEDIA_OK)
    {
        AMediaCodec_delete(mMediaCodec);
        mMediaCodec = NULL;
        return 0;
    }
    for (int i = 0; i < 3; i++)
        mTempOutBuffer[i] = (uint8_t*)malloc(mOriFrameSize*3/2);
    mDecoderFailNumber = 0;
    return 1;
}

int HalMediaCodec::fini()
{
    if (mMediaCodec) {
        AMediaCodec_stop(mMediaCodec);
        AMediaCodec_delete(mMediaCodec);
        mMediaCodec = NULL;
    }
    for (int i = 0; i < 3; i++)
        free(mTempOutBuffer[i]);
    return 1;
}

unsigned int HalMediaCodec::timeGetTime() {
    unsigned int uptime = 0;
    struct timespec on;
    if (clock_gettime(CLOCK_MONOTONIC, &on) == 0)
        uptime = on.tv_sec * 1000 + on.tv_nsec / 1000000;
    return uptime;
}

void HalMediaCodec::NV12toNV21(uint8_t *uvbuf, int width, int height){
    uint8_t* UVBuffer;
    uint8_t  tmp;

    UVBuffer = uvbuf;
    for (int i = 0; i < width * height >> 1; i +=2) {
        tmp = *(UVBuffer + i);
        *(UVBuffer + i) = *(UVBuffer + i + 1);
        *(UVBuffer + i + 1) = tmp;

    }
    return;

}

int HalMediaCodec::decode(uint8_t*bufData, size_t bufSize, uint8_t*outBuf)
{
    if (NULL == mMediaCodec)
        return 0;
    QueueBuffer(bufData,bufSize);

    int ret = DequeueBuffer(outBuf);
    if (!ret)
        mDecoderFailNumber ++;

    if (ret && mDecoderFailNumber >= 1) {
        int r = 0;
        int i = 0;
        for (i = 0 ; i < 3; i++) {
            int value = DequeueBuffer(mTempOutBuffer[i]);
            if (value) {
                ALOGD("%s: read cached data.FailNumber=%d",__FUNCTION__,mDecoderFailNumber);
                mDecoderFailNumber -= 1;
                r = 1;
                continue;
            }
            else
                break;
        }
        if (r && ((i-1) >= 0)) {
            memcpy(outBuf,mTempOutBuffer[i-1],mOriFrameSize*3/2);
            ret = r;
        }
    }
    return ret;
}


void HalMediaCodec::QueueBuffer(uint8_t*bufData,size_t bufSize) {
    ssize_t bufidx = AMediaCodec_dequeueInputBuffer(mMediaCodec, TIMEOUT_US);
    if (bufidx >= 0) {
        size_t outsize;
        uint8_t* inputBuf = AMediaCodec_getInputBuffer(mMediaCodec, bufidx, &outsize);
        if (inputBuf != nullptr && bufSize <= outsize) {
            memcpy(inputBuf, bufData, bufSize);
            unsigned int curTime = timeGetTime();
            AMediaCodec_queueInputBuffer(mMediaCodec, bufidx, 0, bufSize,  curTime , 0);
            //ALOGD("%s: queue input buf success.",__FUNCTION__);
        }else {
            ALOGD("%s: obtained InputBuffer, but no address.",__FUNCTION__);
        }
    }
}

int HalMediaCodec::DequeueBuffer(uint8_t*outBuf) {
    AMediaCodecBufferInfo info;
    ssize_t outbufidx = AMediaCodec_dequeueOutputBuffer(mMediaCodec, &info, TIMEOUT_US);
    if (outbufidx >= 0) {
        size_t outsize;
        uint8_t* outputBuf = AMediaCodec_getOutputBuffer(mMediaCodec, outbufidx, &outsize);
        if (outputBuf != nullptr) {
            //pts = info.presentationTimeUs;
            //int32_t pts32 = (int32_t) pts;

            int width = 0, height = 0;
            int color = 0;
            AMediaFormat *format = AMediaCodec_getOutputFormat(mMediaCodec);
            if (format != NULL)
            {
                AMediaFormat_getInt32(format, "width", &width);
                AMediaFormat_getInt32(format, "height", &height);
                AMediaFormat_getInt32(format, "color-format", &color);
                AMediaFormat_delete(format);
            }else{
                AMediaCodec_releaseOutputBuffer(mMediaCodec, outbufidx, info.size != 0);
                ALOGE("%s: format null.",__FUNCTION__);
                return 0;
            }
            if (width != 0 && height != 0 && color == 21) {
                uint8_t *dst = (uint8_t *)outBuf;
                memcpy(dst, outputBuf, MIN(mOriFrameSize, mFrameSize));
                NV12toNV21(outputBuf + MIN(mOriFrameSize, mFrameSize),mWidth,mHeight);
                memcpy(dst + MIN(mOriFrameSize, mFrameSize),
                        outputBuf + MIN(mOriFrameSize, mFrameSize),
                        MIN(mOriFrameSize, mFrameSize)>>1);
                AMediaCodec_releaseOutputBuffer(mMediaCodec, outbufidx, info.size != 0);
                //ALOGD("%s: get data success.",__FUNCTION__);
                return 1;
            }else{
                AMediaCodec_releaseOutputBuffer(mMediaCodec, outbufidx, info.size != 0);
                ALOGE("%s: format unknow.",__FUNCTION__);
                return 0;
            }
        }else{
            AMediaCodec_releaseOutputBuffer(mMediaCodec, outbufidx, info.size != 0);
            ALOGE("%s: no data return.",__FUNCTION__);
            return 0;
        }
    }
    else {
        switch (outbufidx) {
            case AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED:
            {
                auto format = AMediaCodec_getOutputFormat(mMediaCodec);
                AMediaFormat_getInt32(format, "width", &mWidth);
                AMediaFormat_getInt32(format, "height", &mHeight);
                int32_t localColorFMT;
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &localColorFMT);
                /*
                mColorFormat = getTTFormatFromMC(localColorFMT);
                int32_t stride = 0;
                AMediaFormat_getInt32(format, "stride", &stride);
                if (stride == 0) {
                    stride = mWidth;
                }
                mLineSize[0] = stride;
                mFrameSize    = stride * mHeight;
                mOriFrameSize = stride * mOriHeight;
                */
                return 0;
            }
            case AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED:
                break;
            case -1:
                return 0;
            default:
                break;
        }
    }
    return 1;
}
}
