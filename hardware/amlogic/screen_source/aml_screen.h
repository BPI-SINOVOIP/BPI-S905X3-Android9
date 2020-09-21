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

#ifndef ANDROID_INCLUDE_HARDWARE_AML_SCREEN_H
#define ANDROID_INCLUDE_HARDWARE_AML_SCREEN_H

#include <hardware/hardware.h>
//#include <android/native_window.h>


__BEGIN_DECLS

/*****************************************************************************/

/**
 * The id of this module
 */

#define AML_SCREEN_HARDWARE_MODULE_ID  "screen_source"
#define AML_SCREEN_SOURCE              "screen_source"


/*****************************************************************************/

typedef struct aml_screen_buffer_info {
    long * buffer_mem;
    int    buffer_canvas;
    long   tv_sec;
    long   tv_usec;
} aml_screen_buffer_info_t;

typedef void (*olStateCB)(int state);

typedef void (*app_data_callback)(void* user,
        aml_screen_buffer_info_t *buff_info);

struct aml_screen_device;

typedef struct aml_screen_module {
    struct hw_module_t common;
} aml_screen_module_t;

enum SOURCETYPE{
    WIFI_DISPLAY,
    HDMI_IN,
};

enum aml_screen_mode_e {
    AML_SCREEN_MODE_RATIO = 0,
    AML_SCREEN_MODE_FULL,
    AML_SCREEN_MODE_ADAPTIVE,
    AML_SCREEN_MODE_MAX
};

/**
 * set_port_type() parameter description:
 portType is consisted by 32-bit binary.
 bit 28 : start tvin service flag, 1 : enable,  0 : disable.
 bit 24 : vdin device num : 0 or 1, which means use vdin0 or vdin1.
 bit 15~0 : tvin port type --TVIN_PORT_VIU,TVIN_PORT_HDMI0...
                (port type define in tvin.h)
 */
typedef struct aml_screen_operations {
    int (*start)(struct aml_screen_device*);
    int (*stop)(struct aml_screen_device*);
    int (*pause)(struct aml_screen_device*);
    int (*setStateCallBack)(struct aml_screen_device*, olStateCB);
    //int (*setPreviewWindow)(struct aml_screen_device*, ANativeWindow*);
    int (*setDataCallBack)(struct aml_screen_device*,app_data_callback, void*);
    int (*get_format)(struct aml_screen_device*);
    int (*set_format)(struct aml_screen_device*, int, int, int);
    int (*set_rotation)(struct aml_screen_device*, int);
    int (*set_crop)(struct aml_screen_device*, int, int, int, int);
    int (*get_amlvideo2_crop)(struct aml_screen_device*, int *, int *, int *, int *);
    int (*set_amlvideo2_crop)(struct aml_screen_device*, int, int, int, int);
    int (*aquire_buffer)(struct aml_screen_device*, aml_screen_buffer_info_t*);
    // int (*set_buffer_refcount)(struct aml_screen_device, int*, int);
    int (*release_buffer)(struct aml_screen_device*, long*);
    // int (*inc_buffer_refcount)(struct aml_screen_device*, int*);
    int (*set_frame_rate)(struct aml_screen_device*, int);
    int (*get_current_sourcesize)(struct aml_screen_device*, int *, int *);
    int (*set_screen_mode)(struct aml_screen_device*, int);
    int (*start_v4l2_device)(struct aml_screen_device*);
    int (*stop_v4l2_device)(struct aml_screen_device*);
    int (*get_port_type)(struct aml_screen_device*);
    int (*set_port_type)(struct aml_screen_device*, unsigned int);
    int (*set_mode)(struct aml_screen_device*, int);
} aml_screen_operations_t;

typedef struct aml_screen_device {
    hw_device_t common;
    aml_screen_operations_t ops;
    int device_id;
    void* priv;
} aml_screen_device_t;

/*****************************************************************************/

__END_DECLS

#endif
