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



#ifndef V4L_CAM_ADPT_H
#define V4L_CAM_ADPT_H

#include "CameraHal.h"
#include "BaseCameraAdapter.h"
#include "DebugUtils.h"
#include "Encoder_libjpeg.h"
#include "V4LCameraAdapter.h"


namespace android {

#ifndef DEFAULT_PREVIEW_PIXEL_FORMAT
#define DEFAULT_PREVIEW_PIXEL_FORMAT        V4L2_PIX_FMT_NV21
#define DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT  V4L2_PIX_FMT_RGB24
#endif
#define NB_BUFFER 6

/**
  * Class which completely abstracts the camera hardware interaction from camera hal
  * TODO: Need to list down here, all the message types that will be supported by this class
  */
class V4LCamAdpt : public BaseCameraAdapter
{
public:

    /*--------------------Constant declarations----------------------------------------*/
    static const int32_t MAX_NO_BUFFERS = 20;

    //static const int MAX_NO_PORTS = 6;

    ///Five second timeout
    static const int CAMERA_ADAPTER_TIMEOUT = 5000*1000;

public:

    V4LCamAdpt(size_t sensor_index);
    ~V4LCamAdpt();

    int SetExposure(int camera_fd,const char *sbn);
    int SetExposureMode(int camera_fd, unsigned int mode);
    int set_white_balance(int camera_fd,const char *swb);
    int set_banding(int camera_fd,const char *snm);
    int set_night_mode(int camera_fd,const char *snm);
	int set_effect(int camera_fd,const char *sef);
	int set_flash_mode(int camera_fd, const char *sfm);
	bool get_flash_mode( char *flash_status,
						char *def_flash_status);

	int getValidFrameSize(int pixel_format, char *framesize);
	int getCameraOrientation(bool frontcamera, char* property);
	bool getCameraWhiteBalance(char* wb_modes, char*def_wb_mode);
	bool getCameraBanding(char* banding_modes, char*def_banding_mode);
	bool getCameraExposureValue(int &min, int &max,
						  int &step, int &def);
	bool getCameraAutoFocus( char* focus_mode_str, char*def_focus_mode);
	int set_hflip_mode(int camera_fd, bool mode);
	int get_hflip_mode(int camera_fd);
	int get_supported_zoom(int camera_fd, char * zoom_str);
	int set_zoom_level(int camera_fd, int zoom);

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
	int get_framerate (int camera_fd,int *fps, int *fps_num);
	int enumFramerate ( int *fps, int *fps_num);
#endif
#if 1//ndef AMLOGIC_USB_CAMERA_SUPPORT
	int set_rotate_value(int camera_fd, int value);
#endif

	bool isPreviewDevice(int camera_fd);
    bool isFrontCam( int camera_id );
    bool isVolatileCam();
    bool getCameraHandle();

    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*);
    //virtual status_t initialize(CameraProperties::Properties*, int sensor_index=0);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params);
    virtual void getParameters(CameraParameters& params);

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
            V4LCamAdpt* mAdapter;
        public:
            PreviewThread(V4LCamAdpt* hw) :
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
    status_t getBuffersFormat(int &width, int &height, int &pixelformat);

    //Used for calculation of the average frame rate during preview
    status_t recalculateFPS();

    char * GetFrame(int &index);

    int previewThread();

    static int beginPictureThread(void *cookie);
    int pictureThread();

    static int beginAutoFocusThread(void *cookie);

    int GenExif(ExifElementsTable* exiftable);

    status_t IoctlStateProbe();

public:

private:
    int mPreviewBufferCount;
    KeyedVector<int, int> mPreviewBufs;
    KeyedVector<int, int> mPreviewIdxs;
    mutable Mutex mPreviewBufsLock;

    //TODO use members from BaseCameraAdapter
    camera_memory_t *mCaptureBuf;

    CameraParameters mParams;

    int mPreviewWidth;
    int mPreviewHeight;
    int mCaptureWidth;
    int mCaptureHeight;

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

#ifdef AMLOGIC_TWO_CH_UVC
    int mCamEncodeHandle;
    int mCamEncodeIndex;
#endif

    int nQueued;
    int nDequeued;

    int mZoomlevel;
    unsigned int mPixelFormat;

#if 0//def AMLOGIC_USB_CAMERA_SUPPORT
    int mUsbCameraStatus;
    
    bool mIsDequeuedEIOError;

    enum UsbCameraStatus
    {
        USBCAMERA_NO_INIT,
        USBCAMERA_INITED,
        USBCAMERA_ACTIVED
    };
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

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
    int mPreviewFrameRate;
    struct timeval previewTime1, previewTime2;
#endif
#if 1//ndef AMLOGIC_USB_CAMERA_SUPPORT
    int mRotateValue;
#endif
};
}; //// namespace
#endif //V4L_CAMERA_ADAPTER_H

