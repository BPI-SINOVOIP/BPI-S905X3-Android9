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
* @file V4LCamAdpt.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/

#define LOG_TAG "V4LCamAdpt             "
//reinclude because of a bug with the log macros
#include <utils/Log.h>
#include "DebugUtils.h"

#include "V4LCamAdpt.h"
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
#include <linux/videodev.h>
#include <sys/time.h>

#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "CameraHal.h"


//for private_handle_t TODO move out of private header
#include <gralloc_priv.h>

static int mDebugFps = 0;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define VIRTUAL_DEVICE_PATH(_sensor_index) \
	(_sensor_index == (MAX_CAM_NUM_ADD_VCAM-1) ? "/dev/video11" : "/dev/video11")

namespace android {

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

/*--------------------junk STARTS here-----------------------------*/
#define SYSFILE_CAMERA_SET_PARA "/sys/class/vm/attr2"
#define SYSFILE_CAMERA_SET_MIRROR "/sys/class/vm/mirror"
static int writefile(char* path,char* content)
{
    FILE* fp = fopen(path, "w+");

    CAMHAL_LOGDB("Write file %s(%p) content %s", path, fp, content);

    if (fp) {
        while( ((*content) != '\0') ) {
            if (EOF == fputc(*content,fp)){
                CAMHAL_LOGDA("write char fail");
            }
            content++;
        }

        fclose(fp);
    }
    else
        CAMHAL_LOGDA("open file fail\n");
    return 1;
}
/*--------------------Camera Adapter Class STARTS here-----------------------------*/

status_t V4LCamAdpt::initialize(CameraProperties::Properties* caps)
{
    LOG_FUNCTION_NAME;

    char value[PROPERTY_VALUE_MAX];
    char const*filename = NULL;
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);

    int ret = NO_ERROR;

    // Allocate memory for video info structure
    mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
    if(!mVideoInfo)
    {
	    return NO_MEMORY;
    }


    filename = caps->get(CameraProperties::DEVICE_NAME);
    if(filename == NULL){
        CAMHAL_LOGEB("can't get index=%d's name ", mSensorIndex);
        return -EINVAL;
    }
    if ((mCameraHandle = open( filename, O_RDWR)) == -1)
    {
	    CAMHAL_LOGEB("Error while opening handle to V4L2 Camera: %s", strerror(errno));
	    return -EINVAL;
    }
  

    ret = ioctl (mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);
    if (ret < 0)
    {
	    CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
	    return -EINVAL;
    }

    if ((mVideoInfo->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
    {
	    CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
	    return -EINVAL;
    }

    if (!(mVideoInfo->cap.capabilities & V4L2_CAP_STREAMING))
    {
	    CAMHAL_LOGEA("Error while adapter initialization: Capture device does not support streaming i/o");
	    return -EINVAL;
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

    mPreviewWidth = 0 ;
    mPreviewHeight = 0;
    mCaptureWidth = 0;
    mCaptureHeight = 0;

    IoctlStateProbe();

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
    int fps=0, fps_num=0;
    char *fpsrange=(char *)calloc(32,sizeof(char));

    ret = get_framerate(mCameraHandle, &fps, &fps_num);
    if((fpsrange != NULL)&&(NO_ERROR == ret) && ( 0 !=fps_num )){
        mPreviewFrameRate = fps/fps_num;
        sprintf(fpsrange,"%s%d","10,",fps/fps_num);
        CAMHAL_LOGDB("supported preview rates is %s\n", fpsrange);

        mParams.set(CameraParameters::KEY_PREVIEW_FRAME_RATE,fps/fps_num);
        mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,fpsrange);

        mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,fpsrange);
        mParams.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,fpsrange);
    }else{
        mPreviewFrameRate = 15;
        sprintf(fpsrange,"%s%d","10,",mPreviewFrameRate);
        CAMHAL_LOGDB("default preview rates is %s\n", fpsrange);

        mParams.set(CameraParameters::KEY_PREVIEW_FRAME_RATE, mPreviewFrameRate);
        mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,fpsrange);

        mParams.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,fpsrange);
        mParams.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,fpsrange);
    }
#endif

    writefile((char*)SYSFILE_CAMERA_SET_PARA, (char*)"1");
    //mirror set at here will not work.
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCamAdpt::IoctlStateProbe(void)
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
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) 
	|| (qc.type != V4L2_CTRL_TYPE_INTEGER)){
	mIoctlSupport &= ~IOCTL_MASK_ZOOM;
    }else{
	mIoctlSupport |= IOCTL_MASK_ZOOM;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_ROTATE_ID;  
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)
	|| (qc.type != V4L2_CTRL_TYPE_INTEGER)){
	mIoctlSupport &= ~IOCTL_MASK_ROTATE;
    }else{
	mIoctlSupport |= IOCTL_MASK_ROTATE;
    }
    
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
	CAMHAL_LOGDB("camera %d support capture rotate",mSensorIndex);
    }
    mRotateValue = 0;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_EXPOSURE;

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
    qc.id = V4L2_CID_DO_WHITE_BALANCE;

    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) ){
        mIoctlSupport &= ~IOCTL_MASK_WB;
    }else{
        mIoctlSupport |= IOCTL_MASK_WB;
    }

    mWhiteBalance = qc.default_value;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_BACKLIGHT_COMPENSATION; 
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) 
	|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_FLASH;
    }else{
        mIoctlSupport |= IOCTL_MASK_FLASH;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_COLORFX; 
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) 
	|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_EFFECT;
    }else{
        mIoctlSupport |= IOCTL_MASK_EFFECT;
    }

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_POWER_LINE_FREQUENCY;
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) 
	|| (qc.type != V4L2_CTRL_TYPE_MENU)){
        mIoctlSupport &= ~IOCTL_MASK_BANDING;
    }else{
        mIoctlSupport |= IOCTL_MASK_BANDING;
    }
    mAntiBanding = qc.default_value;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_FOCUS_AUTO;
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)
	|| (qc.type != V4L2_CTRL_TYPE_MENU)){
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

status_t V4LCamAdpt::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{

    status_t ret = NO_ERROR;
    v4l2_buffer hbuf_query;
    memset(&hbuf_query,0,sizeof(v4l2_buffer));

    //LOGD("fillThisBuffer frameType=%d", frameType);
    if (CameraFrame::IMAGE_FRAME == frameType)
    {
	    //if (NULL != mEndImageCaptureCallback)
	        //mEndImageCaptureCallback(mEndCaptureData);
	    if (NULL != mReleaseImageBuffersCallback)
	        mReleaseImageBuffersCallback(mReleaseData);
	    return NO_ERROR;
    }
    if ( !mVideoInfo->isStreaming || !mPreviewing)
    {
	    return NO_ERROR;
    }

    int i = mPreviewBufs.valueFor(( unsigned int )frameBuf);
    if(i<0)
    {
	    return BAD_VALUE;
    }
    if(nQueued>=mPreviewBufferCount)
    {
        CAMHAL_LOGEB("fill buffer error, reach the max preview buff:%d,max:%d",
                nQueued,mPreviewBufferCount);
        return BAD_VALUE;
    }

    hbuf_query.index = i;
    hbuf_query.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    hbuf_query.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(mCameraHandle, VIDIOC_QBUF, &hbuf_query);
    if (ret < 0) {
       CAMHAL_LOGEB("Init: VIDIOC_QBUF %d Failed, errno=%d\n",i, errno);
       return -1;
    }
    //CAMHAL_LOGEB("fillThis Buffer %d",i);
    nQueued++;
    return ret;

}

status_t V4LCamAdpt::setParameters(const CameraParameters &params)
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
        CAMHAL_LOGEB("Zoom Parameter Out of range1------zoom level:%d,max level:%d",zoom,maxzoom);
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
        if(mIoctlSupport & IOCTL_MASK_ZOOM)
            set_zoom_level(mCameraHandle,z);
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

    mParams.getPreviewFpsRange(&min_fps, &max_fps);
    if((min_fps<0)||(max_fps<0)||(max_fps<min_fps))
    {
        rtn = INVALID_OPERATION;
    }

    LOG_FUNCTION_NAME_EXIT;
    return rtn;
}


void V4LCamAdpt::getParameters(CameraParameters& params)
{
    LOG_FUNCTION_NAME;

    // Return the current parameter set
    //params = mParams;
    //that won't work. we might wipe out the existing params

    LOG_FUNCTION_NAME_EXIT;
}


///API to give the buffers to Adapter
status_t V4LCamAdpt::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    switch(mode)
    {
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

status_t V4LCamAdpt::setBuffersFormat(int width, int height, int pixelformat)
{
    int ret = NO_ERROR;
    CAMHAL_LOGDB("Width * Height %d x %d format 0x%x", width, height, pixelformat);

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
        CAMHAL_LOGEB("Open: VIDIOC_S_FMT Failed: %s", strerror(errno));
        return ret;
    }

    return ret;
}

status_t V4LCamAdpt::getBuffersFormat(int &width, int &height, int &pixelformat)
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
    CAMHAL_LOGDB("VIDIOC_G_FMT,W*H: %5d x %5d format 0x%x",
                    width, height, pixelformat);
    return ret;
}

status_t V4LCamAdpt::UseBuffersPreview(void* bufArr, int num)
{
    int ret = NO_ERROR;

    if(NULL == bufArr)
    {
        return BAD_VALUE;
    }

    int width, height;
    mParams.getPreviewSize(&width, &height);

    mPreviewWidth = width;
    mPreviewHeight = height;

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

    setBuffersFormat(width, height, pixfmt);
    //First allocate adapter internal buffers at V4L level for USB Cam
    //These are the buffers from which we will copy the data into overlay buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
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
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        mVideoInfo->mem[i] = mmap (0,
               mVideoInfo->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               mCameraHandle,
               mVideoInfo->buf.m.offset);

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
            return -1;
        }

        uint32_t *ptr = (uint32_t*) bufArr;

        //Associate each Camera internal buffer with the one from Overlay
        CAMHAL_LOGDB("mPreviewBufs.add %#x, %d", ptr[i], i);
        mPreviewBufs.add((int)ptr[i], i);

    }

    for(int i = 0;i < num; i++)
    {
        mPreviewIdxs.add(mPreviewBufs.valueAt(i),i);
    }

    // Update the preview buffer count
    mPreviewBufferCount = num;

    return ret;
}

status_t V4LCamAdpt::UseBuffersCapture(void* bufArr, int num)
{
    int ret = NO_ERROR;

    if(NULL == bufArr)
    {
        return BAD_VALUE;
    }

    if (num != 1)
    {
        CAMHAL_LOGDB("num=%d", num);
    }

    /* This will only be called right before taking a picture, so
     * stop preview now so that we can set buffer format here.
     */
    this->stopPreview();

    int width, height;
    mParams.getPictureSize(&width, &height);
    mCaptureWidth = width;
    mCaptureHeight = height;

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
    setBuffersFormat(width, height, DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT);

    //First allocate adapter internal buffers at V4L level for Cam
    //These are the buffers from which we will copy the data into display buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
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
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        mVideoInfo->mem[i] = mmap (0,
               mVideoInfo->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               mCameraHandle,
               mVideoInfo->buf.m.offset);

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
            return -1;
        }

        uint32_t *ptr = (uint32_t*) bufArr;
        CAMHAL_LOGDB("UseBuffersCapture %#x", ptr[0]);
        mCaptureBuf = (camera_memory_t*)ptr[0];
    }

    return ret;
}

status_t V4LCamAdpt::takePicture()
{
    LOG_FUNCTION_NAME;
    if (createThread(beginPictureThread, this) == false)
        return -1;
    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}


int V4LCamAdpt::beginAutoFocusThread(void *cookie)
{
    V4LCamAdpt *c = (V4LCamAdpt *)cookie;
    struct v4l2_control ctl;
    int ret = -1;

    if( c->mIoctlSupport & IOCTL_MASK_FOCUS){
	    ctl.id = V4L2_CID_FOCUS_AUTO;
	    ctl.value = CAM_FOCUS_MODE_AUTO;//c->cur_focus_mode;
	    ret = ioctl(c->mCameraHandle, VIDIOC_S_CTRL, &ctl);
	    for(int j=0; j<50; j++){
		usleep(30000);//30*50ms=1.5s
		ret = ioctl(c->mCameraHandle, VIDIOC_G_CTRL, &ctl);
		if( (0==ret) ||
		 ((ret < 0)&&(EBUSY != errno)) ){
			break;
		}
	    }
    }

    c->setState(CAMERA_CANCEL_AUTOFOCUS);
    c->commitState();

    if( (c->mIoctlSupport & IOCTL_MASK_FLASH)
    	&&(FLASHLIGHT_ON == c->mFlashMode)){
    	c->set_flash_mode( c->mCameraHandle, "off");
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

status_t V4LCamAdpt::autoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if( (mIoctlSupport & IOCTL_MASK_FLASH)
    	&&(FLASHLIGHT_ON == mFlashMode)){
    	set_flash_mode( mCameraHandle, "on");
    }
    cur_focus_mode_for_conti = CAM_FOCUS_MODE_AUTO;
    if (createThread(beginAutoFocusThread, this) == false)
    {
        ret = UNKNOWN_ERROR;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


status_t V4LCamAdpt::cancelAutoFocus()
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

status_t V4LCamAdpt::startPreview()
{
    status_t ret = NO_ERROR;
    int frame_count = 0,ret_c = 0;
    void *frame_buf = NULL;
    Mutex::Autolock lock(mPreviewBufsLock);

    if(mPreviewing)
    {
        return BAD_VALUE;
    }

    setMirrorEffect();
    
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        set_rotate_value(mCameraHandle,0);
        mRotateValue = 0;
    }

    nQueued = 0;
    for (int i = 0; i < mPreviewBufferCount; i++) 
    {
        frame_count = -1;
        frame_buf = (void *)mPreviewBufs.keyAt(i);

        if((ret_c = getFrameRefCount(frame_buf,CameraFrame::PREVIEW_FRAME_SYNC))>=0)
            frame_count = ret_c;

        //if((ret_c = getFrameRefCount(frame_buf,CameraFrame::VIDEO_FRAME_SYNC))>=0)
        //    frame_count += ret_c;
 
        CAMHAL_LOGDB("startPreview--buffer address:0x%x, refcount:%d",
                                        (uint32_t)frame_buf,frame_count);
        if(frame_count>0)
            continue;
        //mVideoInfo->buf.index = i;
        mVideoInfo->buf.index = mPreviewBufs.valueFor((uint32_t)frame_buf);
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEA("VIDIOC_QBUF Failed");
            return -EINVAL;
        }
        CAMHAL_LOGDB("startPreview --length=%d, index:%d", 
                    mVideoInfo->buf.length,mVideoInfo->buf.index);
        nQueued++;
    }

    enum v4l2_buf_type bufType;
    if (!mVideoInfo->isStreaming) 
    {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
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
    mPreviewThread = new PreviewThread(this);
    CAMHAL_LOGDA("Created preview thread");
    //Update the flag to indicate we are previewing
    mPreviewing = true;
    return ret;
}

status_t V4LCamAdpt::stopPreview()
{
    enum v4l2_buf_type bufType;
    int ret = NO_ERROR;

    Mutex::Autolock lock(mPreviewBufsLock);
    if(!mPreviewing)
    {
	    return NO_INIT;
    }

    mPreviewing = false;
    mFocusMoveEnabled = false;
    mPreviewThread->requestExitAndWait();
    mPreviewThread.clear();


    CAMHAL_LOGDA("stopPreview streamoff..\n");
    if (mVideoInfo->isStreaming) {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
        if (ret < 0) {
            CAMHAL_LOGEB("StopStreaming: Unable to stop capture: %s", strerror(errno));
            return ret;
        }

        mVideoInfo->isStreaming = false;
    }

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

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
	
    }

    mPreviewBufs.clear();
    mPreviewIdxs.clear();
    return ret;

}

char * V4LCamAdpt::GetFrame(int &index)
{
    int ret;

    if(nQueued<=0){
        CAMHAL_LOGEA("GetFrame: No buff for Dequeue");
        return NULL;
    }
    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    ret = ioctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
    if (ret < 0) {
        if(EAGAIN == errno){
            index = -1;
        }else{
            CAMHAL_LOGEB("GetFrame: VIDIOC_DQBUF Failed,errno=%d\n",errno);
        }
        return NULL;
    }
    nDequeued++;
    nQueued--;
    index = mVideoInfo->buf.index;

    return (char *)mVideoInfo->mem[mVideoInfo->buf.index];
}

//API to get the frame size required  to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
status_t V4LCamAdpt::getFrameSize(size_t &width, size_t &height)
{
    status_t ret = NO_ERROR;

    // Just return the current preview size, nothing more to do here.
    mParams.getPreviewSize(( int * ) &width,
                           ( int * ) &height);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t V4LCamAdpt::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    // We don't support meta data, so simply return
    return NO_ERROR;
}

status_t V4LCamAdpt::getPictureBufferSize(size_t &length, size_t bufferCount)
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

status_t V4LCamAdpt::recalculateFPS()
{
    float currentFPS;

    mFrameCount++;

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
    {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
        {
            mFPS = currentFPS;
        }
        else
        {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
        }

        mLastFPS = mFPS;
        mIter++;
    }

    return NO_ERROR;
}

void V4LCamAdpt::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    //LOG_FUNCTION_NAME;

    //LOG_FUNCTION_NAME_EXIT;
}


V4LCamAdpt::V4LCamAdpt(size_t sensor_index)
{
    LOG_FUNCTION_NAME;

    mbDisableMirror = false;
    mSensorIndex = sensor_index;
    mCameraHandle = -1;

#ifdef AMLOGIC_TWO_CH_UVC
    mCamEncodeHandle = -1;
#endif
    CAMHAL_LOGDB("mVideoInfo=%p\n", mVideoInfo); 
    mVideoInfo = NULL; 

    LOG_FUNCTION_NAME_EXIT;
}

V4LCamAdpt::~V4LCamAdpt()
{
    LOG_FUNCTION_NAME;

    // Close the camera handle and free the video info structure
    close(mCameraHandle);
#ifdef AMLOGIC_TWO_CH_UVC
    if(mCamEncodeHandle > 0){
        close(mCamEncodeHandle);
    }
#endif

    if (mVideoInfo)
    {
        free(mVideoInfo);
        mVideoInfo = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;
}

/* Preview Thread */
// ---------------------------------------------------------------------------

int V4LCamAdpt::previewThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;
    unsigned delay;
    unsigned uFrameInvals;

    if (mPreviewing)
    {
        int index = 0;

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
        uFrameInvals = (unsigned)(1000000.0f / float(mPreviewFrameRate));
        delay = uFrameInvals >>2;
#else
        int previewFrameRate = mParams.getPreviewFrameRate();
        delay = (unsigned)(1000000.0f / float(previewFrameRate));
#endif

#ifdef AMLOGIC_USB_CAMERA_DECREASE_FRAMES
        usleep(delay*5);
#else
        usleep(delay);
#endif

        char *fp = this->GetFrame(index);
#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
        if((-1==index)||!fp)
        {
            return 0;
        }
#else
        if(!fp){
            int previewFrameRate = mParams.getPreviewFrameRate();
            delay = (unsigned)(1000000.0f / float(previewFrameRate)) >> 1;
            CAMHAL_LOGEB("Preview thread get frame fail, need sleep:%d",delay);
            usleep(delay);
            return BAD_VALUE;
        }
#endif

        uint8_t* ptr = (uint8_t*) mPreviewBufs.keyAt(mPreviewIdxs.valueFor(index));

        if (!ptr)
        {
            CAMHAL_LOGEA("Preview thread mPreviewBufs error!");
            return BAD_VALUE;
        }

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
        gettimeofday( &previewTime2, NULL);
        unsigned bwFrames = previewTime2.tv_sec - previewTime1.tv_sec;
        bwFrames = bwFrames*1000000 + previewTime2.tv_usec -previewTime1.tv_usec;
        if( bwFrames + 10000 <  uFrameInvals ) {
            //cts left 20ms(Android 4.1), we left 10ms, Android may cut this 20ms;
            CAMHAL_LOGDB("bwFrames=%d, uFrameInvals=%d\n", bwFrames, uFrameInvals);
            fillThisBuffer( ptr, CameraFrame::PREVIEW_FRAME_SYNC);
            return 0;
        }else{
            memcpy( &previewTime1, &previewTime2, sizeof( struct timeval));
        }
#endif

        uint8_t* dest = NULL;
#ifdef AMLOGIC_CAMERA_OVERLAY_SUPPORT
        camera_memory_t* VideoCameraBufferMemoryBase = (camera_memory_t*)ptr;
        dest = (uint8_t*)VideoCameraBufferMemoryBase->data; //ptr;
#else
        private_handle_t* gralloc_hnd = (private_handle_t*)ptr;
        dest = (uint8_t*)gralloc_hnd->base; //ptr;
#endif
        int width, height;
        uint8_t* src = (uint8_t*) fp;
        if((mPreviewWidth <= 0)||(mPreviewHeight <= 0)){
            mParams.getPreviewSize(&width, &height);
        }else{
            width = mPreviewWidth;
            height = mPreviewHeight;
        }

        if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ // 422I
            frame.mLength = width*height*2;
            memcpy(dest,src,frame.mLength);
        }else if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){ //420sp
            frame.mLength = width*height*3/2;
            if ( CameraFrame::PIXEL_FMT_NV21 == mPixelFormat){
                memcpy(dest,src,frame.mLength);
            }else{
                yv12_adjust_memcpy(dest,src,width,height);
            }

        }else{ //default case
            frame.mLength = width*height*3/2;
            memcpy(dest,src,frame.mLength);            
        }

        frame.mFrameMask |= CameraFrame::PREVIEW_FRAME_SYNC;
        
        if(mRecording){
            frame.mFrameMask |= CameraFrame::VIDEO_FRAME_SYNC;
        }
        frame.mBuffer = ptr; //dest
        frame.mAlignment = width;
        frame.mOffset = 0;
        frame.mYuv[0] = 0;
        frame.mYuv[1] = 0;
        frame.mWidth = width;
        frame.mHeight = height;
        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
        frame.mPixelFmt = mPixelFormat;
        ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
        if (ret){
            CAMHAL_LOGEB("setInitFrameRefCount err=%d", ret);
        }else
            ret = sendFrameToSubscribers(&frame);
        //LOGD("previewThread /sendFrameToSubscribers ret=%d", ret);           
    }
    if( (mIoctlSupport & IOCTL_MASK_FOCUS_MOVE) && mFocusMoveEnabled ){
		getFocusMoveStatus();
    }

    return ret;
}

/* Image Capture Thread */
// ---------------------------------------------------------------------------
int V4LCamAdpt::GenExif(ExifElementsTable* exiftable)
{
    char exifcontent[256];

    //Make
    exiftable->insertElement("Make",
            (const char*)mParams.get(ExCameraParameters::KEY_EXIF_MAKE));

    //Model
    exiftable->insertElement("Model",
            (const char*)mParams.get(ExCameraParameters::KEY_EXIF_MODEL));

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

    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        orientation = 1;
        if((mRotateValue==90)||(mRotateValue==270)){
            int temp = width;
            width = height;
            height = temp;
        }
    }

    sprintf(exifcontent,"%d",orientation);
    exiftable->insertElement("Orientation",(const char*)exifcontent);
    
    sprintf(exifcontent,"%d",width);
    exiftable->insertElement("ImageWidth",(const char*)exifcontent);
    sprintf(exifcontent,"%d",height);
    exiftable->insertElement("ImageLength",(const char*)exifcontent);

    //focal length  RATIONAL
    float focallen = mParams.getFloat(CameraParameters::KEY_FOCAL_LENGTH);
    if(focallen >= 0)
    {
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
    if(times != -1)
    {
        struct tm tmstruct;
        tmstruct = *(gmtime(&times));//convert to standard time
        //date
        strftime(exifcontent, 20, "%Y:%m:%d", &tmstruct);
        exiftable->insertElement("GPSDateStamp",(const char*)exifcontent);
        //time
        sprintf(exifcontent,"%d/%d,%d/%d,%d/%d",
                tmstruct.tm_hour,1,tmstruct.tm_min,1,tmstruct.tm_sec,1);
        exiftable->insertElement("GPSTimeStamp",(const char*)exifcontent);
    }

    //gps latitude info
    char* latitudestr = (char*)mParams.get(CameraParameters::KEY_GPS_LATITUDE);
    if(latitudestr!=NULL)
    {
        int offset = 0;
        float latitude = mParams.getFloat(CameraParameters::KEY_GPS_LATITUDE);
        if(latitude < 0.0)
        {
            offset = 1;
            latitude*= (float)(-1);
        }

        int latitudedegree = latitude;
        float latitudeminuts = (latitude-(float)latitudedegree)*60;
        int latitudeminuts_int = latitudeminuts;
        float latituseconds = (latitudeminuts-(float)latitudeminuts_int)*60+0.5;
        int latituseconds_int = latituseconds;
        sprintf(exifcontent,"%d/%d,%d/%d,%d/%d",
                latitudedegree,1,latitudeminuts_int,1,latituseconds_int,1);
        exiftable->insertElement("GPSLatitude",(const char*)exifcontent);

        exiftable->insertElement("GPSLatitudeRef",(offset==1)?"S":"N");
    }

    //gps Longitude info
    char* longitudestr = (char*)mParams.get(CameraParameters::KEY_GPS_LONGITUDE);
    if(longitudestr!=NULL)
    {
        int offset = 0;
        float longitude = mParams.getFloat(CameraParameters::KEY_GPS_LONGITUDE);
        if(longitude < 0.0)
        {
            offset = 1;
            longitude*= (float)(-1);
        }

        int longitudedegree = longitude;
        float longitudeminuts = (longitude-(float)longitudedegree)*60;
        int longitudeminuts_int = longitudeminuts;
        float longitudeseconds = (longitudeminuts-(float)longitudeminuts_int)*60+0.5;
        int longitudeseconds_int = longitudeseconds;
        sprintf(exifcontent,"%d/%d,%d/%d,%d/%d",
                longitudedegree,1,longitudeminuts_int,1,longitudeseconds_int,1);
        exiftable->insertElement("GPSLongitude",(const char*)exifcontent);

        exiftable->insertElement("GPSLongitudeRef",(offset==1)?"S":"N");
    }

    //gps Altitude info
    char* altitudestr = (char*)mParams.get(CameraParameters::KEY_GPS_ALTITUDE);
    if(altitudestr!=NULL)
    {
        int offset = 0;
        float altitude = mParams.getFloat(CameraParameters::KEY_GPS_ALTITUDE);
        if(altitude < 0.0)
        {
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
    char* processmethod = 
            (char*)mParams.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    if(processmethod!=NULL)
    {
        memset(exifcontent,0,sizeof(exifcontent));
        char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };//asicii
        memcpy(exifcontent,ExifAsciiPrefix,8);
        memcpy(exifcontent+8,processmethod,strlen(processmethod));
        exiftable->insertElement("GPSProcessingMethod",(const char*)exifcontent);
    }
    return 1;
}

/*static*/ int V4LCamAdpt::beginPictureThread(void *cookie)
{
    V4LCamAdpt *c = (V4LCamAdpt *)cookie;
    return c->pictureThread();
}

int V4LCamAdpt::pictureThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;
    int dqTryNum = 3;

    setMirrorEffect();

    if( (mIoctlSupport & IOCTL_MASK_FLASH)
    	&&(FLASHLIGHT_ON == mFlashMode)){
    	set_flash_mode( mCameraHandle, "on");
    }
    if (true)
    {
        mVideoInfo->buf.index = 0;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0) 
        {
            CAMHAL_LOGEA("VIDIOC_QBUF Failed");
            return -EINVAL;
        }
        nQueued ++;

        if(mIoctlSupport & IOCTL_MASK_ROTATE){
            set_rotate_value(mCameraHandle,mRotateValue);
        }

        enum v4l2_buf_type bufType;
        if (!mVideoInfo->isStreaming) 
        {
            bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
            if (ret < 0) {
                CAMHAL_LOGEB("StartStreaming: Unable to start capture: %s",
                                                                strerror(errno));
                return ret;
            }

            mVideoInfo->isStreaming = true;
        }

        int index = 0;
        char *fp = this->GetFrame(index);

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
		while(!fp && (-1 == index) ){
			usleep( 10000 );
			fp = this->GetFrame(index);
		}
#else
		if(!fp)
		{
			CAMHAL_LOGDA("GetFrame fail, this may stop preview\n");
			return 0; //BAD_VALUE;
		}
#endif
        if (!mCaptureBuf || !mCaptureBuf->data)
        {
	        return 0; //BAD_VALUE;
        }

        int width, height;
        uint8_t* dest = (uint8_t*)mCaptureBuf->data;
        uint8_t* src = (uint8_t*) fp;
        if((mCaptureWidth <= 0)||(mCaptureHeight <= 0)){
            mParams.getPictureSize(&width, &height);
        }else{
            width = mCaptureWidth;
            height = mCaptureHeight;
        }
        
        if((mRotateValue==90)||(mRotateValue==270)){
            int temp = 0;
            temp = width;
            width = height;
            height = temp;
        }
        
        CAMHAL_LOGDB("mCaptureBuf=%p,dest=%p,fp=%p,index=%d\n"
                     "w=%d h=%d,len=%d,bytesused=%d\n",
                     mCaptureBuf, dest, fp,index, width, height,
                     mVideoInfo->buf.length, mVideoInfo->buf.bytesused);

        if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_RGB24){ // rgb24
            frame.mLength = width*height*3;
            frame.mQuirks = CameraFrame::ENCODE_RAW_RGB24_TO_JPEG
                            | CameraFrame::HAS_EXIF_DATA;
            memcpy(dest,src,mVideoInfo->buf.length);
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ //   422I
            frame.mLength = width*height*2;
            frame.mQuirks = CameraFrame::ENCODE_RAW_YUV422I_TO_JPEG
                            | CameraFrame::HAS_EXIF_DATA;
            memcpy(dest, src, mVideoInfo->buf.length);
        }else if(DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){ //   420sp
            frame.mLength = width*height*3/2;
            frame.mQuirks = CameraFrame::ENCODE_RAW_YUV420SP_TO_JPEG
                                | CameraFrame::HAS_EXIF_DATA;
            memcpy(dest,src,mVideoInfo->buf.length);
        }else{ //default case
            frame.mLength = width*height*3;
            frame.mQuirks = CameraFrame::ENCODE_RAW_RGB24_TO_JPEG
                            | CameraFrame::HAS_EXIF_DATA;
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
        frame.mWidth = width;
        frame.mHeight = height;
        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
        
        if (mVideoInfo->isStreaming) 
        {
            bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
            if (ret < 0) 
            {
                CAMHAL_LOGEB("StopStreaming: Unable to stop capture: %s",
                                                            strerror(errno));
                return ret;
            }

            mVideoInfo->isStreaming = false;
        }

        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        nQueued = 0;
        nDequeued = 0;

        /* Unmap buffers */
        if (munmap(mVideoInfo->mem[0], mVideoInfo->buf.length) < 0){
            CAMHAL_LOGEA("Unmap failed");
        }


    }

    if( (mIoctlSupport & IOCTL_MASK_FLASH)
    	&&(FLASHLIGHT_ON == mFlashMode)){
    	set_flash_mode( mCameraHandle, "off");
    }
    if(mIoctlSupport & IOCTL_MASK_ROTATE){
        set_rotate_value(mCameraHandle,0);
        mRotateValue = 0;
    }

    // start preview thread again after stopping it in UseBuffersCapture
    {
        Mutex::Autolock lock(mPreviewBufferLock);
        UseBuffersPreview(mPreviewBuffers, mPreviewBufferCount);        
    }
    startPreview();

    ret = setInitFrameRefCount(frame.mBuffer, frame.mFrameMask);
    if (ret){
        CAMHAL_LOGEB("setInitFrameRefCount err=%d", ret);
    }else{
        ret = sendFrameToSubscribers(&frame);
    }

    return ret;
}


status_t V4LCamAdpt::disableMirror(bool bDisable) {
    CAMHAL_LOGDB("disableMirror %d",bDisable);
    mbDisableMirror = bDisable;
    setMirrorEffect();
    return NO_ERROR;
}

status_t V4LCamAdpt::setMirrorEffect() {

    bool bEnable = mbFrontCamera&&(!mbDisableMirror);
    CAMHAL_LOGDB("setmirror effect %d",bEnable);
    
    if(mIoctlSupport & IOCTL_MASK_HFLIP){
        if(set_hflip_mode(mCameraHandle,bEnable))
            writefile((char *)SYSFILE_CAMERA_SET_MIRROR,(char*)(bEnable?"1":"0"));
    }else{
        writefile((char *)SYSFILE_CAMERA_SET_MIRROR,(char*)(bEnable?"1":"0"));
    }
    return NO_ERROR;
}



// ---------------------------------------------------------------------------


bool V4LCamAdpt::isPreviewDevice(int camera_fd)
{
	int ret;
	int index;
	struct v4l2_fmtdesc fmtdesc;
	
	for(index=0;;index++){
		memset(&fmtdesc, 0, sizeof(struct v4l2_fmtdesc));
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmtdesc.index = index;
		ret = ioctl( camera_fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if((V4L2_PIX_FMT_YUYV==fmtdesc.pixelformat) ||
           (V4L2_PIX_FMT_NV21==fmtdesc.pixelformat)){ 
			return true;
		}
		if(ret < 0)
			break;
	}
	
	return false;
}

int V4LCamAdpt::getValidFrameSize( int pixel_format, char *framesize)
{
    struct v4l2_frmsizeenum frmsize;
    int i=0;
    char tempsize[12];
    framesize[0] = '\0';

    memset(&frmsize,0,sizeof(v4l2_frmsizeenum));
    for(i=0;;i++){
        frmsize.index = i;
        frmsize.pixel_format = pixel_format;
        if(ioctl(mCameraHandle, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
            if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE){ //only support this type

                snprintf(tempsize, sizeof(tempsize), "%dx%d,",
                        frmsize.discrete.width, frmsize.discrete.height);
                strcat(framesize, tempsize);

            }
            else
                break;
        }
        else
            break;
    }

    if(framesize[0] == '\0')
        return -1;
    else
        return 0;
}

int V4LCamAdpt::getCameraOrientation(bool frontcamera, char* property)
{
    int degree = -1;
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
    return degree;
}

static int enumCtrlMenu(int camera_fd, struct v4l2_queryctrl *qi,
			char* menu_items, char*def_menu_item)
{
    struct v4l2_queryctrl qc;
    struct v4l2_querymenu qm;
    int ret;
    int mode_count = -1;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = qi->id;
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if( (ret<0) || (qc.flags == V4L2_CTRL_FLAG_DISABLED) ){
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

bool V4LCamAdpt::getCameraWhiteBalance( char* wb_modes, char*def_wb_mode)
{
    struct v4l2_queryctrl qc;
    int item_count=0;

    memset( &qc, 0, sizeof(qc));

    qc.id = V4L2_CID_DO_WHITE_BALANCE;
    item_count = enumCtrlMenu( mCameraHandle, &qc, wb_modes, def_wb_mode);

    if(0 >= item_count){
        strcpy( wb_modes, "auto,daylight,incandescent,fluorescent");
        strcpy(def_wb_mode, "auto");
    }
    return true;
}

bool V4LCamAdpt::getCameraBanding(char* banding_modes, char*def_banding_mode)
{
    struct v4l2_queryctrl qc;
    int item_count=0;
    char *tmpbuf=NULL;

    memset( &qc, 0, sizeof(qc));
    qc.id = V4L2_CID_POWER_LINE_FREQUENCY;
    
    item_count = enumCtrlMenu( mCameraHandle, &qc, banding_modes, def_banding_mode);
    
    if(0 >= item_count){
        strcpy( banding_modes, "50hz,60hz");
        strcpy( def_banding_mode, "50hz");
    }
    return true;
}

#define MAX_LEVEL_FOR_EXPOSURE 16
#define MIN_LEVEL_FOR_EXPOSURE 3

bool V4LCamAdpt::getCameraExposureValue(int &min, int &max,
						  int &step, int &def)
{
    struct v4l2_queryctrl qc;
    int ret=0;
    int level = 0;
    int middle = 0;

    memset( &qc, 0, sizeof(qc));

    qc.id = V4L2_CID_EXPOSURE;
    ret = ioctl( mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if(ret<0){
    	CAMHAL_LOGDB("QUERYCTRL failed, errno=%d\n", errno);
        min = -4;
        max = 4;
        def = 0;
        step = 1;
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
    	CAMHAL_LOGDB("not in[min,max], min=%d, max=%d, def=%d, step=%d\n",
                                        min, max, def, step);
        return true;
    }

    middle = (qc.minimum+qc.maximum)/2;
    min = qc.minimum - middle; 
    max = qc.maximum - middle;
    def = qc.default_value - middle;
    step = qc.step;

    return true;
}

bool V4LCamAdpt::getCameraAutoFocus( char* focus_mode_str, char*def_focus_mode)
{
    struct v4l2_queryctrl qc;    
    struct v4l2_querymenu qm;
    bool auto_focus_enable = false;
    int menu_num = 0;
    int mode_count = 0;

    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_CID_FOCUS_AUTO;
    menu_num = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED)
            ||( menu_num < 0) || (qc.type != V4L2_CTRL_TYPE_MENU)){
        auto_focus_enable = false;
        CAMHAL_LOGDB("camera handle %d can't support auto focus",mCameraHandle);
    }else {
        memset(&qm, 0, sizeof(qm));
        qm.id = V4L2_CID_FOCUS_AUTO;
        qm.index = qc.default_value;
        strcpy(def_focus_mode, "auto");

        for (int index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_FOCUS_AUTO;
            qm.index = index;
            if(ioctl (mCameraHandle, VIDIOC_QUERYMENU, &qm) < 0){
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

bool V4LCamAdpt::getCameraHandle() 
{
    return mCameraHandle;
}

bool V4LCamAdpt::isVolatileCam()
{

    char *bus_info;
    bool ret = true;
    int size = 0;

    size = sizeof(mVideoInfo->cap.bus_info);
    bus_info = (char *)calloc( 1, size);
    memset( bus_info, 0, size);
    
    strncpy( bus_info, (char *)&mVideoInfo->cap.bus_info, size);
    if( strstr( bus_info, "usb")){
        ret = true; 
        CAMHAL_LOGDA("usb device\n")
    }else{
        ret = false; 
        CAMHAL_LOGDA("not usb device\n")
    }
    CAMHAL_LOGDB("bus_info=%s\n", bus_info);

    if(bus_info){
        free(bus_info);
        bus_info = NULL; 
    }

    return ret;

}
bool V4LCamAdpt::isFrontCam( int camera_id )
{
    int bFrontCam = false;
		
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
    return bFrontCam;
    //return true;// virtual camera is a front camera.
}

extern "C" void newloadCaps(int camera_id, CameraProperties::Properties* params) {
    const char DEFAULT_BRIGHTNESS[] = "50";
    const char DEFAULT_CONTRAST[] = "100";
    const char DEFAULT_IPP[] = "ldc-nsf";
    const char DEFAULT_GBCE[] = "disable";
    const char DEFAULT_ISO_MODE[] = "auto";
    const char DEFAULT_PICTURE_FORMAT[] = "jpeg";
    const char DEFAULT_PICTURE_SIZE[] = "640x480";
    const char PREVIEW_FORMAT_420SP[] = "yuv420sp";
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

    bool bFrontCam = false;
    int tempid = camera_id;
    int camera_fd = -1;
    V4LCamAdpt v(camera_id);
    
    const char *device_name = VIRTUAL_DEVICE_PATH(camera_id);
	if(device_name){
		params->set(CameraProperties::DEVICE_NAME, device_name);
	}else{
		CAMHAL_LOGDA("no virtual camera device node\n");
		params->set(CameraProperties::DEVICE_NAME, "/dev/video11");
	}

    int iret = 0;
	if(v.initialize( params ) != NO_ERROR){
		CAMHAL_LOGEA("Unable to create or initialize V4LCamAdpt!!");
	}
    
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
    params->set(CameraProperties::RELOAD_WHEN_OPEN, "1");
#else
    params->set(CameraProperties::RELOAD_WHEN_OPEN, "0");
#endif
    
    bFrontCam = v.isFrontCam( camera_id ); 
    CAMHAL_LOGVB("%s\n", bFrontCam?"front cam":"back cam");
    //should changed while the screen orientation changed.
    int degree = -1;
    char property[64];
    memset(property,0,sizeof(property));
    if(bFrontCam == true) {
        params->set(CameraProperties::FACING_INDEX, ExCameraParameters::FACING_FRONT);
        if(v.getCameraOrientation(bFrontCam,property)>=0){
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
        if( v.getCameraOrientation(bFrontCam,property)>=0){
            params->set(CameraProperties::ORIENTATION_INDEX,property);
        }else{
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
            params->set(CameraProperties::ORIENTATION_INDEX,"180");
#else
            params->set(CameraProperties::ORIENTATION_INDEX,"90");
#endif
        }
    }

    params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,"yuv420sp,yuv420p"); //yuv420p for cts
    if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_YUYV){ // 422I
        //params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,PREVIEW_FORMAT_422I);
        params->set(CameraProperties::PREVIEW_FORMAT,PREVIEW_FORMAT_422I);
    }else if(DEFAULT_PREVIEW_PIXEL_FORMAT == V4L2_PIX_FMT_NV21){ //420sp
        //params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,PREVIEW_FORMAT_420SP);
        params->set(CameraProperties::PREVIEW_FORMAT,PREVIEW_FORMAT_420SP);
    }else{ //default case
        //params->set(CameraProperties::SUPPORTED_PREVIEW_FORMATS,PREVIEW_FORMAT_420SP);
        params->set(CameraProperties::PREVIEW_FORMAT,PREVIEW_FORMAT_420SP);
    }

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
    int fps=0, fps_num=0;
    int ret;
    char *fpsrange=(char *)calloc(32,sizeof(char));
	
    ret = v.enumFramerate(&fps, &fps_num);
    if((fpsrange != NULL)&&(NO_ERROR == ret) && ( 0 !=fps_num )){
	    sprintf(fpsrange,"%s%d","10,",fps/fps_num);
	    CAMHAL_LOGDA("O_NONBLOCK operation to do previewThread\n");

	    params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, fpsrange);
	    params->set(CameraProperties::PREVIEW_FRAME_RATE, fps/fps_num);

	    memset( fpsrange, 0, 32*sizeof(char));
	    sprintf(fpsrange,"%s%d","10000,",fps*1000/fps_num);
	    params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, fpsrange);
	    params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, fpsrange);

	    memset( fpsrange, 0, 32*sizeof(char));
	    sprintf(fpsrange,"(%s%d)","5000,",fps*1000/fps_num);
	    params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, fpsrange);
	    memset( fpsrange, 0, 32*sizeof(char));
	    sprintf(fpsrange,"%s%d","5000,",fps*1000/fps_num);
	    params->set(CameraProperties::FRAMERATE_RANGE, fpsrange);
    }else{
	    if(NO_ERROR != ret){
		    CAMHAL_LOGDA("sensor driver need to implement enum framerate!!!\n");
            }
	    params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, "10,15");
	    params->set(CameraProperties::PREVIEW_FRAME_RATE, "15");

	    params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "(5000,26623)");
	    params->set(CameraProperties::FRAMERATE_RANGE, "5000,26623");
	    params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, "10000,15000");
	    params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, "10000,15000");
    }
#else
	    params->set(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES, "10,15");
	    params->set(CameraProperties::PREVIEW_FRAME_RATE, "15");

	    params->set(CameraProperties::FRAMERATE_RANGE_SUPPORTED, "(5000,26623)");
	    params->set(CameraProperties::FRAMERATE_RANGE, "5000,26623");
	    params->set(CameraProperties::FRAMERATE_RANGE_IMAGE, "10000,15000");
	    params->set(CameraProperties::FRAMERATE_RANGE_VIDEO, "10000,15000");
#endif

	//get preview size & set
    char *sizes = (char *) calloc (1, 1024);
    if(!sizes){
        CAMHAL_LOGDA("Alloc string buff error!");
        return;
    }        
    
    memset(sizes,0,1024);
    uint32_t preview_format = DEFAULT_PREVIEW_PIXEL_FORMAT;
    if (!v.getValidFrameSize( preview_format, sizes)) {
        int len = strlen(sizes);
        unsigned int supported_w = 0,  supported_h = 0,w = 0,h = 0;
        if(len>1){
            if(sizes[len-1] == ',')
                sizes[len-1] = '\0';
        }

        char small_size[8] = "176x144"; //for cts
        if(strstr(sizes,small_size)==NULL){
            if((len+sizeof(small_size))<(1024-1)){
                strcat(sizes,",");
                strcat(sizes,small_size);
            }
        }

        params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES, sizes);

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
        //char * b = strrchr(sizes, ',');
        //if (b) 
        //    b++;
        //else 
        //    b = sizes;
        params->set(CameraProperties::PREVIEW_SIZE, sizes);
    }
    else
    {
        params->set(CameraProperties::SUPPORTED_PREVIEW_SIZES,
                "640x480,352x288,176x144");
        params->set(CameraProperties::PREVIEW_SIZE,"640x480");
    }

    params->set(CameraProperties::SUPPORTED_PICTURE_FORMATS, DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::PICTURE_FORMAT,DEFAULT_PICTURE_FORMAT);
    params->set(CameraProperties::JPEG_QUALITY, 90);

    //must have >2 sizes and contain "0x0"
    params->set(CameraProperties::SUPPORTED_THUMBNAIL_SIZES, "180x160,0x0");
    params->set(CameraProperties::JPEG_THUMBNAIL_SIZE, "180x160");
    params->set(CameraProperties::JPEG_THUMBNAIL_QUALITY, 90);

    //get & set picture size
    memset(sizes,0,1024);
    uint32_t picture_format = DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT;
    CAMHAL_LOGDB("default-picture-format=%d", DEFAULT_IMAGE_CAPTURE_PIXEL_FORMAT);
    if (!v.getValidFrameSize( picture_format, sizes)) {
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
        //char * b = strrchr(sizes, ',');
        //if (b) 
        //    b++;
        //else 
        //    b = sizes;
        params->set(CameraProperties::PICTURE_SIZE, sizes);
    } 
    else
    {
        params->set(CameraProperties::SUPPORTED_PICTURE_SIZES, "640x480");
        params->set(CameraProperties::PICTURE_SIZE,"640x480");
    }
    if(sizes){
        free(sizes);
        sizes = NULL;
    }

    char *focus_mode = (char *) calloc (1, 256);
    char * def_focus_mode = (char *) calloc (1, 64);
    if((focus_mode)&&(def_focus_mode)){
        memset(focus_mode,0,256);
        memset(def_focus_mode,0,64);
        if(v.getCameraAutoFocus( focus_mode,def_focus_mode)) {
            params->set(CameraProperties::SUPPORTED_FOCUS_MODES, focus_mode);
            params->set(CameraProperties::FOCUS_MODE, def_focus_mode);
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
	
        v.getCameraBanding(banding_mode, def_banding_mode);
        params->set(CameraProperties::SUPPORTED_ANTIBANDING, banding_mode);
        params->set(CameraProperties::ANTIBANDING, def_banding_mode);
        CAMHAL_LOGDB("def_banding=%s, banding=%s\n", def_banding_mode, banding_mode);
    }else{
        params->set(CameraProperties::SUPPORTED_ANTIBANDING, "50hz,60hz");
        params->set(CameraProperties::ANTIBANDING, "50hz");
        CAMHAL_LOGDA("banding default value\n");
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

    char *wb_mode = (char *) calloc (1, 256);
    char *def_wb_mode = (char *) calloc (1, 64);


    if( wb_mode && def_wb_mode){
        memset(wb_mode, 0, 256);
	    memset(def_wb_mode, 0, 64);
	    v.getCameraWhiteBalance( wb_mode, def_wb_mode);
	    params->set(CameraProperties::SUPPORTED_WHITE_BALANCE, wb_mode);
	    params->set(CameraProperties::WHITEBALANCE, def_wb_mode);
    }else{

	
	  	params->set(CameraProperties::SUPPORTED_WHITE_BALANCE, "auto");
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

    params->set(CameraProperties::AUTO_WHITEBALANCE_LOCK, DEFAULT_AWB_LOCK);

    params->set(CameraProperties::SUPPORTED_EFFECTS, "none,negative,sepia");
    params->set(CameraProperties::EFFECT, "none");

    char *flash_mode = (char *) calloc (1, 256);
    char *def_flash_mode = (char *) calloc (1, 64);
    if((flash_mode)&&(def_flash_mode)){
        memset(flash_mode,0,256);
        memset(def_flash_mode,0,64);
        if (v.get_flash_mode( flash_mode,def_flash_mode)) {
            params->set(CameraProperties::SUPPORTED_FLASH_MODES, flash_mode);
            params->set(CameraProperties::FLASH_MODE, def_flash_mode);
            CAMHAL_LOGDB("def_flash_mode=%s, flash_mode=%s\n",
                            def_flash_mode, flash_mode);
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
    v.getCameraExposureValue( min, max, step, def);
    params->set(CameraProperties::SUPPORTED_EV_MAX, max);
    params->set(CameraProperties::SUPPORTED_EV_MIN, min);
    params->set(CameraProperties::EV_COMPENSATION, def);
    params->set(CameraProperties::SUPPORTED_EV_STEP, step);

    //don't support digital zoom now

    params->set(CameraProperties::ZOOM_SUPPORTED,"false");
    params->set(CameraProperties::SMOOTH_ZOOM_SUPPORTED,"false");
    params->set(CameraProperties::SUPPORTED_ZOOM_RATIOS,"100");
    params->set(CameraProperties::SUPPORTED_ZOOM_STAGES,0);	//think the zoom ratios as a array, the max zoom is the max index
    params->set(CameraProperties::ZOOM, 0);//default should be 0

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

    params->set(CameraProperties::VIDEO_SIZE, DEFAULT_VIDEO_SIZE);
    params->set(CameraProperties::PREFERRED_PREVIEW_SIZE_FOR_VIDEO, DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO);


    CAMHAL_LOGDA("newloadCaps end!\n");
}

#ifdef AMLOGIC_VCAM_NONBLOCK_SUPPORT
/* gets video device defined frame rate (not real - consider it a maximum value)
 * args:
 *
 * returns: VIDIOC_G_PARM ioctl result value
*/
int V4LCamAdpt::get_framerate ( int camera_fd, int *fps, int *fps_num)
{
	int ret=0;

	struct v4l2_streamparm streamparm;

	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl( camera_fd,VIDIOC_G_PARM,&streamparm);
	if (ret < 0) 
	{
		CAMHAL_LOGDA("VIDIOC_G_PARM - Unable to get timeperframe");
	} 
	else 
	{
		if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
			// it seems numerator is allways 1 but we don't do assumptions here :-)
			*fps = streamparm.parm.capture.timeperframe.denominator;
			*fps_num = streamparm.parm.capture.timeperframe.numerator;
		}
	}

	return ret;
}

int V4LCamAdpt::enumFramerate ( int *fps, int *fps_num)
{
	int ret=0;
	int framerate=0;
	int temp_rate=0;
	struct v4l2_frmivalenum fival;
	int i,j;

	int pixelfmt_tbl[]={
		V4L2_PIX_FMT_NV21,
		V4L2_PIX_FMT_YVU420,
	};
	struct v4l2_frmsize_discrete resolution_tbl[]={
		{1280,720},
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

			while ((ret = ioctl( mCameraHandle, 
                            VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0)
			{
				if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
				{
					temp_rate = fival.discrete.denominator/fival.discrete.numerator;
					if(framerate < temp_rate){
						framerate = temp_rate;
					}
				}
				else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS)
				{
					framerate = fival.stepwise.max.denominator
                                /fival.stepwise.max.numerator;
                    CAMHAL_LOGDB("pixelfmt=%d,resolution:%dx%d,"
							"FRAME TYPE is continuous,step=%d/%d s\n",
                                pixelfmt_tbl[i],
                                resolution_tbl[j].width,
                                resolution_tbl[j].height,
                                fival.stepwise.max.numerator,
                                fival.stepwise.max.denominator);
					break;
				}
				else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE)
				{
					CAMHAL_LOGDB("pixelfmt=%d,resolution:%dx%d,"
							"FRAME TYPE is step wise,step=%d/%d s\n",
                                    pixelfmt_tbl[i],
                                    resolution_tbl[j].width,
                                    resolution_tbl[j].height,
                                    fival.stepwise.step.numerator,
                                    fival.stepwise.step.denominator);
					framerate = fival.stepwise.max.denominator
                                /fival.stepwise.max.numerator;
					break;
				}

				fival.index++;
			}
		}
	}

	*fps = framerate;
	*fps_num = 1;

	CAMHAL_LOGDB("enum framerate=%d\n", framerate);
	if( framerate <= 1){
		return -1;
	}

	return 0;
}
#endif


int V4LCamAdpt::set_white_balance(int camera_fd,const char *swb)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

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

    if(mWhiteBalance == ctl.value){
	return 0;
    }else{
	mWhiteBalance = ctl.value;
    }
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set white balance fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

status_t V4LCamAdpt::getFocusMoveStatus()
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
	if ( 0 > ret ){
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

extern "C" int V4LCamAdpt::SetExposure(int camera_fd,const char *sbn)
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

    ctl.id = V4L2_CID_EXPOSURE;
    ctl.value = level + (mEVmax - mEVmin)/2;

    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set Exposure fail: %s. ret=%d", strerror(errno),ret);
    }

    return ret ;
}

int V4LCamAdpt::set_effect(int camera_fd,const char *sef)
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

    return ret;
}

int V4LCamAdpt::set_night_mode(int camera_fd,const char *snm)
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

    return ret;
}

extern "C" int V4LCamAdpt::set_banding(int camera_fd,const char *snm)
{
    int ret = 0;
    struct v4l2_control ctl;

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
       	CAMHAL_LOGDB("Set banding fail: %s. ret=%d",
                                strerror(errno),ret);
    }
    return ret ;
}

bool V4LCamAdpt::get_flash_mode(char *flash_status,
					char *def_flash_status)
{
    struct v4l2_queryctrl qc;    
    struct v4l2_querymenu qm;
    bool flash_enable = false;
    int ret = NO_ERROR;
    int status_count = 0;

    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_BACKLIGHT_COMPENSATION;
    ret = ioctl (mCameraHandle, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) || (qc.type != V4L2_CTRL_TYPE_MENU)){
        flash_enable = false;
        CAMHAL_LOGDB("can't support flash, %s, ret=%d, %s\n",
                (qc.flags == V4L2_CTRL_FLAG_DISABLED)? "disable":"",
                ret,  qc.type != V4L2_CTRL_TYPE_MENU?"":"type not right");
    }else {
        memset(&qm, 0, sizeof(qm));
        qm.id = V4L2_CID_BACKLIGHT_COMPENSATION;
        qm.index = qc.default_value;
        if(ioctl ( mCameraHandle, VIDIOC_QUERYMENU, &qm) < 0){
            strcpy(def_flash_status, "off");
        } else {
            strcpy(def_flash_status, (char*)qm.name);
        }
        int index = 0;
        for (index = qc.minimum; index <= qc.maximum; index+= qc.step) {
            memset(&qm, 0, sizeof(struct v4l2_querymenu));
            qm.id = V4L2_CID_BACKLIGHT_COMPENSATION;
            qm.index = index;
            if(ioctl (mCameraHandle, VIDIOC_QUERYMENU, &qm) < 0){
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

int V4LCamAdpt::set_flash_mode(int camera_fd, const char *sfm)
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

int V4LCamAdpt::get_hflip_mode(int camera_fd)
{
    struct v4l2_queryctrl qc;    
    int ret = 0;

    if(camera_fd<0){
        CAMHAL_LOGEA("Get_hflip_mode --camera handle is invalid\n");
        return -1;
    }

    memset(&qc, 0, sizeof(qc));
    qc.id = V4L2_CID_HFLIP;
    ret = ioctl (camera_fd, VIDIOC_QUERYCTRL, &qc);
    if((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0) || (qc.type != V4L2_CTRL_TYPE_INTEGER)){
        ret = -1;
        CAMHAL_LOGDB("can't support HFlip! %s ret=%d %s\n",
            (qc.flags == V4L2_CTRL_FLAG_DISABLED)?"disable":"",
            ret, (qc.type != V4L2_CTRL_TYPE_INTEGER)?"":"type not right");
    }else{
        CAMHAL_LOGDB("camera handle %d supports HFlip!\n",camera_fd);
    }
    return ret;
}


int V4LCamAdpt::set_hflip_mode(int camera_fd, bool mode)
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
        CAMHAL_LOGEB("Set hflip mode fail: %s. ret=%d", strerror(errno),ret);
    }
    return ret ;
}

int V4LCamAdpt::get_supported_zoom(int camera_fd, char * zoom_str)
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

int V4LCamAdpt::set_zoom_level(int camera_fd, int zoom)
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
        CAMHAL_LOGEB("Set zoom level fail: %s. ret=%d", strerror(errno),ret);
    }

    return ret ;
}

int V4LCamAdpt::set_rotate_value(int camera_fd, int value)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;

    if((value!=0)&&(value!=90)&&(value!=180)&&(value!=270)){
        CAMHAL_LOGEB("Set rotate value invalid: %d.", value);
        return -1;
    }

    memset( &ctl, 0, sizeof(ctl));

    ctl.value=value;

    ctl.id = V4L2_ROTATE_ID;

    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGEB("Set rotate value fail: %s. ret=%d", strerror(errno),ret);
    }

    return ret ;
}

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

