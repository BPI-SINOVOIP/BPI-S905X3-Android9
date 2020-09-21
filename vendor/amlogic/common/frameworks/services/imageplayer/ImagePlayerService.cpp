/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

/** @file ImagePlayerService.cpp
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work
 *  All Rights Reserved
 *  - The information contained herein is the confidential property
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc.
 *  @author   Tellen Yu
 *  @version  2.0
 *  @date     2015/06/18
 *  @par function description:
 *  - 1 show picture in video layer
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ImagePlayerService"

#include "utils/Log.h"
#include "ImagePlayerService.h"

#include <stdlib.h>
#include <string.h>
//#include <cutils/properties.h>
#include <utils/Errors.h>

#include <SkPixmap.h>
#include <SkRefCnt.h>
#include <SkCanvas.h>
#include <SkCodec.h>
#include <SkAndroidCodec.h>
#include <SkColorPriv.h>
#include <SkColorSpace.h>
#include <SkColorSpaceXform.h>
#include <SkHalf.h>
#include <SkMatrix44.h>
#include <SkPM4f.h>
#include <SkPM4fPriv.h>
#include <SkUnPreMultiply.h>
#include <SkData.h>
#include <SkImageEncoder.h>
#include <SkColorTable.h>
#include <SkFrontBufferedStream.h>

#include <media/IMediaHTTPService.h>
#include <media/IMediaHTTPConnection.h>
#include "media/libstagefright/include/NuCachedSource2.h"
#include "media/libstagefright/include/HTTPBase.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <binder/Binder.h>
#include <media/DataSource.h>
#include <stagefright/DataSourceFactory.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include "RGBPicture.h"

#include "ImagePlayerProcessData.h"
#include "GetInMemory.h"

#define CHECK assert
#define CHECK_EQ(a,b) CHECK((a)==(b))

#define Min(a, b) ((a) < (b) ? (a) : (b))

#define SURFACE_4K_WIDTH            3840
#define SURFACE_4K_HEIGHT           2160
#define SMALL_WIDTH               1920
#define SMALL_HEIGHT              1080

#define PICDEC_SYSFS                "/dev/picdec"
#define PICDEC_IOC_MAGIC            'P'
#define PICDEC_IOC_FRAME_RENDER     _IOW(PICDEC_IOC_MAGIC, 0x00, FrameInfo_t)
#define PICDEC_IOC_FRAME_POST       _IOW(PICDEC_IOC_MAGIC, 0x01, unsigned int)

#define VIDEO_ZOOM_SYSFS            "/sys/class/video/zoom"
#define VIDEO_INUSE                 "/sys/class/video/video_inuse"
#define VIDEO_VDEC                  "/sys/class/vdec/core"

#define CHECK_VIDEO_INUSE_RETRY_COUNT 50

#define VIDEO_LAYER_FORMAT_RGB      0
#define VIDEO_LAYER_FORMAT_RGBA     1
#define VIDEO_LAYER_FORMAT_ARGB     2

using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::android::hardware::hidl_string;

namespace android {
    static bool P2pDisplay = false;
    class DeathNotifier: public IBinder::DeathRecipient {
      public:
        DeathNotifier(sp<ImagePlayerService> imageplayerservice) {
            mImagePlayService = imageplayerservice;
        }

        void binderDied(const wp<IBinder>& who) {
            ALOGW("native image player client binder died!");
            mImagePlayService->release();
        }
      private:
        sp<ImagePlayerService> mImagePlayService;
    };
    class SkHttpStream : public SkStreamAsset {
      public:
        SkHttpStream(const char url[] = NULL,
                     const sp<IMediaHTTPService> &httpservice = NULL)
            : fURL(strdup(url)), dataSource(NULL), totalSize(0),
              isConnect(false), haveRead(0), httpsService(httpservice) {
            connect();
            totalSize = getLength();
        }

        virtual ~SkHttpStream() {
            if (dataSource != NULL) {
                dataSource.clear();
                dataSource = NULL;
                isConnect = false;
                haveRead = 0;
            }

            free(fURL);
        }
        bool seek(size_t position) {
            return false;
        }
        bool move(long offset) {
            return false;
        }
        bool connect() {
            dataSource = DataSourceFactory::CreateFromURI(httpsService, fURL);

            if (dataSource == NULL) {
                ALOGE("data source create from URI is NULL");
                isConnect = false;
                return false;
            } else {
                isConnect = true;
                return true;
            }
        }
        size_t getPosition() const {
            return 0;
        }
        bool rewind() {
            if (dataSource != NULL) {
                dataSource.clear();
                dataSource = NULL;
                isConnect = false;
                haveRead = 0;
            }

            return connect();
        }


        SkStreamAsset*  onDuplicate() const {
            return new SkHttpStream(fURL, httpsService);
        }

        SkStreamAsset* onFork() const {
            return new SkHttpStream(fURL, httpsService);
        }
        size_t read(void* buffer, size_t size) {
            ssize_t ret;

            if ((buffer == NULL) && (size == 0)) {
                totalSize = getLength();
                return totalSize;
            }

            if ((buffer == NULL) && (size > 0)) {
                haveRead += size;
                return size;
            }

            if ( totalSize <= 0 ) {
                getLength();
            }

            if (isConnect && (dataSource != NULL) && (buffer != NULL)) {
                ret = dataSource->readAt(haveRead, buffer, size);

                if ((ret <= 0) || (ret > (int)size)) {
                    return 0;
                }

                haveRead += ret;
                return ret;
            } else {
                return 0;
            }
        }

        size_t getLength() const {
            off64_t size;

            if (isConnect && (dataSource != NULL)) {
                int ret = dataSource->getSize(&size);

                if (ERROR_UNSUPPORTED == ret) {
                    return 8192;
                } else if ( size > 0 ) {
                    return (size_t)size;
                }
            }

            return 0;
        }

        //if read return 0, mean the stream is end
        virtual bool isAtEnd() const {
            return ( haveRead > 0 ) && ( haveRead == totalSize );
        }

      private:
        char *fURL;
        sp<DataSource> dataSource;
        bool isConnect;
        off64_t haveRead;
        off64_t totalSize;
        sp<IMediaHTTPService> httpsService;
    };

}  // namespace android

namespace {
    using android::SkHttpStream;

#define BYTES_TO_BUFFER 64

    static SkColorType colorTypeForScaledOutput(SkColorType colorType) {
        switch (colorType) {
            case kUnknown_SkColorType:
            case kAlpha_8_SkColorType:
                return kN32_SkColorType;

            default:
                break;
        }

        return colorType;
    }

    static bool verifyBySkCodec(SkStreamAsset *stream, SkBitmap **bitmap) {
        std::unique_ptr<SkStreamAsset> s = stream->fork();
        std::unique_ptr<SkCodec> codec(SkCodec::MakeFromStream(std::move(s)));

        if (!codec) {
            return false;
        }

        SkImageInfo imageInfo = codec->getInfo();
        auto alphaType = imageInfo.isOpaque() ? kOpaque_SkAlphaType :
                         kPremul_SkAlphaType;
        auto info = SkImageInfo::Make(imageInfo.width(), imageInfo.height(),
                                      kN32_SkColorType, alphaType);

        if (bitmap != NULL) {
            *bitmap = new SkBitmap();
        }

        (*bitmap)->setInfo(info);
        (*bitmap)->tryAllocPixels(info);
        SkCodec::Result result = codec->getPixels(info, (*bitmap)->getPixels(),
                                 (*bitmap)->rowBytes());

        switch (result) {
            case SkCodec::kSuccess:
            case SkCodec::kIncompleteInput:
                return true;

            default:
                return false;
        }
    }

    static bool isPhotoByExtenName(const char *url) {
        if (!url)
            return false;

        const char *ptr = NULL;
        ptr = strrchr(url, '.');

        if (ptr == NULL) {
            ALOGE("isPhotoByExtenName ptr is NULL!!!");
            return false;
        }

        ptr = ptr + 1;

        if ((strcasecmp(ptr, "bmp") == 0)
                || (strncasecmp(ptr, "bmp?", 4) == 0)
                || (strcasecmp(ptr, "png") == 0)
                || (strncasecmp(ptr, "png?", 4) == 0)
                || (strcasecmp(ptr, "jpg") == 0)
                || (strncasecmp(ptr, "jpg?", 4) == 0)
                || (strcasecmp(ptr, "jpeg") == 0)
                || (strncasecmp(ptr, "jpeg?", 5) == 0)
                || (strcasecmp(ptr, "mpo") == 0)
                || (strncasecmp(ptr, "mpo?", 4) == 0)
                || (strcasecmp(ptr, "gif") == 0)
                || (strncasecmp(ptr, "gif?", 4) == 0)
                || (strcasecmp(ptr, "ico") == 0)
                || (strncasecmp(ptr, "ico?", 4) == 0)
                || (strcasecmp(ptr, "wbmp") == 0)
                || (strncasecmp(ptr, "wbmp?", 5) == 0)) {
            return true;
        } else {
            return false;
        }
    }

    static bool isTiffByExtenName(const char *url) {
        if (!url)
            return false;

        const char *ptr = NULL;
        ptr = strrchr(url, '.');

        if (ptr == NULL) {
            ALOGE("isTiffByExtenName ptr is NULL!!!");
            return false;
        }

        ptr = ptr + 1;

        if ((strcasecmp(ptr, "tif") == 0)
                || (strncasecmp(ptr, "tiff", 4) == 0)) {
            return true;
        } else {
            return false;
        }
    }

    static bool isMovieByExtenName(const char *url) {
        if (!url)
            return false;

        const char *ptr = NULL;
        ptr = strrchr(url, '.');

        if (ptr == NULL) {
            ALOGE("isMovieByExtenName ptr is NULL!!!");
            return false;
        }

        ptr = ptr + 1;

        if ((strcasecmp(ptr, "gif") == 0)
                || (strncasecmp(ptr, "gif?", 4) == 0)) {
            return true;
        } else {
            return false;
        }
    }

    static bool isFdSupportedBySkImageDecoder(int fd, SkBitmap **bitmap) {
        char buf[1024];
        snprintf(buf, 1024, "/proc/self/fd/%d", fd);

        int len;
        int size = 1024;
        char *url;
        url = (char *) calloc(size, sizeof(char));

        while (1) {
            if (!url)
                return false;

            len = readlink(buf, url, size - 1);

            if (len == -1)
                break;

            if (len < (size - 1))
                break;

            size *= 2;
            url = (char*)realloc(url, size);
        }

        if (len != -1) {
            url[len] = 0;
            bool ret = isPhotoByExtenName(url);
            free(url);

            if (!ret)
                return false;
        } else {
            free(url);
            return false;
        }

        sk_sp<SkData> data(SkData::MakeFromFD(fd));

        if (data.get() == NULL) {
            return false;
        }

        SkMemoryStream *stream = new SkMemoryStream(data);

        bool ret = verifyBySkCodec(stream, bitmap);
        delete stream;
        return ret;
    }

    static bool isSupportedBySkImageDecoder(const char *uri, SkBitmap **bitmap) {
        bool ret = isPhotoByExtenName(uri);

        if (!ret)
            return false;

        if (!strncasecmp("file://", uri, 7)) {
            FILE* file = fopen(uri+7,"rb");
            if ( file == NULL ) {
                ALOGE("cannot open file %s %d\n",uri+7, errno);
                return false;
            }
            SkFILEStream stream(file);
            ret = verifyBySkCodec(&stream, bitmap);
            fclose(file);
            return ret;
        }

        if (!strncasecmp("http://", uri, 7) || !strncasecmp("https://", uri, 8)) {
            SkHttpStream httpStream(uri);
            return verifyBySkCodec(&httpStream, bitmap);
        }

        return false;
    }

    static SkBitmap* cropBitmapRect(SkBitmap *srcBitmap, int x, int y, int width,
                                    int height) {
        SkBitmap *dstBitmap = NULL;
        dstBitmap = new SkBitmap();
        SkIRect r;

        r.set(x, y, x + width, y + height);
        //srcBitmap->setIsOpaque(true);
        srcBitmap->setIsVolatile(true);

        bool ret = srcBitmap->extractSubset(dstBitmap, r);
        //srcBitmap->setIsOpaque(false);
        srcBitmap->setIsVolatile(false);

        if (!ret) {
            if (dstBitmap != NULL) {
                if (!dstBitmap->isNull()) {
                    dstBitmap->reset();
                    delete dstBitmap;
                    dstBitmap = NULL;
                }else {
                    dstBitmap=NULL;
                }
            }
            return NULL;
        }

        return dstBitmap;
    }

    static SkBitmap* cropAndFillBitmap(SkBitmap *srcBitmap, int dstWidth,
                                       int dstHeight) {
        if (srcBitmap == NULL)
            return NULL;

        SkBitmap *devBitmap = new SkBitmap();
        SkCanvas *canvas = NULL;

        SkColorType colorType = colorTypeForScaledOutput(srcBitmap->colorType());
        devBitmap->setInfo(SkImageInfo::Make(dstWidth, dstHeight,
                                             colorType, srcBitmap->alphaType()));

        devBitmap->allocPixels();
        devBitmap->eraseARGB(0, 0, 0, 0);

        canvas = new SkCanvas(*devBitmap);

        int minWidth = Min(srcBitmap->width(), dstWidth);
        int minHeight = Min(srcBitmap->height(), dstHeight);
        int srcx = (srcBitmap->width() - minWidth) / 2;
        int srcy = (srcBitmap->height() - minHeight) / 2;
        int dstx = (dstWidth - minWidth) / 2;
        int dsty = (dstHeight - minHeight) / 2;

        SkPaint paint;
        //paint.setFilterBitmap(true);
        SkRect dst = SkRect::MakeXYWH(dstx, dsty, minWidth, minHeight);

        SkRect src = SkRect::MakeXYWH(srcx, srcy, minWidth, minHeight);
#if ANDROID_PLATFORM_SDK_VERSION >= 24 //Nougat
        canvas->drawBitmapRect(*srcBitmap, src, dst, &paint);
#else
        canvas->drawBitmapRectToRect(*srcBitmap, &src, dst, &paint);
#endif

        delete canvas;

        return devBitmap;
    }

    static SkBitmap* translateAndCropAndFillBitmap(SkBitmap *srcBitmap,
            int dstWidth, int dstHeight, int tx, int ty) {
        if (srcBitmap == NULL)
            return NULL;

        SkBitmap *devBitmap = new SkBitmap();
        SkCanvas *canvas = NULL;
        SkColorType colorType = colorTypeForScaledOutput(srcBitmap->colorType());
        devBitmap->setInfo(SkImageInfo::Make(dstWidth, dstHeight,
                                             colorType, srcBitmap->alphaType()));
        devBitmap->allocPixels();
        devBitmap->eraseARGB(0, 0, 0, 0);

        canvas = new SkCanvas(*devBitmap);

        int minWidth = Min(srcBitmap->width(), dstWidth);
        int minHeight = Min(srcBitmap->height(), dstHeight);
        int srcx = (srcBitmap->width() - minWidth) / 2;
        int srcy = (srcBitmap->height() - minHeight) / 2;
        int dstx = (dstWidth - minWidth) / 2;
        int dsty = (dstHeight - minHeight) / 2;

        ALOGD("translateAndCropAndFillBitmap, minWidth: %d, minHeight: %d, srcx:%d, srcy:%d, dstx:%d, dsty:%d",
              minWidth, minHeight, srcx, srcy, dstx, dsty);

        int aftertranslatesrcx = srcx + tx;
        int aftertranslatesrcy = srcy + ty;

        if (tx > 0 && srcx < tx) {
            aftertranslatesrcx = srcx * 2;
        } else if (tx < 0 && srcx < (0 - tx)) {
            aftertranslatesrcx = 0;
        }

        if (ty > 0 && srcy < ty) {
            aftertranslatesrcy = srcy * 2;
        } else if (ty < 0 && srcy < (0 - ty)) {
            aftertranslatesrcy = 0;
        }

        ALOGD("translateAndCropAndFillBitmap, after translate minWidth: %d, minHeight: %d, aftertranslatesrcx:%d, aftertranslatesrcy:%d",
              minWidth, minHeight, aftertranslatesrcx, aftertranslatesrcy);

        SkPaint paint;
        //paint.setFilterBitmap(true);
        SkRect dst = SkRect::MakeXYWH(dstx, dsty, minWidth, minHeight);

        SkIRect src = SkIRect::MakeXYWH(aftertranslatesrcx, aftertranslatesrcy,
                                        minWidth, minHeight);
#if ANDROID_PLATFORM_SDK_VERSION >= 24 //Nougat
        canvas->drawBitmapRect(*srcBitmap, src, dst, &paint);
#else
        canvas->drawBitmapRect(*srcBitmap, &src, dst, &paint);
#endif
        delete canvas;

        return devBitmap;
    }

    static __inline int RGBToY(uint8_t r, uint8_t g, uint8_t b) {
        return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
    }
    static __inline int RGBToU(uint8_t r, uint8_t g, uint8_t b) {
        return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
    }
    static __inline int RGBToV(uint8_t r, uint8_t g, uint8_t b) {
        return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
    }

    static __inline void ARGBToYUV422Row_C(const uint8_t* src_argb,
                                           uint8_t* dst_yuyv, int width) {
        for (int x = 0; x < width - 1; x += 2) {
            uint8_t ar = (src_argb[0] + src_argb[4]) >> 1;
            uint8_t ag = (src_argb[1] + src_argb[5]) >> 1;
            uint8_t ab = (src_argb[2] + src_argb[6]) >> 1;
            dst_yuyv[0] = RGBToY(src_argb[2], src_argb[1], src_argb[0]);
            dst_yuyv[1] = RGBToU(ar, ag, ab);
            dst_yuyv[2] = RGBToY(src_argb[6], src_argb[5], src_argb[4]);
            dst_yuyv[3] = RGBToV(ar, ag, ab);
            src_argb += 8;
            dst_yuyv += 4;
        }

        if (width & 1) {
            dst_yuyv[0] = RGBToY(src_argb[2], src_argb[1], src_argb[0]);
            dst_yuyv[1] = RGBToU(src_argb[2], src_argb[1], src_argb[0]);
            dst_yuyv[2] = 0x00;     // garbage, needs crop
            dst_yuyv[3] = RGBToV(src_argb[2], src_argb[1], src_argb[0]);
        }
    }

    static __inline void RGB565ToYUVRow_C(const uint8_t* src_rgb565,
                                          uint8_t* dst_yuyv, int width) {
        const uint8_t* next_rgb565 = src_rgb565 + width * 2;

        for (int x = 0; x < width - 1; x += 2) {
            uint8_t b0 = src_rgb565[0] & 0x1f;
            uint8_t g0 = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
            uint8_t r0 = src_rgb565[1] >> 3;
            uint8_t b1 = src_rgb565[2] & 0x1f;
            uint8_t g1 = (src_rgb565[2] >> 5) | ((src_rgb565[3] & 0x07) << 3);
            uint8_t r1 = src_rgb565[3] >> 3;
            uint8_t b2 = next_rgb565[0] & 0x1f;
            uint8_t g2 = (next_rgb565[0] >> 5) | ((next_rgb565[1] & 0x07) << 3);
            uint8_t r2 = next_rgb565[1] >> 3;
            uint8_t b3 = next_rgb565[2] & 0x1f;
            uint8_t g3 = (next_rgb565[2] >> 5) | ((next_rgb565[3] & 0x07) << 3);
            uint8_t r3 = next_rgb565[3] >> 3;
            uint8_t b = (b0 + b1 + b2 + b3);  // 565 * 4 = 787.
            uint8_t g = (g0 + g1 + g2 + g3);
            uint8_t r = (r0 + r1 + r2 + r3);
            b = (b << 1) | (b >> 6);  // 787 -> 888.
            r = (r << 1) | (r >> 6);
            dst_yuyv[0] = RGBToY(r, g, b);
            dst_yuyv[1] = RGBToV(r, g, b);
            dst_yuyv[2] = RGBToY(r, g, b);
            dst_yuyv[3] = RGBToU(r, g, b);
            src_rgb565 += 4;
            next_rgb565 += 4;
            dst_yuyv += 4;
        }

        if (width & 1) {
            uint8_t b0 = src_rgb565[0] & 0x1f;
            uint8_t g0 = (src_rgb565[0] >> 5) | ((src_rgb565[1] & 0x07) << 3);
            uint8_t r0 = src_rgb565[1] >> 3;
            uint8_t b2 = next_rgb565[0] & 0x1f;
            uint8_t g2 = (next_rgb565[0] >> 5) | ((next_rgb565[1] & 0x07) << 3);
            uint8_t r2 = next_rgb565[1] >> 3;
            uint8_t b = (b0 + b2);  // 565 * 2 = 676.
            uint8_t g = (g0 + g2);
            uint8_t r = (r0 + r2);
            b = (b << 2) | (b >> 4);  // 676 -> 888
            g = (g << 1) | (g >> 6);
            r = (r << 2) | (r >> 4);
            dst_yuyv[0] = RGBToY(r, g, b);
            dst_yuyv[1] = RGBToV(r, g, b);
            dst_yuyv[2] = 0x00; // garbage, needs crop
            dst_yuyv[3] = RGBToU(r, g, b);
        }
    }

    static __inline void Index8ToYUV422Row_C(const uint8_t* src_argb,
            uint8_t* dst_yuyv, int width, SkColorTable* table) {
        uint8_t ar = 0;
        uint8_t ag = 0;
        uint8_t ab = 0;
        SkPMColor pre = 0;
        SkPMColor late = 0;

        for (int x = 0; x < width - 1; x += 2) {
            pre = (*table)[src_argb[0]];
            late = (*table)[src_argb[1]];

            ar = (SkGetPackedR32(pre)  + SkGetPackedR32(late)) >> 1;
            ag = (SkGetPackedG32(pre) + SkGetPackedG32(late)) >> 1;
            ab = (SkGetPackedB32(pre) + SkGetPackedB32(late)) >> 1;

            dst_yuyv[0] = RGBToY(SkGetPackedB32(pre), SkGetPackedG32(pre),
                                 SkGetPackedR32(pre));
            dst_yuyv[1] = RGBToU(ar, ag, ab);
            dst_yuyv[2] = RGBToY(SkGetPackedB32(late), SkGetPackedG32(late),
                                 SkGetPackedR32(late));
            dst_yuyv[3] = RGBToV(ar, ag, ab);
            src_argb += 2;
            dst_yuyv += 4;
        }

        if (width & 1) {
            pre = (*table)[src_argb[0]];
            dst_yuyv[0] = RGBToY(SkGetPackedB32(pre) , SkGetPackedG32(pre),
                                 SkGetPackedR32(pre));
            dst_yuyv[1] = RGBToU(SkGetPackedB32(pre) , SkGetPackedG32(pre),
                                 SkGetPackedR32(pre));
            dst_yuyv[2] = 0x00;     // garbage, needs crop
            dst_yuyv[3] = RGBToV(SkGetPackedB32(pre) , SkGetPackedG32(pre),
                                 SkGetPackedR32(pre));
        }
    }

    //////////////////// ToColor procs

    typedef void (*ToColorProc)(SkColor dst[], const void* src, int width);

    static void ToColor_F16_Alpha(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        uint64_t* s = (uint64_t*)src;

        do {
            *dst++ = SkPM4f::FromF16((const uint16_t*) s++).unpremul().toSkColor();
        } while (--width != 0);
    }

    static void ToColor_F16_Raw(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        uint64_t* s = (uint64_t*)src;

        do {
            *dst++ = Sk4f_toS32(swizzle_rb(SkHalfToFloat_finite_ftz(*s++)));
        } while (--width != 0);
    }

    static void ToColor_S32_Alpha(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const SkPMColor* s = (const SkPMColor*)src;

        do {
            *dst++ = SkUnPreMultiply::PMColorToColor(*s++);
        } while (--width != 0);
    }

    static void ToColor_S32_Raw(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const SkPMColor* s = (const SkPMColor*)src;

        do {
            SkPMColor c = *s++;
            *dst++ = SkColorSetARGB(SkGetPackedA32(c), SkGetPackedR32(c),
                                    SkGetPackedG32(c), SkGetPackedB32(c));
        } while (--width != 0);
    }

    static void ToColor_S32_Opaque(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const SkPMColor* s = (const SkPMColor*)src;

        do {
            SkPMColor c = *s++;
            *dst++ = SkColorSetRGB(SkGetPackedR32(c), SkGetPackedG32(c),
                                   SkGetPackedB32(c));
        } while (--width != 0);
    }

    static void ToColor_S4444_Alpha(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const SkPMColor16* s = (const SkPMColor16*)src;

        do {
            *dst++ = SkUnPreMultiply::PMColorToColor(SkPixel4444ToPixel32(*s++));
        } while (--width != 0);
    }

    static void ToColor_S4444_Raw(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const SkPMColor16* s = (const SkPMColor16*)src;

        do {
            SkPMColor c = SkPixel4444ToPixel32(*s++);
            *dst++ = SkColorSetARGB(SkGetPackedA32(c), SkGetPackedR32(c),
                                    SkGetPackedG32(c), SkGetPackedB32(c));
        } while (--width != 0);
    }

    static void ToColor_S4444_Opaque(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const SkPMColor16* s = (const SkPMColor16*)src;

        do {
            SkPMColor c = SkPixel4444ToPixel32(*s++);
            *dst++ = SkColorSetRGB(SkGetPackedR32(c), SkGetPackedG32(c),
                                   SkGetPackedB32(c));
        } while (--width != 0);
    }

    static void ToColor_S565(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const uint16_t* s = (const uint16_t*)src;

        do {
            uint16_t c = *s++;
            *dst++ =  SkColorSetRGB(SkPacked16ToR32(c), SkPacked16ToG32(c),
                                    SkPacked16ToB32(c));
        } while (--width != 0);
    }

    static void ToColor_SA8(SkColor dst[], const void* src, int width) {
        SkASSERT(width > 0);
        const uint8_t* s = (const uint8_t*)src;

        do {
            uint8_t c = *s++;
            *dst++ = SkColorSetARGB(c, 0, 0, 0);
        } while (--width != 0);
    }

    // can return NULL
    static ToColorProc ChooseToColorProc(const SkBitmap& src) {
        switch (src.colorType()) {
            case kN32_SkColorType:
                switch (src.alphaType()) {
                    case kOpaque_SkAlphaType:
                        return ToColor_S32_Opaque;

                    case kPremul_SkAlphaType:
                        return ToColor_S32_Alpha;

                    case kUnpremul_SkAlphaType:
                        return ToColor_S32_Raw;

                    default:
                        return NULL;
                }

            case kARGB_4444_SkColorType:
                switch (src.alphaType()) {
                    case kOpaque_SkAlphaType:
                        return ToColor_S4444_Opaque;

                    case kPremul_SkAlphaType:
                        return ToColor_S4444_Alpha;

                    case kUnpremul_SkAlphaType:
                        return ToColor_S4444_Raw;

                    default:
                        return NULL;
                }

            case kRGB_565_SkColorType:
                return ToColor_S565;

            case kAlpha_8_SkColorType:
                return ToColor_SA8;

            case kRGBA_F16_SkColorType:
                switch (src.alphaType()) {
                    case kOpaque_SkAlphaType:
                        return ToColor_F16_Raw;

                    case kPremul_SkAlphaType:
                        return ToColor_F16_Alpha;

                    case kUnpremul_SkAlphaType:
                        return ToColor_F16_Raw;

                    default:
                        return NULL;
                }

            default:
                break;
        }

        return NULL;
    }

    static void ToF16_SA8(void* dst, const void* src, int width) {
        SkASSERT(width > 0);
        uint64_t* d = (uint64_t*)dst;
        const uint8_t* s = (const uint8_t*)src;

        for (int i = 0; i < width; i++) {
            uint8_t c = *s++;
            SkPM4f a;
            a.fVec[SkPM4f::R] = 0.0f;
            a.fVec[SkPM4f::G] = 0.0f;
            a.fVec[SkPM4f::B] = 0.0f;
            a.fVec[SkPM4f::A] = c / 255.0f;
            *d++ = a.toF16();
        }
    }

    static void chmodSysfs(const char *sysfs, int mode) {
        char sysCmd[1024];
        sprintf(sysCmd, "chmod %d %s", mode, sysfs);

        if (system(sysCmd)) {
            ALOGE("exec cmd:%s fail\n", sysCmd);
        }
    }

    static int setSysfs(const char *path, const char *val) {
        int bytes;
        int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);

        if (fd >= 0) {
            bytes = write(fd, val, strlen(val));
            ALOGI("set sysfs %s = %s\n", path, val);
            close(fd);
            return 0;
        } else {
        }

        return -1;
    }

}  // anonymous namespace

namespace android {
    ImagePlayerService* ImagePlayerService::instantiate() {
        ImagePlayerService *mImagePlayer = new ImagePlayerService();
        android::status_t ret = defaultServiceManager()->addService(
                                    String16("image.player"), mImagePlayer);

        if (ret != android::OK) {
            ALOGE("Couldn't register image.player service!");
            return NULL;
        }

        ALOGI("instantiate add service result:%d", ret);

        //chmodSysfs(PICDEC_SYSFS, 644);
        return mImagePlayer;
    }

    ImagePlayerService::ImagePlayerService()
        : mWidth(0), mHeight(0), mBitmap(NULL), mBufBitmap(NULL),
          mSampleSize(1), mFileDescription(-1), mFrameIndex(0), mTif(NULL),
          surfaceWidth(SURFACE_4K_WIDTH), surfaceHeight(SURFACE_4K_HEIGHT),
          mScalingDirect(SCALE_NORMAL), mScalingStep(1.0f), mScalingBitmap(NULL),
          mRotateBitmap(NULL), mMovieImage(false), mMovieThread(NULL),
          mNeedResetHWScale(false),
          mTranslatingDirect(TRANSLATE_NORMAL), mTranslateImage(false), mTx(0.0f),
          mTy(0.0f),
          mTranslateToXLEdge(false), mTranslateToXREdge(false), mTranslateToYTEdge(false),
          mTranslateToYBEdge(false),
          mMovieDegree(0), mMovieScale(1.0f), mParameter(NULL), mDisplayFd(-1),
          mSysWrite(NULL) {
        mSysWrite = new SysWrite();
    }

    ImagePlayerService::~ImagePlayerService() {
        if (!mSysWrite) {
            delete mSysWrite;
            mSysWrite = NULL;
        }
    }

    void ImagePlayerService::initVideoAxis() {
        if (mSysWrite != NULL) {
            mSysWrite->writeSysfs("/sys/class/video/axis", "0 0  0 0");
        } else {
            ALOGE("Couldn't get connection to system control\n");
        }

        /*
        int ret = setSysfs("/sys/class/vfm/map", "rm default");
        if (ret == -1) {
            ALOGW("enable osd video rm default failed");
            ret = setSysfs("/sys/class/vfm/map", "rm default");
        }
        ret = setSysfs("/sys/class/vfm/map", "add default decoder ppmgr deinterlace amvideo");
        */
    }

    int ImagePlayerService::init() {
        mParameter = new InitParameter();
        mParameter->degrees = 0.0f;
        mParameter->scaleX = 1.0f;
        mParameter->scaleY = 1.0f;
        mParameter->cropX = 0;
        mParameter->cropY = 0;
        mParameter->cropWidth = SURFACE_4K_WIDTH;
        mParameter->cropHeight = SURFACE_4K_HEIGHT;

        mMovieThread = new MovieThread(this);
        mDeathNotifier = new DeathNotifier(this);

        //if video exit with some exception, need restore video attribute
        initVideoAxis();

        if (mDisplayFd >= 0) {
            close(mDisplayFd);
        }

        /*if (!checkVideoInUse(CHECK_VIDEO_INUSE_RETRY_COUNT)) {
            return RET_ERR_BAD_VALUE;
        }*/

        mDisplayFd = open(PICDEC_SYSFS, O_RDWR);

        if (mDisplayFd < 0) {
            ALOGE("init: mDisplayFd(%d) failure error: '%s' (%d)", mDisplayFd,
                  strerror(errno), errno);
            return RET_ERR_OPEN_SYSFS;
        }
        char value[32];
        mSysWrite->setProperty("persist.sys.is.image.player", "1");
        mSysWrite->getProperty("persist.sys.is.image.player",value);
        ALOGI("init, persist.sys.is.image.player:%s",value);
        /*mSysWrite->writeSysfs("/sys/class/amvecm/pc_mode", "0");*/
        mSysWrite->writeSysfs("sys/module/di/parameters/nr2_en", "0");
        mSysWrite->writeSysfs("/sys/class/amvecm/lc", "lc disable");
        mSysWrite->writeSysfs("/sys/class/amvecm/pq_user_set", "blk_ext_di");
        P2pDisplay = mSysWrite->writeSysfs("/sys/module/picdec/parameters/p2p_mode","1");
        ALOGD("write p2p_mode %d",P2pDisplay);

#if 0   //workround: need post a frame to video layer
        FrameInfo_t info;

        char* bitmap_addr = (char*)malloc(100 * 100 * 3);
        memset(bitmap_addr, 0, 100 * 100 * 3);
        info.pBuff = bitmap_addr;
        info.frame_width = 100;
        info.frame_height = 100;
        info.format = VIDEO_LAYER_FORMAT_RGB;
        info.rotate = 0;

        copy_data_to_mmap_buf(mDisplayFd, bitmap_addr,
                             info.frame_width, info.frame_height,
                             info.format);

        ioctl(mDisplayFd, PICDEC_IOC_FRAME_RENDER, &info);
        ioctl(mDisplayFd, PICDEC_IOC_FRAME_POST, NULL);

        free(bitmap_addr);
#endif

        ALOGI("init success display fd:%d", mDisplayFd);

        return RET_OK;
    }

    int ImagePlayerService::setDataSource (const sp<IMediaHTTPService> &httpService,
                                           const char *srcUrl) {
        ALOGI("setDataSource URL uri:%s", srcUrl);

        if (httpService == NULL) {
            ALOGE("setDataSource httpService is NULL");
            return RET_ERR_PARAMETER;
        }

        mHttpService = httpService;
        setDataSource(srcUrl);
        return RET_OK;
    }

    int ImagePlayerService::notifyProcessDied (const sp<IBinder> &binder) {
        ALOGI("notifyProcessDied");

        if (binder == NULL) {
            ALOGE("notifyProcessDied binder is NULL");
            return RET_ERR_PARAMETER;
        }

        binder->linkToDeath(mDeathNotifier);
        return RET_OK;
    }

    int ImagePlayerService::setDataSource(const char *uri) {
        Mutex::Autolock autoLock(mLock);

        ALOGI("setDataSource uri:%s", uri);

        if (mBitmap != NULL) {
            clearBmp(mBitmap);
            mBitmap = NULL;
        }

        if (!strncasecmp("file://", uri, 7)) {
            strncpy(mImageUrl, uri + 7, MAX_FILE_PATH_LEN - 1);
        } else if (!strncasecmp("http://", uri, 7)
                   || !strncasecmp("https://", uri, 8)) {
            strncpy(mImageUrl, uri, MAX_FILE_PATH_LEN - 1);
        } else {
            ALOGE("setDataSource error uri:%s", uri);
            return RET_ERR_INVALID_OPERATION;
        }
        MovieThreadStop();
        if (isMovieByExtenName(mImageUrl)) {
            mMovieImage = true;
        }else {
            mMovieImage = false;
        }
        ALOGI("setDataSource end uri:%s", uri);
        return RET_OK;
    }

    int ImagePlayerService::setDataSource(int fd, int64_t offset, int64_t length) {
        Mutex::Autolock autoLock(mLock);

        ALOGI("setDataSource fd:%d, offset:%d, length:%d", fd, (int)offset,
              (int)length);

        if (mBitmap != NULL) {
            clearBmp(mBitmap);
            mBitmap = NULL;
        }

        if (mFileDescription >= 0) {
            close(mFileDescription);
            mFileDescription = -1;
        }

        mFileDescription = dup(fd);

        if (!isFdSupportedBySkImageDecoder(fd, &mBitmap)) {
            return RET_ERR_INVALID_OPERATION;
        }

        if (mBitmap != NULL) {
            mWidth = mBitmap->width();
            mHeight = mBitmap->height();
            clearBmp(mBitmap);
            mBitmap = NULL;
        }

        return RET_OK;
    }

    int ImagePlayerService::setSampleSurfaceSize(int sampleSize, int surfaceW,
            int surfaceH) {
        mSampleSize = sampleSize;
        surfaceWidth = surfaceW;
        surfaceHeight = surfaceH;

        if (surfaceW > SURFACE_4K_WIDTH) {
            surfaceWidth = SURFACE_4K_WIDTH;
        }

        if (surfaceH > SURFACE_4K_HEIGHT) {
            surfaceHeight = SURFACE_4K_HEIGHT;
        }

        ALOGD("setSampleSurfaceSize sampleSize:%d, surfaceW:%d, surfaceH:%d",
              sampleSize, surfaceW, surfaceH);

        return RET_OK;
    }

    int ImagePlayerService::setRotate(float degrees, int autoCrop) {
        Mutex::Autolock autoLock(mLock);

        bool isAutoCrop = autoCrop != 0;
        ALOGD("called setRotate degrees:%f, isAutoCrop:%d", degrees, isAutoCrop);

        //ratate always use the origin bitmap
        //reset rotate and scale, because rotate is the always first state
        resetRotateScale();

        resetTranslate();

        if (mMovieImage) {
            //reset scale
            mMovieScale = 1.0f;
            mMovieDegree = degrees;
            mNeedResetHWScale = true;
            return RET_OK;
        }
        if (degrees == 0) {
             if (mRotateBitmap != NULL) {
                mRotateBitmap->reset();
                mRotateBitmap = NULL;
            }
            renderAndShow(mBitmap);
            return RET_OK;
        }
        SkBitmap *dstBitmap = NULL;
        ALOGD("preCropAndRotate %d %d %d",(int)degrees, mBitmap->width(),mBitmap->height());
        if (((int)degrees + 360) % 180 == 90
                && ( mBitmap->width() > surfaceHeight ||  mBitmap->height() > surfaceWidth)) {
            ALOGD("preCropAndRotate");
            dstBitmap = preCropAndRotate(mBitmap,degrees);
        }else {
            dstBitmap = rotate(mBitmap, degrees);
            if (dstBitmap != NULL) {
            if (isAutoCrop && needFillSurface(mBitmap, 1, 1)) {
                    SkBitmap *fillBitmap = fillSurface(dstBitmap);

                    if (fillBitmap != NULL) {
                        clearBmp(dstBitmap);
                        dstBitmap = fillBitmap;
                    }
                }
            }
        }

        ALOGD("After rotate, Width: %d, Height: %d", dstBitmap->width(),
              dstBitmap->height());
        if (dstBitmap != NULL) {
            renderAndShow(dstBitmap);
            clearBmp(dstBitmap);
            return RET_OK;
        }

        return RET_ERR_DECORDER;
    }

    int ImagePlayerService::setScale(float sx, float sy, int autoCrop) {

        Mutex::Autolock autoLock(mLock);

        resetTranslate();
        resetRotateScale();

        bool isAutoCrop = autoCrop != 0;
        ALOGD("setScale sx:%f, sy:%f, isAutoCrop:%d", sx, sy, isAutoCrop);

        if ((sx > 16.0f) || (sy > 16.0f)) {
            ALOGE("setScale max x scale up or y scale up is 16");
            return RET_ERR_INVALID_OPERATION;
        }

        if (mMovieImage) {
            mMovieScale = sx;
            ALOGE("///////////////%f",mMovieScale);
            return RET_OK;
        }
        if (sx == 1.0 && sy == 1.0f ) {
            renderAndShow(mBitmap);
            return RET_OK;
        }
        SkBitmap *dstBitmap = NULL;
        if (sx != sy) {
            ALOGW("scale x and y not the same");

            SkBitmap *devbmp = NULL;
            if (needFillSurface(mBitmap, 1, 1)) {
                float originalZoomPersent = ( surfaceWidth*1.0/mBitmap->width()) > (surfaceHeight*1.0/mBitmap->height())?
                                           (surfaceHeight*1.0/mBitmap->height()):( surfaceWidth*1.0/mBitmap->width());
                sx  = originalZoomPersent*sx;
                sy = originalZoomPersent*sy;
            }
            dstBitmap = scale(mBitmap, sx, sy);

            if (dstBitmap != NULL) {
                if (isAutoCrop && needFillSurface(mBitmap, sx, sy)) {
                    SkBitmap *fillBitmap = fillSurface(dstBitmap);

                    if (fillBitmap != NULL) {
                        clearBmp(dstBitmap);
                        dstBitmap = fillBitmap;
                    }
                }

                ALOGD("After scale, Width: %d, Height: %d", dstBitmap->width(),
                      dstBitmap->height());
                renderAndShow(dstBitmap);
                clearBmp(dstBitmap);
                dstBitmap = NULL;
            } else {
                return RET_ERR_DECORDER;
            }
        } else {
           // ALOGD("setScale, cucale, current direction:%d [0:normal, 1:up, 2:down], current step: %f",
           //       mScalingDirect, mScalingStep);
            clearBmp(mScalingBitmap);
            mScalingBitmap = NULL;

            if (needFillSurface(mBitmap, 1, 1)) {
                float zoompercent = ( surfaceWidth*1.0/mBitmap->width()) > (surfaceHeight*1.0/mBitmap->height())?
                                           (surfaceHeight*1.0/mBitmap->height()):( surfaceWidth*1.0/mBitmap->width());
                sx = sx*zoompercent;
                sy = sx*zoompercent;
            }
            dstBitmap = scaleAndCrop(mBitmap, sx, sy);
            if (dstBitmap != NULL) {
                renderAndShow(dstBitmap);
                clearBmp(dstBitmap);
                dstBitmap = NULL;
            }
        }

        return RET_OK;
    }

    int ImagePlayerService::setHWScale(float sc) {
        Mutex::Autolock autoLock(mLock);

        resetTranslate();

        ALOGD("setHWScale sc:%f", sc);

        int zoomValue = 100;

        if (mSysWrite != NULL) {
            char* videoZoomValue;
            mSysWrite->readSysfs(VIDEO_ZOOM_SYSFS, videoZoomValue);
            zoomValue = atoi(videoZoomValue);
            ALOGD("setHWScale zoomValue:%d", zoomValue);
        } else {
            ALOGE("Couldn't get connection to system control\n");
            return RET_ERR_INVALID_OPERATION;
        }

        int scZoomValue = sc * zoomValue;
        ALOGD("setHWScale sc_zoom_value:%d", scZoomValue);

        if (scZoomValue > 300) {
            ALOGE("setHWScale max scale up is 300");
            mSysWrite->writeSysfs(VIDEO_ZOOM_SYSFS, "300");
            return RET_OK_OPERATION_SCALE_MAX;
        }

        if (scZoomValue < 26) {
            ALOGE("setHWScale min scale down is 26");
            mSysWrite->writeSysfs(VIDEO_ZOOM_SYSFS, "26");
            return RET_OK_OPERATION_SCALE_MIN;
        }

        char scValue[32];
        memset(scValue, '\0', 32);
        sprintf(scValue, "%d", scZoomValue);
        mSysWrite->writeSysfs(VIDEO_ZOOM_SYSFS, scValue);

        return RET_OK;
    }

    bool ImagePlayerService::needFillSurface(SkBitmap *bitmap, float sx,float sy) {
        if (P2pDisplay == 0) return true;
        if (bitmap == NULL) return false;
        int srcWidth = bitmap->width();
        int srcHeight = bitmap->height();
        //ALOGD("needFillSurface src Bitmap size scale to:%d x %d->%f x %f",
        //            srcWidth, srcHeight, srcWidth*sx, srcHeight*sy);
        if ( srcWidth >= SMALL_WIDTH || srcHeight >= SMALL_HEIGHT) {
            return true;
        } else if (srcWidth*sx > surfaceWidth ||srcHeight*sy > surfaceHeight) {
            return true;
        }
        return false;
    }
    int ImagePlayerService::setTranslate(float tx, float ty) {
        Mutex::Autolock autoLock(mLock);

        ALOGD("setTranslate tx:%f, ty:%f", tx, ty);

        if (mScalingBitmap == NULL)
            return RET_ERR_INVALID_OPERATION;

        if (((mScalingBitmap->width() < surfaceWidth) && (ty == 0))
                || ((tx == 0) && (mScalingBitmap->height() < surfaceHeight))
                || ((mScalingBitmap->width() < surfaceWidth)
                    && (mScalingBitmap->height() < surfaceHeight))) {
            return RET_ERR_INVALID_OPERATION;
        }

        if (mMovieImage) {
            return RET_OK;
        }

        mTranslateImage = true;

        int preTranslatingDirect = mTranslatingDirect;

        if (tx > 0 && ty == 0) {
            mTranslatingDirect = TRANSLATE_RIGHT;
        } else if (tx < 0 && ty == 0) {
            mTranslatingDirect = TRANSLATE_LEFT;
        } else if (ty > 0 && tx == 0) {
            mTranslatingDirect = TRANSLATE_DOWN;
        } else if (ty < 0 && tx == 0) {
            mTranslatingDirect = TRANSLATE_UP;
        } else if (tx < 0 && ty < 0) {
            mTranslatingDirect = TRANSLATE_LEFTUP;
        } else if (tx < 0 && ty > 0) {
            mTranslatingDirect = TRANSLATE_LEFTDOWN;
        } else if (tx > 0 && ty < 0) {
            mTranslatingDirect = TRANSLATE_RIGHTUP;
        } else if (tx > 0 && ty > 0) {
            mTranslatingDirect = TRANSLATE_RIGHTDOWN;
        }

        if (preTranslatingDirect == TRANSLATE_NORMAL) {
            mTx = tx;
            mTy = ty;
            mTranslateToXLEdge = false;
            mTranslateToXREdge = false;
            mTranslateToYTEdge = false;
            mTranslateToYBEdge = false;
        } else if (mTranslatingDirect == TRANSLATE_LEFT) {
            if (mTranslateToXLEdge) {
                return RET_OK;
            }

            mTx = mTx + tx;
        } else if (mTranslatingDirect == TRANSLATE_RIGHT) {
            if (mTranslateToXREdge) {
                return RET_OK;
            }

            mTx = mTx + tx;
        } else if (mTranslatingDirect == TRANSLATE_UP) {
            if (mTranslateToYTEdge) {
                return RET_OK;
            }

            mTy = mTy + ty;
        } else if (mTranslatingDirect == TRANSLATE_DOWN) {
            if (mTranslateToYBEdge) {
                return RET_OK;
            }

            mTy = mTy + ty;
        } else if (mTranslatingDirect == TRANSLATE_LEFTUP) {
            if (mTranslateToXLEdge && mTranslateToYTEdge) {
                return RET_OK;
            } else if (mTranslateToXLEdge && !mTranslateToYTEdge) {
                mTy = mTy + ty;
            } else if (!mTranslateToXLEdge && mTranslateToYTEdge) {
                mTx = mTx + tx;
            } else {
                mTx = mTx + tx;
                mTy = mTy + ty;
            }
        } else if (mTranslatingDirect == TRANSLATE_LEFTDOWN) {
            if (mTranslateToXLEdge && mTranslateToYBEdge) {
                return RET_OK;
            } else if (mTranslateToXLEdge && !mTranslateToYBEdge) {
                mTy = mTy + ty;
            } else if (!mTranslateToXLEdge && mTranslateToYBEdge) {
                mTx = mTx + tx;
            } else {
                mTx = mTx + tx;
                mTy = mTy + ty;
            }
        } else if (mTranslatingDirect == TRANSLATE_RIGHTUP) {
            if (mTranslateToXREdge && mTranslateToYTEdge) {
                return RET_OK;
            } else if (mTranslateToXREdge && !mTranslateToYTEdge) {
                mTy = mTy + ty;
            } else if (!mTranslateToXREdge && mTranslateToYTEdge) {
                mTx = mTx + tx;
            } else {
                mTx = mTx + tx;
                mTy = mTy + ty;
            }
        } else if (mTranslatingDirect == TRANSLATE_RIGHTDOWN) {
            if (mTranslateToXREdge && mTranslateToYBEdge) {
                return RET_OK;
            } else if (mTranslateToXREdge && !mTranslateToYBEdge) {
                mTy = mTy + ty;
            } else if (!mTranslateToXREdge && mTranslateToYBEdge) {
                mTx = mTx + tx;
            } else {
                mTx = mTx + tx;
                mTy = mTy + ty;
            }
        }

        float realScale = 1.0f;
        realScale = mScalingStep * 1.0f;
        clearBmp(mScalingBitmap);

        mScalingBitmap = scaleStep((mRotateBitmap != NULL) ? mRotateBitmap : mBitmap,
                                   realScale, realScale);
        mScalingStep = realScale;
        renderAndShow(mScalingBitmap);
        clearBmp(mScalingBitmap);
        mScalingBitmap = NULL;
        return RET_OK;
    }

    int ImagePlayerService::setRotateScale(float degrees, float sx, float sy,
                                           int autoCrop) {
        if (degrees == 0.0f && sx == 1.0 && sy == 1.0) {
            renderAndShow(mBitmap);
            return RET_OK;
        }
        Mutex::Autolock autoLock(mLock);

        bool isAutoCrop = autoCrop != 0;
        ALOGD("setRotateScale degrees:%f, sx:%f, sy:%f, isAutoCrop:%d", degrees, sx, sy,
              isAutoCrop);

        if ((sx > 16.0f) || (sy > 16.0f)) {
            ALOGE("setRotateScale max x scale up or y scale up is 16");
            return RET_ERR_INVALID_OPERATION;
        }

        //ratate and scale, always use the origin bitmap
        //reset rotate and scale, because rotate is the always first state
        resetRotateScale();

        resetTranslate();

        if (mMovieImage) {
            mMovieDegree = degrees;
            mMovieScale = sx;
            return RET_OK;
        }

        SkBitmap *dstBitmap = NULL;
        dstBitmap = rotateAndScale(mBitmap, degrees, sx, sy, isAutoCrop);

        if (dstBitmap != NULL) {
            renderAndShow(dstBitmap);
            clearBmp(dstBitmap);
            dstBitmap = NULL;
            return RET_OK;
        }

        return RET_ERR_DECORDER;
    }

    int ImagePlayerService::setCropRect(int cropX, int cropY, int cropWidth,
                                        int cropHeight) {
        Mutex::Autolock autoLock(mLock);

        ALOGD("setCropRect cropX:%d, cropY:%d, cropWidth:%d, cropHeight:%d", cropX,
              cropY, cropWidth, cropHeight);

        if (mBitmap == NULL) {
            ALOGD("Warning: mBitmap is NULL");
            return RET_ERR_BAD_VALUE;
        }

        if ((-1 < cropX) && (cropX < mBitmap->width()) && (-1 < cropY)
                && (cropY < mBitmap->height())
                && (0 < cropWidth) && (0 < cropHeight)
                && ((cropX + cropWidth) <= mBitmap->width())
                && ((cropY + cropHeight) <= mBitmap->height())) {

            showBitmapRect(mBitmap, cropX, cropY, cropWidth, cropHeight);

        } else {
            ALOGD("Warning: parameters is not valid");
            return RET_ERR_PARAMETER;
        }

        return RET_OK;
    }

    int ImagePlayerService::start() {
        ALOGI("start");

        prepare();
        show();
        return RET_OK;
    }

    int ImagePlayerService::release() {
        ALOGI("release");


        clearBmp(mBitmap);
        mBitmap = NULL;
        clearBmp(mBufBitmap);
        mBufBitmap = NULL;
        delete mParameter;
        mParameter = NULL;

        if (mFileDescription >= 0) {
            close(mFileDescription);
            mFileDescription = -1;
        }

        if (mDisplayFd >= 0) {
            close(mDisplayFd);
            mDisplayFd = -1;
        }

        if (mMovieImage) {
            if (mMovieThread->isRunning())
                mMovieThread->requestExitAndWait();

            mMovieThread.clear();
            mMovieImage = false;
            mFrameIndex = 0;
        }

        if (NULL != mTif) {
            delete mTif;
            mTif = NULL;
        }

        mSysWrite->writeSysfs(VIDEO_INUSE, "0");
        mSysWrite->writeSysfs("/sys/module/picdec/parameters/p2p_mode","2");
        mSysWrite->setProperty("persist.sys.is.image.player", "0");
        /*mSysWrite->writeSysfs("/sys/class/amvecm/pc_mode", "1");*/
        mSysWrite->writeSysfs("sys/module/di/parameters/nr2_en", "1");
        mSysWrite->writeSysfs("/sys/class/amvecm/lc", "lc enable");
        mSysWrite->writeSysfs("/sys/class/amvecm/pq_user_set", "blk_ext_en");
        resetRotateScale();
        resetTranslate();
        resetHWScale();
        return RET_OK;
    }

    SkBitmap* ImagePlayerService::preCropAndRotate(SkBitmap *srcBitmap, float degrees) {
        int sourceWidth = srcBitmap->width();
        int sourceHeight = srcBitmap->height();

        int dstHeight = sourceWidth;
        int dstWidth = sourceHeight;
        float scaleSample = surfaceHeight*1.0/dstHeight < surfaceWidth*1.0/dstWidth ?
                     surfaceHeight*1.0/dstHeight:surfaceWidth*1.0/dstWidth;
        /* if (destHeight*1.0/surfaceHeight > destWidth*1.0/surfaceWidth) {
            destHeight = surfaceHeight;
            destWidth = surfaceWidth*surfaceHeight/destHeight;
        } else {
            destWidth = surfaceWidth;
            destHeight = surfaceWidth*surfaceHeight/destWidth;
        }
        ALOGE("prescale to [%dX%d]",destWidth,destHeight);

        canvas = new SkCanvas(*src);
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setDither(true);
        canvas->clipRect(*srcBitmap, (SkRect::MakeXYWH((sourceWidth-destWidth)/2, (sourceHeight-destHeight)/2, destWidth, destHeight));
        delete canvas;*/
        ALOGD("preScale And rotae %f",scaleSample);
        SkBitmap *devBitmap = new SkBitmap();
        SkMatrix *matrix = new SkMatrix();
        double radian = SkDegreesToRadians(degrees);

        dstWidth = sourceWidth * fabs(cos(radian)) + sourceHeight * fabs(sin(
                           radian));
        dstHeight = sourceHeight * fabs(cos(radian)) + sourceWidth * fabs(sin(
                            radian));

        SkColorType colorType = colorTypeForScaledOutput(srcBitmap->colorType());
        SkImageInfo info = SkImageInfo::Make((int)(scaleSample*dstWidth), (int)(scaleSample*dstHeight),
                                             colorType, srcBitmap->alphaType());
        devBitmap->setInfo(info);
        devBitmap->tryAllocPixels(info);
        ALOGD("preScale And rotae %d x %d",((int)(scaleSample*dstWidth), (int)(scaleSample*dstHeight)));
        SkCanvas *canvas = new SkCanvas(*devBitmap);

        matrix->postRotate(degrees, sourceWidth / 2, sourceHeight / 2);
        matrix->postTranslate((dstWidth - sourceWidth) / 2,
                              (dstHeight - sourceHeight) / 2);
        matrix->postScale(scaleSample,scaleSample);
        SkPaint paint;

        paint.setAntiAlias(true);
        paint.setDither(true);
        canvas->save();
        canvas->concat(*matrix);
        canvas->drawBitmap(*srcBitmap, 0, 0, &paint);
        canvas->restore();
        delete canvas;
        delete matrix;
        return devBitmap;
    }
    SkBitmap* ImagePlayerService::decode(SkStreamAsset *stream,
                                         InitParameter *mParameter) {
        std::unique_ptr<SkStream> s = stream->fork();
        std::unique_ptr<SkCodec> c(SkCodec::MakeFromStream(std::move(s)));

        if (!c) {
            ALOGE("decode make codec from stream null!");
            return NULL;
        }
        std::unique_ptr<SkAndroidCodec> codec = SkAndroidCodec::MakeFromCodec(std::move(c),
            SkAndroidCodec::ExifOrientationBehavior::kRespect);

        SkImageInfo imageInfo = codec->getInfo();
        auto alphaType = imageInfo.isOpaque() ? kOpaque_SkAlphaType :
                         kPremul_SkAlphaType;
        int originWidth = imageInfo.width();
        int originHeight = imageInfo.height();

        ALOGI("original bmpinfo %d %d %d\n", imageInfo.width(), imageInfo.height(),
              SkCodec::kIncompleteInput);
        int samplesize = 1;

        if (imageInfo.width() > SURFACE_4K_WIDTH || imageInfo.height() > SURFACE_4K_HEIGHT) {
            samplesize = imageInfo.width()/SURFACE_4K_WIDTH > imageInfo.height()/SURFACE_4K_HEIGHT ?
                                imageInfo.width()/SURFACE_4K_WIDTH : imageInfo.height()/SURFACE_4K_HEIGHT;
        }
        SkISize scaledDims = codec->getSampledDimensions(samplesize);
        ALOGI("scale down origin picture %d %d %d\n",samplesize,scaledDims.width(), scaledDims.height());

        SkImageInfo scaledInfo = imageInfo.makeWH(scaledDims.width(), scaledDims.height());
             //   .makeColorType(kN32_SkColorType);
        SkAndroidCodec::AndroidOptions options;
        options.fSampleSize = samplesize;
        SkBitmap *bmp = new SkBitmap();
        bmp->setInfo(scaledInfo);
        bmp->tryAllocPixels(scaledInfo);//tryAllocPixels use HeapAllocator already
        SkCodec::Result result = codec->getAndroidPixels(scaledInfo, bmp->getPixels(),
                                 bmp->rowBytes(),&options);

        if ((SkCodec::kSuccess != result) && (SkCodec::kIncompleteInput != result)) {
            ALOGE("codec getPixels fail result:%d\n", result);
            if (bmp != NULL && !bmp->isNull()) {
                bmp->reset();
            }
            return NULL;
        }
        SkBitmap *dest = new SkBitmap();
        if (kGray_8_SkColorType == bmp->colorType()) {
            ALOGD("the pic is Gray");
            covert8to32(*bmp,dest);
            if (dest != NULL) {
                ALOGD("swap bmp for gray pic");
                clearBmp(bmp);
                bmp = dest;
            }
        }
        if (bmp != NULL ) {
            mWidth = bmp->width();
            mHeight = bmp->height();
            SkCanvas *canvas = new SkCanvas(*bmp);
            canvas->drawBitmap(*bmp, 0, 0);
            delete canvas;
            ALOGD("Image raw size, width:%d, height:%d", mWidth, mHeight);
        }
        return bmp;
    }

    SkBitmap* ImagePlayerService::decodeTiff(const char *filePath) {
        int width = 0;
        int height = 0;

        if (NULL != mTif) {
            delete mTif;
            mTif = NULL;
        }

        mTif = new TIFF2RGBA();
        mTif->tiffDecodeBound(filePath, &width, &height);

        if ((width > MAX_PIC_SIZE) || (height > MAX_PIC_SIZE)) {
            ALOGE("decode tiff size is too large, we only support w < %d and h < %d, now image size w:%d, h:%d",
                  MAX_PIC_SIZE, MAX_PIC_SIZE, width, height);
            delete mTif;
            mTif = NULL;
        } else {
            SkBitmap *bitmap = new SkBitmap();
            int ret = mTif->tiffDecoder(filePath, bitmap);
            ALOGI("decode tiff result:%d, width:%d, height:%d", ret, bitmap->width(),
                  bitmap->height());

            mWidth = bitmap->width();
            mHeight = bitmap->height();

            if ((bitmap->width() > 0) && (bitmap->height() > 0)) {
                return bitmap;
            } else {
                clearBmp(bitmap);
                bitmap = NULL;
            }
        }

        return NULL;
    }
    /*not delete source bmp*/
    SkBitmap* ImagePlayerService::scale(SkBitmap *srcBitmap, float sx, float sy) {

        int sourceWidth = srcBitmap->width();
        int sourceHeight = srcBitmap->height();

        int dstWidth = sourceWidth*sx;
        int dstHeight = sourceHeight*sy;
        ALOGE("scale to[%d %d]->[%d,%d]",sourceWidth,sourceHeight,dstWidth,dstHeight);
        if (dstWidth < surfaceWidth && dstHeight < surfaceHeight && P2pDisplay) {
            return simplescale(srcBitmap, sx, sy);
        }

        SkBitmap *devBitmap = new SkBitmap();
        SkRect srcRect;
        SkRect destRect;
        SkCanvas *canvas = NULL;

        SkColorType colorType = colorTypeForScaledOutput(srcBitmap->colorType());
        SkImageInfo info;
        if (sx >= 1.0) {
            //scale 2 fullscreen
            int destPicWidth = (sourceWidth*sx) > surfaceWidth ? surfaceWidth : (sourceWidth*sx);
            int destPicHeight = (sourceHeight*sy) > surfaceHeight? surfaceHeight : (sourceHeight*sy);
            destRect = SkRect::MakeLTRB((sourceWidth-destPicWidth/sx)/2,(sourceHeight-destPicHeight/sx)/2,
                            (sourceWidth+destPicWidth/sx)/2, (sourceHeight+destPicHeight/sx)/2);
            srcRect = SkRect::MakeLTRB(0,0,destPicWidth, destPicHeight);
            ALOGE("scale to [%d %d]",destPicWidth, destPicHeight);

            info = SkImageInfo::Make(destPicWidth, destPicHeight,
                                                 colorType, srcBitmap->alphaType());
        }else{
            destRect = SkRect::MakeLTRB(0,0, sourceWidth,sourceHeight);
            srcRect = SkRect::MakeLTRB((sourceWidth-dstWidth)/2,(sourceHeight-dstHeight)/2,(sourceWidth+dstWidth)/2,(sourceHeight+dstHeight)/2);

            info = SkImageInfo::Make(sourceWidth, sourceHeight,
                                         colorType, srcBitmap->alphaType());
        }
        devBitmap->setInfo(info);
        devBitmap->tryAllocPixels(info);

        canvas = new SkCanvas(*devBitmap);
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setDither(true);
        canvas->save();
        canvas->drawBitmapRect(*srcBitmap, destRect, srcRect, &paint,SkCanvas::kFast_SrcRectConstraint);
        canvas->restore();
        delete canvas;
        return devBitmap;
    }
    SkBitmap* ImagePlayerService::simplescale(SkBitmap *srcBitmap, float sx, float sy) {
        if (srcBitmap == NULL)
            return NULL;

        int sourceWidth = srcBitmap->width();
        int sourceHeight = srcBitmap->height();
        int dstWidth = sourceWidth * sx;
        int dstHeight = sourceHeight * sy;

        if ((dstWidth <= 0) || (dstHeight <= 0)) {
            return NULL;
        }

        SkBitmap *devBitmap = new SkBitmap();
        SkMatrix *matrix = new SkMatrix();
        SkCanvas *canvas = NULL;

        SkColorType colorType = colorTypeForScaledOutput(srcBitmap->colorType());
        devBitmap->setInfo(SkImageInfo::Make(dstWidth, dstHeight,
                                             colorType, srcBitmap->alphaType()));

        devBitmap->allocPixels();

        canvas = new SkCanvas(*devBitmap);

        matrix->postScale(sx, sy);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setDither(true);
        canvas->save();
        canvas->concat(*matrix);
        canvas->drawBitmap(*srcBitmap, 0, 0, &paint);
        canvas->restore();
        delete canvas;
        delete matrix;

        return devBitmap;
    }

    SkBitmap* ImagePlayerService::rotate(SkBitmap *srcBitmap, float degrees) {
        if (srcBitmap == NULL)
            return NULL;

        SkBitmap *devBitmap = new SkBitmap();
        SkMatrix *matrix = new SkMatrix();
        SkCanvas *canvas = NULL;

        int sourceWidth = srcBitmap->width();
        int sourceHeight = srcBitmap->height();
        double radian = SkDegreesToRadians(degrees);

        int dstWidth = sourceWidth * fabs(cos(radian)) + sourceHeight * fabs(sin(
                           radian));
        int dstHeight = sourceHeight * fabs(cos(radian)) + sourceWidth * fabs(sin(
                            radian));

        SkColorType colorType = colorTypeForScaledOutput(srcBitmap->colorType());
        SkImageInfo info = SkImageInfo::Make(dstWidth, dstHeight,
                                             colorType, srcBitmap->alphaType());
        devBitmap->setInfo(info);
        devBitmap->tryAllocPixels(info);

        canvas = new SkCanvas(*devBitmap);

        matrix->postRotate(degrees, sourceWidth / 2, sourceHeight / 2);
        matrix->postTranslate((dstWidth - sourceWidth) / 2,
                              (dstHeight - sourceHeight) / 2);

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setDither(true);
        canvas->save();
        canvas->concat(*matrix);
        canvas->drawBitmap(*srcBitmap, 0, 0, &paint);
        canvas->restore();
        delete canvas;
        delete matrix;

        return devBitmap;
    }

    SkBitmap* ImagePlayerService::rotateAndScale(SkBitmap *srcBitmap, float degrees,
            float sx, float sy, bool isAutoCrop) {
        if (srcBitmap == NULL)
            return NULL;
        ALOGE("rotateAndScale %d %d rotate %f scale to %f",srcBitmap->height(),srcBitmap->width(),degrees,sx);
        SkBitmap *dstBitmap = NULL;
        if (degrees == 0 /*&& (sx != 1.0 || sy != 1.0)*/) {

            SkBitmap *devbmp = NULL;
            if (needFillSurface(mBitmap, 1, 1)) {
                devbmp = fillSurface(mBitmap);
                if (devbmp != NULL) {
                    ALOGE("only scale and fillSurface %d %d",devbmp->width(),devbmp->height());
                    dstBitmap = scale(devbmp, sx, sy);
                    if (dstBitmap != NULL) {
                        clearBmp(devbmp);
                        devbmp = NULL;
                    }
                }
            }
            if (dstBitmap == NULL)
                dstBitmap = scale(srcBitmap, sx, sy);
        }else if (degrees != 0) {
            dstBitmap = rotate(srcBitmap, degrees);
            //ALOGE("rotate %d %d-",srcBitmap->height(),srcBitmap->width());
            if (dstBitmap != NULL) {
                if (sx != 1.0 || sy != 1.0) {
                    if (needFillSurface(mBitmap, 1, 1)) {
                        SkBitmap *fillBitmap = fillSurface(dstBitmap);

                        if (fillBitmap != NULL) {
                            clearBmp(dstBitmap);
                            dstBitmap = fillBitmap;
                        }
                    }

                    ALOGD("totateAndScale After rotate, Width: %d, Height: %d", dstBitmap->width(),
                          dstBitmap->height());

                    if ((dstBitmap->width() > surfaceWidth)
                            || (dstBitmap->height() > surfaceHeight)) {
                        ALOGD("cropAndFillBitmap");
                        SkBitmap *dstCrop = cropAndFillBitmap(dstBitmap, surfaceWidth, surfaceHeight);

                        if (NULL != dstCrop) {
                            clearBmp(dstBitmap);
                            dstBitmap = dstCrop;
                        }
                    }

                    SkBitmap *scalBitmap = scale(dstBitmap, sx, sy);
                    if (scalBitmap != NULL) {
                        clearBmp(dstBitmap);
                        dstBitmap = scalBitmap;
                    }
                }
            }
        }
        if (dstBitmap != NULL ) {
            if (dstBitmap->width() > surfaceWidth || dstBitmap->height() > surfaceHeight) {
                ALOGD("rotateAndScale end and fillSurface");
                SkBitmap *fillBitmap = fillSurface(dstBitmap);

                if (fillBitmap != NULL) {
                    clearBmp(dstBitmap);
                    dstBitmap = fillBitmap;
                }
            }
            ALOGD("After rotate and scale, Width: %d, Height: %d", dstBitmap->width(),
              dstBitmap->height());
        }

        return dstBitmap;
    }

    SkStreamAsset* ImagePlayerService::getSkStream() {
        SkStreamAsset *stream;

        if (mMovieImage && !isMovieByExtenName(mImageUrl)) {
            mMovieImage = false;
            return NULL;
        }
        if (mFileDescription >= 0) {
            sk_sp<SkData> data(SkData::MakeFromFD(mFileDescription));

            if (data.get() == NULL) {
                return NULL;
            }

            stream = new SkMemoryStream(data);
        } else if (!strncasecmp("http://", mImageUrl, 7)
                   || !strncasecmp("https://", mImageUrl, 8)) {
            ALOGI("SkHttpStream:%s", mImageUrl);
            stream = getStreamForHttps(mImageUrl);
        } else {
            //ALOGI("SkFILEStream:%s", mImageUrl);
            stream = new SkFILEStream(mImageUrl);
        }

        return stream;
    }

    SkStreamAsset* ImagePlayerService::getStreamForHttps(char* url) {
        ALOGD("getStreamForHttps url %s", url);
        char* path = getFileByCurl(url);
        if (path == nullptr) {
            ALOGE("getStreamForHttps fails");
            return nullptr;
        }
        ALOGD("getStreamForHttps image path %s", path);
        SkStreamAsset *stream = new SkFILEStream(path);
        return stream;
    }

    //render to video layer
    int ImagePlayerService::prepare() {
        Mutex::Autolock autoLock(mLock);
        ALOGI("prepare image path:%s", mImageUrl);
        FrameInfo_t info;

        if ((mFileDescription < 0) && (0 == strlen(mImageUrl))) {
            ALOGE("prepare decode image fd error");
            return RET_ERR_BAD_VALUE;
        }

        clearBmp(mRotateBitmap);
        clearBmp(mBitmap);
        mBitmap = NULL;
        mRotateBitmap = NULL;
        mMovieImage = false;
        mFrameIndex = 0;

        if (isMovieByExtenName(mImageUrl)) {
            ALOGI("it's a movie image, show it with thread");

            mMovieImage = true;

            if (MovieInit(mImageUrl)) {
                return RET_OK;
            } else {
                ALOGI("decode gif failed use jpeg decode");
                mMovieImage = false;
                mFrameIndex = 0;
            }
        }
        if (isTiffByExtenName(mImageUrl)) {
            mBitmap = decodeTiff(mImageUrl);
        } else {
            SkStreamAsset *stream = getSkStream();
            if (stream == NULL) {
                return RET_ERR_BAD_VALUE;
            }
            mBitmap = decode(stream, NULL);
            delete stream;
        }

        if (mBitmap == NULL) {
            ALOGE("prepare decode result bitmap is NULL");
            return RET_ERR_BAD_VALUE;
        }

        if (mWidth <= 0 || mHeight <= 0) {
            ALOGE("prepare decode result bitmap size error");
            return RET_ERR_BAD_VALUE;
        }

        if (mDisplayFd < 0) {
            ALOGE("render, but displayFd can not ready");
            return RET_ERR_BAD_VALUE;
        }
        if (needFillSurface(mBitmap, 1, 1)) {
            SkBitmap *dstBitmap = fillSurface(mBitmap);

            if (dstBitmap != NULL) {
                resetRotateScale();
                resetTranslate();
                render(VIDEO_LAYER_FORMAT_RGBA, dstBitmap);
                clearBmp(mBitmap);
                mBitmap = dstBitmap;
                return RET_OK;
            }
        }

        resetRotateScale();
        resetTranslate();
        render(VIDEO_LAYER_FORMAT_RGBA, mBitmap);
        ALOGI("prepare render is OK");
        return RET_OK;
    }

    int ImagePlayerService::render(int format, SkBitmap *bitmap) {
        FrameInfo_t info;

        if (mDisplayFd < 0) {
            ALOGE("render, but displayFd can not ready");
            return RET_ERR_BAD_VALUE;
        }

        if (NULL == bitmap) {
            ALOGE("render, bitmap is NULL");
            return RET_ERR_BAD_VALUE;
        }

        ALOGI("render format:%d [0:RGB 1:RGBA 2:ARGB], bitmap w:%d, h:%d", format,
              bitmap->width(), bitmap->height());

        switch (format) {
            case VIDEO_LAYER_FORMAT_RGB: {
                char* bitmapAddr = NULL;
                int len = bitmap->width() * bitmap->height() * 3; //RGBA -> RGB
                bitmapAddr = (char*)malloc(len);

                if (NULL == bitmapAddr) {
                    ALOGE("render, not enough memory");
                    return RET_ERR_NO_MEMORY;
                }

                memset(bitmapAddr, 0, len);

                //bitmap->lockPixels();
                convertRGBA8888toRGB(bitmapAddr, bitmap);
                //bitmap->unlockPixels();

                info.pBuff = bitmapAddr;
                info.format = format;
                info.frame_width = bitmap->width();
                info.frame_height = bitmap->height();

                ioctl(mDisplayFd, PICDEC_IOC_FRAME_RENDER, &info);

                if (NULL != bitmapAddr)
                    free(bitmapAddr);
            }
            break;

            case VIDEO_LAYER_FORMAT_RGBA: {
                //bitmap->lockPixels();
                info.pBuff = (char*)bitmap->getPixels();
                info.format = format;
                info.frame_width = bitmap->width();
                info.frame_height = bitmap->height();

                copy_data_to_mmap_buf(mDisplayFd, info.pBuff,
                             info.frame_width, info.frame_height,
                             info.format);

                ioctl(mDisplayFd, PICDEC_IOC_FRAME_RENDER, &info);
                //bitmap->unlockPixels();
            }
            break;

            case VIDEO_LAYER_FORMAT_ARGB:
            default:
                break;
        }

        return RET_OK;
    }
    void ImagePlayerService::covert8to32(SkBitmap& src, SkBitmap *dst) {
         SkImageInfo k32Info = SkImageInfo::Make(src.width(),src.height(),kBGRA_8888_SkColorType,
                                src.alphaType(),SkColorSpace::MakeSRGB());
         dst->allocPixels(k32Info);
         char* dst32 = (char*)dst->getPixels();
         uint8_t* src8 = (uint8_t*)src.getPixels();
         const int w = src.width();
         const int h = src.height();
         const bool isGRAY = (kGray_8_SkColorType == src.colorType());
         ALOGD("raw bytes:%d raw pixels is %d-->%d",src.rowBytes(),w,dst->rowBytes());
         for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                uint8_t s = src8[x];
                dst32[x*4] = s;
                dst32[x*4+1] = s;
                dst32[x*4+2] = s;
                dst32[x*4+3] = 0xFF;
            }
            src8 = (uint8_t*)((uint8_t*)src8+src.rowBytes());
            dst32 = (char*)((char*)dst32 + dst->rowBytes());
         }
    }
    //post to display device
    int ImagePlayerService::post() {
        if (mDisplayFd < 0) {
            ALOGE("post, but displayFd has not ready");
            return RET_ERR_BAD_VALUE;
        }

        resetHWScale();

        ALOGI("post picture to display fd:%d", mDisplayFd);
        ioctl(mDisplayFd, PICDEC_IOC_FRAME_POST, NULL);
        return RET_OK;
    }

    //post to display device
    int ImagePlayerService::show() {
        Mutex::Autolock autoLock(mLock);

        ALOGI("show, is movie image:%d", mMovieImage);

       // MovieThreadStop();
        if (mMovieImage)
            return MovieThreadStart();
        int ret =  post();
        return ret;
    }

    int ImagePlayerService::showImage(const char* uri) {
        int res = setDataSource(uri);
        if (res!= RET_OK) {
            return res;
        }
        res = prepare();
        if (res!= RET_OK) {
            return res;
        }
        res = show();
        return res;
    }

    bool ImagePlayerService::copyTo(SkBitmap* dst, SkColorType dstCT,
                                    const SkBitmap& src,
                                    SkBitmap::Allocator* alloc) {
        SkPixmap srcPM;

        if (!src.peekPixels(&srcPM)) {
            return false;
        }

        SkImageInfo dstInfo = srcPM.info().makeColorType(dstCT);

        switch (dstCT) {
            case kRGB_565_SkColorType:

                // copyTo() has never been strict on alpha type.  Here we set the src to opaque to
                // allow the call to readPixels() to succeed and preserve this lenient behavior.
                if (kOpaque_SkAlphaType != srcPM.alphaType()) {
                    srcPM = SkPixmap(srcPM.info().makeAlphaType(kOpaque_SkAlphaType), srcPM.addr(),
                                     srcPM.rowBytes());
                    dstInfo = dstInfo.makeAlphaType(kOpaque_SkAlphaType);
                }

                break;

            case kRGBA_F16_SkColorType:
                // The caller does not have an opportunity to pass a dst color space.  Assume that
                // they want linear sRGB.
                dstInfo = dstInfo.makeColorSpace(SkColorSpace::MakeSRGBLinear());

                if (!srcPM.colorSpace()) {
                    // Skia needs a color space to convert to F16.  nullptr should be treated as sRGB.
                    srcPM.setColorSpace(SkColorSpace::MakeSRGB());
                }

                break;

            default:
                break;
        }

        if (!dst->setInfo(dstInfo)) {
            return false;
        }

        if (!dst->tryAllocPixels(alloc)) {
            return false;
        }

        // Skia does not support copying from kAlpha8 to types that are not alpha only.
        // We will handle this case here.
        if (kAlpha_8_SkColorType == srcPM.colorType()
                && kAlpha_8_SkColorType != dstCT) {
            switch (dstCT) {
                case kRGBA_8888_SkColorType:
                case kBGRA_8888_SkColorType: {
                    for (int y = 0; y < src.height(); y++) {
                        const uint8_t* srcRow = srcPM.addr8(0, y);
                        uint32_t* dstRow = dst->getAddr32(0, y);
                        ToColor_SA8(dstRow, srcRow, src.width());
                    }

                    return true;
                }

                case kRGB_565_SkColorType: {
                    for (int y = 0; y < src.height(); y++) {
                        uint16_t* dstRow = dst->getAddr16(0, y);
                        memset(dstRow, 0, sizeof(uint16_t) * src.width());
                    }

                    return true;
                }

                case kRGBA_F16_SkColorType: {
                    for (int y = 0; y < src.height(); y++) {
                        const uint8_t* srcRow = srcPM.addr8(0, y);
                        void* dstRow = dst->getAddr(0, y);
                        ToF16_SA8(dstRow, srcRow, src.width());
                    }

                    return true;
                }

                default:
                    return false;
            }
        }

        SkPixmap dstPM;

        if (!dst->peekPixels(&dstPM)) {
            return false;
        }

        // Skia needs a color space to convert from F16.  nullptr should be treated as sRGB.
        if (kRGBA_F16_SkColorType == srcPM.colorType() && !dstPM.colorSpace()) {
            dstPM.setColorSpace(SkColorSpace::MakeSRGB());
        }

        // readPixels does not support color spaces with parametric transfer functions.  This
        // works around that restriction when the color spaces are equal.
        if (kRGBA_F16_SkColorType != dstCT
                && kRGBA_F16_SkColorType != srcPM.colorType() &&
                dstPM.colorSpace() == srcPM.colorSpace()) {
            dstPM.setColorSpace(nullptr);
            srcPM.setColorSpace(nullptr);
        }

        return srcPM.readPixels(dstPM);
    }


    //internal use
    bool ImagePlayerService::renderAndShow(SkBitmap *bitmap) {
        render(VIDEO_LAYER_FORMAT_RGBA, bitmap);
        post();
        return true;
    }

    void ImagePlayerService::resetRotateScale() {
        mScalingDirect = SCALE_NORMAL;
        mScalingStep = 1.0f;
        mMovieScale = 1.0f;
        clearBmp(mScalingBitmap);
        clearBmp(mRotateBitmap);
        mScalingBitmap = NULL;
        mRotateBitmap = NULL;

    }

    void ImagePlayerService::resetHWScale() {
        if (mSysWrite != NULL) {
            mSysWrite->writeSysfs(VIDEO_ZOOM_SYSFS, "100");
        } else {
            ALOGE("Couldn't get connection to system control\n");
        }
    }

    void ImagePlayerService::resetTranslate() {
        mTranslatingDirect = TRANSLATE_NORMAL;
        mTx = 0;
        mTy = 0;
        mTranslateToXLEdge = false;
        mTranslateToXREdge = false;
        mTranslateToYTEdge = false;
        mTranslateToYBEdge = false;
    }

    void ImagePlayerService::isTranslateToEdge(SkBitmap *srcBitmap, int dstWidth,
            int dstHeight, int tx, int ty) {
        if (srcBitmap == NULL)
            return;

        int minWidth = Min(srcBitmap->width(), dstWidth);
        int minHeight = Min(srcBitmap->height(), dstHeight);
        int srcx = (srcBitmap->width() - minWidth) / 2;
        int srcy = (srcBitmap->height() - minHeight) / 2;
        int dstx = (dstWidth - minWidth) / 2;
        int dsty = (dstHeight - minHeight) / 2;

        ALOGD("isTranslateToEdge, minWidth: %d, minHeight: %d, srcx:%d, srcy:%d, dstx:%d, dsty:%d",
              minWidth, minHeight, srcx, srcy, dstx, dsty);

        int aftertranslatesrcx = srcx + tx;
        int aftertranslatesrcy = srcy + ty;

        if (tx > 0) {
            if (srcx < tx) {
                aftertranslatesrcx = srcx * 2;
                mTranslateToXREdge = true;
            } else {
                mTranslateToXREdge = false;
            }

            mTranslateToXLEdge = false;
        } else if (tx < 0) {
            if (srcx < (0 - tx)) {
                aftertranslatesrcx = 0;
                mTranslateToXLEdge = true;
            } else {
                mTranslateToXLEdge = false;
            }

            mTranslateToXREdge = false;
        }

        if (ty > 0) {
            if (srcy < ty) {
                aftertranslatesrcy = srcy * 2;
                mTranslateToYBEdge = true;
            } else {
                mTranslateToYBEdge = false;
            }

            mTranslateToYTEdge = false;
        } else if (ty < 0) {
            if (srcy < (0 - ty)) {
                aftertranslatesrcy = 0;
                mTranslateToYTEdge = true;
            } else {
                mTranslateToYTEdge = false;
            }

            mTranslateToYBEdge = false;
        }
    }

    SkBitmap* ImagePlayerService::scaleStep(SkBitmap *srcBitmap, float sx,
                                            float sy) {
        int STEP_EXP_4      = 4;
        int STEP_EXP_3      = 3;
        int STEP_EXP_2      = 2;
        float SETP_LENGTH   = 2.0f;

        int stepCount = 0;
        SkBitmap *scalingBitmap = NULL;

        if (srcBitmap == NULL)
            return NULL;

        if ((sx > 16.0f) || (sy > 16.0f)) {
            ALOGE("scaleStep max x scale up or y scale up is 16");
            return NULL;
        }

        ALOGD("scaleStep, bitmap Width: %d, Height: %d, sx:%f, sy:%f",
              srcBitmap->width(), srcBitmap->height(), sx, sy);

        if ((sx == 16.0f) || (sy == 16.0f)) {
            stepCount = STEP_EXP_4;
        } else if ((sx == 8.0f) || (sy == 8.0f)) {
            stepCount = STEP_EXP_3;
        } else if ((sx == 4.0f) || (sy == 4.0f)) {
            stepCount = STEP_EXP_2;
        } else if ((sx == 2.0f) || (sy == 2.0f)) {
            scalingBitmap = scaleAndCrop(srcBitmap, sx, sy);
        } else if ((sx < 1.0f) || (sy < 1.0f)) {
            scalingBitmap = scaleAndCrop(srcBitmap, sx, sy);
        } else {
            ALOGW("scaleStep, scale directly, but maybe have not enough memory!!");
            scalingBitmap = scaleAndCrop(srcBitmap, sx, sy);
        }

        if (stepCount > 0) {
            int step = 1;
            float scalex = SETP_LENGTH;
            float scaley = SETP_LENGTH;
            SkBitmap *retBitmap = scaleAndCrop(srcBitmap, scalex, scaley);

            while (true) {
                clearBmp(scalingBitmap);
                scalingBitmap = retBitmap;
                clearBmp(retBitmap);
                retBitmap = scaleAndCrop(scalingBitmap, scalex, scaley);
                clearBmp(scalingBitmap);
                scalingBitmap = NULL;
                step++;

                if (step >= stepCount) {
                    scalingBitmap = retBitmap;
                    break;
                }
            }
        }

        return scalingBitmap;
    }

    SkBitmap* ImagePlayerService::scaleAndCrop(SkBitmap *srcBitmap, float sx,
            float sy) {
        if (srcBitmap == NULL || (sx == 1.0 && sy == 1.0))
            return NULL;

        SkBitmap *retBitmap = scale(srcBitmap, sx, sy);

        if (retBitmap == NULL)
            return NULL;

        ALOGD("scaleAndCrop, after scale, Width: %d, Height: %d, surface w:%d, h:%d",
              retBitmap->width(), retBitmap->height(), surfaceWidth, surfaceHeight);

        if ((retBitmap->width() > surfaceWidth)
                || (retBitmap->height() > surfaceHeight)) {
            if (mTranslateImage) {
                SkBitmap *dstCrop = translateAndCropAndFillBitmap(retBitmap, surfaceWidth,
                                    surfaceHeight, mTx, mTy);
                isTranslateToEdge(retBitmap, surfaceWidth, surfaceHeight, mTx, mTy);

                if (NULL != dstCrop) {
                    clearBmp(retBitmap);
                    retBitmap = dstCrop;
                }

                mTranslateImage = false;
                return retBitmap;
            }

            SkBitmap *dstCrop = cropAndFillBitmap(retBitmap, surfaceWidth, surfaceHeight);

            if (NULL != dstCrop) {
                clearBmp(retBitmap);
                retBitmap = dstCrop;
            }
        }

        return retBitmap;
    }

    SkBitmap* ImagePlayerService::fillSurface(SkBitmap *bitmap) {
        if (NULL == bitmap ) {
            return NULL;
        }

        int bitmapW = bitmap->width();
        int bitmapH = bitmap->height();

        float originalZoomPersent = ( surfaceWidth*1.0/bitmapW) > (surfaceHeight*1.0/bitmapH)?
                                           (surfaceHeight*1.0/bitmapH):( surfaceWidth*1.0/bitmapW);
        ALOGD("fillSurface %f",originalZoomPersent);
        if (originalZoomPersent != 1.0f) {
            SkBitmap *dstBitmap = simplescale(bitmap, originalZoomPersent, originalZoomPersent);
            return dstBitmap;
        }

        return NULL;
    }

    bool ImagePlayerService::showBitmapRect(SkBitmap *bitmap, int cropX, int cropY,
                                            int cropWidth, int cropHeight) {
        FrameInfo_t info;
        char* bitmapAddr = NULL;
        int len = cropWidth * cropHeight * 3; //RGBA -> RGB
        bitmapAddr = (char*)malloc(len);

        if (NULL == bitmapAddr) {
            ALOGE("showBitmapRect, not enough memory");
            return false;
        }

        memset(bitmapAddr, 0, len);

        uint8_t *pDst = (uint8_t*)bitmapAddr;
        uint8_t *pSrc = (uint8_t*)bitmap->getPixels();
        uint32_t u32DstStride = cropWidth * 3;

        for (int y = 0; y < cropHeight; y++) {
            uint32_t srcOffset = bitmap->rowBytes() * (cropY + y) + 4 * cropX;

            for (int x = 0; x < cropWidth; x++) {
                pDst[3 * x + 0] = pSrc[4 * x + srcOffset + 0]; //B
                pDst[3 * x + 1] = pSrc[4 * x + srcOffset + 1]; //G
                pDst[3 * x + 2] = pSrc[4 * x + srcOffset + 2]; //R
                //pSrc[4*x+3]; A
            }

            pDst += u32DstStride;
        }

        info.pBuff = bitmapAddr;
        info.format = VIDEO_LAYER_FORMAT_RGB;
        info.frame_width = cropWidth;
        info.frame_height = cropHeight;

        ioctl(mDisplayFd, PICDEC_IOC_FRAME_RENDER, &info);

        if (NULL != bitmapAddr)
            free(bitmapAddr);

        post();
        return true;
    }

    bool ImagePlayerService::isSupportFromat(const char *uri, SkBitmap **bitmap) {
        bool ret = isPhotoByExtenName(uri);

        if (!ret)
            return false;

        if (!strncasecmp("file://", uri, 7)) {
            FILE* file = fopen(uri+7,"rb");
            if ( file == NULL ) {
                ALOGE("cannot open file %s %d\n",uri+7, errno);
                return false;
            }
            SkFILEStream stream(file);
            ret = verifyBySkCodec(&stream, bitmap);
            fclose(file);
            return ret;
        }

        /*
        if (!strncasecmp("http://", uri, 7) || !strncasecmp("https://", uri, 8)) {
            SkHttpStream httpStream(uri, mHttpService);
            return verifyBySkCodec(&httpStream, bitmap);
        }
        */

        return false;
    }

    int ImagePlayerService::convertRGBA8888toRGB(void *dst, const SkBitmap *src) {
        uint8_t *pDst = (uint8_t*)dst;
        uint8_t *pSrc = (uint8_t*)src->getPixels();
        uint32_t u32SrcStride = src->rowBytes();
        uint32_t u32DstStride = src->width() * 3;

        for (int y = 0; y < src->height(); y++) {
            for (int x = 0; x < src->width(); x++) {
                pDst[3 * x + 0] = pSrc[4 * x + 0]; //B
                pDst[3 * x + 1] = pSrc[4 * x + 1]; //G
                pDst[3 * x + 2] = pSrc[4 * x + 2]; //R
                //pSrc[4*x+3]; A
            }

            pSrc += u32SrcStride;
            pDst += u32DstStride;
        }

        return RET_OK;
    }

    int ImagePlayerService::convertARGB8888toYUYV(void *dst, const SkBitmap *src) {
        uint8_t *pDst = (uint8_t*)dst;
        uint8_t *pSrc = (uint8_t*)src->getPixels();
        uint32_t u32SrcStride = src->rowBytes();
        uint32_t u32DstStride = ((src->width() + 15) & ~15) * 2; //YUYV

        for (int y = 0; y < src->height(); y++) {
            ARGBToYUV422Row_C(pSrc, pDst, src->width());
            pSrc += u32SrcStride;
            pDst += u32DstStride;
        }

        return RET_OK;
    }

    int ImagePlayerService::convertRGB565toYUYV(void *dst, const SkBitmap *src) {
        uint8_t *pDst = (uint8_t*)dst;
        uint8_t *pSrc = (uint8_t*)src->getPixels();
        uint32_t u32SrcStride = src->rowBytes();
        uint32_t u32DstStride = ((src->width() + 15) & ~15) * 2; //YUYV

        for (int y = 0; y < src->height() - 1; y++) {
            RGB565ToYUVRow_C(pSrc, pDst, src->width());
            pSrc += u32SrcStride;
            pDst += u32DstStride;
        }

        return RET_OK;
    }

    int ImagePlayerService::convertIndex8toYUYV(void *dst, const SkBitmap *src) {
        uint8_t *pDst = (uint8_t*)dst;
        const uint8_t *pSrc = (const uint8_t *)src->getPixels();
        uint32_t u32SrcStride = src->rowBytes();
        uint32_t u32DstStride = ((src->width() + 15) & ~15) * 2; //YUYV
        SkColorTable* table;// = src->getColorTable();

        for (int y = 0; y < src->height(); y++) {
            Index8ToYUV422Row_C(pSrc, pDst, src->width(), table);
            pSrc += u32SrcStride;
            pDst += u32DstStride;
        }

        return RET_OK;
    }

    bool ImagePlayerService::MovieInit(const char *file) {
        mMovieDegree = 0;
        mMovieScale = 1.0f;
        auto data = SkData::MakeFromFileName(file);
        ALOGD("MovieInit %s",file);
        if (!data) {
            ALOGE("Could not get %s", file);
            return false;
        }
        auto codec = SkCodec::MakeFromData(data);
        if (!codec) {
            ALOGE("Could not create codec for %s", file);
            return false;
        }
        std::vector<SkCodec::FrameInfo> frameInfos = codec->getFrameInfo();
        int frameCount = frameInfos.size() == 0 ? 1 : frameInfos.size();
        mFrameIndex = 0;
        ALOGD("gif picture frame size is %d",frameCount);
        return (frameCount > 1);
    }

    bool ImagePlayerService::MovieShow() {
        if (!mMovieImage || mMovieThread->check()) {
            ALOGE("MovieShow not movie!");
            return false;
        }
        Mutex::Autolock autoLock(mLock);

        auto data = SkData::MakeFromFileName(mImageUrl);
        if (!data) {
            ALOGE("Could not get %s", mImageUrl);
            return false;
        }
        auto codec = SkCodec::MakeFromData(data);
        if (!codec) {
            ALOGE("Could not create codec for %s", mImageUrl);
            return false;
        }
        std::vector<SkCodec::FrameInfo> frameInfos = codec->getFrameInfo();
        int frameCount = frameInfos.size() == 0 ? 1 : frameInfos.size();
        std::vector<SkBitmap> cachedFrames(frameCount);

        if (mFrameIndex >= frameCount) {
            mFrameIndex = 0;
        }
        SkBitmap *bitmap = &cachedFrames[mFrameIndex];

        if (!bitmap->getPixels()) {
            const SkImageInfo info = codec->getInfo().makeColorType(kN32_SkColorType);
            //bitmap->setInfo(info);
            bitmap->allocPixels(info);

            SkCodec::Options opts;
            opts.fFrameIndex = mFrameIndex;

            //opts.fHasPriorFrame = false;
            if (&frameInfos[mFrameIndex] == nullptr) {
                ALOGE("MovieShow Could not get frame %d", mFrameIndex);
                return false;
            }
            const size_t requiredFrame = frameInfos[mFrameIndex].fRequiredFrame;

            if (requiredFrame != SkCodec::kNone) {
                SkBitmap::HeapAllocator allocator;
                // For simplicity, do not try to cache old frames
                if (copyTo(bitmap, kN32_SkColorType, cachedFrames[requiredFrame], &allocator)) {
                    //opts.fHasPriorFrame = true;
                    ALOGE("Failed to copy %s frame %i", mImageUrl, requiredFrame);
                }
            }

            if (SkCodec::kSuccess != codec->getPixels(info, bitmap->getPixels(),
                    bitmap->rowBytes(), &opts)) {
                ALOGD("Could not getPixels for frame %d", mFrameIndex);
                return false;
            }
        }
        SkBitmap *scaleBitmap = NULL;
        SkBitmap *rotateBitmap = NULL;

        if (needFillSurface(bitmap,1,1)) {
            ALOGW("MovieShow, origin width:%d or height:%d > surface w:%d or h:%d",
                  bitmap->width(), bitmap->height(), surfaceWidth, surfaceHeight);
            SkBitmap *dstCrop = fillSurface(bitmap);

            if (NULL != dstCrop) {
                if (bitmap != NULL && !bitmap->isNull()) {
                    bitmap->reset();
                }
                bitmap = dstCrop;
            }
        }
        if (0 != mMovieDegree || 1.0f != mMovieScale) {
            SkBitmap *fill = rotateAndScale(bitmap, mMovieDegree, mMovieScale, mMovieScale, true);
            if (NULL != fill) {
                if (bitmap != NULL && !bitmap->isNull()) {
                    bitmap->reset();
                }
                bitmap = fill;
            }
        }

        MovieRenderPost(bitmap);
        if (bitmap != NULL && !bitmap->isNull()) {
            bitmap->reset();
            bitmap = NULL;
        }
        mFrameIndex++;
        return true;
    }

    bool ImagePlayerService::checkVideoInUse(int retryNum) {
        ALOGD("checkVideoInUse");
        int videoInUseInt = -1;
        char videoInUse[MAX_STR_LEN + 1] = {0};
        std::string vdec;
        char* vdec_empty = "connected vdec list empty";

        do {
            mSysWrite->readSysfs(VIDEO_INUSE, videoInUse);
            readSysfs(std::string(VIDEO_VDEC), vdec);
            videoInUseInt = atoi(videoInUse);
            usleep(100 * 1000);
        } while((retryNum-- != 0) && (videoInUseInt != 0 || (strcmp(vdec.c_str(), vdec_empty) != 0)));

        if (videoInUseInt == 0 && (strcmp(vdec.c_str(), vdec_empty) == 0)) {
            ALOGD("checkVideoInUse succeeds.");
            return true;
        }
        ALOGE("checkVideoInUse fails. %d vdec %s", videoInUseInt, vdec.c_str());
        return false;
    }

    void ImagePlayerService::readSysfs(const std::string& path, std::string& value) {
        if (mSystemControl == nullptr) {
            mSystemControl = ISystemControl::tryGetService();
        }
        if (mSystemControl != nullptr) {
            mSystemControl->readSysfs(path, [&value](const Result &ret, const hidl_string& v) {
                if (Result::OK == ret) {
                    value = v;
                }
            });
            ALOGD("readSysfs path %s value %s", path.c_str(), value.c_str());
            return;
        }
        ALOGE("readSysfs can't get systemcontrol service");

        return;
    }

    void ImagePlayerService::MovieRenderPost(SkBitmap *bitmap) {
        //don't use renderAndShow, too many logs
        //renderAndShow(bitmap);
        FrameInfo_t info;

        if (mDisplayFd < 0) {
            ALOGE("MovieShow, but displayFd can not ready");
            return;
        }
        //render to buffer
        //bitmap->lockPixels();
        info.pBuff = (char*)bitmap->getPixels();
        info.format = VIDEO_LAYER_FORMAT_RGBA;
        info.frame_width = bitmap->width();
        info.frame_height = bitmap->height();

        copy_data_to_mmap_buf(mDisplayFd, info.pBuff,
                        info.frame_width, info.frame_height,
                        info.format);

        ioctl(mDisplayFd, PICDEC_IOC_FRAME_RENDER, &info);
        ALOGE("Movie show w=%d x h=%d", bitmap->width(),bitmap->height());
        //bitmap->unlockPixels();
        if (mNeedResetHWScale) {
            resetHWScale();
            mNeedResetHWScale = false;
        }

        //post to screen
        ioctl(mDisplayFd, PICDEC_IOC_FRAME_POST, NULL);
    }

    int ImagePlayerService::MovieThreadStart() {
        ALOGI("start movie image thread is running:%d", mMovieThread->isRunning());

        status_t result = mMovieThread->run("MovieThread", PRIORITY_URGENT_DISPLAY);

        if (result) {
            ALOGE("Could not start MovieThread due to error %d.", result);
            return RET_ERR_DECORDER;
        }

        return RET_OK;
    }

    int ImagePlayerService::MovieThreadStop() {
        if (mMovieThread != nullptr && mMovieThread->isRunning()) {
            ALOGI("MovieThread is running, need stop it firstly");
            status_t result = mMovieThread->requestExitAndWait();

            if (result) {
                ALOGE("Could not stop MovieThread due to error %d.", result);
                return RET_ERR_DECORDER;
            }
        }

        return RET_OK;
    }
    void ImagePlayerService::clearBmp(SkBitmap* bmp) {
        if (bmp != NULL) {
            if (!bmp->isNull()) {
                bmp->reset();
                delete bmp;
            }
        }

    }
    status_t ImagePlayerService::dump(int fd, const Vector<String16>& args) {
        const size_t SIZE = 256;
        char buffer[SIZE];
        String8 result;

        if (checkCallingPermission(String16("android.permission.DUMP")) == false) {
            snprintf(buffer, SIZE, "Permission Denial: "
                     "can't dump ImagePlayerService from pid=%d, uid=%d\n",
                     IPCThreadState::self()->getCallingPid(),
                     IPCThreadState::self()->getCallingUid());
            result.append(buffer);
        } else {
            Mutex::Autolock lock(mLock);

            result.appendFormat("ImagePlayerService: mDisplayFd:%d, mFileDescription:%d\n",
                                mDisplayFd, mFileDescription);
            result.appendFormat("ImagePlayerService: mImageUrl:%s, mBitmap mWidth:%d, mHeight:%d\n",
                                mImageUrl, mWidth, mHeight);
            result.appendFormat("ImagePlayerService: mSampleSize:%d, surfaceWidth:%d, surfaceHeight:%d\n",
                                mSampleSize, surfaceWidth, surfaceHeight);

            if (NULL != mBufBitmap)
                result.appendFormat("ImagePlayerService: mBufBitmap width:%d, height:%d\n",
                                    mBufBitmap->width(), mBufBitmap->height());

            int n = args.size();

            for (int i = 0; i + 1 < n; i++) {
                String16 option("-d");

                if (args[i] == option) {
                    String8 path(args[i + 1]);

                    if (NULL != mBitmap) {
                        if ((int)mBitmap->height()*mBitmap->rowBytes() < 4 * mBitmap->width()
                                *mBitmap->height()) {
                            result.appendFormat("ImagePlayerService: [error]save origin bitmap RGBA data to file:%s, mBitmap size:%d, request size:%d\n",
                                                path.string(), (int)mBitmap->height()*mBitmap->rowBytes(),
                                                4 * mBitmap->width()*mBitmap->height());
                        } else {
                            RGBA2bmp((char *)mBitmap->getPixels(),
                                     mBitmap->width(), mBitmap->height(), (char *)path.string());
                            result.appendFormat("ImagePlayerService: save origin bitmap RGBA data to file:%s\n",
                                                path.string());
                        }
                    }

                    if (NULL != mBufBitmap) {
                        char bufPath[256] = {0};
                        strcat(bufPath, path.string());
                        strcat(bufPath, "_buf.bmp");

                        if ((int)mBufBitmap->height()*mBufBitmap->rowBytes() < 4 * mBufBitmap->width()
                                *mBufBitmap->height()) {
                            result.appendFormat("ImagePlayerService: [error]save bitmap buffer RGBA data to file:%s, mBufBitmap size:%d, request size:%d\n",
                                                bufPath, (int)mBufBitmap->height()*mBufBitmap->rowBytes(),
                                                4 * mBufBitmap->width()*mBufBitmap->height());
                        } else {
                            RGBA2bmp((char *)mBufBitmap->getPixels(),
                                     mBufBitmap->width(), mBufBitmap->height(), bufPath);
                            result.appendFormat("ImagePlayerService: save bitmap buffer RGBA data to file:%s\n",
                                                bufPath);
                        }
                    }

                    if (NULL != mRotateBitmap) {
                        char bufPath[256] = {0};
                        strcat(bufPath, path.string());
                        strcat(bufPath, "_rotate.bmp");

                        if ((int)mRotateBitmap->height()*mRotateBitmap->rowBytes() < 4 *
                                mRotateBitmap->width()*mRotateBitmap->height()) {
                            result.appendFormat("ImagePlayerService: [error]save rotate RGBA data to file:%s, mRotateBitmap size:%d, request size:%d\n",
                                                bufPath, (int)mRotateBitmap->height()*mRotateBitmap->rowBytes(),
                                                4 * mRotateBitmap->width()*mRotateBitmap->height());
                        } else {
                            RGBA2bmp((char *)mRotateBitmap->getPixels(),
                                     mRotateBitmap->width(), mRotateBitmap->height(), bufPath);
                            result.appendFormat("ImagePlayerService: save rotate RGBA data to file:%s\n",
                                                bufPath);
                        }
                    }

                    if (NULL != mScalingBitmap) {
                        char bufPath[256] = {0};
                        strcat(bufPath, path.string());
                        strcat(bufPath, "_scale.bmp");

                        if ((int)mScalingBitmap->height()*mScalingBitmap->rowBytes() < 4 *
                                mScalingBitmap->width()*mScalingBitmap->height()) {
                            result.appendFormat("ImagePlayerService: [error]save scale RGBA data to file:%s, mScalingBitmap size:%d, request size:%d\n",
                                                bufPath, (int)mScalingBitmap->height()*mScalingBitmap->rowBytes(),
                                                4 * mScalingBitmap->width()*mScalingBitmap->height());
                        } else {
                            RGBA2bmp((char *)mScalingBitmap->getPixels(),
                                     mScalingBitmap->width(), mScalingBitmap->height(), bufPath);
                            result.appendFormat("ImagePlayerService: save scale RGBA data to file:%s\n",
                                                bufPath);
                        }
                    }

                    if (mMovieImage) {
                        char bufPath[256] = {0};
                        strcat(bufPath, path.string());
                        strcat(bufPath, "_movie.png");

                        SkBitmap copy;
                        SkStreamRewindable *stream;
                        stream =  /*dynamic_cast<SkStream*>*/ (getSkStream());

                        if (stream == NULL) {
                            return RET_ERR_BAD_VALUE;
                        }

                        std::unique_ptr<SkStream> s(stream->duplicate());
                        std::unique_ptr<SkCodec> codec(SkCodec::MakeFromStream(std::move(s)));

                        if (!codec) {
                            return false;
                        }

                        std::vector<SkCodec::FrameInfo> frameInfos = codec->getFrameInfo();
                        int frameCount = frameInfos.size() == 0 ? 1 : frameInfos.size();
                        std::vector<SkBitmap> cachedFrames(frameCount);

                        if (mFrameIndex >= frameCount) {
                            mFrameIndex = 0;
                        }

                        SkBitmap bm = cachedFrames[mFrameIndex];

                        if (!bm.getPixels()) {
                            const SkImageInfo info = codec->getInfo().makeColorType(kN32_SkColorType);
                            bm.allocPixels(info);
                            SkCodec::Options opts;
                            opts.fFrameIndex = mFrameIndex;
                            //opts.fHasPriorFrame = false;
                            const size_t requiredFrame = frameInfos[mFrameIndex].fRequiredFrame;

                            if (requiredFrame != SkCodec::kNone) {
                                // For simplicity, do not try to cache old frames
                                SkBitmap::HeapAllocator allocator;

                                if (copyTo(&bm, kN32_SkColorType, cachedFrames[requiredFrame], &allocator)) {
                                    //opts.fHasPriorFrame = true;
                                }
                            }

                            if (SkCodec::kSuccess != codec->getPixels(info, bm.getPixels(),
                                    bm.rowBytes(), &opts)) {
                                ALOGD("Could not getPixels for frame %d", mFrameIndex);
                                return false;
                            }
                        }

                        SkBitmap::HeapAllocator allocator;
                        copyTo(&copy, kN32_SkColorType, bm, &allocator);
                        SkFILEWStream file(bufPath);

                        if (!SkEncodeImage(&file, copy, SkEncodedImageFormat::kPNG, 100)) {
                            result.appendFormat("ImagePlayerService: [error]encode to png file:%s\n",
                                                bufPath);
                        } else {
                            result.appendFormat("ImagePlayerService: encode to png file:%s, w:%d, h:%d\n",
                                                bufPath, copy.width(), copy.height());
                        }
                    }
                }
            }
        }

        write(fd, result.string(), result.size());
        return NO_ERROR;
    }

    // --- MovieThread ---
    MovieThread::MovieThread(const sp<ImagePlayerService>& player)
        : Thread(/*canCallJava*/ false), mPlayer(player) {
        ALOGI("MovieThread construtor");
    }

    MovieThread::~MovieThread() {
        ALOGI("~MovieThread");
    }
    bool MovieThread::check() {
       return exitPending();
    }
    // Good place to do one-time initializations
    status_t MovieThread::readyToRun() {
        return NO_ERROR;
    }

    /*
        1) loop: if returns true, it will be called again if requestExit() wasn't called.
        2) once: if returns false, the thread will exit.
    */
    bool MovieThread::threadLoop() {
        usleep(40 * 1000); //delay 40ms
        return mPlayer->MovieShow();
    }
}
