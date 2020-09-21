/*
 * Copyright 2014 The Android Open Source Project
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

#define LOG_TAG "TvInput"
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/native_handle.h>

#include <hardware/tv_input.h>
#include "tv_input.h"
#include <tvcmd.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBuffer.h>
#include <gralloc_priv.h>
#include <gralloc_helper.h>

#if PLATFORM_SDK_VERSION > 23
#include <gralloc_usage_ext.h>
#endif

#include <hardware/hardware.h>
#include <linux/videodev2.h>
#include <android/native_window.h>

static const int SCREENSOURCE_GRALLOC_USAGE = (
    GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER |
    GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_NEVER);

static int capWidth;
static int capHeight;

struct sideband_handle_t *pTvStream = nullptr;

void EventCallback::onTvEvent (const source_connect_t &scrConnect) {
    tv_input_private_t *priv = (tv_input_private_t *)(mPri);

    ALOGI("callback::onTvEvent msgType = %d", scrConnect.msgType);
    switch (scrConnect.msgType) {
        case SOURCE_CONNECT_CALLBACK: {
            tv_source_input_t source = (tv_source_input_t)scrConnect.source;
            int connectState = scrConnect.state;
            ALOGI("callback::onTvEvent source = %d, status = %d", source, connectState);
            if (SOURCE_AV1 <= source && source <= SOURCE_HDMI4) {
                notifyDeviceStatus(priv, source, TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED);
            }
        }
        break;

        default:
            break;
    }
}

void channelControl(tv_input_private_t *priv, bool opsStart, int device_id) {
    if (priv->mpTv) {
        ALOGI ("%s, device id:%d ,startTV:%d\n", __FUNCTION__, device_id, opsStart?1:0);

        if (SOURCE_DTVKIT == device_id)
            return;
        if (opsStart) {
            priv->mpTv->startTv();
            priv->mpTv->switchSourceInput((tv_source_input_t) device_id);
        } else if (priv->mpTv->getCurrentSourceInput() == device_id) {
            priv->mpTv->stopTv();
        }
    }
}

int notifyDeviceStatus(tv_input_private_t *priv, tv_source_input_t inputSrc, int type)
{
    tv_input_event_t event;
    const char address[64] = {0};
    event.device_info.device_id = inputSrc;
    event.device_info.audio_type = AUDIO_DEVICE_NONE;
    event.device_info.audio_address = address;
    event.type = type;
    switch (inputSrc) {
        case SOURCE_TV:
        case SOURCE_DTV:
        case SOURCE_ADTV:
        case SOURCE_DTVKIT:
            event.device_info.type = TV_INPUT_TYPE_TUNER;
            event.device_info.audio_type = AUDIO_DEVICE_IN_TV_TUNER;
            break;
        case SOURCE_AV1:
        case SOURCE_AV2:
            event.device_info.type = TV_INPUT_TYPE_COMPOSITE;
            event.device_info.audio_type = AUDIO_DEVICE_IN_LINE;
            break;
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        case SOURCE_HDMI4:
            event.device_info.type = TV_INPUT_TYPE_HDMI;
            event.device_info.hdmi.port_id = inputSrc - SOURCE_YPBPR2;
            event.device_info.audio_type = AUDIO_DEVICE_IN_HDMI;
            break;
        case SOURCE_SPDIF:
            event.device_info.type = TV_INPUT_TYPE_OTHER_HARDWARE;
            event.device_info.audio_type = AUDIO_DEVICE_IN_SPDIF;
            break;
        case SOURCE_AUX:
            event.device_info.type = TV_INPUT_TYPE_OTHER_HARDWARE;
            event.device_info.audio_type = AUDIO_DEVICE_IN_LINE;
            break;
        case SOURCE_ARC:
            event.device_info.type = TV_INPUT_TYPE_OTHER_HARDWARE;
            event.device_info.audio_type = AUDIO_DEVICE_IN_HDMI_ARC;
            break;
        default:
            break;
    }
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notifyCaptureSucceeded(tv_input_private_t *priv, int device_id, int stream_id, uint32_t seq)
{
    tv_input_event_t event;
    event.type = TV_INPUT_EVENT_CAPTURE_SUCCEEDED;
    event.capture_result.device_id = device_id;
    event.capture_result.stream_id = stream_id;
    event.capture_result.seq = seq;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static int notifyCaptureFail(tv_input_private_t *priv, int device_id, int stream_id, uint32_t seq)
{
    tv_input_event_t event;
    event.type = TV_INPUT_EVENT_CAPTURE_FAILED;
    event.capture_result.device_id = device_id;
    event.capture_result.stream_id = stream_id;
    event.capture_result.seq = seq;
    priv->callback->notify(&priv->device, &event, priv->callback_data);
    return 0;
}

static bool getAvailableStreamConfigs(int dev_id __unused, int *num_configurations, const tv_stream_config_t **configs)
{
    static tv_stream_config_t mconfig[2];
    mconfig[0].stream_id = STREAM_ID_NORMAL;
    mconfig[0].type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE ;
    mconfig[0].max_video_width = 1920;
    mconfig[0].max_video_height = 1080;
    mconfig[1].stream_id = STREAM_ID_FRAME_CAPTURE;
    mconfig[1].type = TV_STREAM_TYPE_BUFFER_PRODUCER ;
    mconfig[1].max_video_width = 1920;
    mconfig[1].max_video_height = 1080;
    *num_configurations = 2;
    *configs = mconfig;
    return true;
}

static int getUnavailableStreamConfigs(int dev_id __unused, int *num_configurations, const tv_stream_config_t **configs)
{
    *num_configurations = 0;
    *configs = NULL;
    return 0;
}

static int getTvStream(tv_stream_t *stream)
{
    if (stream->stream_id == STREAM_ID_NORMAL) {
        if (pTvStream == nullptr) {
            pTvStream = (struct sideband_handle_t *)native_handle_create(0, 2);
            if (pTvStream == nullptr) {
                ALOGE("tvstream can not be initialized");
                return -EINVAL;
            }
        }
        pTvStream->identflag = 0xabcdcdef; //magic word
        pTvStream->usage = GRALLOC_USAGE_AML_VIDEO_OVERLAY;
        stream->type = TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE;
        stream->sideband_stream_source_handle = (native_handle_t *)pTvStream;
    } else if (stream->stream_id == STREAM_ID_FRAME_CAPTURE) {
        stream->type = TV_STREAM_TYPE_BUFFER_PRODUCER;
    }
    return 0;
}

void initTvDevices(tv_input_private_t *priv)
{
    int supportDevices[20];
    int count = 0;
    priv->mpTv->getSupportInputDevices(supportDevices, &count);

    if (count == 0) {
        ALOGE("tv.source.input.ids.default is not set.");
        return;
    }

    bool isHotplugDetectOn = priv->mpTv->getHdmiAvHotplugDetectOnoff();
    ALOGI("hdmi/av hotplug detect on: %s", isHotplugDetectOn?"YES":"NO");
    if (isHotplugDetectOn)
        priv->mpTv->setTvObserver(priv->eventCallback);

    for (int i = 0; i < count; i++) {
        tv_source_input_t inputSrc = (tv_source_input_t)supportDevices[i];
         notifyDeviceStatus(priv, inputSrc, TV_INPUT_EVENT_DEVICE_AVAILABLE);
    }
}

static int tv_input_initialize(struct tv_input_device *dev,
                               const tv_input_callback_ops_t *callback, void *data)
{
    if (dev == NULL || callback == NULL) {
        return -EINVAL;
    }
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    /*if (priv->callback != NULL) {
        ALOGE("tv input had been init done, do not need init again");
        return -EEXIST;
    }*/
    priv->callback = callback;
    priv->callback_data = data;

    initTvDevices(priv);
    return 0;
}

static int tv_input_get_stream_configurations(const struct tv_input_device *dev __unused,
        int device_id, int *num_configurations,
        const tv_stream_config_t **configs)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    int isHotplugDetectOn = priv->mpTv->getHdmiAvHotplugDetectOnoff();
    if (isHotplugDetectOn == 0) {
        LOGD("%s:Hot plug disabled!\n", __FUNCTION__);
        getAvailableStreamConfigs(device_id, num_configurations, configs);
    } else {
        LOGD("%s:Hot plug enabled!\n", __FUNCTION__);
        bool status = true;
        if (SOURCE_AV1 <= device_id && device_id <= SOURCE_HDMI4) {
            status = priv->mpTv->getSourceConnectStatus((tv_source_input_t)device_id);
        }
        LOGD("tv_input_get_stream_configurations  source = %d, status = %d", device_id, status);
        if (status) {
            getAvailableStreamConfigs(device_id, num_configurations, configs);
        } else {
            getUnavailableStreamConfigs(device_id, num_configurations, configs);
        }
    }

    return 0;
}


static int tv_input_open_stream(struct tv_input_device *dev, int device_id,
                                tv_stream_t *stream)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (priv) {
        if (getTvStream(stream) != 0) {
            return -EINVAL;
        }
        if (stream->stream_id == STREAM_ID_NORMAL) {
            channelControl(priv, true, device_id);
        }
        else if (stream->stream_id == STREAM_ID_FRAME_CAPTURE) {
            aml_screen_module_t* screenModule;
            if (hw_get_module(AML_SCREEN_HARDWARE_MODULE_ID, (const hw_module_t **)&screenModule) < 0) {
                ALOGE("can not get screen source module");
            } else {
                screenModule->common.methods->open((const hw_module_t *)screenModule,
                    AML_SCREEN_SOURCE, (struct hw_device_t**)&(priv->mDev));
                //do test here, we can use ops of mDev to operate vdin source
            }

            if (priv->mDev) {
                if (capWidth == 0 || capHeight == 0) {
                    capWidth = stream->buffer_producer.width;
                    capHeight = stream->buffer_producer.height;
                }
                priv->mDev->ops.set_format(priv->mDev, capWidth, capHeight, V4L2_PIX_FMT_NV21);
                priv->mDev->ops.set_port_type(priv->mDev, (int)0x4000); //TVIN_PORT_HDMI0 = 0x4000
                priv->mDev->ops.start_v4l2_device(priv->mDev);
            }
        }
        return 0;
    }
    return -EINVAL;
}

static int tv_input_close_stream(struct tv_input_device *dev, int device_id,
                                 int stream_id)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (stream_id == STREAM_ID_NORMAL) {
        channelControl(priv, false, device_id);
        return 0;
    } else if (stream_id == STREAM_ID_FRAME_CAPTURE) {
        if (priv->mDev) {
            priv->mDev->ops.stop_v4l2_device(priv->mDev);
        }
        return 0;
    }
    return -EINVAL;
}

static int tv_input_request_capture(
    struct tv_input_device *dev, int device_id,
    int stream_id, buffer_handle_t buffer, uint32_t seq)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    unsigned char *dest = NULL;
    if (priv->mDev) {
        aml_screen_buffer_info_t buffInfo;
        int ret = priv->mDev->ops.aquire_buffer(priv->mDev, &buffInfo);
        if (ret != 0 || (buffInfo.buffer_mem == nullptr)) {
            ALOGE("Get V4l2 buffer failed");
            notifyCaptureFail(priv,device_id,stream_id,--seq);
            return -EWOULDBLOCK;
        }
        long *src = (long *)buffInfo.buffer_mem;

        ANativeWindowBuffer *buf = container_of(buffer, ANativeWindowBuffer, handle);
        sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf->handle, GraphicBuffer::WRAP_HANDLE,
                buf->width, buf->height,
                buf->format, buf->layerCount,
                buf->usage, buf->stride));
        graphicBuffer->lock(SCREENSOURCE_GRALLOC_USAGE, (void **)&dest);
        if (dest == NULL) {
            ALOGE("Invalid Gralloc Handle");
            return -EWOULDBLOCK;
        }
        memcpy(dest, src, capWidth*capHeight);
        graphicBuffer->unlock();
        graphicBuffer.clear();
        priv->mDev->ops.release_buffer(priv->mDev, src);

        notifyCaptureSucceeded(priv, device_id, stream_id, seq);
        return 0;
    }
    return -EWOULDBLOCK;
}

static int tv_input_cancel_capture(struct tv_input_device *, int, int, uint32_t)
{
    return -EINVAL;
}
/*
static int tv_input_set_capturesurface_size(struct tv_input_device *dev __unused, int width, int height)
{
    if (width == 0 || height == 0) {
        return -EINVAL;
    } else {
        capWidth = width;
        capHeight = height;
        return 1;
    }
}
*/
static int tv_input_device_close(struct hw_device_t *dev)
{
    tv_input_private_t *priv = (tv_input_private_t *)dev;
    if (priv) {
        if (priv->mpTv) {
            delete priv->mpTv;
            priv->mpTv = nullptr;
        }

        if (priv->mDev) {
            delete priv->mDev;
            priv->mDev = nullptr;
        }

        if (priv->eventCallback) {
            delete priv->eventCallback;
            priv->eventCallback = nullptr;
        }
        free(priv);

        native_handle_delete((native_handle_t*)pTvStream);
    }
    return 0;
}

static int tv_input_device_open(const struct hw_module_t *module,
                                const char *name, struct hw_device_t **device)
{
    int status = -EINVAL;
    if (!strcmp(name, TV_INPUT_DEFAULT_DEVICE)) {
        tv_input_private_t *dev = (tv_input_private_t *)malloc(sizeof(*dev));
        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));
        dev->mpTv = new TvInputIntf();
        dev->eventCallback = new EventCallback(dev);
        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = TV_INPUT_DEVICE_API_VERSION_0_1;
        dev->device.common.module = const_cast<hw_module_t *>(module);
        dev->device.common.close = tv_input_device_close;

        dev->device.initialize = tv_input_initialize;
        dev->device.get_stream_configurations =
            tv_input_get_stream_configurations;
        dev->device.open_stream = tv_input_open_stream;
        dev->device.close_stream = tv_input_close_stream;
        dev->device.request_capture = tv_input_request_capture;
        dev->device.cancel_capture = tv_input_cancel_capture;
        //dev->device.set_capturesurface_size = tv_input_set_capturesurface_size;

        *device = &dev->device.common;
        status = 0;
    }
    return status;
}

static struct hw_module_methods_t tv_input_module_methods = {
    .open = tv_input_device_open
};

/*
 * The tv input Module
 */
tv_input_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .version_major      = 0,
        .version_minor      = 1,
        .id                 = TV_INPUT_HARDWARE_MODULE_ID,
        .name               = "Amlogic tv input Module",
        .author             = "Amlogic Corp.",
        .methods            = &tv_input_module_methods,
    }
};
