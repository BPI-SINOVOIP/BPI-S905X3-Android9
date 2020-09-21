/*
 * Copyright (C) 2011 The Android Open Source Project
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



#ifndef V4L_CAMERA_ADAPTER_H
#define V4L_CAMERA_ADAPTER_H

#include "CameraHal.h"
#include "BaseCameraAdapter.h"
#include "DebugUtils.h"
#include "Encoder_libjpeg.h"
#include <ion/ion.h>

namespace android {

#ifdef AMLOGIC_USB_CAMERA_SUPPORT

#define DEFAULT_PREVIEW_PIXEL_FORMAT        V4L2_PIX_FMT_NV21
//#define DEFAULT_PREVIEW_PIXEL_FORMAT        V4L2_PIX_FMT_YUYV
#define DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT  V4L2_PIX_FMT_RGB24
//#define DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT  V4L2_PIX_FMT_NV21
#else
#define DEFAULT_PREVIEW_PIXEL_FORMAT        V4L2_PIX_FMT_NV21
#define DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT  V4L2_PIX_FMT_RGB24
//#define DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT  V4L2_PIX_FMT_NV21
#endif
#define NB_BUFFER 6

#define MAX_LIMITED_RATE_NUM 6
//#define ENCODE_TIME_DEBUG
//#define PRODUCE_TIME_DEBUG

typedef enum device_type_e{
        DEV_MMAP = 0,
        DEV_ION,
        DEV_ION_MPLANE,
        DEV_DMA,
        DEV_CANVAS_MODE,
        DEV_USB,
}device_type_t;

struct VideoInfo {
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    unsigned int canvas[NB_BUFFER];
    bool isStreaming;
    bool canvas_mode;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
    uint32_t idVendor;
    uint32_t idProduct;
};

typedef enum camera_mode_e{
        CAM_PREVIEW = 0,
        CAM_CAPTURE,
        CAM_RECORD,
}camera_mode_t;

struct camera_fmt {
        uint32_t pixelfmt;
        int support;
} camera_fmt_t;

typedef enum camera_light_mode_e {
    ADVANCED_AWB = 0,
    SIMPLE_AWB,
    MANUAL_DAY,
    MANUAL_A,
    MANUAL_CWF,
    MANUAL_CLOUDY,
}camera_light_mode_t;

typedef enum camera_saturation_e {
    SATURATION_N4_STEP = 0,
    SATURATION_N3_STEP,
    SATURATION_N2_STEP,
    SATURATION_N1_STEP,
    SATURATION_0_STEP,
    SATURATION_P1_STEP,
    SATURATION_P2_STEP,
    SATURATION_P3_STEP,
    SATURATION_P4_STEP,
}camera_saturation_t;


typedef enum camera_brightness_e {
    BRIGHTNESS_N4_STEP = 0,
    BRIGHTNESS_N3_STEP,
    BRIGHTNESS_N2_STEP,
    BRIGHTNESS_N1_STEP,
    BRIGHTNESS_0_STEP,
    BRIGHTNESS_P1_STEP,
    BRIGHTNESS_P2_STEP,
    BRIGHTNESS_P3_STEP,
    BRIGHTNESS_P4_STEP,
}camera_brightness_t;

typedef enum camera_contrast_e {
    CONTRAST_N4_STEP = 0,
    CONTRAST_N3_STEP,
    CONTRAST_N2_STEP,
    CONTRAST_N1_STEP,
    CONTRAST_0_STEP,
    CONTRAST_P1_STEP,
    CONTRAST_P2_STEP,
    CONTRAST_P3_STEP,
    CONTRAST_P4_STEP,
}camera_contrast_t;

typedef enum camera_hue_e {
    HUE_N180_DEGREE = 0,
    HUE_N150_DEGREE,
    HUE_N120_DEGREE,
    HUE_N90_DEGREE,
    HUE_N60_DEGREE,
    HUE_N30_DEGREE,
    HUE_0_DEGREE,
    HUE_P30_DEGREE,
    HUE_P60_DEGREE,
    HUE_P90_DEGREE,
    HUE_P120_DEGREE,
    HUE_P150_DEGREE,
}camera_hue_t;

typedef enum camera_special_effect_e {
    SPECIAL_EFFECT_NORMAL = 0,
    SPECIAL_EFFECT_BW,
    SPECIAL_EFFECT_BLUISH,
    SPECIAL_EFFECT_SEPIA,
    SPECIAL_EFFECT_REDDISH,
    SPECIAL_EFFECT_GREENISH,
    SPECIAL_EFFECT_NEGATIVE,
}camera_special_effect_t;

typedef enum camera_exposure_e {
    EXPOSURE_N4_STEP = 0,
    EXPOSURE_N3_STEP,
    EXPOSURE_N2_STEP,
    EXPOSURE_N1_STEP,
    EXPOSURE_0_STEP,
    EXPOSURE_P1_STEP,
    EXPOSURE_P2_STEP,
    EXPOSURE_P3_STEP,
    EXPOSURE_P4_STEP,
}camera_exposure_t;


typedef enum camera_sharpness_e {
    SHARPNESS_1_STEP = 0,
    SHARPNESS_2_STEP,
    SHARPNESS_3_STEP,
    SHARPNESS_4_STEP,
    SHARPNESS_5_STEP,
    SHARPNESS_6_STEP,
    SHARPNESS_7_STEP,
    SHARPNESS_8_STEP,
    SHARPNESS_AUTO_STEP,
}camera_sharpness_t;

typedef enum camera_mirror_flip_e {
    MF_NORMAL = 0,
    MF_MIRROR,
    MF_FLIP,
    MF_MIRROR_FLIP,
}camera_mirror_flip_t;


typedef enum camera_wb_flip_e {
    CAM_WB_AUTO = 0,
    CAM_WB_CLOUD,
    CAM_WB_DAYLIGHT,
    CAM_WB_INCANDESCENCE,
    CAM_WB_TUNGSTEN,
    CAM_WB_FLUORESCENT,
    CAM_WB_MANUAL,
    CAM_WB_SHADE,
    CAM_WB_TWILIGHT,
    CAM_WB_WARM_FLUORESCENT,
}camera_wb_flip_t;
typedef enum camera_night_mode_flip_e {
    CAM_NM_AUTO = 0,
	CAM_NM_ENABLE,
}camera_night_mode_flip_t;
typedef enum camera_banding_mode_flip_e {
    	CAM_ANTIBANDING_DISABLED= V4L2_CID_POWER_LINE_FREQUENCY_DISABLED,
	CAM_ANTIBANDING_50HZ	= V4L2_CID_POWER_LINE_FREQUENCY_50HZ,
	CAM_ANTIBANDING_60HZ 	= V4L2_CID_POWER_LINE_FREQUENCY_60HZ,
	CAM_ANTIBANDING_AUTO,
    	CAM_ANTIBANDING_OFF,
}camera_banding_mode_flip_t;

typedef enum camera_effect_flip_e {
    CAM_EFFECT_ENC_NORMAL = 0,
	CAM_EFFECT_ENC_GRAYSCALE,
	CAM_EFFECT_ENC_SEPIA,
	CAM_EFFECT_ENC_SEPIAGREEN,
	CAM_EFFECT_ENC_SEPIABLUE,
	CAM_EFFECT_ENC_COLORINV,
}camera_effect_flip_t;

typedef enum camera_flashlight_status_e{
	FLASHLIGHT_AUTO = 0,
	FLASHLIGHT_ON,
	FLASHLIGHT_OFF,
	FLASHLIGHT_TORCH,
	FLASHLIGHT_RED_EYE,
}camera_flashlight_status_t;

typedef enum camera_focus_mode_e {
    CAM_FOCUS_MODE_RELEASE = 0,
    CAM_FOCUS_MODE_FIXED,
    CAM_FOCUS_MODE_INFINITY,
    CAM_FOCUS_MODE_AUTO,
    CAM_FOCUS_MODE_MACRO,
    CAM_FOCUS_MODE_EDOF,
    CAM_FOCUS_MODE_CONTI_VID,
    CAM_FOCUS_MODE_CONTI_PIC,
}camera_focus_mode_t;

typedef struct cam_cache_buf{
    char *bufPtr;
    int index;
    unsigned canvas;
}cache_buf_t;

typedef struct cam_LimitedRate_Item{
    int width;
    int height;
    int framerate;
}RateInfo_t;

typedef struct cam_LimitedRate_Info{
    int num;
    RateInfo_t arg[MAX_LIMITED_RATE_NUM];
}LimitedRate_t;

#define V4L2_ROTATE_ID 0x980922  //V4L2_CID_ROTATE

#define V4L2_CID_AUTO_FOCUS_STATUS              (V4L2_CID_CAMERA_CLASS_BASE+30)
#define V4L2_AUTO_FOCUS_STATUS_IDLE             (0 << 0)
#define V4L2_AUTO_FOCUS_STATUS_BUSY             (1 << 0)
#define V4L2_AUTO_FOCUS_STATUS_REACHED          (1 << 1)
#define V4L2_AUTO_FOCUS_STATUS_FAILED           (1 << 2)


#define	IOCTL_MASK_HFLIP	(1<<0)
#define	IOCTL_MASK_ZOOM		(1<<1)
#define IOCTL_MASK_FLASH	(1<<2)
#define IOCTL_MASK_FOCUS	(1<<3)
#define IOCTL_MASK_WB		(1<<4)
#define IOCTL_MASK_EXPOSURE	(1<<5)
#define IOCTL_MASK_EFFECT	(1<<6)
#define IOCTL_MASK_BANDING	(1<<7)
#define IOCTL_MASK_ROTATE	(1<<8)
#define IOCTL_MASK_FOCUS_MOVE	(1<<9)

/**
  * Class which completely abstracts the camera hardware interaction from camera hal
  * TODO: Need to list down here, all the message types that will be supported by this class
  */
class V4LCameraAdapter : public BaseCameraAdapter
{
public:

    /*--------------------Constant declarations----------------------------------------*/
    static const int32_t MAX_NO_BUFFERS = 20;

    //static const int MAX_NO_PORTS = 6;

    ///Five second timeout
    static const int CAMERA_ADAPTER_TIMEOUT = 5000*1000;

public:

    V4LCameraAdapter(size_t sensor_index);
    ~V4LCameraAdapter();

    int SetExposure(int camera_fd,const char *sbn);
    int SetExposureMode(int camera_fd, unsigned int mode);
    int set_white_balance(int camera_fd,const char *swb);
    int set_focus_area(int camera_fd, const char *focusarea);
    int set_banding(int camera_fd,const char *snm);
    status_t allocImageIONBuf(CameraProperties::Properties* caps);

    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*);
    //virtual status_t initialize(CameraProperties::Properties*, int sensor_index=0);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params);
    virtual void getParameters(CameraParameters& params);
    virtual status_t sendCommand(CameraCommands operation, int value1 = 0, int value2 = 0, int value3 = 0 );
    // API
    virtual status_t UseBuffersPreview(void* bufArr, int num);
    virtual status_t UseBuffersCapture(void* bufArr, int num);

    //API to flush the buffers for preview
    status_t flushBuffers();

protected:

//----------Parent class method implementation------------------------------------
    virtual status_t takePicture();
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t startPreview();
    virtual status_t stopPreview();
    virtual status_t useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable);
    virtual status_t fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType);
    virtual status_t getFrameSize(size_t &width, size_t &height);
    virtual status_t getPictureBufferSize(size_t &length, size_t bufferCount);
    virtual status_t getFrameDataSize(size_t &dataFrameSize, size_t bufferCount);
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt);
//-----------------------------------------------------------------------------
	status_t 		disableMirror(bool bDisable);
	status_t 		setMirrorEffect();
	status_t 		getFocusMoveStatus();

private:

    class PreviewThread : public Thread {
            V4LCameraAdapter* mAdapter;
        public:
            PreviewThread(V4LCameraAdapter* hw) :
                    Thread(false), mAdapter(hw) { }
            virtual void onFirstRef() {
                run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
            }
            virtual bool threadLoop() {
                mAdapter->previewThread();
                // loop until we need to quit
                return true;
            }
        };

    status_t setBuffersFormat(int width, int height, int pixelformat);
    status_t tryBuffersFormat(int width, int height, int pixelformat);
    status_t setCrop(int width, int height);
    status_t getBuffersFormat(int &width, int &height, int &pixelformat);

    //Used for calculation of the average frame rate during preview
    status_t recalculateFPS();

    char * GetFrame(int &index, unsigned int* canvas);

    int previewThread();

    static int beginPictureThread(void *cookie);
    int pictureThread();

    static int beginAutoFocusThread(void *cookie);

    int GenExif(ExifElementsTable* exiftable);

    status_t IoctlStateProbe();
    
    status_t force_reset_sensor();

public:

private:
    int mPreviewBufferCount;
    KeyedVector<int, int> mPreviewBufs;
    KeyedVector<int, int> mPreviewIdxs;
    mutable Mutex mPreviewBufsLock;

    //TODO use members from BaseCameraAdapter
    camera_memory_t *mCaptureBuf;
    int                 mImageFd;
    int                 mIonFd;
    ion_user_handle_t   mIonHnd;

    CameraParameters mParams;

    int mPreviewWidth;
    int mPreviewHeight;
    int mCaptureWidth;
    int mCaptureHeight;
    int mPreviewOriation;
    int mCaptureOriation;
    bool mPreviewing;
    bool mCapturing;
    Mutex mLock;

    int mFrameCount;
    int mLastFrameCount;
    unsigned int mIter;
    nsecs_t mLastFPSTime;

    //variables holding the estimated framerate
    float mFPS, mLastFPS;

    int mSensorIndex;
    bool mbFrontCamera;
    bool mbDisableMirror;

    // protected by mLock
    sp<PreviewThread>   mPreviewThread;

    struct VideoInfo *mVideoInfo;
    int mCameraHandle;
#ifdef ION_MODE_FOR_METADATA_MODE
    bool ion_mode;
    int mIonClient;
    unsigned int mPhyAddr[NB_BUFFER];
#endif
    enum device_type_e  m_eDeviceType;
    enum v4l2_memory    m_eV4l2Memory;

#ifdef AMLOGIC_TWO_CH_UVC
    int mCamEncodeHandle;
    int mCamEncodeIndex;
#endif

    int nQueued;
    int nDequeued;

    int mZoomlevel;
    unsigned int mPixelFormat;
    unsigned int mSensorFormat;	

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    bool mIsDequeuedEIOError;
#endif
    //int maxQueueable;//the max queued buffers in v4l

    camera_focus_mode_t cur_focus_mode;	
    camera_focus_mode_t cur_focus_mode_for_conti;
    bool bFocusMoveState;

    bool mEnableContiFocus;
    camera_flashlight_status_t mFlashMode;
    unsigned int mIoctlSupport;

    int mWhiteBalance;
    int mEV;
    int mEVdef;
    int mEVmin;
    int mEVmax;
    int mAntiBanding;
    int mFocusWaitCount;
    //suppose every 17frames to check the focus is running;
    //in continuous mode
    static const int FOCUS_PROCESS_FRAMES = 17;

#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
    struct timeval previewTime1, previewTime2;
    bool mFirstBuff;
    int mFrameInvAdjust;		
    int mFrameInv;
    cache_buf_t mCache;
#endif
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    int mRotateValue;
#endif
    LimitedRate_t LimitedRate;
#ifdef PREVIEW_TIME_DEBUG
    DurationTimer preTimer;
    int precount;
#endif
    int mLimitedFrameRate;
    unsigned mExpectedFrameInv;
    bool mUseMJPEG;
    bool mSupportMJPEG;
    bool mDebugMJPEG;
    int mFramerate;
    unsigned mFailedCnt;
    unsigned mEagainCnt;
    uint32_t *mPreviewCache;
    uint32_t mResetTH;
    struct timeval mStartTime;
    struct timeval mEndTime;
    struct timeval mEagainStartTime;
    struct timeval mEagainEndTime;
    bool mEnableDump;
    int mDumpCnt;
};
}; //// namespace
#endif //V4L_CAMERA_ADAPTER_H

