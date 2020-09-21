/* Copyright (C) 2011 The Android Open Source Project
 *
 * Original code licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This implements a lights hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/lights.goldfish.so
 *
 * It will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from
 * ./frameworks/base/services/jni/com_android_server_HardwareService.cpp
 */

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "Lights"
#endif
#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>
#include <hardware/hardware.h>

/* Set to 1 to enable debug messages to the log */
#define DEBUG 0
#if DEBUG
# define D(...) ALOGD(__VA_ARGS__)
#else
# define D(...) do{}while(0)
#endif

#define  E(...)  ALOGE(__VA_ARGS__)
#define BACKLIGHT "/sys/class/backlight/aml-bl/brightness"

/* set backlight brightness by LIGHTS_SERVICE_NAME service. */
static int
set_light_backlight( struct light_device_t* dev, struct light_state_t const* state )

{
    int nwr, ret = -1, fd;
    char value[20];
    int light_level;
    light_level =state->color&0xff;
    
    fd = open(BACKLIGHT, O_RDWR);
    if (fd > 0) {
        nwr = sprintf(value, "%d\n", light_level);
        ret = write(fd, value, nwr);
        close(fd);
    }

    return ret;
}

static int
set_light_buttons( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    D( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}
static int
set_light_battery( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    D( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}
static int
set_light_keyboard( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    D( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}
static int
set_light_notifications( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    D( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}
static int
set_light_attention( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    D( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}
/** Close the lights device */
static int
close_lights( struct light_device_t *dev )
{
    free( dev );

    return 0;
}
/**
 * module methods
 */
/** Open a new instance of a lights device using name */
static int
open_lights( const struct hw_module_t* module, char const *name,
        struct hw_device_t **device )
{
    void* set_light;

    if (0 == strcmp( LIGHT_ID_BACKLIGHT, name )) {
        set_light = set_light_backlight;
    } else if (0 == strcmp( LIGHT_ID_KEYBOARD, name )) {
        set_light = set_light_keyboard;
    } else if (0 == strcmp( LIGHT_ID_BUTTONS, name )) {
        set_light = set_light_buttons;
    } else if (0 == strcmp( LIGHT_ID_BATTERY, name )) {
        set_light = set_light_battery;
    } else if (0 == strcmp( LIGHT_ID_NOTIFICATIONS, name )) {
        set_light = set_light_notifications;
    } else if (0 == strcmp( LIGHT_ID_ATTENTION, name )) {
        set_light = set_light_attention;
    } else {
        D( "%s: %s light isn't supported yet.", __FUNCTION__, name );
        return -EINVAL;
    }

    struct light_device_t *dev = malloc( sizeof(struct light_device_t) );
    if (dev == NULL) {
        return -EINVAL;
    }
    memset( dev, 0, sizeof(*dev) );

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The emulator lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Amlogic lights Module",
    .author = "Amlogic",
    .methods = &lights_module_methods,
};
