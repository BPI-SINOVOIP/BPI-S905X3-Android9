/*
 * Copyright (C) 2013 The Android Open Source Project
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
#define LOG_TAG "screen_source"
#include <hardware/hardware.h>
#include <aml_screen.h>

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


#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include "v4l2_vdin.h"

#ifndef LOGD
#define LOGD ALOGD
#endif
#ifndef LOGV
#define LOGV ALOGV
#endif
#ifndef LOGE
#define LOGE ALOGE
#endif
#ifndef LOGI
#define LOGI ALOGI
#endif

#define MAX_DEVICES_SUPPORTED    2

static unsigned int gAmlScreenOpen = 0;
static android::Mutex gAmlScreenLock;
static android::vdin_screen_source* gScreenHals[MAX_DEVICES_SUPPORTED];

/*****************************************************************************/

static int aml_screen_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t aml_screen_module_methods = {
    .open = aml_screen_device_open
};

aml_screen_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = AML_SCREEN_HARDWARE_MODULE_ID,
        .name = "aml screen source module",
        .author = "Amlogic",
        .methods = &aml_screen_module_methods,
        .dso = NULL,
        .reserved = {0},
    }
};

/*****************************************************************************/

static int aml_screen_device_close(struct hw_device_t *dev)
{
    //android::vdin_screen_source* source = NULL;
    aml_screen_device_t* ctx = (aml_screen_device_t*)dev;
    android::Mutex::Autolock lock(gAmlScreenLock);
    if (ctx) {
        if (gScreenHals[ctx->device_id]) {
            delete gScreenHals[ctx->device_id];
            gScreenHals[ctx->device_id] = NULL;
            gAmlScreenOpen--;
        }
        free(ctx);
        ctx = NULL;
    }
    return 0;
}

int screen_source_start(struct aml_screen_device* dev)
{
    return gScreenHals[dev->device_id]->start();
}

int screen_source_stop(struct aml_screen_device* dev)
{
    return gScreenHals[dev->device_id]->stop();
}

int screen_source_pause(struct aml_screen_device* dev)
{
    return gScreenHals[dev->device_id]->pause();
}

int screen_source_get_format(struct aml_screen_device* dev)
{
    return gScreenHals[dev->device_id]->get_format();
}

int screen_source_set_format(struct aml_screen_device* dev, int width, int height, int pix_format)
{
    if ((width > 0) && (height > 0) && ((pix_format == V4L2_PIX_FMT_NV21) ||
            (pix_format == V4L2_PIX_FMT_YUV420) ||
            (pix_format == V4L2_PIX_FMT_RGB24) ||
            (pix_format == V4L2_PIX_FMT_RGB565X) ||
            (pix_format == V4L2_PIX_FMT_RGB32) ||
            (pix_format == V4L2_PIX_FMT_NV12))) {
                return gScreenHals[dev->device_id]->set_format(width, height, pix_format);
    } else {
        return gScreenHals[dev->device_id]->set_format();
    }
}

int screen_source_set_rotation(struct aml_screen_device* dev, int degree)
{
    return gScreenHals[dev->device_id]->set_rotation(degree);
}

int screen_source_set_crop(struct aml_screen_device* dev, int x, int y, int width, int height)
{
    if ((x >= 0) && (y >= 0) && (width > 0) && (height > 0))
        return gScreenHals[dev->device_id]->set_crop(x, y, width, height);

    return android::BAD_VALUE;
}

int screen_source_get_amlvideo2_crop(struct aml_screen_device* dev, int *x, int *y, int *width, int *height)
{
    if ((x != NULL) && (y != NULL) && (width != NULL) && (height != NULL))
        return gScreenHals[dev->device_id]->get_amlvideo2_crop(x, y, width, height);

    return android::BAD_VALUE;
}

int screen_source_set_amlvideo2_crop(struct aml_screen_device* dev, int x, int y, int width, int height)
{
    if ((x >= 0) && (y >= 0) && (width > 0) && (height > 0))
        return gScreenHals[dev->device_id]->set_amlvideo2_crop(x, y, width, height);

    return android::BAD_VALUE;
}

int screen_source_aquire_buffer(struct aml_screen_device* dev, aml_screen_buffer_info_t* buff_info)
{
    return gScreenHals[dev->device_id]->aquire_buffer(buff_info);
}

int screen_source_release_buffer(struct aml_screen_device* dev, long* ptr)
{
      return gScreenHals[dev->device_id]->release_buffer(ptr);
}

int screen_source_set_state_callback(struct aml_screen_device* dev, olStateCB callback)
{
       return gScreenHals[dev->device_id]->set_state_callback(callback);
}
/*
int screen_source_set_preview_window(struct aml_screen_device* dev, ANativeWindow* window)
{
     return gScreenHals[dev->device_id]->set_preview_window(window);
}
*/
int screen_source_set_data_callback(struct aml_screen_device* dev, app_data_callback callback, void* user)
{
      return gScreenHals[dev->device_id]->set_data_callback(callback, user);
}

int screen_source_set_frame_rate(struct aml_screen_device* dev, int frameRate)
{
       return gScreenHals[dev->device_id]->set_frame_rate(frameRate);
}

int screen_source_get_current_sourcesize(struct aml_screen_device* dev, int *w, int *h)
{
     return gScreenHals[dev->device_id]->get_current_sourcesize(w, h);
}

int screen_source_set_screen_mode(struct aml_screen_device* dev, int  mode)
{
     return gScreenHals[dev->device_id]->set_screen_mode(mode);
}

int screen_source_start_v4l2_device(struct aml_screen_device* dev)
{
     return gScreenHals[dev->device_id]->start_v4l2_device();
}

int screen_source_stop_v4l2_device(struct aml_screen_device* dev)
{
     return gScreenHals[dev->device_id]->stop_v4l2_device();
}

int screen_source_get_port_type(struct aml_screen_device* dev)
{
     return gScreenHals[dev->device_id]->get_port_type();
}
/**
 * set_port_type() parameter description:
 portType is consisted by 32-bit binary.
 bit 28 : start tvin service flag, 1 : enable,	0 : disable.
 bit 24 : vdin device num : 0 or 1, which means use vdin0 or vdin1.
 bit 15~0 : tvin port type --TVIN_PORT_VIU,TVIN_PORT_HDMI0...
               (port type define in tvin.h)
 */
int screen_source_set_port_type(struct aml_screen_device* dev,unsigned int portType)
{
     return gScreenHals[dev->device_id]->set_port_type(portType);
}

int screen_source_set_mode(struct aml_screen_device* dev, int displayMode)
{
       return gScreenHals[dev->device_id]->set_mode(displayMode);
}

/* int screen_source_inc_buffer_refcount(struct aml_screen_device* dev, int* ptr)
{
     android::vdin_screen_source* source = (android::vdin_screen_source*)dev->priv;
    return source->inc_buffer_refcount(ptr);
} */

/*****************************************************************************/

static int aml_screen_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int deviceid;
    int status = -EINVAL;
    android::vdin_screen_source* source = NULL;
    android::Mutex::Autolock lock(gAmlScreenLock);

    LOGV("aml_screen_device_open");

    if (name != NULL) {
        if (gAmlScreenOpen >=  MAX_DEVICES_SUPPORTED) {
            ALOGD("aml screen device already open");
            *device = NULL;
            return -EINVAL;
        }

        deviceid = atoi(name);

        if (deviceid >=  MAX_DEVICES_SUPPORTED) {
            ALOGD("provided device id out of bounds , deviceid = %d .\n" , deviceid);
            *device = NULL;
            return -EINVAL;
        }

        aml_screen_device_t *dev = (aml_screen_device_t*)malloc(sizeof(aml_screen_device_t));

        if (!dev) {
            ALOGE("no memory for the screen source device");
            return -ENOMEM;
        }
        /* initialize handle here */
        memset(dev, 0, sizeof(*dev));

        source = new android::vdin_screen_source;
        if (!source) {
            ALOGE("no memory for class of vdin_screen_source");
            free (dev);
            return -ENOMEM;
        }

        if (source->init(deviceid)!= 0) {
            ALOGE("open vdin_screen_source failed!");
            free (dev);
            delete source;
            return -1;
        }

        dev->priv = (void*)source;

        /* initialize the procs */
        dev->common.tag  = HARDWARE_DEVICE_TAG;
        dev->common.version = 0;
        dev->common.module = const_cast<hw_module_t*>(module);
        dev->common.close = aml_screen_device_close;
        dev->ops.start = screen_source_start;
        dev->ops.stop = screen_source_stop;
        dev->ops.pause = screen_source_pause;
        dev->ops.get_format = screen_source_get_format;
        dev->ops.set_format = screen_source_set_format;
        dev->ops.set_rotation = screen_source_set_rotation;
        dev->ops.set_crop = screen_source_set_crop;
        dev->ops.get_amlvideo2_crop = screen_source_get_amlvideo2_crop;
        dev->ops.set_amlvideo2_crop = screen_source_set_amlvideo2_crop;
        dev->ops.aquire_buffer = screen_source_aquire_buffer;
        dev->ops.release_buffer = screen_source_release_buffer;
        dev->ops.setStateCallBack = screen_source_set_state_callback;
        //dev->ops.setPreviewWindow = screen_source_set_preview_window;
        dev->ops.setDataCallBack = screen_source_set_data_callback;
        dev->ops.set_frame_rate = screen_source_set_frame_rate;
        dev->ops.get_current_sourcesize = screen_source_get_current_sourcesize;
        dev->ops.set_screen_mode = screen_source_set_screen_mode;
        // dev->ops.inc_buffer_refcount = screen_source_inc_buffer_refcount;
        dev->ops.start_v4l2_device = screen_source_start_v4l2_device;
        dev->ops.stop_v4l2_device = screen_source_stop_v4l2_device;
        dev->ops.get_port_type = screen_source_get_port_type;
        dev->ops.set_port_type = screen_source_set_port_type;
        dev->ops.set_mode = screen_source_set_mode;
        dev->device_id = deviceid;
        *device = &dev->common;
        gScreenHals[deviceid] = source;
        gAmlScreenOpen++;
        status = 0;

    }
    return status;
}
