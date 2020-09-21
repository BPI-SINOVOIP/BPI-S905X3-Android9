/** @file ImagePlayerService.h
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

#ifndef ANDROID_IMAGEPLAYERSERVICE_H
#define ANDROID_IMAGEPLAYERSERVICE_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Vector.h>
#include <utils/Thread.h>
#include <media/MediaPlayerInterface.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include "SkBRDAllocator.h"
#include "SkCodec.h"
#include "IImagePlayerService.h"
#include "TIFF2RGBA.h"
#include <binder/Binder.h>
#include "SysWrite.h"
#include <vendor/amlogic/hardware/systemcontrol/1.1/ISystemControl.h>
#include <vendor/amlogic/hardware/systemcontrol/1.0/types.h>

#define MAX_FILE_PATH_LEN           1024
#define MAX_PIC_SIZE                8000

using ::vendor::amlogic::hardware::systemcontrol::V1_1::ISystemControl;

namespace android {

    class MovieThread;
    class DeathNotifier;

    typedef struct {
        char* pBuff;
        int frame_width;
        int frame_height;
        int format;
        int rotate;
    } FrameInfo_t;

    struct InitParameter {
        float degrees;
        float scaleX;
        float scaleY;
        int cropX;
        int cropY;
        int cropWidth;
        int cropHeight;
    };

    enum RetType {
        RET_OK_OPERATION_SCALE_MIN      = 2,
        RET_OK_OPERATION_SCALE_MAX      = 1,
        RET_OK                          = 0,
        RET_ERR_OPEN_SYSFS              = -1,
        RET_ERR_OPEN_FILE               = -2,
        RET_ERR_INVALID_OPERATION       = -3,
        RET_ERR_DECORDER                = -4,
        RET_ERR_PARAMETER               = -5,
        RET_ERR_BAD_VALUE               = -6,
        RET_ERR_NO_MEMORY               = -7
    };

    enum ScaleDirect {
        SCALE_NORMAL                    = 0,
        SCALE_UP                        = 1,
        SCALE_DOWN                      = 2,
    };

    enum TranslateDirect {
        TRANSLATE_NORMAL                    = 0,
        TRANSLATE_LEFT                      = 1,
        TRANSLATE_RIGHT                     = 2,
        TRANSLATE_UP                        = 3,
        TRANSLATE_DOWN                      = 4,
        TRANSLATE_LEFTUP                    = 5,
        TRANSLATE_LEFTDOWN                  = 6,
        TRANSLATE_RIGHTUP                   = 7,
        TRANSLATE_RIGHTDOWN                 = 8,
    };

    /*
    class HeapAllocator : public SkBRDAllocator {
    public:
       HeapAllocator() { };
        ~HeapAllocator() { };

        virtual bool allocPixelRef(SkBitmap* bitmap) override;

        Bitmap* getStorageObjAndReset() {
            return mStorage.release();
        };

        SkCodec::ZeroInitialized zeroInit() const override { return SkCodec::kYes_ZeroInitialized; }
    private:
        sk_sp<Bitmap> mStorage;
    };
    */

    class ImagePlayerService :  public BnImagePlayerService {
      public:
        ImagePlayerService();
        virtual ~ImagePlayerService();

        virtual int init();
        virtual int setDataSource(const char* uri);
        virtual int setDataSource (
            const sp<IMediaHTTPService> &httpService,
            const char *srcUrl);
        virtual int setDataSource(int fd, int64_t offset, int64_t length);
        virtual int setSampleSurfaceSize(int sampleSize, int surfaceW, int surfaceH);
        virtual int setRotate(float degrees, int autoCrop) ;
        virtual int setScale(float sx, float sy, int autoCrop);
        virtual int setHWScale(float sc);
        virtual int setTranslate(float tx, float ty);
        virtual int setRotateScale(float degrees, float sx, float sy, int autoCrop);
        virtual int setCropRect(int cropX, int cropY, int cropWidth, int cropHeight);
        virtual int start();
        virtual int prepare();
        virtual int show();
        virtual int showImage(const char* uri);
        virtual int release();
        static ImagePlayerService* instantiate();
        virtual int notifyProcessDied(const sp<IBinder> &binder);

        //use to show gif etc. images
        bool MovieInit(const char *file);
        bool MovieShow();
        void MovieRenderPost(SkBitmap *bitmap);
        int MovieThreadStart();
        int MovieThreadStop();

        virtual status_t dump(int fd, const Vector<String16>& args);

      private:
        void initVideoAxis();
        int convertRGBA8888toRGB(void *dst, const SkBitmap *src);
        int convertARGB8888toYUYV(void *dst, const SkBitmap *src);
        int convertRGB565toYUYV(void *dst, const SkBitmap *src);
        int convertIndex8toYUYV(void *dst, const SkBitmap *src);
        SkBitmap* preCropAndRotate(SkBitmap *src, float degrees);
        int post();
        int render(int format, SkBitmap *bitmap);
        bool copyTo(SkBitmap* dst, SkColorType dstCT, const SkBitmap& src,
                    SkBitmap::Allocator* alloc);
        SkStreamAsset* getSkStream();
        SkStreamAsset* getStreamForHttps(char* url);
        SkBitmap* decode(SkStreamAsset *stream, InitParameter *parameter);
        SkBitmap* decodeTiff(const char *filePath);
        SkBitmap* scale(SkBitmap *srcBitmap, float sx, float sy);
        SkBitmap* simplescale(SkBitmap *srcBitmap, float sx, float sy);
        SkBitmap* rotate(SkBitmap *srcBitmap, float degrees);
        SkBitmap* rotateAndScale(SkBitmap *srcBitmap, float degrees, float sx,
                                 float sy, bool isCrop);
        void clearBmp(SkBitmap* bmp);
        bool needFillSurface(SkBitmap* bmp,float sx,float sy);
        bool renderAndShow(SkBitmap *bitmap);
        bool showBitmapRect(SkBitmap *bitmap, int cropX, int cropY, int cropWidth,
                            int cropHeight);
        void resetRotateScale();
        void resetTranslate();
        void resetHWScale();
        void isTranslateToEdge(SkBitmap *srcBitmap, int dstWidth, int dstHeight, int tx,
                               int ty);
        SkBitmap* scaleStep(SkBitmap *srcBitmap, float sx, float sy);
        SkBitmap* scaleAndCrop(SkBitmap *srcBitmap, float sx, float sy);
        SkBitmap* fillSurface(SkBitmap *bitmap);
        bool isSupportFromat(const char *uri, SkBitmap **bitmap);
        bool checkVideoInUse(int retryNum);
        void readSysfs(const std::string& path, std::string& value);
        void covert8to32(SkBitmap& src, SkBitmap *dstBitmap);
        TIFF2RGBA *mTif;

        mutable Mutex mLock;
        int mWidth, mHeight;
        SkBitmap *mBitmap;
        SkBitmap *mBufBitmap;
        // sample-size, if set to > 1, tells the decoder to return a smaller than
        // original bitmap, sampling 1 pixel for every size pixels. e.g. if sample
        // size is set to 3, then the returned bitmap will be 1/3 as wide and high,
        // and will contain 1/9 as many pixels as the original.
        int mSampleSize;

        char mImageUrl[MAX_FILE_PATH_LEN];
        int mFileDescription;
        //bool isAutoCrop;
        int surfaceWidth, surfaceHeight;
        //0:normal 1: scale up 2:scale down
        int mScalingDirect;
        float mScalingStep;
        SkBitmap *mScalingBitmap;
        SkBitmap *mRotateBitmap;
        bool mMovieImage;
        int mFrameIndex;
        float mMovieDegree;
        bool mNeedResetHWScale;
        //0:normal 1: translate left 2:translate right 3: translate up 4:translate down
        int mTranslatingDirect;
        bool mTranslateImage;
        bool mTranslateToXLEdge;  //Translate To X Left Edge
        bool mTranslateToXREdge;  //Translate To X Right Edge
        bool mTranslateToYTEdge;  //Translate To Y Top Edge
        bool mTranslateToYBEdge;  //Translate To Y Bottom Edge
        int mTx;
        int mTy;
        float mMovieScale;
        sp<MovieThread> mMovieThread;

        InitParameter *mParameter;
        int mDisplayFd;

        sp<IMediaHTTPService> mHttpService;
        sp<DeathNotifier> mDeathNotifier;
        SysWrite* mSysWrite;
        sp<ISystemControl> mSystemControl;
    };
    class MovieThread : public Thread {
      public:
        MovieThread(const sp<ImagePlayerService>& player);
        virtual ~MovieThread();

        bool check();
      private:
        sp<ImagePlayerService> mPlayer;

        virtual status_t readyToRun();
        virtual bool threadLoop();
    };
}  // namespace android

#endif // ANDROID_IMAGEPLAYERSERVICE_H
