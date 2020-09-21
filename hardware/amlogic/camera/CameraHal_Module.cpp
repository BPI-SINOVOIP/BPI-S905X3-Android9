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

//#define LOG_NDEBUG 0
#define LOG_TAG "CameraHAL_Module       "

#include <utils/threads.h>

#include "DebugUtils.h"
#include "CameraHal.h"
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
#include "VirtualCamHal.h"
#endif

#include "CameraProperties.h"
#include "ExCameraParameters.h"

static android::CameraProperties gCameraProperties;
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
static android::CameraHal* gCameraHals[MAX_CAM_NUM_ADD_VCAM-1];
static android::VirtualCamHal* gVCameraHals;
#else
static android::CameraHal* gCameraHals[MAX_CAMERAS_SUPPORTED];
#endif
static unsigned int gCamerasOpen = 0;
static unsigned int gCamerasSupported = 0;
static android::Mutex gCameraHalDeviceLock;

static int camera_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device);
static int camera_device_close(hw_device_t* device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);

#ifdef CAMHAL_USER_MODE
volatile int32_t gCamHal_LogLevel = 4;
#else
volatile int32_t gCamHal_LogLevel = 6;
#endif
static void setLogLevel(void *p){ 
        int level = (int) p;
        android_atomic_write(level, &gCamHal_LogLevel);
}

static const char *macro_info[]={
#ifdef CAMHAL_USER_MODE
        "user mode",
#endif
#ifdef AMLOGIC_FRONT_CAMERA_SUPPORT
        "front board camera",
#endif
#ifdef AMLOGIC_BACK_CAMERA_SUPPORT
        "back board camera",
#endif
#ifdef AMLOGIC_USB_CAMERA_SUPPORT
        "usb camera",
#endif
#ifdef AMLOGIC_TWO_CH_UVC
        "usb is two channel",
#endif
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
        "virtual camera enable",
#endif
#ifdef AMLOGIC_CAMERA_NONBLOCK_SUPPORT
        "nonblock mode",
#endif
};


static struct hw_module_methods_t camera_module_methods = {
        open: camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
         tag: HARDWARE_MODULE_TAG,
         version_major: 1,
         version_minor: 0,
         id: CAMERA_HARDWARE_MODULE_ID,
         name: "CameraHal Module",
         author: "Amlogic",
         methods: &camera_module_methods,
         dso: NULL, /* remove compilation warnings */
         reserved: {0}, /* remove compilation warnings */
    },
    get_number_of_cameras: camera_get_number_of_cameras,
    get_camera_info: camera_get_camera_info,
    set_callbacks: NULL,
    get_vendor_tag_ops: NULL,
    open_legacy: NULL,
};

typedef struct aml_camera_device {
    camera_device_t base;
    int cameraid;
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    int type;
#endif
} aml_camera_device_t;

/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/

int camera_set_preview_window(struct camera_device * device,
        struct preview_stream_ops *window)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->setPreviewWindow(window);
    }
#endif

    rv = gCameraHals[aml_dev->cameraid]->setPreviewWindow(window);

    return rv;
}

void camera_set_callbacks(struct camera_device * device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
	return;
    }
#endif
    gCameraHals[aml_dev->cameraid]->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->enableMsgType(msg_type);
        return ;
    }
#endif
    gCameraHals[aml_dev->cameraid]->enableMsgType(msg_type);
}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->disableMsgType(msg_type);
	return;
    }
#endif
    gCameraHals[aml_dev->cameraid]->disableMsgType(msg_type);
}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return 0;

    aml_dev = (aml_camera_device_t*) device;

    return gCameraHals[aml_dev->cameraid]->msgTypeEnabled(msg_type);
}

int camera_start_preview(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
	return gVCameraHals->startPreview();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->startPreview();

    return rv;
}

void camera_stop_preview(struct camera_device * device)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->stopPreview();
	return ;
    }
#endif
    gCameraHals[aml_dev->cameraid]->stopPreview();
}

int camera_preview_enabled(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->previewEnabled();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->previewEnabled();
    return rv;
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

    //  TODO: meta data buffer not current supported
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->storeMetaDataInBuffers(enable);
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->storeMetaDataInBuffers(enable);
    return rv;
    //return enable ? android::INVALID_OPERATION: android::OK;
}

int camera_start_recording(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->startRecording();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->startRecording();
    return rv;
}

void camera_stop_recording(struct camera_device * device)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->stopRecording();
	return;
    }
#endif
    gCameraHals[aml_dev->cameraid]->stopRecording();
}

int camera_recording_enabled(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->recordingEnabled();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->recordingEnabled();
    return rv;
}

void camera_release_recording_frame(struct camera_device * device,
                const void *opaque)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->releaseRecordingFrame(opaque);
	return;
    }
#endif
    gCameraHals[aml_dev->cameraid]->releaseRecordingFrame(opaque);
}

int camera_auto_focus(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->autoFocus();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->autoFocus();
    return rv;
}

int camera_cancel_auto_focus(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->cancelAutoFocus();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->cancelAutoFocus();
    return rv;
}

int camera_take_picture(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->takePicture();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->takePicture();
    return rv;
}

int camera_cancel_picture(struct camera_device * device)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->cancelPicture();
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->cancelPicture();
    return rv;
}

int camera_set_parameters(struct camera_device * device, const char *params)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->setParameters(params);
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->setParameters(params);
    return rv;
}

char* camera_get_parameters(struct camera_device * device)
{
    char* param = NULL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return NULL;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->getParameters();
    }
#endif
    param = gCameraHals[aml_dev->cameraid]->getParameters();

    return param;
}

static void camera_put_parameters(struct camera_device *device, char *parms)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->putParameters(parms);
	return ;
    }
#endif
    gCameraHals[aml_dev->cameraid]->putParameters(parms);
}

int camera_send_command(struct camera_device * device,
            int32_t cmd, int32_t arg1, int32_t arg2)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->sendCommand(cmd, arg1, arg2);
    }
#endif
    rv = gCameraHals[aml_dev->cameraid]->sendCommand(cmd, arg1, arg2);
    return rv;
}

void camera_release(struct camera_device * device)
{
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    if(!device)
        return;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        gVCameraHals->release();
	return ;
    }
#endif
    gCameraHals[aml_dev->cameraid]->release();
}

int camera_dump(struct camera_device * device, int fd)
{
    int rv = -EINVAL;
    aml_camera_device_t* aml_dev = NULL;
    LOG_FUNCTION_NAME;

    if(!device)
        return rv;

    aml_dev = (aml_camera_device_t*) device;

#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        return gVCameraHals->dump(fd);
    }
#endif

    setLogLevel(aml_dev->base.priv);

    rv = gCameraHals[aml_dev->cameraid]->dump(fd);
    return rv;
}

extern "C" void heaptracker_free_leaked_memory(void);

int camera_device_close(hw_device_t* device)
{
    int ret = 0;
    aml_camera_device_t* aml_dev = NULL;

    LOG_FUNCTION_NAME;

    android::Mutex::Autolock lock(gCameraHalDeviceLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    aml_dev = (aml_camera_device_t*) device;

    if (aml_dev) {
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
    if( 1 == aml_dev->type ){
        if (gVCameraHals) {
            delete gVCameraHals;
            gVCameraHals = NULL;
            gCamerasOpen--;
        }
    }else{
        if (gCameraHals[aml_dev->cameraid]) {
            delete gCameraHals[aml_dev->cameraid];
            gCameraHals[aml_dev->cameraid] = NULL;
            gCamerasOpen--;
        }
    }
#else
    if (gCameraHals[aml_dev->cameraid]) {
            delete gCameraHals[aml_dev->cameraid];
            gCameraHals[aml_dev->cameraid] = NULL;
            gCamerasOpen--;
    }
#endif

        if (aml_dev->base.ops) {
            free(aml_dev->base.ops);
        }
        free(aml_dev);
    }
done:
#ifdef HEAPTRACKER
    heaptracker_free_leaked_memory();
#endif
    return ret;
}

/*******************************************************************
 * implementation of camera_module functions
 *******************************************************************/

/* open device handle to one of the cameras
 *
 * assume camera service will keep singleton of each camera
 * so this function will always only be called once per camera instance
 */

int camera_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device)
{
    int rv = 0;
    int num_cameras = 0;
    int cameraid;
    aml_camera_device_t* camera_device = NULL;
    camera_device_ops_t* camera_ops = NULL;
    android::CameraHal* camera = NULL;
    android::CameraProperties::Properties* properties = NULL;

    android::Mutex::Autolock lock(gCameraHalDeviceLock);

    CAMHAL_LOGIA("camera_device open");

    if (name != NULL) {
        cameraid = atoi(name);
        num_cameras = gCameraProperties.camerasSupported();

        if(cameraid > num_cameras)
        {
            CAMHAL_LOGEB("camera service provided cameraid out of bounds,"
                    "cameraid = %d, num supported = %d",
                    cameraid, num_cameras);
            rv = -EINVAL;
            goto fail;
        }

#ifndef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
        if(gCamerasOpen >= MAX_SIMUL_CAMERAS_SUPPORTED)
        {
            CAMHAL_LOGEA("maximum number of cameras already open");
            rv = -ENOMEM;
            goto fail;
        }
#else
        if((gCamerasOpen >= MAX_SIMUL_CAMERAS_SUPPORTED) &&
            (!gVCameraHals) )
        {
            CAMHAL_LOGEA("maximum number of cameras already open");
            rv = -ENOMEM;
            goto fail;
        }

        CAMHAL_LOGDB("cameraid=%d, num_cameras-1=%d\n", cameraid, num_cameras-1);
        CAMHAL_LOGDB("max_add-1=%d\n", gCamerasSupported-1);

    if( cameraid == (gCamerasSupported-1) )
	{
		camera_device = (aml_camera_device_t*)malloc(sizeof(*camera_device));
		if(!camera_device)
		{
		    CAMHAL_LOGEA("camera_device allocation fail");
		    rv = -ENOMEM;
		    goto fail;
		}

		camera_ops = (camera_device_ops_t*)malloc(sizeof(*camera_ops));
		if(!camera_ops)
		{
		    CAMHAL_LOGEA("camera_ops allocation fail");
		    rv = -ENOMEM;
		    goto fail;
		}

		memset(camera_device, 0, sizeof(*camera_device));
		memset(camera_ops, 0, sizeof(*camera_ops));

		camera_device->base.common.tag = HARDWARE_DEVICE_TAG;
		camera_device->base.common.version = 0;
		camera_device->base.common.module = (hw_module_t *)(module);
		camera_device->base.common.close = camera_device_close;
		camera_device->base.ops = camera_ops;

		camera_ops->set_preview_window = camera_set_preview_window;
		camera_ops->set_callbacks = camera_set_callbacks;
		camera_ops->enable_msg_type = camera_enable_msg_type;
		camera_ops->disable_msg_type = camera_disable_msg_type;
		camera_ops->msg_type_enabled = camera_msg_type_enabled;
		camera_ops->start_preview = camera_start_preview;
		camera_ops->stop_preview = camera_stop_preview;
		camera_ops->preview_enabled = camera_preview_enabled;
		camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
		camera_ops->start_recording = camera_start_recording;
		camera_ops->stop_recording = camera_stop_recording;
		camera_ops->recording_enabled = camera_recording_enabled;
		camera_ops->release_recording_frame = camera_release_recording_frame;
		camera_ops->auto_focus = camera_auto_focus;
		camera_ops->cancel_auto_focus = camera_cancel_auto_focus;
		camera_ops->take_picture = camera_take_picture;
		camera_ops->cancel_picture = camera_cancel_picture;
		camera_ops->set_parameters = camera_set_parameters;
		camera_ops->get_parameters = camera_get_parameters;
		camera_ops->put_parameters = camera_put_parameters;
		camera_ops->send_command = camera_send_command;
		camera_ops->release = camera_release;
		camera_ops->dump = camera_dump;

		*device = &camera_device->base.common;

		// -------- vendor specific stuff --------

                CAMHAL_LOGDB("virtual num_cameras=%d cameraid=%d", num_cameras, cameraid);
		camera_device->cameraid = cameraid;
		camera_device->type = 1;

		if(gCameraProperties.getProperties(cameraid, &properties) < 0)
		{
		    CAMHAL_LOGEA("Couldn't get virtual camera properties");
		    rv = -ENOMEM;
		    goto fail;
		}

		gVCameraHals = new android::VirtualCamHal(cameraid);
        CAMHAL_LOGDA("Virtual CameraHal\n");

		if(!gVCameraHals)
		{
		    CAMHAL_LOGEA("Couldn't create instance of VirtualCameraHal class");
		    rv = -ENOMEM;
		    goto fail;
		}

		if(properties && (gVCameraHals->initialize(properties) != android::NO_ERROR))
		{
		    CAMHAL_LOGEA("Couldn't initialize virtual camera instance");
		    rv = -ENODEV;
		    goto fail;
		}

		gCamerasOpen++;
		
		return rv;
	}
#endif

        camera_device = (aml_camera_device_t*)malloc(sizeof(*camera_device));
        if(!camera_device)
        {
            CAMHAL_LOGEA("camera_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        camera_ops = (camera_device_ops_t*)malloc(sizeof(*camera_ops));
        if(!camera_ops)
        {
            CAMHAL_LOGEA("camera_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(camera_device, 0, sizeof(*camera_device));
        memset(camera_ops, 0, sizeof(*camera_ops));

        camera_device->base.common.tag = HARDWARE_DEVICE_TAG;
        camera_device->base.common.version = 0;
        camera_device->base.common.module = (hw_module_t *)(module);
        camera_device->base.common.close = camera_device_close;
        camera_device->base.ops = camera_ops;

        camera_ops->set_preview_window = camera_set_preview_window;
        camera_ops->set_callbacks = camera_set_callbacks;
        camera_ops->enable_msg_type = camera_enable_msg_type;
        camera_ops->disable_msg_type = camera_disable_msg_type;
        camera_ops->msg_type_enabled = camera_msg_type_enabled;
        camera_ops->start_preview = camera_start_preview;
        camera_ops->stop_preview = camera_stop_preview;
        camera_ops->preview_enabled = camera_preview_enabled;
        camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
        camera_ops->start_recording = camera_start_recording;
        camera_ops->stop_recording = camera_stop_recording;
        camera_ops->recording_enabled = camera_recording_enabled;
        camera_ops->release_recording_frame = camera_release_recording_frame;
        camera_ops->auto_focus = camera_auto_focus;
        camera_ops->cancel_auto_focus = camera_cancel_auto_focus;
        camera_ops->take_picture = camera_take_picture;
        camera_ops->cancel_picture = camera_cancel_picture;
        camera_ops->set_parameters = camera_set_parameters;
        camera_ops->get_parameters = camera_get_parameters;
        camera_ops->put_parameters = camera_put_parameters;
        camera_ops->send_command = camera_send_command;
        camera_ops->release = camera_release;
        camera_ops->dump = camera_dump;

        *device = &camera_device->base.common;

        // -------- vendor specific stuff --------

        CAMHAL_LOGDB("num_cameras=%d cameraid=%d", num_cameras, cameraid);
        camera_device->cameraid = cameraid;
#ifdef AMLOGIC_VIRTUAL_CAMERA_SUPPORT
	camera_device->type = 0;
#endif

        if(gCameraProperties.getProperties(cameraid, &properties) < 0)
        {
            CAMHAL_LOGEA("Couldn't get camera properties");
            rv = -ENOMEM;
            goto fail;
        }

        camera = new android::CameraHal(cameraid);

        if(!camera)
        {
            CAMHAL_LOGEA("Couldn't create instance of CameraHal class");
            rv = -ENOMEM;
            goto fail;
        }

        if(properties && (camera->initialize(properties) != android::NO_ERROR))
        {
            CAMHAL_LOGEA("Couldn't initialize camera instance");
            rv = -ENODEV;
            goto fail;
        }

        gCameraHals[cameraid] = camera;
        gCamerasOpen++;
    }

    return rv;

fail:
    if(camera_device) {
        free(camera_device);
        camera_device = NULL;
    }
    if(camera_ops) {
        free(camera_ops);
        camera_ops = NULL;
    }
    if(camera) {
        delete camera;
        camera = NULL;
    }
    *device = NULL;
    return rv;
}

extern "C"  int CameraAdapter_CameraNum();
int camera_get_number_of_cameras(void)
{
    int num_cameras = CameraAdapter_CameraNum();
    gCamerasSupported = num_cameras;
    CAMHAL_LOGDB("gCamerasSupported=%d,num_cameras=%d\n",
					gCamerasSupported, num_cameras);

#ifdef HAVE_VERSION_INFO
    CAMHAL_LOGIB("\n--------------------------------\n" 
                  "author:aml.sh multi-media team\n"
                  "branch name:   %s\n"
                  "git version:   %s \n"
                  "last changed:  %s\n"
                  "build-time:    %s\n"
                  "build-name:    %s\n"
                  "uncommitted-file-num:%d\n"
                  "ssh user@%s, cd %s\n"
                  "hostname %s\n"
                  "--------------------------------\n",
                  CAMHAL_BRANCH_NAME,
                  CAMHAL_GIT_VERSION,
                  CAMHAL_LAST_CHANGED,
                  CAMHAL_BUILD_TIME,
                  CAMHAL_BUILD_NAME,
                  CAMHAL_GIT_UNCOMMIT_FILE_NUM,
                  CAMHAL_IP, CAMHAL_PATH, CAMHAL_HOSTNAME
                  );
#endif
    for(unsigned i = 0;i<sizeof(macro_info)/sizeof(macro_info[0]) ;i++){
        CAMHAL_LOGIB("%s\n", macro_info[i]);
    }
    return num_cameras;
}

int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    int rv = 0;
    int face_value = CAMERA_FACING_BACK;
    int orientation = 0;
    const char *valstr = NULL;
    android::CameraProperties::Properties* properties = NULL;

    CAMHAL_LOGDB("camera_get_camera_info camera_id=%d", camera_id);
    // this going to be the first call from camera service
    // initialize camera properties here...
    if( ( gCamerasOpen == 0 ) 
        && (gCameraProperties.initialize(camera_id) != android::NO_ERROR))
    {
        CAMHAL_LOGEA("Unable to create or initialize CameraProperties");
        return -EINVAL;
    }

    //Get camera properties for camera index
    if(gCameraProperties.getProperties(camera_id, &properties) < 0)
    {
        CAMHAL_LOGEA("Couldn't get camera properties");
        rv = -EINVAL;
        goto end;
    }

    if(properties)
    {
        valstr = properties->get(android::CameraProperties::FACING_INDEX);
        if(valstr != NULL)
        {
            if (strcmp(valstr, (const char *) android::ExCameraParameters::FACING_FRONT) == 0)
            {
                face_value = CAMERA_FACING_FRONT;
            }
            else if (strcmp(valstr, (const char *) android::ExCameraParameters::FACING_BACK) == 0)
            {
                face_value = CAMERA_FACING_BACK;
            }
         }

         valstr = properties->get(android::CameraProperties::ORIENTATION_INDEX);
         if(valstr != NULL)
         {
             orientation = atoi(valstr);
         }
    }
    else
    {
        CAMHAL_LOGEB("getProperties() returned a NULL property set for Camera id %d", camera_id);
    }

    info->facing = face_value;
    info->orientation = orientation;

end:
    return rv;
}





