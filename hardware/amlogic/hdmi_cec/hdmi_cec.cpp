/*
 * Copyright (C) 2014 The Android Open Source Project
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
/*
 *     Amlogic HDMITX CEC HAL
 *       Copyright (C) 2014
 *
 * This implements a hdmi cec hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/hdmi_cec.so
 *
 * It will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from
 * frameworks/base/services/core/jni/com_android_server_hdmi_HdmiCecController.cpp
 */

#include <cutils/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/hdmi_cec.h>
#include <hardware/hardware.h>
#include <cutils/properties.h>
#include "hdmi_cec.h"
#include <jni.h>
#include <JNIHelp.h>

//#include <HdmiCecClient.h>
#include <HdmiCecHidlClient.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CEC"
#else
#define LOG_TAG "CEC"
#endif

/* Set to 1 to enable debug messages to the log */
#define DEBUG 1
#if DEBUG
# define D(format, args...) ALOGD("[%s]" format, __FUNCTION__, ##args)
#else
# define D(...) do{}while(0)
#endif

#define  E(format, args...) ALOGE("[%s]" format, __FUNCTION__, ##args)

using namespace android;

typedef struct aml_cec_hal {
    hdmi_cec_device_t          device;
    //sp<HdmiCecClient>          client;
    HdmiCecHidlClient          *hidlClient;
    void                       *cb_data;
    event_callback_t           cb;
    int                        fd;
} aml_cec_hal_t;

struct aml_cec_hal *hal_info = NULL;

class HdmiCecCallback : public HdmiCecEventListener {
public:
    HdmiCecCallback(){}
    ~HdmiCecCallback(){}
    virtual void onEventUpdate(const hdmi_cec_event_t* event);
};

void HdmiCecCallback::onEventUpdate(const hdmi_cec_event_t* cecEvent)
{
    if (cecEvent == NULL)
        return;

    int type = cecEvent->eventType;

    if ((cecEvent->eventType & HDMI_EVENT_CEC_MESSAGE) != 0 && hal_info->cb) {
        D("send cec message, event type = %d", type);
        hdmi_event_t event;
        event.type = HDMI_EVENT_CEC_MESSAGE;
        event.dev = &hal_info->device;
        event.cec.initiator = cecEvent->cec.initiator;
        event.cec.destination = cecEvent->cec.destination;
        event.cec.length = cecEvent->cec.length;
        memcpy(event.cec.body, cecEvent->cec.body, event.cec.length);
        hal_info->cb(&event, hal_info->cb_data);
    } else if ((cecEvent->eventType & HDMI_EVENT_HOT_PLUG) != 0 && hal_info->cb) {
        D("cec hot plug, event type = %d", type);
        hdmi_event_t event;
        event.type = HDMI_EVENT_HOT_PLUG;
        event.dev = &hal_info->device;
        event.hotplug.connected = cecEvent->hotplug.connected;
        event.hotplug.port_id = cecEvent->hotplug.port_id;
        hal_info->cb(&event, hal_info->cb_data);
    }
}

/*
 * (*add_logical_address)() passes the logical address that will be used
 * in this system.
 *
 * HAL may use it to configure the hardware so that the CEC commands addressed
 * the given logical address can be filtered in. This method can be called
 * as many times as necessary in order to support multiple logical devices.
 * addr should be in the range of valid logical addresses for the call
 * to succeed.
 *
 * Returns 0 on success or -errno on error.
 */
static int cec_add_logical_address(const struct hdmi_cec_device* dev, cec_logical_address_t addr)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //return priv->client->addLogicalAddress(addr);
    return priv->hidlClient->addLogicalAddress(addr);
}

/*
 * (*clear_logical_address)() tells HAL to reset all the logical addresses.
 *
 * It is used when the system doesn't need to process CEC command any more,
 * hence to tell HAL to stop receiving commands from the CEC bus, and change
 * the state back to the beginning.
 */
static void cec_clear_logical_address(const struct hdmi_cec_device* dev)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //priv->client->clearLogicaladdress();
    priv->hidlClient->clearLogicaladdress();
}

/*
 * (*get_physical_address)() returns the CEC physical address. The
 * address is written to addr.
 *
 * The physical address depends on the topology of the network formed
 * by connected HDMI devices. It is therefore likely to change if the cable
 * is plugged off and on again. It is advised to call get_physical_address
 * to get the updated address when hot plug event takes place.
 *
 * Returns 0 on success or -errno on error.
 */
static int cec_get_physical_address(const struct hdmi_cec_device* dev, uint16_t* addr)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //return priv->client->getPhysicalAddress(addr);
    return priv->hidlClient->getPhysicalAddress(addr);
}

/*
 * (*send_message)() transmits HDMI-CEC message to other HDMI device.
 *
 * The method should be designed to return in a certain amount of time not
 * hanging forever, which can happen if CEC signal line is pulled low for
 * some reason. HAL implementation should take the situation into account
 * so as not to wait forever for the message to get sent out.
 *
 * It should try retransmission at least once as specified in the standard.
 *
 * Returns error code. See HDMI_RESULT_SUCCESS, HDMI_RESULT_NACK, and
 * HDMI_RESULT_BUSY.
 */
static int cec_send_message(const struct hdmi_cec_device* dev, const cec_message_t* msg)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //return priv->client->sendMessage(msg);
    return priv->hidlClient->sendMessage(msg);
}

/*
 * (*register_event_callback)() registers a callback that HDMI-CEC HAL
 * can later use for incoming CEC messages or internal HDMI events.
 * When calling from C++, use the argument arg to pass the calling object.
 * It will be passed back when the callback is invoked so that the context
 * can be retrieved.
 */
static void cec_register_event_callback(const struct hdmi_cec_device* dev __unused,
        event_callback_t callback, void* arg)
{
    if (!hal_info || hal_info->fd < 0)
        return;

    hal_info->cb      = callback;
    hal_info->cb_data = arg;
}

/*
 * (*get_version)() returns the CEC version supported by underlying hardware.
 */
static void cec_get_version(const struct hdmi_cec_device* dev, int* version)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //priv->client->getVersion(version);
    priv->hidlClient->getVersion(version);
}

/*
 * (*get_vendor_id)() returns the identifier of the vendor. It is
 * the 24-bit unique company ID obtained from the IEEE Registration
 * Authority Committee (RAC).
 */
static void cec_get_vendor_id(const struct hdmi_cec_device* dev, uint32_t* vendor_id)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //priv->client->getVendorId(vendor_id);
    priv->hidlClient->getVendorId(vendor_id);
}

/*
 * (*get_port_info)() returns the hdmi port information of underlying hardware.
 * info is the list of HDMI port information, and 'total' is the number of
 * HDMI ports in the system.
 */
static void cec_get_port_info(const struct hdmi_cec_device* dev,
        struct hdmi_port_info* list[], int* total)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //priv->client->getPortInfos(list, total);
    priv->hidlClient->getPortInfos(list, total);
}

/*
 * (*set_option)() passes flags controlling the way HDMI-CEC service works down
 * to HAL implementation. Those flags will be used in case the feature needs
 * update in HAL itself, firmware or microcontroller.
 */
static void cec_set_option(const struct hdmi_cec_device* dev, int flag, int value)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //priv->client->setOption(flag, value);
    priv->hidlClient->setOption(flag, value);
}


/*
 * (*set_audio_return_channel)() configures ARC circuit in the hardware logic
 * to start or stop the feature. Flag can be either 1 to start the feature
 * or 0 to stop it.
 *
 * Returns 0 on success or -errno on error.
 */
static void cec_set_audio_return_channel(const struct hdmi_cec_device* dev, int port_id, int flag)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //priv->client->setAudioReturnChannel(port_id, flag);
    priv->hidlClient->setAudioReturnChannel(port_id, flag);
}

/*
 * (*is_connected)() returns the connection status of the specified port.
 * Returns HDMI_CONNECTED if a device is connected, otherwise HDMI_NOT_CONNECTED.
 * The HAL should watch for +5V power signal to determine the status.
 */
static int cec_is_connected(const struct hdmi_cec_device* dev, int port_id)
{
    aml_cec_hal_t *priv = (aml_cec_hal_t *)dev;
    //return priv->client->isConnected(port_id);
    return priv->hidlClient->isConnected(port_id);
}

/** Close the hdmi cec device */
static int cec_close(struct hw_device_t *dev)
{
     if (hal_info != NULL) {
        //return hal_info->client->closeCecDevice();
        return hal_info->hidlClient->closeCecDevice();
     }
     free(dev);
    return -1;
}

/**
 * module methods
 */
static int open_cec( const struct hw_module_t* module, char const *name,
        struct hw_device_t **device )
{

    if (strcmp(name, HDMI_CEC_HARDWARE_INTERFACE) != 0) {
        D("cec strcmp fail !!!");
        return -EINVAL;
    }

    if (device == NULL) {
        D("NULL cec device on open");
        return -EINVAL;
    }

    aml_cec_hal_t *dev = (aml_cec_hal_t*)malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));
    //dev->client = HdmiCecClient::connect();
    //dev->client->setEventObserver(new HdmiCecCallback());
    //dev->fd = dev->client->openCecDevice();
    dev->hidlClient = HdmiCecHidlClient::connect(CONNECT_TYPE_HAL);
    dev->hidlClient->setEventObserver(new HdmiCecCallback());
    dev->fd = dev->hidlClient->openCecDevice();

    if (dev->fd < 0) {
        D("can't open CEC Device!!");
        free(dev);
        return -EINVAL;
    }

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = 0;
    dev->device.common.module = (struct hw_module_t*) module;
    dev->device.common.close = cec_close;

    dev->device.add_logical_address      = cec_add_logical_address;
    dev->device.clear_logical_address    = cec_clear_logical_address;
    dev->device.get_physical_address     = cec_get_physical_address;
    dev->device.send_message             = cec_send_message;
    dev->device.register_event_callback  = cec_register_event_callback;
    dev->device.get_version              = cec_get_version;
    dev->device.get_vendor_id            = cec_get_vendor_id;
    dev->device.get_port_info            = cec_get_port_info;
    dev->device.set_option               = cec_set_option;
    dev->device.set_audio_return_channel = cec_set_audio_return_channel;
    dev->device.is_connected             = cec_is_connected;

    *device = &dev->device.common;

    hal_info = dev;
    return 0;
}

static struct hw_module_methods_t hdmi_cec_module_methods = {
    .open =  open_cec,
};

/*
 * The hdmi cec Module
 */
struct hdmi_cec_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = HDMI_CEC_MODULE_API_VERSION_1_0,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = HDMI_CEC_HARDWARE_MODULE_ID,
        .name               = "Amlogic hdmi cec Module",
        .author             = "Amlogic Corp.",
        .methods            = &hdmi_cec_module_methods,
    },
};

