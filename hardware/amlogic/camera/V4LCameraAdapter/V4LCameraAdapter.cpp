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

/**
* @file V4LCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "CAMHAL_V4LCameraAdapter"
//reinclude because of a bug with the log macros
#include <utils/Log.h>
#include "DebugUtils.h"

#include "V4LCameraAdapter.h"
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
#include "V4LCamAdpt.h"
#endif
#include "CameraHal.h"
#include "ExCameraParameters.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>
#include <sys/time.h>

#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "CameraHal.h"
extern "C"{
    #include "usb_fmt.h"
}
#include "libyuv.h"

//for private_handle_t TODO move out of private header
#include <ion/ion.h>
#include <gralloc_priv.h>
#include <MetadataBufferType.h>

static int iCamerasNum = -1;

#ifdef ION_MODE_FOR_METADATA_MODE
#define ION_IOC_MESON_PHYS_ADDR 8

struct meson_phys_data{
    int handle;
    unsigned int phys_addr;
    unsigned int size;
};
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif
#ifdef AMLOGIC_USB_CAMERA_SUPPORT

const char *SENSOR_PATH[]={ 
		    "/dev/video0",
		    "/dev/video1",
		    "/dev/video2",
		};
#define DEVICE_PATH(_sensor_index) (SENSOR_PATH[_sensor_index])
#else
#define DEVICE_PATH(_sensor_index) (_sensor_index == 0 ? "/dev/video0" : "/dev/video1")
#endif
#define DUMP_FILE "/data/preview_dump"
namespace android {

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

Mutex gAdapterLock;

extern "C" int set_night_mode(int camera_fd,const char *snm);
extern "C" int set_effect(int camera_fd,const char *sef);
extern "C" int SYS_set_zoom(int zoom);
extern "C" int set_flash_mode(int camera_fd, const char *sfm);
static bool get_flash_mode(int camera_fd, char *flash_status,
					char *def_flash_status);

static int set_hflip_mode(int camera_fd, bool mode);
static int get_hflip_mode(int camera_fd);
static int get_supported_zoom(int camera_fd, char * zoom_str);
static int set_zoom_level(int camera_fd, int zoom);
static bool is_mjpeg_supported(int camera_fd);
static void ParserLimitedRateInfo(LimitedRate_t* rate);
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
extern "C" int get_framerate (int camera_fd,int *fps, int *fps_num);
extern "C" int enumFramerate ( int camera_fd, int *fps, int *fps_num);
#endif
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
static int set_rotate_value(int camera_fd, int value);
#endif

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
#ifdef AMLOGIC_TWO_CH_UVC
extern "C" bool isPreviewDevice(int camera_fd);
extern "C" status_t getVideodevId(int &camera_id, int &main_id);
#endif
#endif
/*--------------------junk STARTS here-----------------------------*/
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
#define SYSFILE_CAMERA_SET_PARA "/sys/class/vm/attr2"
#define SYSFILE_CAMERA_SET_MIRROR "/sys/class/vm/mirror"
static int writefile(char* path,char* content)
{
    FILE* fp = fopen(path, "w+");

    CAMHAL_LOGDB("Write file %s(%p) content %s", path, fp, content);

    if (fp){
        while( ((*content) != '\0') ) {
            if (EOF == fputc(*content,fp)){
               CAMHAL_LOGDA("write char fail");
            }
            content++;
        }
        fclose(fp);
    }else{
        CAMHAL_LOGDA("open file fail\n");
    }
    return 1;
}
#ifndef CAMHAL_USER_MODE
//
//usage
//+    char property1[80];
//+
//+    readfile((char*)SYSFILE_CAMERA_SET_MIRROR, property1);
//+    CAMHAL_LOGDB("mirror =%s\n", property1);
//
static int readfile(char *path,char *content)
{
        char *tmp=content;

        FILE *fp = fopen(path,"r");

        if(fp == NULL) {
                CAMHAL_LOGDA("readfile open fail");
                return -1;
        }
        int ch;
        while ((ch=fgetc(fp)) != EOF ) {
                *content = (char)ch;
                content++;
        }
        fclose(fp);
        *content='\0';

        return  0;
}
#endif
#endif
/*--------------------Camera Adapter Class STARTS here-----------------------------*/
status_t V4LCameraAdapter::sendCommand(CameraCommands operation, int value1, int value2, int value3) {
    if(operation==CAMERA_APK) {
        mPreviewOriation=value1;
        mCaptureOriation=value2;
        return 1; 
    }else{
        return BaseCameraAdapter::sendCommand(operation,  value1,  value2, value3); 
    }
}

status_t V4LCameraAdapter::initialize(CameraProperties::Properties* caps)
{
    LOG_FUNCTION_NAME;

    //char value[PROPERTY_VALUE_MAX];
    //property_get("debug.camera.showfps", value, "0");

    int ret = NO_ERROR;
    int oflag = O_RDWR;

#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
    oflag = O_RDWR | O_NONBLOCK;
#endif

    // Allocate memory for video info structure
    mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
    if(!mVideoInfo){
        return NO_MEMORY;
    }

    memset(mVideoInfo,0,sizeof(struct VideoInfo));

#ifdef ION_MODE_FOR_METADATA_MODE
    ion_mode = false;
    mIonClient = -1;
    memset(mPhyAddr, 0, sizeof(mPhyAddr));
#endif

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
#ifdef AMLOGIC_TWO_CH_UVC
    mCamEncodeIndex = -1;
    mCamEncodeHandle = -1;
    ret = getVideodevId( mSensorIndex, mCamEncodeIndex);
    if(NO_ERROR == ret){
        if ((mCameraHandle = open(DEVICE_PATH(mSensorIndex), oflag )) != -1){
            CAMHAL_LOGDB("open %s success to preview\n", DEVICE_PATH(mSensorIndex));
        }
        if ((0<= mCamEncodeIndex)&& (mCamEncodeIndex < (int)ARRAY_SIZE(SENSOR_PATH))&&
          ((mCamEncodeHandle = open(DEVICE_PATH(mCamEncodeIndex), O_RDWR)) != -1)){
            CAMHAL_LOGDB("open %s success to encode\n", DEVICE_PATH(mCamEncodeIndex));
        }
    }
#else
    while(mSensorIndex < (int)ARRAY_SIZE(SENSOR_PATH)){
        if ((mCameraHandle = open(DEVICE_PATH(mSensorIndex), oflag)) != -1){
            CAMHAL_LOGDB("open %s success!\n", DEVICE_PATH(mSensorIndex));
            break;
        }
        mSensorIndex++;
    }
    if(mSensorIndex >= (int)ARRAY_SIZE(SENSOR_PATH)){
        CAMHAL_LOGEB("opening %dth Camera, error: %s", mSensorIndex, strerror(errno));
        return -EINVAL;
    }
#endif
#else
    if ((mCameraHandle = open(DEVICE_PATH(mSensorIndex), oflag)) == -1){
        CAMHAL_LOGEB("Error while opening handle to V4L2 Camera: %s", strerror(errno));
        return -EINVAL;
    }
#endif

    ret = ioctl (mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);
    if (ret < 0){
        CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
        return -EINVAL;
    }

    if ((mVideoInfo->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0){
        CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
        return -EINVAL;
    }

    if (!(mVideoInfo->cap.capabilities & V4L2_CAP_STREAMING)){
        CAMHAL_LOGEA("Error while adapter initialization: Capture device does not support streaming i/o");
        return -EINVAL;
    }

    if (V4L2_CAP_VIDEO_M2M == (mVideoInfo->cap.capabilities & V4L2_CAP_VIDEO_M2M)){
        m_eDeviceType = DEV_ION;
        m_eV4l2Memory = V4L2_MEMORY_DMABUF;
        CAMHAL_LOGIA("Capture device support streaming ION\n");
    } else {
        m_eDeviceType = DEV_MMAP;
        m_eV4l2Memory = V4L2_MEMORY_MMAP;
    }
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    m_eDeviceType = DEV_USB;
    mVideoInfo->idVendor =  mVideoInfo->cap.reserved[0] >> 16;
    mVideoInfo->idProduct =  mVideoInfo->cap.reserved[0] & 0xffff;
#endif

    mVideoInfo->canvas_mode = false;

    char* str = strchr((const char *)mVideoInfo->cap.card,'.');
    if(str){
        if(!strncmp(str,".canvas",strlen(str))){
            mVideoInfo->canvas_mode = true;
            CAMHAL_LOGDB("Camera %d use canvas mode",mSensorIndex);
        }
    }
    if (strcmp(caps->get(CameraProperties::FACING_INDEX), (const char *) android::ExCameraParameters::FACING_FRONT) == 0)
        mbFrontCamera = true;
    else
        mbFrontCamera = false;
    CAMHAL_LOGDB("mbFrontCamera=%d",mbFrontCamera);

    // Initialize flags
    mPreviewing = false;
    mVideoInfo->isStreaming = false;
    mRecording = false;
    mZoomlevel = -1;
    mEnableContiFocus = false;
    cur_focus_mode_for_conti = CAM_FOCUS_MODE_RELEASE;
    mFlashMode = FLASHLIGHT_OFF;
    mPixelFormat = 0;
    mSensorFormat = 0;

    mPreviewWidth = 0 ;
    mPreviewHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    mIsDequeuedEIOError = false;
#endif

    IoctlStateProbe();

    mSupportMJPEG = false;
    {
        char property[PROPERTY_VALUE_MAX];
        int enable = 0;
        memset(property,0,sizeof(property));
        if (property_get("ro.media.camera_preview.usemjpeg", property, NULL) > 0) {
            enable = atoi(property);
        }
        mUseMJPEG = (enable!=0)?true:false;
        enable = 0;
        if(property_get("camera.preview.DebugMJPEG", property, NULL) > 0){
            enable = atoi(property);
        }
        mDebugMJPEG = (enable!=0)?true:false;

    }
    if(mUseMJPEG == true){
        mSupportMJPEG = is_mjpeg_supported(mCameraHandle);
        if(mSupportMJPEG == true){
            CAMHAL_LOGDA("Current Camera's preview format set as MJPEG\n");
        }
    }

    ParserLimitedRateInfo(&LimitedRate);
    if(LimitedRate.num>0){
        CAMHAL_LOGDB("Current Camera's succeed parser %d limited rate parameter(s)\n",LimitedRate.num);
        for(int k = 0;k<LimitedRate.num;k++){
            CAMHAL_LOGVB("limited rate parameter %d : %dx%dx%d\n",LimitedRate.num,LimitedRate.arg[k].width,LimitedRate.arg[k].height,LimitedRate.arg[k].framerate);
        } 
    }

    mLimitedFrameRate = 0;  // no limited
    mExpectedFrameInv = (unsigned) (1000000)/15;
    mFramerate = 15;

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    writefile((char*)SYSFILE_CAMERA_SET_PARA, (char*)"1");
#endif
    if (DEV_ION == m_eDeviceType) {
            ret = allocImageIONBuf(caps);
    }
    mResetTH = 5000000;  // initial reset threshold is 5s,then set it to 3s
    mPreviewCache = NULL;
    mEnableDump = false;
    mDumpCnt = 0;
    //mirror set at here will not work.
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::allocImageIONBuf(CameraProperties::Properties* caps)
{
        status_t ret = NO_ERROR;
        int max_width, max_height;
        int bytes;

        DBG_LOGB("picture size=%s\n", caps->get(CameraProperties::PICTURE_SIZE));
        if (NULL == caps->get(CameraProperties::PICTURE_SIZE))
                return -EINVAL;
        ret = sscanf(caps->get(CameraProperties::PICTURE_SIZE), "%dx%d", &max_width, &max_height);
        if ((ret < 0) || (max_width <0) || (max_height < 0))
                return -EINVAL;

        max_width = ALIGN(max_width, 32);
        max_height = ALIGN(max_height, 32);

        if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_RGB24){ // rgb24

                bytes = max_width*max_height*3;
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ //   422I

                bytes = max_width*max_height*2;
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){

                bytes = max_width*max_height*3/2;
        }else{

                bytes = max_width*max_height*3;
        }


        mIonFd = ion_open();
        if (mIonFd < 0){
                CAMHAL_LOGVB("ion_open failed, errno=%d", errno);
                return -EINVAL;
        }
        ret = ion_alloc(mIonFd, bytes, 0, ION_HEAP_CARVEOUT_MASK, 0, &mIonHnd);
        if (ret < 0){
                ion_close(mIonFd);
                CAMHAL_LOGVB("ion_alloc failed, errno=%d", errno);
                mIonFd = -1;
                return -ENOMEM;
        }
        ret = ion_share(mIonFd, mIonHnd, &mImageFd);
        if (ret < 0){
                CAMHAL_LOGVB("ion_share failed, errno=%d", errno);
                ion_free(mIonFd, mIonHnd);
                ion_close(mIonFd);
                mIonFd = -1;
                return -EINVAL;
        }

        DBG_LOGB("allocate ion buffer for capture, ret=%d, bytes=%d, max_width=%d, max_height=%d\n",
                        ret, bytes, max_width, max_height);
        return NO_ERROR;
}

status_t V4LCameraAdapter::IoctlStateProbe(void)
{
    struct v4l2_queryctrl qc;  
    int ret = 0;

    LOG_FUNCTION_NAME;

    mIoctlSupport = 0;
    if(get_hflip_mode(mCameraHandle)==0){
        mIoctlSupport |= IOCTL_MASK_HFLIP;
    }else{
        mIoctlSupport &= ~IOCTL_MASK_HFLIP;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_ZOOM_ABSOLUTE;  
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_INTEGER)){
        mIoctlSupport &= ~IOCTL_MASK_ZOOM;
    }else{
        mIoctlSupport |= IOCTL_MASK_ZOOM;
    }

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_ROTATE_ID;  
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_INTEGER)){
        mIoctlSupport &= ~IOCTL_MASK_ROTATE;
    }else{
        mIoctlSupport |= IOCTL_MASK_ROTATE;
    }
    
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        CAMHAL_LOGDB("camera %d support capture rotate",mSensorIndex);
    }
    mRotateValue = 0;
#endif

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    qc.id = V4L2_CID_EXPOSURE_ABSOLUTE;  
#else
    qc.id = V4L2_CID_EXPOSURE;
#endif
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) ){
        mIoctlSupport &= ~IOCTL_MASK_EXPOSURE;
        mEVdef = 4;
        mEVmin = 0;
        mEVmax = 8;
    }else{
        mIoctlSupport |= IOCTL_MASK_EXPOSURE;
        mEVdef = qc.default_value;
        mEVmin = qc.minimum;
        mEVmax = qc.maximum;
    }
    mEV = mEVdef;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    qc.id = V4L2_CID_AUTO_WHITE_BALANCE;
#else
    qc.id = V4L2_CID_DO_WHITE_BALANCE;
#endif
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)){
        mIoctlSupport &= ~IOCTL_MASK_WB;
    }else{
        mIoctlSupport |= IOCTL_MASK_WB;
    }

    mWhiteBalance = qc.default_value;
    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_BACKLIGHT_COMPENSATION; 
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_FLASH;
    }else{
        mIoctlSupport |= IOCTL_MASK_FLASH;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_COLORFX; 
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_EFFECT;
    }else{
        mIoctlSupport |= IOCTL_MASK_EFFECT;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_POWER_LINE_FREQUENCY;
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_BANDING;
    }else{
        mIoctlSupport |= IOCTL_MASK_BANDING;
    }
    mAntiBanding = qc.default_value;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_FOCUS_AUTO;
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_FOCUS;
    }else{
        mIoctlSupport |= IOCTL_MASK_FOCUS;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_AUTO_FOCUS_STATUS;
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)){
        mIoctlSupport &= ~IOCTL_MASK_FOCUS_MOVE;
    }else{
        mIoctlSupport |= IOCTL_MASK_FOCUS_MOVE;
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{
    status_t ret = NO_ERROR;
    v4l2_buffer hbuf_query;
    private_handle_t* gralloc_hnd;

    memset(&hbuf_query,0,sizeof(v4l2_buffer));

    if (CameraFrame::IMAGE_FRAME == frameType){
        //if (NULL != mEndImageCaptureCallback)
            //mEndImageCaptureCallback(mEndCaptureData);
        if (NULL != mReleaseImageBuffersCallback)
            mReleaseImageBuffersCallback(mReleaseData);
        return NO_ERROR;
    }
    if ( !mVideoInfo->isStreaming || !mPreviewing){
        return NO_ERROR;
    }
    {
	    Mutex::Autolock lock(mPreviewBufsLock);// add this to protect previewbufs when reset sensor	
	    int i = mPreviewBufs.valueFor(( unsigned int )frameBuf);
	    if(i<0){
	        return BAD_VALUE;
	    }
	    if(nQueued>=mPreviewBufferCount){
	        CAMHAL_LOGEB("fill buffer error, reach the max preview buff:%d,max:%d",nQueued,mPreviewBufferCount);
	        return BAD_VALUE;
	    }
	
	    hbuf_query.index = i;
	    hbuf_query.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    hbuf_query.memory = m_eV4l2Memory;
	    if (V4L2_MEMORY_DMABUF == m_eV4l2Memory)
	    {
	        gralloc_hnd = (private_handle_t *)frameBuf;
	        hbuf_query.m.fd = gralloc_hnd->share_fd;
	    }
	
	    ret = ioctl(mCameraHandle, VIDIOC_QBUF, &hbuf_query);
	    if (ret < 0) {
	        CAMHAL_LOGEB("Init: VIDIOC_QBUF %d Failed, errno=%d\n",i, errno);
	        return -1;
	    }
	    nQueued++;
	}
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    if(mIsDequeuedEIOError){
        CAMHAL_LOGEA("DQBUF EIO error has occurred!\n");
        this->stopPreview();
        close(mCameraHandle);
        mCameraHandle = -1;
        return -1;
    }
#endif
    return ret;
}

status_t V4LCameraAdapter::setParameters(const CameraParameters &params)
{
    LOG_FUNCTION_NAME;
    status_t rtn = NO_ERROR;

    // Update the current parameter set
    mParams = params;
    //check zoom value
    int zoom = mParams.getInt(CameraParameters::KEY_ZOOM);
    int maxzoom = mParams.getInt(CameraParameters::KEY_MAX_ZOOM);
    char *p = (char *)mParams.get(CameraParameters::KEY_ZOOM_RATIOS);

    if(zoom > maxzoom){
        rtn = INVALID_OPERATION;
        CAMHAL_LOGDB("Zoom Out of range, level:%d,max:%d",zoom,maxzoom);
        zoom = maxzoom;
        mParams.set((const char*)CameraParameters::KEY_ZOOM, maxzoom);
    }else if(zoom <0) {
        rtn = INVALID_OPERATION;
        zoom = 0;
        CAMHAL_LOGEB("Zoom Parameter Out of range2------zoom level:%d,max level:%d",zoom,maxzoom);
        mParams.set((const char*)CameraParameters::KEY_ZOOM, zoom);
    }

    if ((p) && (zoom >= 0)&&(zoom!=mZoomlevel)) {
        int z = (int)strtol(p, &p, 10);
        int i = 0;
        while (i < zoom) {
            if (*p != ',') break;
            z = (int)strtol(p+1, &p, 10);
            i++;
        }
        CAMHAL_LOGDB("Change the zoom level---old:%d,new:%d",mZoomlevel,zoom);
        mZoomlevel = zoom;
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
        if(mIoctlSupport & IOCTL_MASK_ZOOM)
            set_zoom_level(mCameraHandle,z);
        else 
            SYS_set_zoom(z);
#endif
        notifyZoomSubscribers((mZoomlevel<0)?0:mZoomlevel,true);
    }
 
    int min_fps,max_fps;
    const char *white_balance=NULL;
    const char *exposure=NULL;
    const char *effect=NULL;
    //const char *night_mode=NULL;
    const char *qulity=NULL;
    const char *banding=NULL;
    const char *flashmode=NULL;
    const char *focusmode=NULL;
    const char *supportfocusmode=NULL;
    const char *focusarea = NULL;

    qulity=mParams.get(CameraParameters::KEY_JPEG_QUALITY);
    
    flashmode = mParams.get(CameraParameters::KEY_FLASH_MODE);
    if((mIoctlSupport & IOCTL_MASK_FLASH) && flashmode){
        if(strcasecmp(flashmode, "torch")==0){
            set_flash_mode(mCameraHandle, flashmode);
            mFlashMode = FLASHLIGHT_TORCH;
        }else if(strcasecmp(flashmode, "on")==0){
            if( FLASHLIGHT_TORCH == mFlashMode){
                set_flash_mode(mCameraHandle, "off");
            }
            mFlashMode = FLASHLIGHT_ON;
        }else if(strcasecmp(flashmode, "off")==0){
            set_flash_mode(mCameraHandle, flashmode);
            mFlashMode = FLASHLIGHT_OFF;
        }
    }

    exposure=mParams.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    if( (mIoctlSupport & IOCTL_MASK_EXPOSURE) && exposure){
        SetExposure(mCameraHandle,exposure);
    }

    white_balance=mParams.get(CameraParameters::KEY_WHITE_BALANCE);
    if((mIoctlSupport & IOCTL_MASK_WB) && white_balance){
        set_white_balance(mCameraHandle,white_balance);
    }

    effect=mParams.get(CameraParameters::KEY_EFFECT);
    if( (mIoctlSupport & IOCTL_MASK_EFFECT) && effect){
        set_effect(mCameraHandle,effect);
    }

    banding=mParams.get(CameraParameters::KEY_ANTIBANDING);
    if((mIoctlSupport & IOCTL_MASK_BANDING) && banding){
        set_banding(mCameraHandle,banding);
    }

    focusmode = mParams.get(CameraParameters::KEY_FOCUS_MODE);
    if(focusmode) {
        if(strcasecmp(focusmode,"fixed")==0)
            cur_focus_mode = CAM_FOCUS_MODE_FIXED;
        else if(strcasecmp(focusmode,"auto")==0)
            cur_focus_mode = CAM_FOCUS_MODE_AUTO;
        else if(strcasecmp(focusmode,"infinity")==0)
            cur_focus_mode = CAM_FOCUS_MODE_INFINITY;
        else if(strcasecmp(focusmode,"macro")==0)
            cur_focus_mode = CAM_FOCUS_MODE_MACRO;
        else if(strcasecmp(focusmode,"edof")==0)
            cur_focus_mode = CAM_FOCUS_MODE_EDOF;
        else if(strcasecmp(focusmode,"continuous-video")==0)
            cur_focus_mode = CAM_FOCUS_MODE_CONTI_VID;
        else if(strcasecmp(focusmode,"continuous-picture")==0)
            cur_focus_mode = CAM_FOCUS_MODE_CONTI_PIC;
        else
            cur_focus_mode = CAM_FOCUS_MODE_FIXED;
    }
    supportfocusmode = mParams.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
    if(  NULL != strstr(supportfocusmode, "continuous")){
        if(CAM_FOCUS_MODE_AUTO != cur_focus_mode_for_conti){
            struct v4l2_control ctl;
            if( (CAM_FOCUS_MODE_CONTI_VID != cur_focus_mode_for_conti ) &&
              ( (CAM_FOCUS_MODE_AUTO == cur_focus_mode )
              ||( CAM_FOCUS_MODE_CONTI_PIC == cur_focus_mode )
              ||( CAM_FOCUS_MODE_CONTI_VID == cur_focus_mode ) )){
                mEnableContiFocus = true;
                ctl.id = V4L2_CID_FOCUS_AUTO;
                ctl.value = CAM_FOCUS_MODE_CONTI_VID;
                if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
                    CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_CONTI_VID!\n");
                }
                mFocusWaitCount = FOCUS_PROCESS_FRAMES;
                bFocusMoveState = true;
                cur_focus_mode_for_conti = CAM_FOCUS_MODE_CONTI_VID;
            }else if( (CAM_FOCUS_MODE_CONTI_VID != cur_focus_mode_for_conti)&&
              (CAM_FOCUS_MODE_AUTO != cur_focus_mode) &&
              ( CAM_FOCUS_MODE_CONTI_PIC != cur_focus_mode )&&
              ( CAM_FOCUS_MODE_CONTI_VID != cur_focus_mode )){
                mEnableContiFocus = false;
                ctl.id = V4L2_CID_FOCUS_AUTO;
                ctl.value = CAM_FOCUS_MODE_RELEASE;
                if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
                    CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_RELEASE!\n");
                }
                cur_focus_mode_for_conti = CAM_FOCUS_MODE_RELEASE;
            }else if( (CAM_FOCUS_MODE_INFINITY != cur_focus_mode_for_conti)&&
              (CAM_FOCUS_MODE_INFINITY == cur_focus_mode) ){
                mEnableContiFocus = false;
                ctl.id = V4L2_CID_FOCUS_AUTO;
                ctl.value = CAM_FOCUS_MODE_INFINITY;
                if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
                    CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_INFINITY!\n");
                }
                cur_focus_mode_for_conti = CAM_FOCUS_MODE_INFINITY;
            }
        }
    }else{
        mEnableContiFocus = false;
        CAMHAL_LOGDA("not support continuous mode!\n");
    }

    focusarea = mParams.get(CameraParameters::KEY_FOCUS_AREAS);
    if(focusarea){
        set_focus_area( mCameraHandle, focusarea);
    }

    min_fps = mParams.getPreviewFrameRate();
    if( min_fps ){
            mExpectedFrameInv = (unsigned) (1000000)/min_fps;
    }else{
            mExpectedFrameInv = (unsigned) (1000000)/15;
    }
	mFramerate = min_fps ? min_fps : 15;
    mParams.getPreviewFpsRange(&min_fps, &max_fps);
    if((min_fps<0)||(max_fps<0)||(max_fps<min_fps)){
        rtn = INVALID_OPERATION;
    }

    LOG_FUNCTION_NAME_EXIT;
    return rtn;
}


void V4LCameraAdapter::getParameters(CameraParameters& params)
{
    LOG_FUNCTION_NAME;

    // Return the current parameter set
    //params = mParams;
    //that won't work. we might wipe out the existing params

    LOG_FUNCTION_NAME_EXIT;
}

///API to give the buffers to Adapter
status_t V4LCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    Mutex::Autolock lock(mLock);
	mPreviewCache = NULL;
    switch(mode){
        case CAMERA_PREVIEW:
            ret = UseBuffersPreview(bufArr, num);
            //maxQueueable = queueable;
            break;
        case CAMERA_IMAGE_CAPTURE:
            ret = UseBuffersCapture(bufArr, num);
            break;
        case CAMERA_VIDEO:
            //@warn Video capture is not fully supported yet
            ret = UseBuffersPreview(bufArr, num);
            //maxQueueable = queueable;
            break;
        default:
            break;
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::tryBuffersFormat(int width, int height, int pixelformat)
{
    int ret = NO_ERROR;
    CAMHAL_LOGIB("try Width * Height %d x %d pixelformat:%.4s\n",
                  width, height, (char*)&pixelformat);

    mVideoInfo->width = width;
    mVideoInfo->height = height;
    mVideoInfo->framesizeIn = (width * height << 1);
    mVideoInfo->formatIn = pixelformat;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->format.fmt.pix.width = width;
    mVideoInfo->format.fmt.pix.height = height;
    mVideoInfo->format.fmt.pix.pixelformat = pixelformat;

    ret = ioctl(mCameraHandle, VIDIOC_TRY_FMT, &mVideoInfo->format);
    if (ret < 0) {
        CAMHAL_LOGVB("VIDIOC_TRY_FMT Failed: %s, wxd=%dx%d, ret=%d\n", strerror(errno), width, height, ret);
    }

    if ( ((int)mVideoInfo->format.fmt.pix.width != width) ||
         ((int)mVideoInfo->format.fmt.pix.height != height) ) {
            CAMHAL_LOGVB("VIDIOC_TRY_FMT Failed: %s, wxd=%dx%d, ret=%d\n", strerror(errno), width, height, ret);
            ret = -EINVAL;
    }

    return ret;
}

status_t V4LCameraAdapter::setBuffersFormat(int width, int height, int pixelformat)
{
    int ret = NO_ERROR;
    CAMHAL_LOGIB("Width * Height %d x %d pixelformat:%.4s\n",
                  width, height, (char*)&pixelformat);

    mVideoInfo->width = width;
    mVideoInfo->height = height;
    mVideoInfo->framesizeIn = (width * height << 1);
    mVideoInfo->formatIn = pixelformat;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->format.fmt.pix.width = width;
    mVideoInfo->format.fmt.pix.height = height;
    mVideoInfo->format.fmt.pix.pixelformat = pixelformat;

    ret = ioctl(mCameraHandle, VIDIOC_S_FMT, &mVideoInfo->format);
    if (ret < 0) {
        CAMHAL_LOGEB("Open: VIDIOC_S_FMT Failed: %s, ret=%d\n", strerror(errno), ret);
    }
    return ret;
}

status_t V4LCameraAdapter::getBuffersFormat(int &width, int &height, int &pixelformat)
{
    int ret = NO_ERROR;
    struct v4l2_format format;
	
    memset(&format, 0,sizeof(struct v4l2_format));

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mCameraHandle, VIDIOC_G_FMT, &format);
    if (ret < 0) {
        CAMHAL_LOGDB("Open: VIDIOC_G_FMT Failed: %s", strerror(errno));
        return ret;
    }
    width = format.fmt.pix.width;
    height = format.fmt.pix.height;
    pixelformat = format.fmt.pix.pixelformat;
    CAMHAL_LOGDB("VIDIOC_G_FMT, w*h: %5dx%5d, format 0x%x", width, height, pixelformat);	
    return ret;
}

status_t V4LCameraAdapter::setCrop(int width, int height)
{
        int ret = NO_ERROR;
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
        struct v4l2_crop crop;

        memset (&crop, 0, sizeof(crop));
        crop.c.width = width;
        crop.c.height = height;
        ret = ioctl(mCameraHandle, VIDIOC_S_CROP, &crop);
        if (ret < 0) {
                CAMHAL_LOGVB("VIDIOC_S_CROP Failed: %s, ret=%d\n", strerror(errno), ret);
        }

#endif
        return ret;
}

status_t V4LCameraAdapter::UseBuffersPreview(void* bufArr, int num)
{
    int ret = NO_ERROR;
    private_handle_t* gralloc_hnd;
    uint32_t limit_pixelfmt = 0;

    if(NULL == bufArr)
        return BAD_VALUE;

    int width, height,k = 0;
    mParams.getPreviewSize(&width, &height);

    mPreviewWidth = width;
    mPreviewHeight = height;

    mLimitedFrameRate = 0;

    for(k = 0; k<LimitedRate.num; k++){
        if((mPreviewWidth == LimitedRate.arg[k].width)&&(mPreviewHeight == LimitedRate.arg[k].height)){
            mLimitedFrameRate = LimitedRate.arg[k].framerate;
            CAMHAL_LOGVB("UseBuffersPreview, Get the limited rate: %dx%dx%d", mPreviewWidth, mPreviewHeight, mLimitedFrameRate);
            break;
        }
    }

    const char *pixfmtchar;
    int pixfmt = V4L2_PIX_FMT_NV21;

    pixfmtchar = mParams.getPreviewFormat();
    if(strcasecmp( pixfmtchar, "yuv420p")==0){
        pixfmt = V4L2_PIX_FMT_YVU420;
        mPixelFormat =CameraFrame::PIXEL_FMT_YV12;
    }else if(strcasecmp( pixfmtchar, "yuv420sp")==0){
        pixfmt = V4L2_PIX_FMT_NV21;
        mPixelFormat = CameraFrame::PIXEL_FMT_NV21;
    }else if(strcasecmp( pixfmtchar, "yuv422")==0){
        pixfmt = V4L2_PIX_FMT_YUYV;
        mPixelFormat = CameraFrame::PIXEL_FMT_YUYV;
    }
    
    mSensorFormat = pixfmt;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    if((mUseMJPEG == true)&&(mSupportMJPEG == true)) {
            mSensorFormat = V4L2_PIX_FMT_MJPEG;
            ret = tryBuffersFormat(width, height, mSensorFormat);
            if( ret < 0) {
                    CAMHAL_LOGVB("try format :%4s wxh=%dx%d not support\n", (char *)&mSensorFormat, width, height);
                    mSensorFormat = V4L2_PIX_FMT_YUYV;
                    mSupportMJPEG = false;
            }

    } else {
            mSensorFormat = V4L2_PIX_FMT_YUYV;
    }



    limit_pixelfmt = query_aml_usb_pixelfmt(mVideoInfo->idVendor, mVideoInfo->idProduct, width, height);
    if (limit_pixelfmt) {
            mSensorFormat = limit_pixelfmt;
    }
    if(mDebugMJPEG) {
            mSensorFormat = V4L2_PIX_FMT_YUYV;
            mSupportMJPEG = false;
            CAMHAL_LOGDA("use YUYV for debug purpose\n");
    }
#endif
    ret = setBuffersFormat(width, height, mSensorFormat);
    if( 0 > ret ){
        CAMHAL_LOGEB("VIDIOC_S_FMT failed: %s", strerror(errno));
        return BAD_VALUE;
    }
    //First allocate adapter internal buffers at V4L level for USB Cam
    //These are the buffers from which we will copy the data into overlay buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = m_eV4l2Memory;
    mVideoInfo->rb.count = num;

    ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }
    uint32_t *ptr = (uint32_t*) bufArr;
	mPreviewCache = (uint32_t*) bufArr;
	{
		Mutex::Autolock lock(mPreviewBufsLock);
	    for (int i = 0; i < num; i++) {
	        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));
	        mVideoInfo->buf.index = i;
	        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	        mVideoInfo->buf.memory = m_eV4l2Memory;
	        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
	        if (ret < 0) {
	            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
	            return ret;
	        }
	
	        if (V4L2_MEMORY_DMABUF == m_eV4l2Memory)
	        {
	            gralloc_hnd = (private_handle_t*)ptr[i];
	            mVideoInfo->mem[i] = mmap (0,
	                                mVideoInfo->buf.length,
	                                PROT_READ | PROT_WRITE,
	                                MAP_SHARED,
	                                gralloc_hnd->share_fd,
	                                0);
	        } else {
	            mVideoInfo->mem[i] = mmap (0,
	                                mVideoInfo->buf.length,
	                                PROT_READ | PROT_WRITE,
	                                MAP_SHARED,
	                                mCameraHandle,
	                                mVideoInfo->buf.m.offset);
	        }
	
	        if (mVideoInfo->mem[i] == MAP_FAILED) {
	            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
	            return -1;
	        }
	
	        if(mVideoInfo->canvas_mode){
	            mVideoInfo->canvas[i] = mVideoInfo->buf.reserved;
	        }
	        //Associate each Camera internal buffer with the one from Overlay
	        CAMHAL_LOGDB("mPreviewBufs.add %#x, %d", ptr[i], i);
	        mPreviewBufs.add((int)ptr[i], i);
	    }
	
	    for(int i = 0;i < num; i++){
	        mPreviewIdxs.add(mPreviewBufs.valueAt(i),i);
	    }
	}

    // Update the preview buffer count
    mPreviewBufferCount = num;
    return ret;
}

status_t V4LCameraAdapter::UseBuffersCapture(void* bufArr, int num)
{
    int ret = NO_ERROR;
    uint32_t limit_pixelfmt = 0;

    LOG_FUNCTION_NAME;

    if(NULL == bufArr)
        return BAD_VALUE;

    if (num != 1){
        CAMHAL_LOGDB("num=%d\n", num);
    }

    int width, height;
    mParams.getPictureSize(&width, &height);
    mCaptureWidth = width;
    mCaptureHeight = height;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    if((mUseMJPEG == true)&&(mSupportMJPEG == true)) {
            mSensorFormat = V4L2_PIX_FMT_MJPEG;
            ret = tryBuffersFormat(width, height, mSensorFormat);
            if( ret < 0) {
                    CAMHAL_LOGVB("try wxh=%dx%d not support mjpeg\n", width, height);
                    mSensorFormat = V4L2_PIX_FMT_YUYV;
                    mSupportMJPEG = false;
            }
    } else {
            mSensorFormat = V4L2_PIX_FMT_YUYV;
    }


    limit_pixelfmt = query_aml_usb_pixelfmt(mVideoInfo->idVendor, mVideoInfo->idProduct, width, height);
    if (limit_pixelfmt) {
            mSensorFormat = limit_pixelfmt;
    }
#else
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        int temp = 0;
        mRotateValue = mParams.getInt(CameraParameters::KEY_ROTATION);
        if((mRotateValue!=0)&&(mRotateValue!=90)&&(mRotateValue!=180)&&(mRotateValue!=270))
            mRotateValue = 0;
        if((mRotateValue==90)||(mRotateValue==270)){
            temp = width;
            width = height;
            height = temp;
        }
    }
    mSensorFormat = DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT;
#endif

    setCrop( mCaptureWidth, mCaptureHeight);
    /* This will only be called right before taking a picture, so
     * stop preview now so that we can set buffer format here.
     */
    this->stopPreview();

    setBuffersFormat(width, height, mSensorFormat);

    //First allocate adapter internal buffers at V4L level for Cam
    //These are the buffers from which we will copy the data into display buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = m_eV4l2Memory;
    mVideoInfo->rb.count = num;

    ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }

    for (int i = 0; i < num; i++) {
        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));
        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;
        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }
        if (V4L2_MEMORY_DMABUF == m_eV4l2Memory) {
                mVideoInfo->mem[i] = mmap (0,
                                mVideoInfo->buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                mImageFd,
                                0);
        } else {
                mVideoInfo->mem[i] = mmap (0,
                                mVideoInfo->buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                mCameraHandle,
                                mVideoInfo->buf.m.offset);
        }

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
            return -1;
        }
        if(mVideoInfo->canvas_mode)
            mVideoInfo->canvas[i] = mVideoInfo->buf.reserved;

        uint32_t *ptr = (uint32_t*) bufArr;
        mCaptureBuf = (camera_memory_t*)ptr[0];
        CAMHAL_LOGDB("UseBuffersCapture %#x", ptr[0]);
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::takePicture()
{
    LOG_FUNCTION_NAME;
    if (createThread(beginPictureThread, this) == false)
        return -1;
    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}


int V4LCameraAdapter::beginAutoFocusThread(void *cookie)
{
    V4LCameraAdapter *c = (V4LCameraAdapter *)cookie;
    struct v4l2_control ctl;
    int ret = -1;

    if( c->mIoctlSupport & IOCTL_MASK_FOCUS){
        ctl.id = V4L2_CID_FOCUS_AUTO;
        ctl.value = CAM_FOCUS_MODE_AUTO;//c->cur_focus_mode;
        ret = ioctl(c->mCameraHandle, VIDIOC_S_CTRL, &ctl);
        for(int j=0; j<70; j++){
            usleep(30000);//30*70ms=2.1s
            ret = ioctl(c->mCameraHandle, VIDIOC_G_CTRL, &ctl);
            if( (0==ret) || ((ret < 0)&&(EBUSY != errno)) ){
                break;
            }
        }
    }

    c->setState(CAMERA_CANCEL_AUTOFOCUS);
    c->commitState();

    if( (c->mIoctlSupport & IOCTL_MASK_FLASH)&&(FLASHLIGHT_ON == c->mFlashMode)){
        set_flash_mode( c->mCameraHandle, "off");
    }
    if(ret < 0) {
        if( c->mIoctlSupport & IOCTL_MASK_FOCUS){
            CAMHAL_LOGDA("AUTO FOCUS Failed");
        }
        c->notifyFocusSubscribers(false);
    } else {
        c->notifyFocusSubscribers(true);
    }
    // may need release auto focus mode at here.
    return ret;
}

status_t V4LCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    if( (mIoctlSupport & IOCTL_MASK_FLASH)&&(FLASHLIGHT_ON == mFlashMode)){
        set_flash_mode( mCameraHandle, "on");
    }
    cur_focus_mode_for_conti = CAM_FOCUS_MODE_AUTO;

    if (createThread(beginAutoFocusThread, this) == false){
        ret = UNKNOWN_ERROR;
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::cancelAutoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    struct v4l2_control ctl;

    if( (mIoctlSupport & IOCTL_MASK_FOCUS) == 0x00 ){
        return 0;
    }

    if ( !mEnableContiFocus){
        ctl.id = V4L2_CID_FOCUS_AUTO;
        ctl.value = CAM_FOCUS_MODE_RELEASE;
        ret = ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl);
        if(ret < 0) {
            CAMHAL_LOGDA("AUTO FOCUS Failed");
        }
    }else if( CAM_FOCUS_MODE_AUTO == cur_focus_mode_for_conti){
        if(CAM_FOCUS_MODE_INFINITY != cur_focus_mode){
            ctl.id = V4L2_CID_FOCUS_AUTO;
            ctl.value = CAM_FOCUS_MODE_CONTI_VID;
            if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
                CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_CONTI_VID\n");
            }
            cur_focus_mode_for_conti =  CAM_FOCUS_MODE_CONTI_VID;
        }else{
            ctl.id = V4L2_CID_FOCUS_AUTO;
            ctl.value = CAM_FOCUS_MODE_INFINITY;
            if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
                CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_INFINITY\n");
            }
            cur_focus_mode_for_conti =  CAM_FOCUS_MODE_INFINITY;
        }
    }
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t V4LCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;
    int frame_count = 0,ret_c = 0;
    void *frame_buf = NULL;
    

    if(mPreviewing){
        return BAD_VALUE;
    }

#ifdef ION_MODE_FOR_METADATA_MODE
    if ((CAMHAL_GRALLOC_USAGE) & GRALLOC_USAGE_PRIVATE_1) {
        mIonClient = ion_open();
        if (mIonClient >= 0) {
            ion_mode = true;
            memset(mPhyAddr, 0, sizeof(mPhyAddr));
        } else {
            CAMHAL_LOGEA("open ion client error");
        }
    }
#endif

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    setMirrorEffect();

    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        if(mPreviewOriation!=0) {
            set_rotate_value(mCameraHandle,mPreviewOriation); 
            mPreviewOriation=0;
        }else{
            set_rotate_value(mCameraHandle,0);
            mRotateValue = 0;
        }
    }
#endif

    nQueued = 0;
    private_handle_t* gralloc_hnd;
    {
	    Mutex::Autolock lock(mPreviewBufsLock);
	    for (int i = 0; i < mPreviewBufferCount; i++){
	        frame_count = -1;
	        frame_buf = (void *)mPreviewBufs.keyAt(i);
	
	        if((ret_c = getFrameRefCount(frame_buf,CameraFrame::PREVIEW_FRAME_SYNC))>=0)
	            frame_count = ret_c;
	
	        //if((ret_c = getFrameRefCount(frame_buf,CameraFrame::VIDEO_FRAME_SYNC))>=0)
	        //    frame_count += ret_c;
	 
	        CAMHAL_LOGDB("startPreview--buffer address:0x%x, refcount:%d",(uint32_t)frame_buf,frame_count);
	        if(frame_count>0)
	            continue;
	        //mVideoInfo->buf.index = i;
	        mVideoInfo->buf.index = mPreviewBufs.valueFor((uint32_t)frame_buf);
	        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	        mVideoInfo->buf.memory = m_eV4l2Memory;
	        if (V4L2_MEMORY_DMABUF == m_eV4l2Memory) {
	                gralloc_hnd = (private_handle_t *)frame_buf;
	                mVideoInfo->buf.m.fd = gralloc_hnd->share_fd;
	        }

	        ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
	        if (ret < 0) {
	            CAMHAL_LOGEA("VIDIOC_QBUF Failed");
	            return -EINVAL;
	        }
	        CAMHAL_LOGDB("startPreview --length=%d, index:%d", mVideoInfo->buf.length,mVideoInfo->buf.index);
	        nQueued++;
	    }
	}

    enum v4l2_buf_type bufType;
    if (!mVideoInfo->isStreaming){
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
        gettimeofday( &previewTime1, NULL);
#endif
        ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
        if (ret < 0) {
            CAMHAL_LOGEB("StartStreaming: Unable to start capture: %s", strerror(errno));
            return ret;
        }
        mVideoInfo->isStreaming = true;
    }

    if( mEnableContiFocus &&
      (CAM_FOCUS_MODE_AUTO != cur_focus_mode_for_conti) &&
      (CAM_FOCUS_MODE_INFINITY != cur_focus_mode_for_conti)){
        struct v4l2_control ctl;
        ctl.id = V4L2_CID_FOCUS_AUTO;
        ctl.value = CAM_FOCUS_MODE_CONTI_VID;
        if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
            CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_CONTI_VID!\n");
        }
        cur_focus_mode_for_conti = CAM_FOCUS_MODE_CONTI_VID;
    }
    // Create and start preview thread for receiving buffers from V4L Camera
    mFailedCnt = 0;
    mEagainCnt = 0;
    mPreviewThread = new PreviewThread(this);
    CAMHAL_LOGDA("Created preview thread");
    //Update the flag to indicate we are previewing
    mPreviewing = true;
#ifdef PREVIEW_TIME_DEBUG
    gettimeofday(&mTimeStart,NULL);
    precount = 0;
#endif
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
    mFirstBuff = true;
    mFrameInvAdjust = 0;		
    mFrameInv = 0;
    mCache.bufPtr = NULL;
    mCache.index = -1;
#endif
    return ret;
}

status_t V4LCameraAdapter::stopPreview()
{
    enum v4l2_buf_type bufType;
    int ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    Mutex::Autolock lock(mPreviewBufsLock);
    if(!mPreviewing){
        return NO_INIT;
    }

    mPreviewing = false;
    mFocusMoveEnabled = false;
    mPreviewThread->requestExitAndWait();
    mPreviewThread.clear();

    if (mVideoInfo->isStreaming) {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
        if (ret < 0) {
            CAMHAL_LOGDB("StopStreaming: Unable to stop capture: %s", strerror(errno));
        }
        mVideoInfo->isStreaming = false;
    }

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = m_eV4l2Memory;

    nQueued = 0;
    nDequeued = 0;

    if( mEnableContiFocus && 
      (CAM_FOCUS_MODE_AUTO != cur_focus_mode_for_conti) &&
      (CAM_FOCUS_MODE_INFINITY != cur_focus_mode_for_conti)){
        struct v4l2_control ctl;
        ctl.id = V4L2_CID_FOCUS_AUTO;
        ctl.value = CAM_FOCUS_MODE_RELEASE;
        if(ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl)<0){
            CAMHAL_LOGDA("failed to set CAM_FOCUS_MODE_RELEASE!\n");
        }
        cur_focus_mode_for_conti = CAM_FOCUS_MODE_RELEASE;
    }

    /* Unmap buffers */
    for (int i = 0; i < mPreviewBufferCount; i++){
        if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0){
            CAMHAL_LOGEA("Unmap failed");
        }
        mVideoInfo->canvas[i] = 0;        
    }

    if ((DEV_USB == m_eDeviceType) ||
        (DEV_ION == m_eDeviceType) ||
        (DEV_ION_MPLANE == m_eDeviceType))
    {
            mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            mVideoInfo->buf.memory = m_eV4l2Memory;
            mVideoInfo->rb.count = 0;

            ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
            if (ret < 0) {
                CAMHAL_LOGDB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            }else{
                CAMHAL_LOGVA("VIDIOC_REQBUFS delete buffer success\n");
            }
    }

#ifdef ION_MODE_FOR_METADATA_MODE
    if (mIonClient >= 0)
        ion_close(mIonClient);
        mIonClient = -1;
        memset(mPhyAddr, 0, sizeof(mPhyAddr));
#endif

    mPreviewBufs.clear();
    mPreviewIdxs.clear();
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

char * V4LCameraAdapter::GetFrame(int &index, unsigned int* canvas)
{
    int ret;
    if(nQueued<=0){
        CAMHAL_LOGVA("GetFrame: No buff for Dequeue");
        usleep(2000); // add sleep to avoid this case always in
        return NULL;
    }
    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = m_eV4l2Memory;

    /* DQ */
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    if(mIsDequeuedEIOError){
        return NULL;
    }
    if(mEagainCnt == 0)
		gettimeofday(&mEagainStartTime, NULL);

#endif
    ret = ioctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
    if (ret < 0) {
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        if((EIO==errno) || (ENODEV==errno)){
            mIsDequeuedEIOError = true;
            this->stopPreview();
            close(mCameraHandle);
            mCameraHandle = -1;
            CAMHAL_LOGEA("GetFrame: VIDIOC_DQBUF Failed--EIO\n");
            mErrorNotifier->errorNotify(CAMERA_ERROR_SOFT);
        }
#endif
        if(EAGAIN == errno){
            index = -1;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
			mEagainCnt++;
			gettimeofday(&mEagainEndTime, NULL);
            int intreval = (mEagainEndTime.tv_sec - mEagainStartTime.tv_sec) * 1000000 + (mEagainEndTime.tv_usec - mEagainStartTime.tv_usec);
            if(intreval > (int)mResetTH){
            	ALOGD("EAGIN Too Much, Restart");
            	force_reset_sensor();
            	mEagainCnt = 0;
            	mResetTH = 3000000; // for debug
			}
#endif
		}else{
			CAMHAL_LOGEB("GetFrame: VIDIOC_DQBUF Failed,errno=%d\n",errno);
	    }
		return NULL;
		}
	mResetTH = 3000000;
    mEagainCnt = 0;
    nDequeued++;
    nQueued--;
    index = mVideoInfo->buf.index;
    if(mVideoInfo->canvas_mode)
        *canvas = mVideoInfo->canvas[mVideoInfo->buf.index];
    else
        *canvas = 0;
    return (char *)mVideoInfo->mem[mVideoInfo->buf.index];
}

//API to get the frame size required  to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
status_t V4LCameraAdapter::getFrameSize(size_t &width, size_t &height)
{
    status_t ret = NO_ERROR;

    // Just return the current preview size, nothing more to do here.
    mParams.getPreviewSize(( int * ) &width, ( int * ) &height);

    return ret;
}

status_t V4LCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    // We don't support meta data, so simply return
    return NO_ERROR;
}

status_t V4LCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    int width, height;
    mParams.getPictureSize(&width, &height);
    if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_RGB24){ // rgb24
        length = width * height * 3;
    }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ //   422I
        length = width * height * 2;
    }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){
        length = width * height * 3/2;
    }else{
        length = width * height * 3;
    }
    return NO_ERROR;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        CAMHAL_LOGDB("Camera %d Frames, %f FPS", mFrameCount, mFps);
    }
    // XXX: mFPS has the value we want
}

status_t V4LCameraAdapter::recalculateFPS()
{
    float currentFPS;
    mFrameCount++;
    if ( ( mFrameCount % FPS_PERIOD ) == 0 ){
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter ){
            mFPS = currentFPS;
        }else{
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
        }
        mLastFPS = mFPS;
        mIter++;
    }
    return NO_ERROR;
}

void V4LCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    //LOG_FUNCTION_NAME;

    //LOG_FUNCTION_NAME_EXIT;
}


V4LCameraAdapter::V4LCameraAdapter(size_t sensor_index)
{
    LOG_FUNCTION_NAME;
    mbDisableMirror = false;
    mSensorIndex = sensor_index;
    m_eV4l2Memory = V4L2_MEMORY_MMAP;
    m_eDeviceType = DEV_MMAP;
    mImageFd = -1;
    //mImgPtr = NULL;
    mPreviewOriation=0;
    mCaptureOriation=0;
#ifdef ION_MODE_FOR_METADATA_MODE
    ion_mode = false;
    mIonClient = -1;
    memset(mPhyAddr, 0, sizeof(mPhyAddr));
#endif
    LOG_FUNCTION_NAME_EXIT;
}

V4LCameraAdapter::~V4LCameraAdapter()
{
    LOG_FUNCTION_NAME;
    int ret;

    if (mImageFd > 0){
            close(mImageFd);
            ret = ion_free( mIonFd, mIonHnd);
            mImageFd = -1;
            ion_close(mIonFd);
            mIonFd = -1;
            CAMHAL_LOGVA("success to release buffer\n");
    }

    // Close the camera handle and free the video info structure
    if (mCameraHandle > 0) {
            close(mCameraHandle);
            mCameraHandle = -1;
    }
#ifdef AMLOGIC_TWO_CH_UVC
    if(mCamEncodeHandle > 0){
        close(mCamEncodeHandle);
        mCamEncodeHandle = -1;
    }
#endif

#ifdef ION_MODE_FOR_METADATA_MODE
    if (mIonClient >= 0)
        ion_close(mIonClient);
    mIonClient = -1;
    memset(mPhyAddr, 0, sizeof(mPhyAddr));
#endif
    if (mVideoInfo){
        free(mVideoInfo);
        mVideoInfo = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;
}

/* Preview Thread */
// ---------------------------------------------------------------------------

int V4LCameraAdapter::previewThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;
    unsigned delay;
    int previewframeduration = 0;
    int active_duration = 0;
    uint8_t* ptr = NULL; 
    bool noFrame = true;
    unsigned int canvas_id = 0;
    if (mPreviewing){

        int index = -1;

        previewframeduration = mExpectedFrameInv;
        if(mLimitedFrameRate!=0){
            if(mExpectedFrameInv < (unsigned)(1000000.0f / float(mLimitedFrameRate))){
                    previewframeduration = (unsigned)(1000000.0f / float(mLimitedFrameRate));
            }
            mFramerate = mLimitedFrameRate;
        }
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
        delay = 5000;//previewframeduration>>2;
#else
        delay = previewframeduration;
#endif
        if((mPreviewWidth <= 0)||(mPreviewHeight <= 0)){
            mParams.getPreviewSize(&width, &height);
        }else{
            width = mPreviewWidth;
            height = mPreviewHeight;
        }
        if(mSensorFormat != V4L2_PIX_FMT_MJPEG || mFailedCnt > 0 || mEagainCnt > 0 || (width < 640 && height < 480))
            usleep(delay);

        char *fp = this->GetFrame(index, &canvas_id);

        if((-1==index)||!fp){
            noFrame = true;
        }else{ 
            noFrame = false;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
            if(mSensorFormat != V4L2_PIX_FMT_MJPEG){
                if(mVideoInfo->buf.length != mVideoInfo->buf.bytesused){
                    fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index)), CameraFrame::PREVIEW_FRAME_SYNC);
                    CAMHAL_LOGDB("length=%d bytesused=%d index=%d\n", mVideoInfo->buf.length, mVideoInfo->buf.bytesused, index);
                    noFrame = true;
                    index = -1;
                }
            }
#endif
        }

        if(noFrame == true){
            index  = -1;
            fp = NULL;
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
            if(mFirstBuff == true)   // need wait for first frame
                return 0;
#else
            delay = previewframeduration >> 1;
            CAMHAL_LOGEB("Preview thread get frame fail, need sleep:%d",delay);
            usleep(delay);
            return BAD_VALUE;
#endif
        }else{
            ptr = (uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index));
            if (!ptr){
                CAMHAL_LOGEA("Preview thread mPreviewBufs error!");
                return BAD_VALUE;
            }
        }

#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
        if(mFirstBuff == true){
            mFrameInvAdjust  = 0;		
            mFrameInv = 0;
            mFirstBuff = false;	
            mCache.index = -1;
            mCache.bufPtr == NULL;
            mCache.canvas = 0;
            ptr = (uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index));
            gettimeofday(&previewTime1, NULL);
            CAMHAL_LOGDA("Get first preview buff");
        }else{
            gettimeofday( &previewTime2, NULL);
            int bwFrames_tmp = previewTime2.tv_sec - previewTime1.tv_sec;
            bwFrames_tmp = bwFrames_tmp*1000000 + previewTime2.tv_usec -previewTime1.tv_usec;
            mFrameInv += bwFrames_tmp;
            memcpy( &previewTime1, &previewTime2, sizeof( struct timeval));

            active_duration = mFrameInv - mFrameInvAdjust;
            if((mFrameInv + 20000 > (int)mExpectedFrameInv) //kTestSlopMargin = 20ms from CameraGLTest
              &&((active_duration>previewframeduration)||((active_duration + 5000)>previewframeduration))){  // more preview duration -5000 us
                    if(noFrame == false){     //current catch a picture,use it and release tmp buf;	
                        if( mCache.index != -1){
                            fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(mCache.index)), CameraFrame::PREVIEW_FRAME_SYNC);
                        }
                        mCache.index = -1;
                        mCache.canvas = 0;
                    }else if(mCache.index != -1){  //current catch no picture,but have a tmp buf;
                        fp = mCache.bufPtr;
                        ptr = (uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(mCache.index));
                        index = mCache.index;
                        canvas_id = mCache.canvas;
                        mCache.index = -1;
                        mCache.canvas = 0;
                    }else{
                        return 0;
                    }
            } else{ // during this period,should not show any picture,so we cache the current picture,and release the old one firstly;
                if(noFrame == false){	
                    mCache.bufPtr = fp;
                    if(mCache.index != -1){
                        fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(mCache.index)), CameraFrame::PREVIEW_FRAME_SYNC);
                    }
                    mCache.index = index;
                    mCache.canvas = canvas_id;
                }
                return 0;	
            }
        }

        while(active_duration>=(int) previewframeduration){  // skip one or more than one frame
            active_duration -= previewframeduration;
        }

        if((active_duration+10000)>previewframeduration)
            mFrameInvAdjust = previewframeduration - active_duration;
        else
            mFrameInvAdjust = -active_duration;
        mFrameInv = 0;
#endif

        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
        uint8_t* dest = NULL;
#ifdef AMLOGIC_CAMERA_OVERLAY_SUPPORT
        camera_memory_t* VideoCameraBufferMemoryBase = (camera_memory_t*)ptr;
        dest = (uint8_t*)VideoCameraBufferMemoryBase->data; //ptr;
#else
        private_handle_t* gralloc_hnd = (private_handle_t*)ptr;
        dest = (uint8_t*)gralloc_hnd->base; //ptr;
#endif
        uint8_t* src = (uint8_t*) fp;

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        if (mFailedCnt == 0)
            gettimeofday(&mStartTime, NULL);
#endif
        if (mSensorFormat == V4L2_PIX_FMT_MJPEG) { //enable mjpeg
            if (CameraFrame::PIXEL_FMT_YV12 == mPixelFormat) {
               if(ConvertToI420(src, mVideoInfo->buf.bytesused, dest, width,
                dest + width * height + width * height / 4, (width + 1) / 2,
                dest + width * height, (width + 1) / 2, 0, 0, width, height,
                width, height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
                   fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index)),
                        CameraFrame::PREVIEW_FRAME_SYNC);
                   CAMHAL_LOGEB("jpeg decode failed,src:%02x %02x %02x %02x",
                        src[0], src[1], src[2], src[3]);

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
                   mFailedCnt++;
                   gettimeofday(&mEndTime, NULL);
                   int intreval = (mEndTime.tv_sec - mStartTime.tv_sec) * 1000000 +
                        (mEndTime.tv_usec - mStartTime.tv_usec);
                   if (intreval > (int)mResetTH) {
                        CAMHAL_LOGIA("MJPEG Stream error ! Restart Preview");
                        force_reset_sensor();
                        mFailedCnt = 0;
                        mFirstBuff = true;
                   }
#endif
                   return -1;
               }
            } else if (CameraFrame::PIXEL_FMT_NV21 == mPixelFormat) {
               if (ConvertMjpegToNV21(src, mVideoInfo->buf.bytesused, dest, width,
                dest + width * height, (width + 1) / 2,
                width, height, width, height, libyuv::FOURCC_MJPG) != 0) {
                   uint8_t *vBuffer = new uint8_t[width * height / 4];
                   if (vBuffer == NULL)
                        CAMHAL_LOGIA("alloc temperary v buffer failed\n");
                   uint8_t *uBuffer = new uint8_t[width * height / 4];
                   if (uBuffer == NULL)
                        CAMHAL_LOGIA("alloc temperary u buffer failed\n");

                   if(ConvertToI420(src, mVideoInfo->buf.bytesused, dest,
                    width, uBuffer, (width + 1) / 2,
                    vBuffer, (width + 1) / 2, 0, 0, width, height,
                    width, height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
                        fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index)),
                           CameraFrame::PREVIEW_FRAME_SYNC);
                        CAMHAL_LOGEB("jpeg decode failed,src:%02x %02x %02x %02x",
                           src[0], src[1], src[2], src[3]);

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
                        mFailedCnt++;
                        gettimeofday(&mEndTime, NULL);
                        int intreval = (mEndTime.tv_sec - mStartTime.tv_sec) * 1000000 +
                          (mEndTime.tv_usec - mStartTime.tv_usec);
                        if (intreval > (int)mResetTH) {
                           CAMHAL_LOGIA("MJPEG Stream error ! Restart Preview");
                           force_reset_sensor();
                           mFailedCnt = 0;
                           mFirstBuff = true;
                        }
#endif

                        delete vBuffer;
                        delete uBuffer;
                        return -1;
               }

               uint8_t *pUVBuffer = dest + width * height;
               for (int i = 0; i < width * height / 4; i++) {
               *pUVBuffer++ = *(vBuffer + i);
               *pUVBuffer++ = *(uBuffer + i);
               }

               delete vBuffer;
               delete uBuffer;
               }
            }
            mFailedCnt = 0;
            frame.mLength = width*height*3/2;
        }else{
            if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ // 422I
                frame.mLength = width*height*2;
                memcpy(dest,src,frame.mLength);
            }else if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){ //420sp
                frame.mLength = width*height*3/2;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
                if ( CameraFrame::PIXEL_FMT_NV21 == mPixelFormat){
                    //convert yuyv to nv21
                    yuyv422_to_nv21(src,dest,width,height);
                }else{
                    yuyv_to_yv12( src, dest, width, height);
                }
#else
                if (DEV_ION != m_eDeviceType) {
                        if ( CameraFrame::PIXEL_FMT_NV21 == mPixelFormat){
                            if (frame.mLength == mVideoInfo->buf.length) {
                                    memcpy(dest,src,frame.mLength);
                            }else if((mVideoInfo->canvas_mode == true)&&(width == 1920)&&(height == 1080)){
                                    nv21_memcpy_canvas1080 (dest, src, width, height);
                            }else{
                                    nv21_memcpy_align32 (dest, src, width, height);
                            }
                        }else{
                            if (frame.mLength == mVideoInfo->buf.length) {
                                    yv12_adjust_memcpy(dest,src,width,height);
                            }else if((mVideoInfo->canvas_mode == true)&&(width == 1920)&&(height == 1080)){
                                    yv12_memcpy_canvas1080 (dest, src, width, height);
                            }else{
                                    yv12_memcpy_align32 (dest, src, width, height);
                            }
                        }
                } else {

                        if ( CameraFrame::PIXEL_FMT_NV21 == mPixelFormat){
                            //if (frame.mLength != mVideoInfo->buf.length) {
                            if (width%32) {
                                    CAMHAL_LOGDB("frame.mLength =%d, mVideoInfo->buf.length=%d\n",
                                                    frame.mLength,  mVideoInfo->buf.length);
                                    nv21_memcpy_align32 (dest, src, width, height);
                            }
                        }else{
                            //if (frame.mLength != mVideoInfo->buf.length) {
                            if (width%64) {
                                    CAMHAL_LOGDB("frame.mLength =%d, mVideoInfo->buf.length=%d\n",
                                                    frame.mLength,  mVideoInfo->buf.length);
                                    yv12_memcpy_align32 (dest, src, width, height);
                            }
                        }
                }
#endif
            }else{ //default case
                frame.mLength = width*height*3/2;
                memcpy(dest,src,frame.mLength);            
            }
        }

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
			char property[PROPERTY_VALUE_MAX];
	        int enable = 0;
	        memset(property,0,sizeof(property));
	        if(property_get("camera.preview.EnableDump", property, NULL) > 0){
	            enable = atoi(property);
	        }
	        mEnableDump = enable > 0 ? true : false;
	       	CAMHAL_LOGDB("mEnableDump:%d",mEnableDump);
	        if(mEnableDump){
	        	char filename[50];
	        	memset(filename, 0 , 50);
	        	sprintf(filename,"%s%d%s",DUMP_FILE,mDumpCnt,".yuv");
	        	FILE *fdump;
	        	if((fdump = fopen(filename,"w")) != NULL){
	        		fwrite(dest, frame.mLength, 1, fdump);
	        		CAMHAL_LOGDB("previewthread dump frame:%d,length:%d",mDumpCnt,frame.mLength);
	        		fclose(fdump);	
	        	}else
	        		CAMHAL_LOGDB("open failed :%s",strerror(errno));
	        	mDumpCnt++;	
	        }
#endif
        frame.mFrameMask |= CameraFrame::PREVIEW_FRAME_SYNC;

        if(mRecording){
            frame.mFrameMask |= CameraFrame::VIDEO_FRAME_SYNC;
        }
        frame.mBuffer = ptr; //dest
        frame.mAlignment = width;
        frame.mOffset = 0;
        frame.mYuv[0] = 0;
        frame.mYuv[1] = 0;
#ifdef ION_MODE_FOR_METADATA_MODE
        if (ion_mode) {
            int iret;
            struct meson_phys_data phy_data = {
                .handle = ((private_handle_t *)ptr)->share_fd,
                .phys_addr = 0,
                .size = 0,
            };
            struct ion_custom_data custom_data = {
                .cmd = ION_IOC_MESON_PHYS_ADDR,
                .arg = (unsigned long)&phy_data,
            };
            if (mPhyAddr[index] == 0) {
                iret = ioctl(mIonClient, ION_IOC_CUSTOM, (unsigned long)&custom_data);
                if (iret < 0) {
                    CAMHAL_LOGEB("ion custom ioctl %x failed with code %d: %s\n",
                        ION_IOC_MESON_PHYS_ADDR,
                        iret, strerror(errno));
                    frame.metadataBufferType = kMetadataBufferTypeGrallocSource;
                    frame.mCanvas = 0;
                } else {
                    frame.mCanvas = phy_data.phys_addr;
                    mPhyAddr[index] = phy_data.phys_addr;
                    frame.metadataBufferType = kMetadataBufferTypeGrallocSource;
                }
            } else {
                frame.mCanvas = mPhyAddr[index];
                frame.metadataBufferType = kMetadataBufferTypeGrallocSource;
            }
        } else {
            frame.mCanvas = canvas_id;
            frame.metadataBufferType = (canvas_id != 0) ? kMetadataBufferTypeCanvasSource : kMetadataBufferTypeGrallocSource;
        }
#else
        frame.mCanvas = canvas_id;
        frame.metadataBufferType = kMetadataBufferTypeGrallocSource;
#endif
        if (canvas_id != 0) {
            frame.mCanvas = canvas_id;
            frame.metadataBufferType = kMetadataBufferTypeCanvasSource;
        }
        frame.mWidth = width;
        frame.mHeight = height;
        frame.mPixelFmt = mPixelFormat;
        frame.mColorFormat = ((private_handle_t *)ptr)->format;
        ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
        if (ret){
            CAMHAL_LOGEB("setInitFrameRefCount err=%d", ret);
        }else{
            ret = sendFrameToSubscribers(&frame);
        }
    }
    if( (mIoctlSupport & IOCTL_MASK_FOCUS_MOVE) && mFocusMoveEnabled ){
        getFocusMoveStatus();
    }
    return ret;
}

/* Image Capture Thread */
// ---------------------------------------------------------------------------
int V4LCameraAdapter::GenExif(ExifElementsTable* exiftable)
{
    char exifcontent[256];

    //Make
    exiftable->insertElement("Make",(const char*)mParams.get(ExCameraParameters::KEY_EXIF_MAKE));

    //Model
    exiftable->insertElement("Model",(const char*)mParams.get(ExCameraParameters::KEY_EXIF_MODEL));

    //Image orientation
    int orientation = mParams.getInt(CameraParameters::KEY_ROTATION);
    //covert 0 90 180 270 to 0 1 2 3
    CAMHAL_LOGDB("get orientaion %d",orientation);
    if(orientation == 0)
        orientation = 1;
    else if(orientation == 90)
        orientation = 6;
    else if(orientation == 180)
        orientation = 3;
    else if(orientation == 270)
        orientation = 8;

    //Image width,height
    int width,height;
    if((mCaptureWidth <= 0)||(mCaptureHeight <= 0)){
        mParams.getPictureSize(&width, &height);
    }else{
        width = mCaptureWidth;
        height = mCaptureHeight;
    }

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        orientation = 1;
        if((mRotateValue==90)||(mRotateValue==270)){
            int temp = width;
            width = height;
            height = temp;
        }
    }
#endif

    sprintf(exifcontent,"%d",orientation);
    //LOGD("exifcontent %s",exifcontent);
    exiftable->insertElement("Orientation",(const char*)exifcontent);
    
    sprintf(exifcontent,"%d",width);
    exiftable->insertElement("ImageWidth",(const char*)exifcontent);
    sprintf(exifcontent,"%d",height);
    exiftable->insertElement("ImageLength",(const char*)exifcontent);

    //focal length  RATIONAL
    float focallen = mParams.getFloat(CameraParameters::KEY_FOCAL_LENGTH);
    if(focallen >= 0){
        int focalNum = focallen*1000;
        int focalDen = 1000;
        sprintf(exifcontent,"%d/%d",focalNum,focalDen);
        exiftable->insertElement("FocalLength",(const char*)exifcontent);
    }

    //datetime of photo
    time_t times;
    {
        time(&times);
        struct tm tmstruct;
        tmstruct = *(localtime(&times)); //convert to local time

        //date&time
        strftime(exifcontent, 30, "%Y:%m:%d %H:%M:%S", &tmstruct);
        exiftable->insertElement("DateTime",(const char*)exifcontent);
    }

    //gps date stamp & time stamp
    times = mParams.getInt(CameraParameters::KEY_GPS_TIMESTAMP);
    if(times != -1){
        struct tm tmstruct;
        tmstruct = *(gmtime(&times));//convert to standard time
        //date
        strftime(exifcontent, 20, "%Y:%m:%d", &tmstruct);
        exiftable->insertElement("GPSDateStamp",(const char*)exifcontent);
        //time
        sprintf(exifcontent,"%d/%d,%d/%d,%d/%d",tmstruct.tm_hour,1,tmstruct.tm_min,1,tmstruct.tm_sec,1);
        exiftable->insertElement("GPSTimeStamp",(const char*)exifcontent);
    }

    //gps latitude info
    char* latitudestr = (char*)mParams.get(CameraParameters::KEY_GPS_LATITUDE);
    if(latitudestr!=NULL){
        int offset = 0;
        float latitude = mParams.getFloat(CameraParameters::KEY_GPS_LATITUDE);
        if(latitude < 0.0){
            offset = 1;
            latitude*= (float)(-1);
        }

        int latitudedegree = latitude;
        float latitudeminuts = (latitude-(float)latitudedegree)*60;
        int latitudeminuts_int = latitudeminuts;
        float latituseconds = (latitudeminuts-(float)latitudeminuts_int)*60+0.5;
        int latituseconds_int = latituseconds;
        sprintf(exifcontent,"%d/%d,%d/%d,%d/%d",latitudedegree,1,latitudeminuts_int,1,latituseconds_int,1);
        exiftable->insertElement("GPSLatitude",(const char*)exifcontent);
        exiftable->insertElement("GPSLatitudeRef",(offset==1)?"S":"N");
    }

    //gps Longitude info
    char* longitudestr = (char*)mParams.get(CameraParameters::KEY_GPS_LONGITUDE);
    if(longitudestr!=NULL){
        int offset = 0;
        float longitude = mParams.getFloat(CameraParameters::KEY_GPS_LONGITUDE);
        if(longitude < 0.0){
            offset = 1;
            longitude*= (float)(-1);
        }

        int longitudedegree = longitude;
        float longitudeminuts = (longitude-(float)longitudedegree)*60;
        int longitudeminuts_int = longitudeminuts;
        float longitudeseconds = (longitudeminuts-(float)longitudeminuts_int)*60+0.5;
        int longitudeseconds_int = longitudeseconds;
        sprintf(exifcontent,"%d/%d,%d/%d,%d/%d",longitudedegree,1,longitudeminuts_int,1,longitudeseconds_int,1);
        exiftable->insertElement("GPSLongitude",(const char*)exifcontent);
        exiftable->insertElement("GPSLongitudeRef",(offset==1)?"S":"N");
    }

    //gps Altitude info
    char* altitudestr = (char*)mParams.get(CameraParameters::KEY_GPS_ALTITUDE);
    if(altitudestr!=NULL){
        int offset = 0;
        float altitude = mParams.getFloat(CameraParameters::KEY_GPS_ALTITUDE);
        if(altitude < 0.0){
            offset = 1;
            altitude*= (float)(-1);
        }

        int altitudenum = altitude*1000;
        int altitudedec= 1000;
        sprintf(exifcontent,"%d/%d",altitudenum,altitudedec);
        exiftable->insertElement("GPSAltitude",(const char*)exifcontent);
        sprintf(exifcontent,"%d",offset);
        exiftable->insertElement("GPSAltitudeRef",(const char*)exifcontent);
    }

    //gps processing method
    char* processmethod = (char*)mParams.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    if(processmethod!=NULL){
        memset(exifcontent,0,sizeof(exifcontent));
        char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };//asicii
        memcpy(exifcontent,ExifAsciiPrefix,8);
        memcpy(exifcontent+8,processmethod,strlen(processmethod));
        exiftable->insertElement("GPSProcessingMethod",(const char*)exifcontent);
    }
    return 1;
}

/*static*/ int V4LCameraAdapter::beginPictureThread(void *cookie)
{
    V4LCameraAdapter *c = (V4LCameraAdapter *)cookie;
    return c->pictureThread();
}

int V4LCameraAdapter::pictureThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;
    int         dqTryNum = 3;

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    setMirrorEffect();
#endif

    if( (mIoctlSupport & IOCTL_MASK_FLASH)&&(FLASHLIGHT_ON == mFlashMode)){
        set_flash_mode( mCameraHandle, "on");
    }
    //if (true){
        mVideoInfo->buf.index = 0;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;
        if (V4L2_MEMORY_DMABUF == m_eV4l2Memory) {
                mVideoInfo->buf.m.fd = mImageFd;
        }

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        if(mIsDequeuedEIOError){
            CAMHAL_LOGEA("DQBUF EIO has occured!\n");
            return -EINVAL;
        }
#endif
        ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0){
            CAMHAL_LOGDB("VIDIOC_QBUF Failed, errno=%s\n", strerror(errno));
            return -EINVAL;
        }
        nQueued ++;

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
        if(mIoctlSupport & IOCTL_MASK_ROTATE){
            if(mCaptureOriation!=0){
                set_rotate_value(mCameraHandle,mCaptureOriation); 
                mCaptureOriation=0;
            }else{
                set_rotate_value(mCameraHandle,mRotateValue);
            }
        }
#endif

        enum v4l2_buf_type bufType;
        if (!mVideoInfo->isStreaming){
            bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
            if (ret < 0) {
                CAMHAL_LOGEB("StartStreaming: Unable to start capture: %s", strerror(errno));
                return ret;
            }
            mVideoInfo->isStreaming = true;
        }

        int index = 0;
        unsigned int canvas_id = 0;
        char *fp = this->GetFrame(index,&canvas_id);
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        while( (mSensorFormat == V4L2_PIX_FMT_YUYV) &&
               (mVideoInfo->buf.length != mVideoInfo->buf.bytesused) && (dqTryNum>0)) {
            if(NULL != fp){
                mVideoInfo->buf.index = 0;
                mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                mVideoInfo->buf.memory = m_eV4l2Memory;

                if(mIsDequeuedEIOError){
                    CAMHAL_LOGEA("DQBUF EIO has occured!\n");
                    break;
                }

                ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
                if (ret < 0){
                    CAMHAL_LOGEB("VIDIOC_QBUF Failed errno=%d\n", errno);
                    break;
                }
                nQueued ++;
                dqTryNum --;
            }

#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
            usleep( 10000 );
#endif
            fp = this->GetFrame(index,&canvas_id);
	}
#endif

#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
        while(!fp && (-1 == index)){
            usleep( 10000 );
            fp = this->GetFrame(index,&canvas_id);
        }
#else
        if(!fp){
            CAMHAL_LOGDA("GetFrame fail, this may stop preview\n");
            return 0; //BAD_VALUE;
        }
#endif
        if (!mCaptureBuf || !mCaptureBuf->data){
            return 0; //BAD_VALUE;
        }

        uint8_t* dest = (uint8_t*)mCaptureBuf->data;
        uint8_t* src = (uint8_t*) fp;
        if((mCaptureWidth <= 0)||(mCaptureHeight <= 0)){
            mParams.getPictureSize(&width, &height);
        }else{
            width = mCaptureWidth;
            height = mCaptureHeight;
        }
        
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
        if((mRotateValue==90)||(mRotateValue==270)){
            int temp = 0;
            temp = width;
            width = height;
            height = temp;
        }
#endif

        CAMHAL_LOGDB("mCaptureBuf=%p,dest=%p,fp=%p,index=%d\n"
                     "w=%d h=%d,len=%d,bytesused=%d\n",
                     mCaptureBuf, dest, fp,index, width, height,
                     mVideoInfo->buf.length, mVideoInfo->buf.bytesused);

        if(mSensorFormat == V4L2_PIX_FMT_MJPEG){
            if (CameraFrame::PIXEL_FMT_YV12 == mPixelFormat) {
               if(ConvertToI420(src, mVideoInfo->buf.bytesused, dest, width,
                dest + width * height + width * height / 4, (width + 1) / 2,
                dest + width * height, (width + 1) / 2, 0, 0, width, height,
                width, height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
                   fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index)),
                      CameraFrame::PREVIEW_FRAME_SYNC);
                   CAMHAL_LOGEB("jpeg decode failed,src:%02x %02x %02x %02x",
                      src[0], src[1], src[2], src[3]);
                   return -1;
               }
            } else if (CameraFrame::PIXEL_FMT_NV21 == mPixelFormat) {
               if(ConvertMjpegToNV21(src, mVideoInfo->buf.bytesused, dest,
                 width, dest + width * height, (width + 1) / 2,
                 width, height, width, height, libyuv::FOURCC_MJPG) != 0) {
                   uint8_t *vBuffer = new uint8_t[width * height / 4];
                   if (vBuffer == NULL)
                      CAMHAL_LOGIA("alloc temperary v buffer failed\n");
                   uint8_t *uBuffer = new uint8_t[width * height / 4];
                   if (uBuffer == NULL)
                      CAMHAL_LOGIA("alloc temperary u buffer failed\n");

                   if(ConvertToI420(src, mVideoInfo->buf.bytesused, dest,
                    width, uBuffer, (width + 1) / 2,
                    vBuffer, (width + 1) / 2, 0, 0, width, height,
                    width, height, libyuv::kRotate0, libyuv::FOURCC_MJPG) != 0) {
                      fillThisBuffer((uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index)),
                        CameraFrame::PREVIEW_FRAME_SYNC);
                      CAMHAL_LOGEB("jpeg decode failed,src:%02x %02x %02x %02x",
                        src[0], src[1], src[2], src[3]);
                      delete vBuffer;
                      delete uBuffer;
                      return -1;
                   }

                   uint8_t *pUVBuffer = dest + width * height;
                   for (int i = 0; i < width * height / 4; i++) {
                      *pUVBuffer++ = *(vBuffer + i);
                      *pUVBuffer++ = *(uBuffer + i);
                   }

                   delete vBuffer;
                   delete uBuffer;
               }
            }

            frame.mLength = width*height*3/2;
            frame.mQuirks = CameraFrame::ENCODE_RAW_YUV420SP_TO_JPEG | CameraFrame::HAS_EXIF_DATA;
               
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_RGB24){ // rgb24
            frame.mLength = width*height*3;
            frame.mQuirks = CameraFrame::ENCODE_RAW_RGB24_TO_JPEG | CameraFrame::HAS_EXIF_DATA;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
            //convert yuyv to rgb24
            yuyv422_to_rgb24(src,dest,width,height);
#else
            DBG_LOGB("frame.mLength =%d, mVideoInfo->buf.length=%d\n",frame.mLength, mVideoInfo->buf.length);
            if (frame.mLength == mVideoInfo->buf.length) {
                    memcpy (dest, src, frame.mLength);
            }else{
                    rgb24_memcpy( dest, src, width, height);
                    CAMHAL_LOGVB("w*h*3=%d, mLength=%d\n", width*height*3, mVideoInfo->buf.length);
            }
#endif
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ //   422I
            frame.mLength = width*height*2;
            frame.mQuirks = CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG | CameraFrame::HAS_EXIF_DATA;
            memcpy(dest, src, mVideoInfo->buf.length);
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){ //   420sp
            frame.mLength = width*height*3/2;
            frame.mQuirks = CameraFrame::ENCODE_RAW_YUV420SP_TO_JPEG | CameraFrame::HAS_EXIF_DATA;
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
            //convert yuyv to nv21
            yuyv422_to_nv21(src,dest,width,height);
#else
            memcpy(dest,src,mVideoInfo->buf.length);
#endif
        }else{ //default case
            frame.mLength = width*height*3;
            frame.mQuirks = CameraFrame::ENCODE_RAW_RGB24_TO_JPEG | CameraFrame::HAS_EXIF_DATA;
            memcpy(dest, src, mVideoInfo->buf.length);
        }

        notifyShutterSubscribers();
        //TODO correct time to call this?
        if (NULL != mEndImageCaptureCallback)
            mEndImageCaptureCallback(mEndCaptureData);

        //gen  exif message
        ExifElementsTable* exiftable = new ExifElementsTable();
        GenExif(exiftable);

        frame.mFrameMask = CameraFrame::IMAGE_FRAME;
        frame.mFrameType = CameraFrame::IMAGE_FRAME;
        frame.mBuffer = mCaptureBuf->data;
        frame.mCookie2 = (void*)exiftable;
        frame.mAlignment = width;
        frame.mOffset = 0;
        frame.mYuv[0] = 0;
        frame.mYuv[1] = 0;
        frame.mCanvas = canvas_id;
        frame.mWidth = width;
        frame.mHeight = height;
        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
        
        if (mVideoInfo->isStreaming){
            bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
            if (ret < 0){
                CAMHAL_LOGEB("StopStreaming: Unable to stop capture: %s", strerror(errno));
                return ret;
            }
            mVideoInfo->isStreaming = false;
        }

        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;

        nQueued = 0;
        nDequeued = 0;

        /* Unmap buffers */
        if (munmap(mVideoInfo->mem[0], mVideoInfo->buf.length) < 0){
            CAMHAL_LOGEA("Unmap failed");
        }
        mVideoInfo->canvas[0] = 0;

    if ((DEV_USB == m_eDeviceType) ||
        (DEV_ION == m_eDeviceType) ||
        (DEV_ION_MPLANE == m_eDeviceType))
    {
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;
        mVideoInfo->rb.count = 0;

        ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
        if (ret < 0) {
            CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            return ret;
        }else{
            CAMHAL_LOGVA("VIDIOC_REQBUFS delete buffer success\n");
        }
    }

    if( (mIoctlSupport & IOCTL_MASK_FLASH)&&(FLASHLIGHT_ON == mFlashMode)){
        set_flash_mode( mCameraHandle, "off");
    }
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        set_rotate_value(mCameraHandle,0);
        mRotateValue = 0;
    }
#endif

    // start preview thread again after stopping it in UseBuffersCapture
    {
        Mutex::Autolock lock(mPreviewBufferLock);
        UseBuffersPreview(mPreviewBuffers, mPreviewBufferCount);        
    }
    startPreview();
    setCrop( 0, 0); //set to zero and then go preview

    ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
    if (ret){
        CAMHAL_LOGEB("setInitFrameRefCount err=%d", ret);
    }else{
        ret = sendFrameToSubscribers(&frame);
    }
    return ret;
}

status_t V4LCameraAdapter::disableMirror(bool bDisable) 
{
    CAMHAL_LOGDB("disableMirror %d\n",bDisable);
    mbDisableMirror = bDisable;
    setMirrorEffect();
    return NO_ERROR;
}

status_t V4LCameraAdapter::setMirrorEffect() {
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    bool bEnable = mbFrontCamera&&(!mbDisableMirror);
    CAMHAL_LOGDB("setmirror effect %d",bEnable);
    
    if(mIoctlSupport & IOCTL_MASK_HFLIP){
        if(set_hflip_mode(mCameraHandle,bEnable))
            writefile((char *)SYSFILE_CAMERA_SET_MIRROR,(char*)(bEnable?"1":"0"));
    }else{
        writefile((char *)SYSFILE_CAMERA_SET_MIRROR,(char*)(bEnable?"1":"0"));
    }
#endif
    return NO_ERROR;
}

// ---------------------------------------------------------------------------
extern "C" CameraAdapter* CameraAdapter_Factory(size_t sensor_index)
{
    CameraAdapter *adapter = NULL;
    Mutex::Autolock lock(gAdapterLock);

    LOG_FUNCTION_NAME;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( sensor_index == (size_t)(iCamerasNum)){
    //MAX_CAM_NUM_ADD_VCAM-1) ){
        adapter = new V4LCamAdpt(sensor_index);
    }else{
#endif
        adapter = new V4LCameraAdapter(sensor_index);
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    }
#endif

    if ( adapter ) {
        CAMHAL_LOGDB("New V4L Camera adapter instance created for sensor %d", sensor_index);
    } else {
        CAMHAL_LOGEA("Camera adapter create failed!");
    }

    LOG_FUNCTION_NAME_EXIT;
    return adapter;
}

extern "C" int CameraAdapter_Capabilities(CameraProperties::Properties* properties_array,
                                          const unsigned int starting_camera,
                                          const unsigned int camera_num) {
    int num_cameras_supported = 0;
    CameraProperties::Properties* properties = NULL;

    LOG_FUNCTION_NAME;

    if(!properties_array)
        return -EINVAL;

    while (starting_camera + num_cameras_supported < camera_num){
        properties = properties_array + starting_camera + num_cameras_supported;
        properties->set(CameraProperties::CAMERA_NAME, "Camera");
        extern void loadCaps(int camera_id, CameraProperties::Properties* params);
        loadCaps(starting_camera + num_cameras_supported, properties);
        num_cameras_supported++;
    }

    LOG_FUNCTION_NAME_EXIT;
    return num_cameras_supported;
}

extern "C"  int CameraAdapter_CameraNum()
{
#if defined(AMLOGIC_FRONT_CAMERA_SUPPORT) || defined(AMLOGIC_BACK_CAMERA_SUPPORT)
    CAMHAL_LOGDB("CameraAdapter_CameraNum %d",MAX_CAMERAS_SUPPORTED);
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    return MAX_CAM_NUM_ADD_VCAM;
#else
    return MAX_CAMERAS_SUPPORTED;
#endif
#elif  defined ( AMLOGIC_VIRTUAL_CAMERA_SUPPORT)
    iCamerasNum = 0;
    for( int i = 0; i < (int)ARRAY_SIZE(SENSOR_PATH); i++ ){
        if( access(DEVICE_PATH(i), 0) == 0 )
            iCamerasNum++;
    }

    CAMHAL_LOGDB("GetCameraNums %d\n", iCamerasNum+1);
    return iCamerasNum+1;
#elif  defined (AMLOGIC_USB_CAMERA_SUPPORT)
    iCamerasNum = 0;
    for( int i = 0; i < (int)ARRAY_SIZE(SENSOR_PATH); i++ ){
        if( access(DEVICE_PATH(i), 0) == 0 ){
            int camera_fd;
            if((camera_fd = open(DEVICE_PATH(i), O_RDWR)) != -1){
                CAMHAL_LOGIB("try open %s\n", DEVICE_PATH(i));
                close(camera_fd);
                iCamerasNum++;
            }
        }

    }
    iCamerasNum = iCamerasNum > MAX_CAMERAS_SUPPORTED?MAX_CAMERAS_SUPPORTED :iCamerasNum;
    return iCamerasNum;
#else
    CAMHAL_LOGDB("CameraAdapter_CameraNum %d",iCamerasNum);
    if(iCamerasNum == -1){
        iCamerasNum = 0;
        for(int i = 0;i < MAX_CAMERAS_SUPPORTED;i++){
            if( access(DEVICE_PATH(i), 0) == 0 )
                iCamerasNum++;
        }
        CAMHAL_LOGDB("GetCameraNums %d",iCamerasNum);
    }
    return iCamerasNum;
#endif
}

#ifdef AMLOGIC_TWO_CH_UVC
extern "C" bool isPreviewDevice(int camera_fd)
{
    int ret;
    int index;
    struct v4l2_fmtdesc fmtdesc;
	
    for(index=0;;index++){
        memset(&fmtdesc, 0, sizeof(struct v4l2_fmtdesc));
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmtdesc.index = index;
        ret = ioctl( camera_fd, VIDIOC_ENUM_FMT, &fmtdesc);
        if(V4L2_PIX_FMT_YUYV==fmtdesc.pixelformat){ 
            return true;
        }
        if(ret < 0)
            break;
    }
    return false;
}

extern "C" status_t getVideodevId(int &camera_id, int &main_id)
{
    int tmp_id = camera_id;
    int tmp_fd = -1;
    int suc_id = -1;
    int camera_fd = -1;
    int ret = NO_ERROR;
    char cardname[32]="";
    char cardname2[32]="";
    struct v4l2_capability cap;
    bool needPreviewCh=false;
    while(1){
        if ((tmp_fd = open(DEVICE_PATH(tmp_id), O_RDWR)) != -1){
            if(isPreviewDevice(tmp_fd)){
                if(needPreviewCh){
                    memset(&cap, 0, sizeof(struct v4l2_capability));
                    ret = ioctl(tmp_fd,VIDIOC_QUERYCAP,&cap);
                    if(ret < 0){
                        CAMHAL_LOGDB("failed to query %s !\n", DEVICE_PATH(tmp_id));
                    }
                    strncpy(cardname2,(char *)cap.card, sizeof(cardname2));
                    if(strcmp(cardname, cardname2)==0){
                        close(tmp_fd);
                        camera_id = tmp_id;
                        return NO_ERROR;	
                    }
                    suc_id = tmp_id;
                    close(tmp_fd);
                }else{
                    close(tmp_fd);
                    camera_id = tmp_id;
                    return NO_ERROR;
                }
            }else{
                main_id = tmp_id;
                needPreviewCh = true;
                memset(&cap, 0, sizeof(struct v4l2_capability));
                ret = ioctl(tmp_fd,VIDIOC_QUERYCAP,&cap);
                if(ret < 0){
                    CAMHAL_LOGDB("failed to query %s !\n", DEVICE_PATH(tmp_id));
                }
                strncpy(cardname,(char *)cap.card, sizeof(cardname));
                CAMHAL_LOGDB("%s for main channel!\n", DEVICE_PATH(tmp_id));
                close(tmp_fd);
            }
        }
        tmp_id++;
        tmp_id%= ARRAY_SIZE(SENSOR_PATH);
        if(tmp_id ==camera_id){
            needPreviewCh = false;
            camera_id = suc_id;
            return NO_ERROR;
        }
    }
    return NO_ERROR;
}
#endif
int enumFrameFormats(int camera_fd, enum camera_mode_e c)
{
    int ret=0;
    struct v4l2_fmtdesc fmt;
    int i;
    int size = 0;

    struct camera_fmt cam_fmt_preview[] = {
            {
                    .pixelfmt = V4L2_PIX_FMT_NV21,
                    .support  = 0,
            },{
                    .pixelfmt = V4L2_PIX_FMT_MJPEG,
                    .support  = 0,
            },{
                    .pixelfmt = V4L2_PIX_FMT_YUYV,
                    .support  = 0,
            },
    };

    struct camera_fmt cam_fmt_capture[] = {
            {
                    .pixelfmt = V4L2_PIX_FMT_RGB24,
                    .support  = 0,
            },{
                    .pixelfmt = V4L2_PIX_FMT_MJPEG,
                    .support  = 0,
            },{
                    .pixelfmt = V4L2_PIX_FMT_YUYV,
                    .support  = 0,
            },
    };

    struct camera_fmt *cam_fmt = cam_fmt_preview;
    size = ARRAY_SIZE(cam_fmt_preview);
    if (CAM_PREVIEW == c){
            cam_fmt = cam_fmt_preview;
            size = ARRAY_SIZE(cam_fmt_preview);
    } else if (CAM_CAPTURE == c){
            cam_fmt = cam_fmt_capture;
            size = ARRAY_SIZE(cam_fmt_capture);
    }if (CAM_RECORD == c){
            cam_fmt = cam_fmt_preview;
            size = ARRAY_SIZE(cam_fmt_preview);
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while ((ret = ioctl(camera_fd, VIDIOC_ENUM_FMT, &fmt)) == 0)
    {
            fmt.index++;

            CAMHAL_LOGVB("{ pixelformat = '%.4s', description = '%s' }\n",
                            (char *)&fmt.pixelformat,
                            fmt.description);
            for (i = 0; i<size; i++) {
                    if (fmt.pixelformat == cam_fmt[i].pixelfmt) {
                            cam_fmt[i].support = 1;
                            break;
                    }
            }

    }
    if (errno != EINVAL) {
        CAMHAL_LOGDA("VIDIOC_ENUM_FMT - Error enumerating frame formats");
    }

    for (i = 0; i<size; i++) {
            if (1 == cam_fmt[i].support) {
                    return cam_fmt[i].pixelfmt;
            }
    }

    CAMHAL_LOGDA("no camera format found\n");

    return CAM_CAPTURE==c ? V4L2_PIX_FMT_RGB24:V4L2_PIX_FMT_NV21;
}

extern "C" int getValidFrameSize(int camera_fd, int pixel_format, char *framesize, bool preview)
{
    struct v4l2_frmsizeenum frmsize;
    int i=0;
    char tempsize[12];
    framesize[0] = '\0';
    unsigned int support_w,support_h;
    if(preview == true){
        char property[PROPERTY_VALUE_MAX];
        support_w = 10000;
        support_h = 10000;
        memset(property,0,sizeof(property));
        if (property_get("ro.media.camera_preview.maxsize", property, NULL) > 0) {
            CAMHAL_LOGDB("support Max Preview Size :%s",property);
            if(sscanf(property,"%dx%d",&support_w,&support_h)!=2){
                support_w = 10000;
                support_h = 10000;
            }    
        }
    }
    if (camera_fd >= 0) {
        memset(&frmsize,0,sizeof(v4l2_frmsizeenum));
        for(i=0;;i++){
            frmsize.index = i;
            frmsize.pixel_format = pixel_format;
            if(ioctl(camera_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
                if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE){ //only support this type
                    if( preview && (frmsize.discrete.width > support_w) && (frmsize.discrete.height >support_h))
                        continue;
                    if( preview && (0 != (frmsize.discrete.width%16)))
                        continue;
                    snprintf(tempsize, sizeof(tempsize), "%dx%d,", frmsize.discrete.width, frmsize.discrete.height);
                    DBG_LOGB("tmpsize=%s", tempsize);
                    strcat(framesize, tempsize);
                }else{
                    break;
                }
            }else{
                break;
            }
        }
    }
    if(framesize[0] == '\0')
        return -1;
    else
        return 0;
}

static int getCameraOrientation(bool frontcamera, char* p)
{
    int degree = -1;
    char property[PROPERTY_VALUE_MAX];
    if(frontcamera){
        if (property_get("ro.camera.orientation.front", property, NULL) > 0){
            degree = atoi(property);
        }
    }else{
        if (property_get("ro.camera.orientation.back", property, NULL) > 0){
            degree = atoi(property);
        }
    }
    if((degree != 0)&&(degree != 90)
      &&(degree != 180)&&(degree != 270))
        degree = -1;

    memcpy( p, property, sizeof(property));
    return degree;
}

static bool is_mjpeg_supported(int camera_fd)
{
    struct v4l2_fmtdesc fmt;
    int ret;
    memset(&fmt,0,sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while((ret = ioctl(camera_fd,VIDIOC_ENUM_FMT,&fmt)) == 0){
        if(fmt.pixelformat == V4L2_PIX_FMT_MJPEG){
            return true;
        }
        fmt.index++;
    }
    return false;
}

static void ParserLimitedRateInfo(LimitedRate_t* rate)
{
    char property[PROPERTY_VALUE_MAX];
    int w,h,r;
    char* pos = NULL;
    memset(property,0,sizeof(property));
    rate->num = 0;
    if (property_get("ro.media.camera_preview.limitedrate", property, NULL) > 0) {
        pos = &property[0];
        while((pos != NULL)&&(rate->num<MAX_LIMITED_RATE_NUM)){
            if(sscanf(pos,"%dx%dx%d",&w,&h,&r)!=3){
                break;
            }
            rate->arg[rate->num].width = w;
            rate->arg[rate->num].height = h;
            rate->arg[rate->num].framerate = r;
            rate->num++;
            pos = strchr(pos, ',');
            if(pos)
                pos++;
        }
    }
}

static int enumCtrlMenu(int camera_fd, struct v4l2_queryctrl *qi, char* menu_items, char*def_menu_item)
{
    struct v4l2_queryctrl qc;
    struct v4l2_querymenu qm;
    int ret;
    int mode_count = -1;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = qi->id;
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if( (ret<0) || (qc.flags == V4L2_CTRL_FLAG_DISABLED)){
        CAMHAL_LOGDB("camera handle %d can't support this ctrl",camera_fd);
        return mode_count;
    }else if( qc.type != V4L2_CTRL_TYPE_MENU){
        CAMHAL_LOGDB("this ctrl of camera handle %d can't support menu type",camera_fd);
        return 0;
    }else{
        memset(&qm, 0, sizeof(qm));
        qm.id = qi->id;
        qm.index = qc.default_value;
        if(ioctl (camera_fd, VIDIOC_QUERYMENU, &qm) < 0){
            return 0;
        } else {
            strcpy(def_menu_item, (char*)qm.name);
        }
        int index = 0;
        mode_count = 0;

        for (index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = qi->id;
            qm.index = index;
            if(ioctl (camera_fd, VIDIOC_QUERYMENU, &qm) < 0){
                continue;
            } else {
                if(mode_count>0)
                    strcat(menu_items, ",");
                strcat( menu_items, (char*)qm.name);
                mode_count++;
            }
        }
    }
    return mode_count;
}

static bool getCameraWhiteBalance(int camera_fd, char* wb_modes, char*def_wb_mode)
{
    struct v4l2_queryctrl qc;
    int item_count=0;

    memset( &qc, 0, sizeof(qc));
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    qc.id = V4L2_CID_AUTO_WHITE_BALANCE;
#else
    qc.id = V4L2_CID_DO_WHITE_BALANCE;
#endif
    item_count = enumCtrlMenu( camera_fd, &qc, wb_modes, def_wb_mode);
    if(0 >= item_count){
        strcpy( wb_modes, "auto,daylight,incandescent,fluorescent");
        strcpy(def_wb_mode, "auto");
    }
    return true;
}

static bool getCameraBanding(int camera_fd, char* banding_modes, char*def_banding_mode)
{
    struct v4l2_queryctrl qc;
    int item_count=0;
    char *tmpbuf=NULL;

    memset( &qc, 0, sizeof(qc));
    qc.id = V4L2_CID_POWER_LINE_FREQUENCY;
    item_count = enumCtrlMenu( camera_fd, &qc, banding_modes, def_banding_mode);
    
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    char *b;
    tmpbuf = (char *) calloc (1, 256);
    memset( tmpbuf, 0, 256);
    if( (0 < item_count)&&( NULL!= tmpbuf)){
        char *tmp =NULL;
        item_count =0;
        tmp = strstr( banding_modes, "auto");
        if(tmp){
            item_count ++;
            strcat( tmpbuf, "auto,");
        }
        tmp = strstr( banding_modes, "isable");//Disabled
        if(tmp){
            item_count ++;
            strcat( tmpbuf, "off,");
        }
        tmp = strstr( banding_modes, "50");
        if(tmp){
            item_count ++;
            strcat( tmpbuf, "50hz,");	
        }
        tmp = strstr( banding_modes, "60");
        if(tmp){
            item_count ++;
            strcat( tmpbuf, "60hz,");	
        }

        b = strrchr(tmpbuf, ',');
        if(NULL != b){
            b[0] = '\0';
        }
        strcpy( banding_modes, tmpbuf);
        memset(tmpbuf, 0, 256);
        if( NULL != (tmp = strstr(def_banding_mode, "50")) ){
            strcat(tmpbuf, "50hz");	
        }else if( NULL != (tmp = strstr(def_banding_mode, "60")) ){
            strcat(tmpbuf, "60hz");	
        }else if( NULL != (tmp = strstr(def_banding_mode, "isable")) ){
            strcat(tmpbuf, "off");	
        }else if( NULL != (tmp = strstr(def_banding_mode, "auto")) ){
            strcat(tmpbuf, "auto");	
        }
        strcpy( def_banding_mode, tmpbuf);
    }

    if(tmpbuf){
        free(tmpbuf);
        tmpbuf = NULL;
    }
#endif

    if(0 >= item_count){
        strcpy( banding_modes, "50hz,60hz");
        strcpy( def_banding_mode, "50hz");
    }
    if (NULL == strstr(banding_modes, "auto")) {
        strcat( banding_modes, ",auto");
    }

    return true;
}

#define MAX_LEVEL_FOR_EXPOSURE 16
#define MIN_LEVEL_FOR_EXPOSURE 3

static bool getCameraExposureValue(int camera_fd, int &min, int &max, int &step, int &def)
{
    struct v4l2_queryctrl qc;
    int ret=0;
    int level = 0;
    int middle = 0;

    memset( &qc, 0, sizeof(qc));

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    qc.id = V4L2_CID_EXPOSURE_ABSOLUTE;
#else
    qc.id = V4L2_CID_EXPOSURE;
#endif
    ret = ioctl( camera_fd, VIDIOC_QUERYCTRL, &qc);
    if(ret<0){
        CAMHAL_LOGDB("QUERYCTRL failed, errno=%d\n", errno);
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        min = 0;
        max = 0;
        def = 0;
        step = 0;
#else
        min = -4;
        max = 4;
        def = 0;
        step = 1;
#endif
        return true;
    }

    if(0 < qc.step)
        level = ( qc.maximum - qc.minimum + 1 )/qc.step;

    if((level > MAX_LEVEL_FOR_EXPOSURE)
      || (level < MIN_LEVEL_FOR_EXPOSURE)){
        min = -4;
        max = 4;
        def = 0;
        step = 1;
        CAMHAL_LOGDB("not in[min,max], min=%d, max=%d, def=%d, step=%d\n", min, max, def, step);
        return true;
    }

    middle = (qc.minimum+qc.maximum)/2;
    min = qc.minimum - middle; 
    max = qc.maximum - middle;
    def = qc.default_value - middle;
    step = qc.step;
    return true;
}

static bool getCameraAutoFocus(int camera_fd, char* focus_mode_str, char*def_focus_mode)
{
    struct v4l2_queryctrl qc;    
    struct v4l2_querymenu qm;
    bool auto_focus_enable = false;
    int menu_num = 0;
    int mode_count = 0;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_FOCUS_AUTO;
    menu_num = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( menu_num < 0) || (qc.type != V4L2_CTRL_TYPE_MENU)){
        auto_focus_enable = false;
        CAMHAL_LOGDB("can't support auto focus,%sret=%d%s\n",
                qc.flags == V4L2_CTRL_FLAG_DISABLED?"disable,":"",
                menu_num,
                qc.type == V4L2_CTRL_TYPE_MENU? "":",type not right");

    }else {
        memset(&qm, 0, sizeof(qm));
        qm.id = V4L2_CID_FOCUS_AUTO;
        qm.index = qc.default_value;
        strcpy(def_focus_mode, "auto");

        for (int index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_FOCUS_AUTO;
            qm.index = index;
            if(ioctl (camera_fd, VIDIOC_QUERYMENU, &qm) < 0){
                continue;
            } else {
                if(mode_count>0)
                    strcat(focus_mode_str, ",");
                strcat(focus_mode_str, (char*)qm.name);
                mode_count++;
            }
        }
        if(mode_count>0)
            auto_focus_enable = true;
    }
    return auto_focus_enable;
}

static bool getCameraFocusArea(int camera_fd, char* max_num_focus_area, char*focus_area)
{
    struct v4l2_queryctrl qc;    
    int ret = 0;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_FOCUS_ABSOLUTE;
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) || (qc.type != V4L2_CTRL_TYPE_INTEGER)){
        CAMHAL_LOGDB("can't support touch focus,%sret=%d%s\n",
            qc.flags == V4L2_CTRL_FLAG_DISABLED? "disble,":"",
            ret,
            qc.type == V4L2_CTRL_TYPE_INTEGER?"":", type not right");
        return false;
    }

    x0 = qc.minimum & 0xFFFF;
    y0 = (qc.minimum >> 16) & 0xFFFF;
    x1 = qc.maximum & 0xFFFF;
    y1 = (qc.maximum >> 16) & 0xFFFF;
    strcpy(max_num_focus_area, "1"); 
    sprintf(focus_area, "(%d,%d,%d,%d, 1)", x0, y0, x1, y1);
    return true;
}

struct v4l2_frmsize_discrete VIDEO_PREFER_SIZES[]={
    {176, 144},
    {320, 240},
    {352, 288},
    {640, 480},
    {1280,720},
    {1920,1080},
};

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
extern "C" void newloadCaps(int camera_id, CameraProperties::Properties* params);
#endif
//TODO move
extern "C" void loadCaps(int camera_id, CameraProperties::Properties* params) {
    const char DEFAULT_BRIGHTNESS[] = "50";
    const char DEFAULT_CONTRAST[] = "100";
    const char DEFAULT_IPP[] = "ldc-nsf";
    const char DEFAULT_GBCE[] = "disable";
    const char DEFAULT_ISO_MODE[] = "auto";
    const char DEFAULT_PICTURE_FORMAT[] = "jpeg";
    const char DEFAULT_PICTURE_SIZE[] = "640x480";
    const char PREVIEW_FORMAT_420SP[] = "yuv420sp";
    const char PREVIEW_FORMAT_420P[] = "yuv420p";
    const char PREVIEW_FORMAT_422I[] = "yuv422i-yuyv";
    const char DEFAULT_PREVIEW_SIZE[] = "640x480";
    const char DEFAULT_NUM_PREV_BUFS[] = "6";
    const char DEFAULT_NUM_PIC_BUFS[] = "1";
    const char DEFAULT_MAX_FOCUS_AREAS[] = "1";
    const char DEFAULT_SATURATION[] = "100";
    const char DEFAULT_SCENE_MODE[] = "auto";
    const char DEFAULT_SHARPNESS[] = "100";
    const char DEFAULT_VSTAB[] = "false";
    const char DEFAULT_VSTAB_SUPPORTED[] = "true";
    const char DEFAULT_MAX_FD_HW_FACES[] = "0";
    const char DEFAULT_MAX_FD_SW_FACES[] = "0";
    const char DEFAULT_FOCAL_LENGTH_PRIMARY[] = "4.31";
    const char DEFAULT_FOCAL_LENGTH_SECONDARY[] = "1.95";
    const char DEFAULT_HOR_ANGLE[] = "54.8";
    const char DEFAULT_VER_ANGLE[] = "42.5";
    const char DEFAULT_AE_LOCK[] = "false";
    const char DEFAULT_AWB_LOCK[] = "false";
    const char DEFAULT_MAX_NUM_METERING_AREAS[] = "0";
    const char DEFAULT_LOCK_SUPPORTED[] = "true";
    const char DEFAULT_LOCK_UNSUPPORTED[] = "false";
    const char DEFAULT_VIDEO_SIZE[] = "640x480";
    const char DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[] = "640x480";

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( camera_id == iCamerasNum){
        //(MAX_CAM_NUM_ADD_VCAM-1)){
        newloadCaps(camera_id, params);
        CAMHAL_LOGDA("return from newloadCaps\n");
        return ;
    }
#endif
    bool bFrontCam = false;
    int camera_fd = -1;

    if (camera_id == 0) {
#ifdef AMLOGIC_BACK_CAMERA_SUPPORT
        bFrontCam = false;
#elif defined(AMLOGIC_FRONT_CAMERA_SUPPORT)
        bFrontCam = true;
#elif defined(AMLOGIC_USB_CAMERA_SUPPORT)
        bFrontCam = true;
#else//defined nothing, we try by ourself.we assume, the 0 is front camera, 1 is back camera
        bFrontCam = true;
#endif
    } else if (camera_id == 1) {
#if defined(AMLOGIC_BACK_CAMERA_SUPPORT) && defined(AMLOGIC_FRONT_CAMERA_SUPPORT)
        bFrontCam = true;
#else//defined nothing, we try  to by ourself
        bFrontCam = false;
#endif
    }

    //should changed while the screen orientation changed.
    int degree = -1;
    char property[PROPERTY_VALUE_MAX];
    memset(property,0,sizeof(property));
    if(bFrontCam == true) {
        params->set(CameraProperties::FACING_INDEX, ExCameraParameters::FACING_FRONT);
        if(getCameraOrientation(bFrontCam,property)>=0){
            params->set(CameraProperties::ORIENTATION_INDEX,property);
        }else{
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
            params->set(CameraProperties::ORIENTATION_INDEX,"0");
#else
            params->set(CameraProperties::ORIENTATION_INDEX,"270");
#endif
        }
    } else {
        params->set(CameraProperties::FACING_INDEX, ExCameraParameters::FACING_BACK);
        if(getCameraOrientation(bFrontCam,property)>=0){
            params->set(CameraProperties::ORIENTATION_INDEX,property);
        }else{
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
            params->set(CameraProperties::ORIENTATION_INDEX,"180");
#else
            params->set(CameraProperties::ORIENTATION_INDEX,"90");
#endif
        }
    }

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    params->set(CameraProperties::RELOAD_WHEN_OPEN, "1");
#else
    params->set(CameraProperties::RELOAD_WHEN_OPEN, "0");
#endif
    params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,"yuv420sp,yuv420p"); //yuv420p for cts
    if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ // 422I
        //params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,PREVIEW_FORMAT_422I);
        params->set(CameraProperties::PREVIEW_FORMAT,PREVIEW_FORMAT_422I);
    }else if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){ //420sp
        //params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,PREVIEW_FORMAT_420SP);
        params->set(CameraProperties::PREVIEW_FORMAT,PREVIEW_FORMAT_420SP);
    }else{ //default case
        //params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,PREVIEW_FORMAT_420SP);
        params->set(CameraProperties::PREVIEW_FORMAT,PREVIEW_FORMAT_420P);
    }

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
#ifdef AMLOGIC_TWO_CH_UVC
    int main_id = -1;
    if(NO_ERROR == getVideodevId( camera_id,main_id )){
        if ((camera_fd = open(DEVICE_PATH(camera_id), O_RDWR)) != -1){
            CAMHAL_LOGDB("open %s success to loadCaps\n", DEVICE_PATH(camera_id));
        }
    }
#else
    while( camera_id < (int)ARRAY_SIZE(SENSOR_PATH)){
        if ((camera_fd = open(DEVICE_PATH(camera_id), O_RDWR)) != -1){
            CAMHAL_LOGDB("open %s success when loadCaps!\n", DEVICE_PATH(camera_id));
            break;
        }
        camera_id++;
    }
    if(camera_id >= (int)ARRAY_SIZE(SENSOR_PATH)){
        CAMHAL_LOGDB("failed to opening Camera when loadCaps: %s", strerror(errno));
    }
#endif
#else
    camera_fd = open(DEVICE_PATH(camera_id), O_RDWR);
#endif
    if(camera_fd<0){
        CAMHAL_LOGDB("open camera %d error when loadcaps",camera_id);
    }
    
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
    int fps=0, fps_num=0;
    int ret;
    char fpsrange[64];
    memset(fpsrange,0,sizeof(fpsrange));
	
    ret = enumFramerate(camera_fd, &fps, &fps_num);
    if((NO_ERROR == ret) && ( 0 !=fps )){
        CAMHAL_LOGDA("O_NONBLOCK operation to do previewThread\n");
        int tmp_fps = fps/fps_num/5;
        int iter = 0;
        int shift = 0;
        for(iter = 0;iter < tmp_fps;){
            iter++;
            if(iter == tmp_fps)
                sprintf(fpsrange+shift,"%d",iter*5);
            else
                sprintf(fpsrange+shift,"%d,",iter*5);
            if(iter == 1)
                shift += 2;
            else
                shift += 3;
        }
        if((fps/fps_num)%5 != 0)
            sprintf(fpsrange+shift-1,",%d",fps/fps_num);
        params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, fpsrange);
        params->set(CameraProperties::PREVIEW_FRAME_RATE, fps/fps_num);

        memset(fpsrange, 0, sizeof(fpsrange));
        sprintf(fpsrange,"%s%d","5000,",fps*1000/fps_num);
        params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, fpsrange);
        params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, fpsrange);

        memset(fpsrange, 0, sizeof(fpsrange));;
        sprintf(fpsrange,"(%s%d)","5000,15000),(5000,",fps*1000/fps_num);
        params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, fpsrange);
        memset(fpsrange, 0, sizeof(fpsrange));
        sprintf(fpsrange,"%s%d","5000,",fps*1000/fps_num);
        params->set(CameraProperties::FRAMERATE_RANGE, fpsrange);
    }else{
        if(NO_ERROR != ret){
            CAMHAL_LOGDA("sensor driver need to implement enum framerate func!!!\n");
        }
        params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, "5,15");
        params->set(CameraProperties::PREVIEW_FRAME_RATE, "15");

        params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "(5000,15000),(5000,30000)");
        params->set(CameraProperties::FRAMERATE_RANGE, "5000,30000");
        params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, "5000,15000");
        params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, "5000,15000");
    }
#else
    params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, "5,15");
    params->set(CameraProperties::PREVIEW_FRAME_RATE, "15");

    params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "(5000,15000),(5000,30000)");
    params->set(CameraProperties::FRAMERATE_RANGE, "5000,30000");
    params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, "5000,15000");
    params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, "5000,15000");
#endif
    //get preview size & set
    char *sizes = (char *) calloc (1, 1024);
    char *video_sizes = (char *) calloc (1, 1024);
    char vsize_tmp[15];
    if(!sizes || !video_sizes){
        if(sizes){
            free(sizes);
            sizes = NULL;
        }
        if(video_sizes){
            free(video_sizes);
            video_sizes = NULL;
        }
        CAMHAL_LOGDA("Alloc string buff error!");
        return;
    }

    memset(sizes,0,1024);
    uint32_t preview_format = DEFAULT_PREVIEW_PIXEL_FORMAT;
    int useMJPEG = 0;
    preview_format = enumFrameFormats(camera_fd, CAM_PREVIEW);
    memset(property,0,sizeof(property));
    if (property_get("ro.media.camera_preview.usemjpeg", property, NULL) > 0) {
            useMJPEG = atoi(property);
    }
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    if (0 == useMJPEG) {
            preview_format = V4L2_PIX_FMT_YUYV;
    }
#endif

    if (!getValidFrameSize(camera_fd, preview_format, sizes,true)) {
        int len = strlen(sizes);
        unsigned int supported_w = 0,  supported_h = 0,w = 0,h = 0;
        if(len>1){
            if(sizes[len-1] == ',')
                sizes[len-1] = '\0';
        }
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
        char small_size[8] = "176x144"; //for cts
        if(strstr(sizes,small_size)==NULL){
            if((len+sizeof(small_size))<(1024-1)){
                strcat(sizes,",");
                strcat(sizes,small_size);
            }
        }
#endif
        params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, sizes);

        char * b = (char *)sizes;
        int index = 0;

        while(b != NULL){
            if (sscanf(b, "%dx%d", &supported_w, &supported_h) != 2){
                break;
            }
            for(index =0; index< (int)ARRAY_SIZE(VIDEO_PREFER_SIZES);index++){
                if((VIDEO_PREFER_SIZES[index].width == supported_w) && (VIDEO_PREFER_SIZES[index].height == supported_h)){
                    sprintf(vsize_tmp,"%dx%d,", supported_w, supported_h);
                    strncat(video_sizes, vsize_tmp, sizeof(vsize_tmp));
                    break;
                }
            }
            if((supported_w*supported_h)>(w*h)){
                w = supported_w;
                h = supported_h;
            }
            b = strchr(b, ',');
            if(b)
                b++;
        }
        b = strrchr(video_sizes, ',');
        if(NULL != b){
            b[0] = '\0';
        }
        if((w>0)&&(h>0)){
            memset(sizes, 0, 1024);
            sprintf(sizes,"%dx%d",w,h);
        }
        params->set(CameraProperties::PREVIEW_SIZE, sizes);
        params->set(CameraProperties::SUPPORTED_VIDEO_SIZES, video_sizes);
    }else {
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, "320x240,176x144,160x120");
        params->set(CameraProperties::SUPPORTED_VIDEO_SIZES, "320x240,176x144,160x120");
        params->set(CameraProperties::PREVIEW_SIZE,"320x240");
#else
        params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, "640x480,352x288,176x144");
        params->set(CameraProperties::SUPPORTED_VIDEO_SIZES, "640x480,352x288,176x144");
        params->set(CameraProperties::PREVIEW_SIZE,"640x480");
#endif
    }

    params->set(CameraProperties::SUPPORTED_PICTURE_FORMATS, DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::PICTURE_FORMAT,DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::JPEG_QUALITY, 90);

    //must have >2 sizes and contain "0x0"
    params->set(CameraProperties::SUPPORTED_THUMBNAIL_SIZES, "160x120,0x0");
    params->set(CameraProperties::JPEG_THUMBNAIL_SIZE, "160x120");
    params->set(CameraProperties::JPEG_THUMBNAIL_QUALITY, 90);

    //get & set picture size
    memset(sizes,0,1024);
    uint32_t picture_format = DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT;
    picture_format = enumFrameFormats(camera_fd, CAM_CAPTURE);
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    if (0 == useMJPEG) {
            preview_format = V4L2_PIX_FMT_YUYV;
    }
#endif
    if (!getValidFrameSize(camera_fd, picture_format, sizes,false)) {
        int len = strlen(sizes);
        unsigned int supported_w = 0,  supported_h = 0,w = 0,h = 0;
        if(len>1){
            if(sizes[len-1] == ',')
                sizes[len-1] = '\0';
        }

        params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, sizes);

        char * b = (char *)sizes;
        while(b != NULL){
            if (sscanf(b, "%dx%d", &supported_w, &supported_h) != 2){
                break;
            }
            if((supported_w*supported_h)>(w*h)){
                w = supported_w;
                h = supported_h;
            }
            b = strchr(b, ',');
            if(b)
                b++;
        }
        if((w>0)&&(h>0)){
            memset(sizes, 0, 1024);
            sprintf(sizes,"%dx%d",w,h);
        }
        params->set(CameraProperties::PICTURE_SIZE, sizes);
    }else{
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, "320x240");
        params->set(CameraProperties::PICTURE_SIZE,"320x240");
#else
        params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, "640x480");
        params->set(CameraProperties::PICTURE_SIZE,"640x480");
#endif
    }
    if(sizes){
        free(sizes);
        sizes = NULL;
    }
    if(video_sizes){
        free(video_sizes);
        video_sizes = NULL;
    }

    char *focus_mode = (char *) calloc (1, 256);
    char * def_focus_mode = (char *) calloc (1, 64);
    if((focus_mode)&&(def_focus_mode)){
        memset(focus_mode,0,256);
        memset(def_focus_mode,0,64);
        if(getCameraAutoFocus(camera_fd, focus_mode,def_focus_mode)) {
            params->set(CameraProperties::SUPPORTED_FOCUS_MODES, focus_mode);
            params->set(CameraProperties::FOCUS_MODE, def_focus_mode);
            memset(focus_mode,0,256);
            memset(def_focus_mode,0,64);
            if (getCameraFocusArea( camera_fd, def_focus_mode, focus_mode)){
                params->set(CameraProperties::MAX_FOCUS_AREAS, def_focus_mode);
                CAMHAL_LOGDB("focus_area=%s, max_num_focus_area=%s\n", focus_mode, def_focus_mode);
            }
        }else {
            params->set(CameraProperties::SUPPORTED_FOCUS_MODES, "fixed");
            params->set(CameraProperties::FOCUS_MODE, "fixed");
        }
    }else{
        params->set(CameraProperties::SUPPORTED_FOCUS_MODES, "fixed");
        params->set(CameraProperties::FOCUS_MODE, "fixed");
    }
    if(focus_mode){
        free(focus_mode);
        focus_mode = NULL;
    }
    if(def_focus_mode){
        free(def_focus_mode);  
        def_focus_mode = NULL;
    }

    char *banding_mode = (char *) calloc (1, 256);
    char *def_banding_mode = (char *) calloc (1, 64);
    if((banding_mode)&&(def_banding_mode)){
        memset(banding_mode,0,256);
        memset(def_banding_mode,0,64);
        getCameraBanding(camera_fd, banding_mode, def_banding_mode);
        params->set(CameraProperties::SUPPORTED_ANTIBANDING, banding_mode);
        params->set(CameraProperties::ANTIBANDING, def_banding_mode);
    }else{
        params->set(CameraProperties::SUPPORTED_ANTIBANDING, "50hz,60hz,auto");
        params->set(CameraProperties::ANTIBANDING, "50hz");
    }
    if(banding_mode){
        free(banding_mode);
        banding_mode = NULL;
    }
    if(def_banding_mode){
        free(def_banding_mode);
        def_banding_mode = NULL;
    }

    params->set(CameraProperties::FOCAL_LENGTH, "4.31");

    params->set(CameraProperties::HOR_ANGLE,"54.8");
    params->set(CameraProperties::VER_ANGLE,"42.5");

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    params->set(CameraProperties::SUPPORTED_WHITE_BALANCE, "auto");
    params->set(CameraProperties::WHITEBALANCE, "auto");
#else
    char *wb_mode = (char *) calloc (1, 256);
    char *def_wb_mode = (char *) calloc (1, 64);

    if( wb_mode && def_wb_mode){
        memset(wb_mode, 0, 256);
        memset(def_wb_mode, 0, 64);
        getCameraWhiteBalance(camera_fd, wb_mode, def_wb_mode);
        params->set(CameraProperties::SUPPORTED_WHITE_BALANCE, wb_mode);
        params->set(CameraProperties::WHITEBALANCE, def_wb_mode);
    }else{
        params->set(CameraProperties::SUPPORTED_WHITE_BALANCE, "auto,daylight,incandescent,fluorescent");
        params->set(CameraProperties::WHITEBALANCE, "auto");
    }

    if(wb_mode){
        free(wb_mode);
        wb_mode = NULL;
    }
    if(def_wb_mode){
        free(def_wb_mode);
        def_wb_mode = NULL;
    }
#endif

    params->set(CameraProperties::AUTO_WHITEBALANCE_LOCK, DEFAULT_AWB_LOCK);

    params->set(CameraProperties::SUPPORTED_EFFECTS, "none,negative,sepia");
    params->set(CameraProperties::EFFECT, "none");

    char *flash_mode = (char *) calloc (1, 256);
    char *def_flash_mode = (char *) calloc (1, 64);
    if((flash_mode)&&(def_flash_mode)){
        memset(flash_mode,0,256);
        memset(def_flash_mode,0,64);
        if (get_flash_mode(camera_fd, flash_mode,def_flash_mode)) {
            params->set(CameraProperties::SUPPORTED_FLASH_MODES, flash_mode);
            params->set(CameraProperties::FLASH_MODE, def_flash_mode);
            CAMHAL_LOGDB("def_flash_mode=%s, flash_mode=%s\n", def_flash_mode, flash_mode);
        }
    }
    if (flash_mode) {
        free(flash_mode);
        flash_mode = NULL;
    }
    if (def_flash_mode) {
        free(def_flash_mode);
        def_flash_mode = NULL;
    }

    //params->set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,"auto,night,snow");
    //params->set(CameraParameters::KEY_SCENE_MODE,"auto");

    params->set(CameraProperties::EXPOSURE_MODE, "auto");
    params->set(CameraProperties::SUPPORTED_EXPOSURE_MODES, "auto");
    params->set(CameraProperties::AUTO_EXPOSURE_LOCK, DEFAULT_AE_LOCK);

    int min=0, max =0, def=0, step =0;
    getCameraExposureValue( camera_fd, min, max, step, def);
    params->set(CameraProperties::SUPPORTED_EV_MAX, max > 3 ? 3 : max);
    params->set(CameraProperties::SUPPORTED_EV_MIN, min < -3 ? -3 : min);
    params->set(CameraProperties::EV_COMPENSATION, def);
    params->set(CameraProperties::SUPPORTED_EV_STEP, step);

    //don't support digital zoom now
#ifndef AMLOGIC_USB_CAMERA_SUPPORT
    int zoom_level = -1;
    char *zoom_str = (char *) calloc (1, 256);
    if(zoom_str)
        memset(zoom_str,0,256);
    zoom_level = get_supported_zoom(camera_fd,zoom_str);
    if(zoom_level>0){ //new interface by v4l ioctl
        params->set(CameraProperties::ZOOM_SUPPORTED,"true");
        params->set(CameraProperties::SMOOTH_ZOOM_SUPPORTED,"false");
        params->set(CameraProperties::SUPPORTED_ZOOM_RATIOS,zoom_str);
        params->set(CameraProperties::SUPPORTED_ZOOM_STAGES,zoom_level);	//think the zoom ratios as a array, the max zoom is the max index
        params->set(CameraProperties::ZOOM, 0);//default should be 0
     }else{  // by set video layer zoom sys
        params->set(CameraProperties::ZOOM_SUPPORTED,"true");
        params->set(CameraProperties::SMOOTH_ZOOM_SUPPORTED,"false");
        params->set(CameraProperties::SUPPORTED_ZOOM_RATIOS,"100,120,140,160,180,200,220,280,300");
        params->set(CameraProperties::SUPPORTED_ZOOM_STAGES,8);	//think the zoom ratios as a array, the max zoom is the max index
        params->set(CameraProperties::ZOOM, 0);//default should be 0
    }
    if(zoom_str)
        free(zoom_str);
#else
    params->set(CameraProperties::ZOOM_SUPPORTED,"false");
    params->set(CameraProperties::SMOOTH_ZOOM_SUPPORTED,"false");
    params->set(CameraProperties::SUPPORTED_ZOOM_RATIOS,"100");
    params->set(CameraProperties::SUPPORTED_ZOOM_STAGES,0);	//think the zoom ratios as a array, the max zoom is the max index
    params->set(CameraProperties::ZOOM, 0);//default should be 0
#endif

    params->set(CameraProperties::SUPPORTED_ISO_VALUES, "auto");
    params->set(CameraProperties::ISO_MODE, DEFAULT_ISO_MODE);

    params->set(CameraProperties::SUPPORTED_IPP_MODES, DEFAULT_IPP);
    params->set(CameraProperties::IPP, DEFAULT_IPP);

    params->set(CameraProperties::SUPPORTED_SCENE_MODES, "auto");
    params->set(CameraProperties::SCENE_MODE, DEFAULT_SCENE_MODE);

    params->set(CameraProperties::BRIGHTNESS, DEFAULT_BRIGHTNESS);
    params->set(CameraProperties::CONTRAST, DEFAULT_CONTRAST);
    params->set(CameraProperties::GBCE, DEFAULT_GBCE);
    params->set(CameraProperties::SATURATION, DEFAULT_SATURATION);
    params->set(CameraProperties::SHARPNESS, DEFAULT_SHARPNESS);
    params->set(CameraProperties::VSTAB, DEFAULT_VSTAB);
    params->set(CameraProperties::VSTAB_SUPPORTED, DEFAULT_VSTAB_SUPPORTED);
    params->set(CameraProperties::MAX_FD_HW_FACES, DEFAULT_MAX_FD_HW_FACES);
    params->set(CameraProperties::MAX_FD_SW_FACES, DEFAULT_MAX_FD_SW_FACES);
    params->set(CameraProperties::REQUIRED_PREVIEW_BUFS, DEFAULT_NUM_PREV_BUFS);
    params->set(CameraProperties::REQUIRED_IMAGE_BUFS, DEFAULT_NUM_PIC_BUFS);
#ifdef AMLOGIC_ENABLE_VIDEO_SNAPSHOT
    params->set(CameraProperties::VIDEO_SNAPSHOT_SUPPORTED, "true");
#else
    params->set(CameraProperties::VIDEO_SNAPSHOT_SUPPORTED, "false");
#endif
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    params->set(CameraProperties::VIDEO_SIZE,params->get(CameraProperties::PREVIEW_SIZE));
    params->set(CameraProperties::PREFERRED_PREVIEW_SIZE_FOR_VIDEO,params->get(CameraProperties::PREVIEW_SIZE));
#else
    params->set(CameraProperties::VIDEO_SIZE, DEFAULT_VIDEO_SIZE);
    params->set(CameraProperties::PREFERRED_PREVIEW_SIZE_FOR_VIDEO, DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO);
#endif

    if(camera_fd>=0)
        close(camera_fd);
}

#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
/* gets video device defined frame rate (not real - consider it a maximum value)
 * args:
 *
 * returns: VIDIOC_G_PARM ioctl result value
*/
extern "C" int get_framerate ( int camera_fd, int *fps, int *fps_num)
{
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    *fps = 15;
    *fps_num = 1;
    return 0;
#else
    int ret=0;

    struct v4l2_streamparm streamparm;

    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl( camera_fd,VIDIOC_G_PARM,&streamparm);
    if (ret < 0){
        CAMHAL_LOGDA("VIDIOC_G_PARM - Unable to get timeperframe");
    }else{
        if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            // it seems numerator is allways 1 but we don't do assumptions here :-)
            *fps = streamparm.parm.capture.timeperframe.denominator;
            *fps_num = streamparm.parm.capture.timeperframe.numerator;
        }
    }
    return ret;
#endif
}

int enumFramerate (int camera_fd, int *fps, int *fps_num)
{
    int ret=0;
    int framerate=0;
    int temp_rate=0;
    struct v4l2_frmivalenum fival;
    int i,j;

    int pixelfmt_tbl[]={
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        V4L2_PIX_FMT_MJPEG,
        V4L2_PIX_FMT_YUYV,
#else
        V4L2_PIX_FMT_NV21,
#endif
        V4L2_PIX_FMT_YVU420,
    };
    struct v4l2_frmsize_discrete resolution_tbl[]={
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        {960, 720},
#endif
        {640, 480},
        {320, 240},
    };

    for( i = 0; i < (int) ARRAY_SIZE(pixelfmt_tbl); i++){
        for( j = 0; j < (int) ARRAY_SIZE(resolution_tbl); j++){
            memset(&fival, 0, sizeof(fival));
            fival.index = 0;
            fival.pixel_format = pixelfmt_tbl[i];
            fival.width = resolution_tbl[j].width;
            fival.height = resolution_tbl[j].height;

            while ((ret = ioctl(camera_fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0){
                if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE){
                    temp_rate = fival.discrete.denominator/fival.discrete.numerator;
                    if(framerate < temp_rate)
                        framerate = temp_rate;
                }else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS){
                    framerate = fival.stepwise.max.denominator/fival.stepwise.max.numerator;
                    CAMHAL_LOGDB("pixelfmt=%d,resolution:%dx%d,"
                        "FRAME TYPE is continuous,step=%d/%d s\n",
                        pixelfmt_tbl[i],
                        resolution_tbl[j].width,
                        resolution_tbl[j].height,
                        fival.stepwise.max.numerator,
                        fival.stepwise.max.denominator);
                    break;
                }else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE)	{
                    CAMHAL_LOGDB("pixelfmt=%d,resolution:%dx%d,"
                        "FRAME TYPE is step wise,step=%d/%d s\n",
                        pixelfmt_tbl[i],
                        resolution_tbl[j].width,
                        resolution_tbl[j].height,
                        fival.stepwise.step.numerator,
                        fival.stepwise.step.denominator);
                        framerate = fival.stepwise.max.denominator/fival.stepwise.max.numerator;
                    break;
                }
                fival.index++;
            }
        }
    }

    *fps = framerate;
    *fps_num = 1;
    CAMHAL_LOGDB("enum framerate=%d\n", framerate);
    return 0;
}
#endif

extern "C" int V4LCameraAdapter::set_white_balance(int camera_fd,const char *swb)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    memset(&ctl, 0, sizeof(ctl));
    ctl.id = V4L2_CID_AUTO_WHITE_BALANCE;
    ctl.value= true;
#else
    ctl.id = V4L2_CID_DO_WHITE_BALANCE;

    if(strcasecmp(swb,"auto")==0)
        ctl.value=CAM_WB_AUTO;
    else if(strcasecmp(swb,"daylight")==0)
        ctl.value=CAM_WB_DAYLIGHT;
    else if(strcasecmp(swb,"incandescent")==0)
        ctl.value=CAM_WB_INCANDESCENCE;
    else if(strcasecmp(swb,"fluorescent")==0)
        ctl.value=CAM_WB_FLUORESCENT;
    else if(strcasecmp(swb,"cloudy-daylight")==0)
        ctl.value=CAM_WB_CLOUD;
    else if(strcasecmp(swb,"shade")==0)
        ctl.value=CAM_WB_SHADE;
    else if(strcasecmp(swb,"twilight")==0)
        ctl.value=CAM_WB_TWILIGHT;
    else if(strcasecmp(swb,"warm-fluorescent")==0)
        ctl.value=CAM_WB_WARM_FLUORESCENT;
#endif

    if(mWhiteBalance == ctl.value){
        return 0;
    }else{
        mWhiteBalance = ctl.value;
    }
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("AMLOGIC CAMERA Set white balance fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

status_t V4LCameraAdapter::getFocusMoveStatus()
{
    struct v4l2_control ctl;
    int ret;
    if( (cur_focus_mode != CAM_FOCUS_MODE_CONTI_VID) &&
      (cur_focus_mode != CAM_FOCUS_MODE_CONTI_PIC) &&
      (cur_focus_mode != CAM_FOCUS_MODE_AUTO)){
        mFocusMoveEnabled = false;
        return 0;
    }

    mFocusWaitCount --;
    if(mFocusWaitCount >= 0){
        return 0;
    }
    mFocusWaitCount = 0;

    memset( &ctl, 0, sizeof(ctl));
    ctl.id =V4L2_CID_AUTO_FOCUS_STATUS;
    ret = ioctl(mCameraHandle, VIDIOC_G_CTRL, &ctl);
    if (0 > ret ){
        CAMHAL_LOGDA("V4L2_CID_AUTO_FOCUS_STATUS failed\n");
        return -EINVAL;
    }

    if( ctl.value == V4L2_AUTO_FOCUS_STATUS_BUSY ){
        if(!bFocusMoveState){
            bFocusMoveState = true;
            notifyFocusMoveSubscribers(FOCUS_MOVE_START);
        }
    }else {
        mFocusWaitCount = FOCUS_PROCESS_FRAMES;
        if(bFocusMoveState){
            bFocusMoveState = false;
            notifyFocusMoveSubscribers(FOCUS_MOVE_STOP);
        }
    }
    return ctl.value;
}

extern "C" int V4LCameraAdapter::set_focus_area( int camera_fd, const char *focusarea)
{
    struct v4l2_control ctl;
    int ret;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    int weight = 0;
    int tempvalue = 0;
 
    sscanf(focusarea,"(%d,%d,%d,%d,%d)",&x0,&y0,&x1,&y1,&weight);
    if( (x0==x1)&&(y0==y1) ){
        CAMHAL_LOGDA("Invalid position for tap focus!\n");
        return 0;
    }
    memset( &ctl, 0, sizeof(ctl));
    ctl.id = V4L2_CID_FOCUS_ABSOLUTE;
    tempvalue = ((x0+x1)/2 + 1000);
    tempvalue <<= 16;
    ctl.value = tempvalue;
    tempvalue = ((y0+y1)/2 + 1000) & 0xffff;
    ctl.value |= tempvalue;
    ret = ioctl(mCameraHandle, VIDIOC_S_CTRL, &ctl);
    if ( 0 > ret ){
        CAMHAL_LOGDA("focus tap failed\n");
        return -EINVAL;
    }
    return 0;
}

/*
 * use id V4L2_CID_EXPOSURE_AUTO to set exposure mode
 * 0: Auto Mode, commit failure @20120504
 * 1: Manual Mode
 * 2: Shutter Priority Mode, commit failure @20120504
 * 3: Aperture Priority Mode
 */
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
extern "C" int V4LCameraAdapter::SetExposureMode(int camera_fd, unsigned int mode)
{
    int ret = 0;
    struct v4l2_control ctl;

    memset(&ctl, 0, sizeof(ctl));

    ctl.id = V4L2_CID_EXPOSURE_AUTO;
    ctl.value = mode;
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("fail: %s. ret=%d", strerror(errno),ret);
        return ret;
    }
    if( (V4L2_EXPOSURE_APERTURE_PRIORITY ==ctl.value)||(V4L2_EXPOSURE_AUTO ==ctl.value)){
        memset( &ctl, 0, sizeof(ctl));
        ctl.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
        ctl.value = false;
        ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
        if(ret<0){
            CAMHAL_LOGDB("Exposure auto priority Set manual fail: %s. ret=%d", strerror(errno),ret);
            return ret;
        }
    }
    return 0;
}
#endif

extern "C" int V4LCameraAdapter::SetExposure(int camera_fd,const char *sbn)
{
    int ret = 0;
    struct v4l2_control ctl;
    int level;

    if(camera_fd<0)
        return -1;
    level = atoi(sbn);
    if(mEV == level){
        return 0;
    }else{
        mEV = level;
    }
    memset(&ctl, 0, sizeof(ctl));

#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    level ++;
    if(level !=1){
        ret = SetExposureMode( camera_fd, V4L2_EXPOSURE_MANUAL);
        if(ret<0){
            CAMHAL_LOGDA("Exposure Mode change to manual mode failure\n");
            return ret;
        }
    }else{
        ret = SetExposureMode( camera_fd, V4L2_EXPOSURE_APERTURE_PRIORITY);// 3);
        if(ret<0){
            CAMHAL_LOGDA("Exposure Mode change to Aperture mode failure\n");
        }
        return ret;//APERTURE mode cann't set followed control
    }
    ctl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    if(level>=0){
        ctl.value= mEVdef << level;
    }else{
        ctl.value= mEVdef >> (-level);
    }
    ctl.value= ctl.value>mEVmax? mEVmax:ctl.value;
    ctl.value= ctl.value<mEVmin? mEVmin:ctl.value;

    level --;
#else
    ctl.id = V4L2_CID_EXPOSURE;
    ctl.value = level + (mEVmax - mEVmin)/2;
#endif

    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("AMLOGIC CAMERA Set Exposure fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

extern "C" int set_effect(int camera_fd,const char *sef)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

    memset(&ctl, 0, sizeof(ctl));
    ctl.id = V4L2_CID_COLORFX;
    if(strcasecmp(sef,"none")==0)
        ctl.value=CAM_EFFECT_ENC_NORMAL;
    else if(strcasecmp(sef,"negative")==0)
        ctl.value=CAM_EFFECT_ENC_COLORINV;
    else if(strcasecmp(sef,"sepia")==0)
        ctl.value=CAM_EFFECT_ENC_SEPIA;
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set effect fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

extern "C" int set_night_mode(int camera_fd,const char *snm)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

    memset( &ctl, 0, sizeof(ctl));
    if(strcasecmp(snm,"auto")==0)
        ctl.value=CAM_NM_AUTO;
    else if(strcasecmp(snm,"night")==0)
        ctl.value=CAM_NM_ENABLE;
    ctl.id = V4L2_CID_DO_WHITE_BALANCE;
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set night mode fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

extern "C" int V4LCameraAdapter::set_banding(int camera_fd,const char *snm)
{
    int ret = 0;
    struct v4l2_control ctl;

    if(camera_fd<0)
        return -1;

    memset( &ctl, 0, sizeof(ctl));
    if(strcasecmp(snm,"50hz")==0)
        ctl.value= CAM_ANTIBANDING_50HZ;
    else if(strcasecmp(snm,"60hz")==0)
        ctl.value= CAM_ANTIBANDING_60HZ;
    else if(strcasecmp(snm,"auto")==0)
        ctl.value= CAM_ANTIBANDING_AUTO;
    else if(strcasecmp(snm,"off")==0)
        ctl.value= CAM_ANTIBANDING_OFF;

    ctl.id = V4L2_CID_POWER_LINE_FREQUENCY;
    if(mAntiBanding == ctl.value){
        return 0;
    }else{
        mAntiBanding = ctl.value;
    }
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set banding fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

static bool get_flash_mode(int camera_fd, char *flash_status,
					char *def_flash_status)
{
    struct v4l2_queryctrl qc;    
    struct v4l2_querymenu qm;
    bool flash_enable = false;
    int ret = NO_ERROR;
    int status_count = 0;

    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_BACKLIGHT_COMPENSATION;
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) || (qc.type != V4L2_CTRL_TYPE_MENU)){
        flash_enable = false;
        CAMHAL_LOGDB("can't support flash, %sret=%d%s\n",
                (qc.flags == V4L2_CTRL_FLAG_DISABLED)?"disable,":"",
                ret,  (qc.type != V4L2_CTRL_TYPE_MENU)?"":",type not right");
    }else {
        memset(&qm, 0, sizeof(qm));
        qm.id = V4L2_CID_BACKLIGHT_COMPENSATION;
        qm.index = qc.default_value;
        if(ioctl (camera_fd, VIDIOC_QUERYMENU, &qm) < 0){
            strcpy(def_flash_status, "off");
        } else {
            strcpy(def_flash_status, (char*)qm.name);
        }
        int index = 0;
        for (index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_BACKLIGHT_COMPENSATION;
            qm.index = index;
            if(ioctl (camera_fd, VIDIOC_QUERYMENU, &qm) < 0){
                continue;
            } else {
                if(status_count>0)
                    strcat(flash_status, ",");
                strcat(flash_status, (char*)qm.name);
                status_count++;
            }
        }
        if(status_count>0)
            flash_enable = true;
    }
    return flash_enable;
}

extern "C" int set_flash_mode(int camera_fd, const char *sfm)
{
    int ret = NO_ERROR;
    struct v4l2_control ctl;

    memset(&ctl, 0, sizeof(ctl));
    if(strcasecmp(sfm,"auto")==0)
        ctl.value=FLASHLIGHT_AUTO;
    else if(strcasecmp(sfm,"on")==0)
        ctl.value=FLASHLIGHT_ON;
    else if(strcasecmp(sfm,"off")==0)
        ctl.value=FLASHLIGHT_OFF;
    else if(strcasecmp(sfm,"torch")==0)
        ctl.value=FLASHLIGHT_TORCH;
    else if(strcasecmp(sfm,"red-eye")==0)
        ctl.value=FLASHLIGHT_RED_EYE;

    ctl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
    ret = ioctl( camera_fd, VIDIOC_S_CTRL, &ctl);
    if( ret < 0 ){
        CAMHAL_LOGDB("BACKLIGHT_COMPENSATION failed, errno=%d\n", errno);
    }
    return ret;
}

static int get_hflip_mode(int camera_fd)
{
    struct v4l2_queryctrl qc;    
    int ret = 0;

    if(camera_fd<0){
        CAMHAL_LOGDA("Get_hflip_mode --camera handle is invalid\n");
        return -1;
    }

    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_HFLIP;
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) || (qc.type != V4L2_CTRL_TYPE_INTEGER)){
        ret = -1;
        CAMHAL_LOGDB("can't support HFlip! %s ret=%d %s\n",
            (qc.flags == V4L2_CTRL_FLAG_DISABLED)?"disable,":"",
            ret, (qc.type != V4L2_CTRL_TYPE_INTEGER)?"":",type not right");
    }else{
        CAMHAL_LOGDB("camera handle %d supports HFlip!\n",camera_fd);
    }
    return ret;
}

static int set_hflip_mode(int camera_fd, bool mode)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

    memset(&ctl, 0,sizeof(ctl));
    ctl.value=mode?1:0;
    ctl.id = V4L2_CID_HFLIP;
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set hflip mode fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

static int get_supported_zoom(int camera_fd, char * zoom_str)
{
    int ret = 0;
    struct v4l2_queryctrl qc;  
    char str_zoom_element[10];  
    if((camera_fd<0)||(!zoom_str))
        return -1;

    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_ZOOM_ABSOLUTE;  
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) || (qc.type != V4L2_CTRL_TYPE_INTEGER)){
        ret = -1;
        CAMHAL_LOGDB("camera handle %d can't get zoom level!\n",camera_fd);
    }else{
        int i = 0;
        ret = (qc.maximum - qc.minimum)/qc.step;  
        for (i=qc.minimum; i<=qc.maximum; i+=qc.step) {  
            memset(str_zoom_element,0,sizeof(str_zoom_element));
            sprintf(str_zoom_element,"%d,", i);  
            strcat(zoom_str,str_zoom_element);  
        }  
    }  
    return ret ;
}

static int set_zoom_level(int camera_fd, int zoom)
{
    int ret = 0;
    struct v4l2_control ctl;
    if((camera_fd<0)||(zoom<0))
        return -1;

    memset( &ctl, 0, sizeof(ctl));
    ctl.value=zoom;
    ctl.id = V4L2_CID_ZOOM_ABSOLUTE;
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set zoom level fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

#ifndef AMLOGIC_USB_CAMERA_SUPPORT
static int set_rotate_value(int camera_fd, int value)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

    if((value!=0)&&(value!=90)&&(value!=180)&&(value!=270)){
        CAMHAL_LOGDB("Set rotate value invalid: %d.", value);
        return -1;
    }
    memset( &ctl, 0, sizeof(ctl));
    ctl.value=value;
    ctl.id = V4L2_ROTATE_ID;
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set rotate value fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}
#endif

status_t V4LCameraAdapter::force_reset_sensor(){
	CAMHAL_LOGIA("Restart Preview");
	status_t ret = NO_ERROR;
	int frame_count = 0;
	int ret_c = 0;
    void *frame_buf = NULL;
    	
    Mutex::Autolock lock(mPreviewBufsLock);
    enum v4l2_buf_type bufType;
    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
    if (ret < 0) {
        CAMHAL_LOGEB("Stop preview: Unable to stop Preview: %s", strerror(errno));
        return ret;
    }
        /* Unmap buffers */
    for (int i = 0; i < mPreviewBufferCount; i++){
        if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0){
            CAMHAL_LOGEA("Unmap failed");
        }
        mVideoInfo->canvas[i] = 0;        
    }

    if ((DEV_USB == m_eDeviceType) ||
        (DEV_ION == m_eDeviceType) ||
        (DEV_ION_MPLANE == m_eDeviceType))
    {
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;
        mVideoInfo->rb.count = 0;

        ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
        if (ret < 0) {
            CAMHAL_LOGDB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        }else{
            CAMHAL_LOGVA("VIDIOC_REQBUFS delete buffer success\n");
        }
    }
    mPreviewBufs.clear();
	mPreviewIdxs.clear();
	
	CAMHAL_LOGDA("clera preview buffer");
	ret = setBuffersFormat(mPreviewWidth, mPreviewHeight, mSensorFormat);
    if( 0 > ret ){
        CAMHAL_LOGEB("VIDIOC_S_FMT failed: %s", strerror(errno));
        return ret;
    }
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = m_eV4l2Memory;
    mVideoInfo->rb.count = mPreviewBufferCount;

    ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }
    usleep(10000);
    uint32_t *ptr = (uint32_t*) mPreviewCache;

    for (int i = 0; i < mPreviewBufferCount; i++) {
        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));
        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;
        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }
		private_handle_t* gralloc_hnd;
        if (V4L2_MEMORY_DMABUF == m_eV4l2Memory)
        {
            gralloc_hnd = (private_handle_t*)ptr[i];
            mVideoInfo->mem[i] = mmap (0,
                            mVideoInfo->buf.length,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED,
                            gralloc_hnd->share_fd,
                            0);
        } else {
            mVideoInfo->mem[i] = mmap (0,
                                mVideoInfo->buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                mCameraHandle,
                                mVideoInfo->buf.m.offset);
        }

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
            return ret;
        }

        if(mVideoInfo->canvas_mode){
            mVideoInfo->canvas[i] = mVideoInfo->buf.reserved;
        }
        //Associate each Camera internal buffer with the one from Overlay
        CAMHAL_LOGDB("mPreviewBufs.add %#x, %d", ptr[i], i);
        mPreviewBufs.add((int)ptr[i], i);
    }

    for(int i = 0;i < mPreviewBufferCount; i++){
        mPreviewIdxs.add(mPreviewBufs.valueAt(i),i);
    }
	CAMHAL_LOGDA("reset sensor add preview buffer ok");
   	
    nQueued = 0;
    private_handle_t* gralloc_hnd;
    for (int i = 0; i < mPreviewBufferCount; i++){
        frame_count = -1;
        frame_buf = (void *)mPreviewBufs.keyAt(i);

        if((ret_c = getFrameRefCount(frame_buf,CameraFrame::PREVIEW_FRAME_SYNC))>=0)
            frame_count = ret_c;

        CAMHAL_LOGDB("startPreview--buffer address:0x%x, refcount:%d",(uint32_t)frame_buf,frame_count);
        if(frame_count>0)
            continue;         
        mVideoInfo->buf.index = mPreviewBufs.valueFor((uint32_t)frame_buf);
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = m_eV4l2Memory;
        if (V4L2_MEMORY_DMABUF == m_eV4l2Memory) {
            gralloc_hnd = (private_handle_t *)frame_buf;
            mVideoInfo->buf.m.fd = gralloc_hnd->share_fd;
        }

        ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEA("VIDIOC_QBUF Failed");
            return ret;
        }
        CAMHAL_LOGDB("startPreview --length=%d, index:%d", mVideoInfo->buf.length,mVideoInfo->buf.index);
        nQueued++;
    }
    ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
    if (ret < 0) {
        CAMHAL_LOGEB("Start preview: Unable to start Preview: %s", strerror(errno));
        return ret;
    }
    CAMHAL_LOGDA("reset sensor finish");
    return NO_ERROR;
}	

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

