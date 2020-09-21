/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware/hwcomposer2.h>

#include <MesonLog.h>

#include "MesonHwc2.h"

typedef struct hwc2_impl {
    hwc2_device_t base;
    MesonHwc2 * impl;
} hwc2_impl_t;

#define GET_MESON_HWC() \
    hwc2_impl_t * hwc = reinterpret_cast<hwc2_impl_t *>(device); \
    MesonHwc2 * mesonhwc = hwc->impl;

int32_t hwc2_create_virtual_display(
        hwc2_device_t* device, uint32_t width, uint32_t height,
        int32_t* format, hwc2_display_t* outDisplay) {
    GET_MESON_HWC();
    return mesonhwc->createVirtualDisplay(width, height, format, outDisplay);
}

int32_t hwc2_destroy_virtual_display(
        hwc2_device_t* device, hwc2_display_t display) {
    GET_MESON_HWC();
    return mesonhwc->destroyVirtualDisplay(display);
}

void hwc2_dump(
        hwc2_device_t* device, uint32_t* outSize, char* outBuffer) {
    GET_MESON_HWC();
    return mesonhwc->dump(outSize, outBuffer);
}

uint32_t   hwc2_get_max_virtual_display_count(
        hwc2_device_t* device) {
    GET_MESON_HWC();
    return mesonhwc->getMaxVirtualDisplayCount();
}

int32_t  hwc2_accept_display_changes(
        hwc2_device_t* device, hwc2_display_t display) {
    GET_MESON_HWC();
    return mesonhwc->acceptDisplayChanges(display);
}

int32_t hwc2_create_layer(hwc2_device_t* device,
        hwc2_display_t display, hwc2_layer_t* outLayer) {
    GET_MESON_HWC();
    return mesonhwc->createLayer(display, outLayer);
}

int32_t hwc2_destroy_layer(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer) {
    GET_MESON_HWC();
    return mesonhwc->destroyLayer(display, layer);
}

int32_t hwc2_get_active_config(
        hwc2_device_t* device, hwc2_display_t display,
        hwc2_config_t* outConfig) {
    GET_MESON_HWC();
    return mesonhwc->getActiveConfig(display, outConfig);
}

int32_t hwc2_get_changed_composition_types(
        hwc2_device_t* device, hwc2_display_t display,
        uint32_t* outNumElements, hwc2_layer_t* outLayers,
        int32_t*  outTypes) {
    GET_MESON_HWC();
    return mesonhwc->getChangedCompositionTypes(display, outNumElements,
                    outLayers, outTypes);
}

int32_t hwc2_get_client_target_support(
        hwc2_device_t* device, hwc2_display_t display, uint32_t width,
        uint32_t height, int32_t format, int32_t dataspace) {
    GET_MESON_HWC();
    return mesonhwc->getClientTargetSupport(display, width, height, format, dataspace);
}

int32_t  hwc2_get_color_modes(
        hwc2_device_t* device, hwc2_display_t display,
        uint32_t* outNumModes, int32_t*  outModes) {
    GET_MESON_HWC();
    return mesonhwc->getColorModes(display, outNumModes, outModes);
}

int32_t  hwc2_get_display_attribute(
        hwc2_device_t* device, hwc2_display_t display, hwc2_config_t config,
        int32_t attribute, int32_t* outValue) {
    GET_MESON_HWC();
    return mesonhwc->getDisplayAttribute(display, config, attribute, outValue);
}

int32_t  hwc2_get_display_configs(
        hwc2_device_t* device, hwc2_display_t display,
        uint32_t* outNumConfigs, hwc2_config_t* outConfigs) {
    GET_MESON_HWC();
    return mesonhwc->getDisplayConfigs(display, outNumConfigs, outConfigs);
}

int32_t hwc2_get_display_name(
        hwc2_device_t* device, hwc2_display_t display,
        uint32_t* outSize, char* outName) {
    GET_MESON_HWC();
    return mesonhwc->getDisplayName(display, outSize, outName);
}

int32_t hwc2_get_display_requests(
        hwc2_device_t* device, hwc2_display_t display,
        int32_t* outDisplayRequests, uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outLayerRequests) {
    GET_MESON_HWC();
    return mesonhwc->getDisplayRequests(display, outDisplayRequests, outNumElements,
                    outLayers, outLayerRequests);
}

int32_t  hwc2_get_display_type(
        hwc2_device_t* device, hwc2_display_t display,
        int32_t* outType) {
    GET_MESON_HWC();
    return mesonhwc->getDisplayType(display, outType);
}

int32_t hwc2_get_doze_support(
        hwc2_device_t* device, hwc2_display_t display, int32_t* outSupport) {
    GET_MESON_HWC();
    return mesonhwc->getDozeSupport(display, outSupport);
}

int32_t hwc2_get_hdr_capabilities(
        hwc2_device_t* device, hwc2_display_t display, uint32_t* outNumTypes,
        int32_t* outTypes, float* outMaxLuminance,
        float* outMaxAverageLuminance, float* outMinLuminance) {
    GET_MESON_HWC();
    return mesonhwc->getHdrCapabilities(display, outNumTypes, outTypes,
                    outMaxLuminance, outMaxAverageLuminance, outMinLuminance);
}

int32_t hwc2_get_release_fences(
        hwc2_device_t* device, hwc2_display_t display, uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outFences) {
    GET_MESON_HWC();
    return mesonhwc->getReleaseFences(display, outNumElements,
                    outLayers, outFences);
}

int32_t hwc2_present_display(
        hwc2_device_t* device, hwc2_display_t display, int32_t* outPresentFence) {
    GET_MESON_HWC();
    return mesonhwc->presentDisplay(display, outPresentFence);
}

int32_t hwc2_register_callback(
        hwc2_device_t* device, int32_t descriptor,
        hwc2_callback_data_t callbackData, hwc2_function_pointer_t pointer) {
        GET_MESON_HWC();
        return mesonhwc->registerCallback(descriptor, callbackData, pointer);
}

int32_t hwc2_set_active_config(
        hwc2_device_t* device, hwc2_display_t display, hwc2_config_t config) {
        GET_MESON_HWC();
        return mesonhwc->setActiveConfig(display, config);
}

int32_t hwc2_set_client_target(
        hwc2_device_t* device, hwc2_display_t display, buffer_handle_t target,
        int32_t acquireFence, int32_t dataspace, hwc_region_t damage) {
    GET_MESON_HWC();
    return mesonhwc->setClientTarget(display, target, acquireFence, dataspace, damage);
}

int32_t hwc2_set_color_mode(
        hwc2_device_t* device, hwc2_display_t display, int32_t mode) {
    GET_MESON_HWC();
    return mesonhwc->setColorMode(display, mode);
}

int32_t hwc2_set_color_transform(
        hwc2_device_t* device, hwc2_display_t display, const float* matrix, int32_t hint) {
    GET_MESON_HWC();
    return mesonhwc->setColorTransform(display, matrix, hint);
}

int32_t hwc2_set_output_buffer(
        hwc2_device_t* device, hwc2_display_t display, buffer_handle_t buffer,
        int32_t releaseFence) {
    GET_MESON_HWC();
    return mesonhwc->setOutputBuffer(display, buffer, releaseFence);
}

int32_t hwc2_set_power_mode(
        hwc2_device_t* device, hwc2_display_t display, int32_t mode) {
    GET_MESON_HWC();
    return mesonhwc->setPowerMode(display, mode);
}

int32_t hwc2_set_vsync_enabled(
        hwc2_device_t* device, hwc2_display_t display, int32_t enabled) {
    GET_MESON_HWC();
    return mesonhwc->setVsyncEnable(display, enabled);
}

int32_t hwc2_validate_display(
        hwc2_device_t* device, hwc2_display_t display,
        uint32_t* outNumTypes, uint32_t* outNumRequests) {
    GET_MESON_HWC();
    return mesonhwc->validateDisplay(display, outNumTypes, outNumRequests);
}

int32_t hwc2_set_cursor_position(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t x, int32_t y) {
    GET_MESON_HWC();
    return mesonhwc->setCursorPosition(display, layer, x, y);
}

int32_t hwc2_set_layer_buffer(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        buffer_handle_t buffer, int32_t acquireFence) {
    GET_MESON_HWC();
    return mesonhwc->setLayerBuffer(display, layer, buffer, acquireFence);
}

int32_t hwc2_set_layer_surface_damage(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        hwc_region_t damage) {
    GET_MESON_HWC();
    return mesonhwc->setLayerSurfaceDamage(display, layer, damage);
}

int32_t hwc2_set_layer_blend_mode(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t mode) {
    GET_MESON_HWC();
    return mesonhwc->setLayerBlendMode(display, layer, mode);
}

int32_t hwc2_set_layer_color(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        hwc_color_t color) {
    GET_MESON_HWC();
    return mesonhwc->setLayerColor(display, layer, color);
}

int32_t hwc2_set_layer_composition_type(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t type) {
    GET_MESON_HWC();
    return mesonhwc->setLayerCompositionType(display, layer, type);
}

int32_t hwc2_set_layer_dataspace(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t dataspace) {
    GET_MESON_HWC();
    return mesonhwc->setLayerDataspace(display, layer, dataspace);
}

int32_t hwc2_set_layer_display_frame(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        hwc_rect_t frame) {
    GET_MESON_HWC();
    return mesonhwc->setLayerDisplayFrame(display, layer, frame);
}

int32_t hwc2_set_layer_plane_alpha(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        float alpha) {
    GET_MESON_HWC();
    return mesonhwc->setLayerPlaneAlpha(display, layer, alpha);
}

int32_t hwc2_set_layer_sideband_stream(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        const native_handle_t* stream) {
    GET_MESON_HWC();
    return mesonhwc->setLayerSidebandStream(display, layer, stream);
}

int32_t hwc2_set_layer_source_crop(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        hwc_frect_t crop) {
    GET_MESON_HWC();
    return mesonhwc->setLayerSourceCrop(display, layer, crop);
}

int32_t hwc2_set_layer_transform(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t transform) {
    GET_MESON_HWC();
    return mesonhwc->setLayerTransform(display, layer, transform);
}

int32_t hwc2_set_layer_visible_region(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        hwc_region_t visible) {
    GET_MESON_HWC();
    return mesonhwc->setLayerVisibleRegion(display, layer, visible);
}

int32_t  hwc2_set_layer_z_order(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        uint32_t z) {
    GET_MESON_HWC();
    return mesonhwc->setLayerZorder(display, layer, z);
}

#ifdef HWC_HDR_METADATA_SUPPORT
int32_t setLayerPerFrameMetadata(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        uint32_t numElements, const int32_t* /*hw2_per_frame_metadata_key_t*/ keys,
        const float* metadata) {
    GET_MESON_HWC();
    return mesonhwc->setLayerPerFrameMetadata(display, layer, numElements, keys, metadata);
}

int32_t getPerFrameMetadataKeys(
        hwc2_device_t* device, hwc2_display_t display, uint32_t* outNumKeys,
        int32_t* /*hwc2_per_frame_metadata_key_t*/ outKeys) {
    GET_MESON_HWC();
    return mesonhwc->getPerFrameMetadataKeys(display, outNumKeys, outKeys);
}
#endif

hwc2_function_pointer_t hwc2_getFunction(struct hwc2_device* device __unused,
        int32_t descriptor) {
    switch (descriptor) {
        case HWC2_FUNCTION_ACCEPT_DISPLAY_CHANGES:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_accept_display_changes);
        case HWC2_FUNCTION_CREATE_LAYER:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_create_layer);
        case HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_create_virtual_display);
        case HWC2_FUNCTION_DESTROY_LAYER:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_destroy_layer);
        case HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_destroy_virtual_display);
        case HWC2_FUNCTION_DUMP:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_dump);
        case HWC2_FUNCTION_GET_ACTIVE_CONFIG:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_active_config);
        case HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_changed_composition_types);
        case HWC2_FUNCTION_GET_CLIENT_TARGET_SUPPORT:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_client_target_support);
        case HWC2_FUNCTION_GET_COLOR_MODES:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_color_modes);
        case HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_display_attribute);
        case HWC2_FUNCTION_GET_DISPLAY_CONFIGS:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_display_configs);
        case HWC2_FUNCTION_GET_DISPLAY_NAME:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_display_name);
        case HWC2_FUNCTION_GET_DISPLAY_REQUESTS:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_display_requests);
        case HWC2_FUNCTION_GET_DISPLAY_TYPE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_display_type);
        case HWC2_FUNCTION_GET_DOZE_SUPPORT:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_doze_support);
        case HWC2_FUNCTION_GET_HDR_CAPABILITIES:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_hdr_capabilities);
        case HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_max_virtual_display_count);
        case HWC2_FUNCTION_GET_RELEASE_FENCES:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_get_release_fences);
        case HWC2_FUNCTION_PRESENT_DISPLAY:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_present_display);
        case HWC2_FUNCTION_REGISTER_CALLBACK:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_register_callback);
        case HWC2_FUNCTION_SET_ACTIVE_CONFIG:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_active_config);
        case HWC2_FUNCTION_SET_CLIENT_TARGET:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_client_target);
        case HWC2_FUNCTION_SET_COLOR_MODE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_color_mode);
        case HWC2_FUNCTION_SET_COLOR_TRANSFORM:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_color_transform);
        case HWC2_FUNCTION_SET_CURSOR_POSITION:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_cursor_position);
        case HWC2_FUNCTION_SET_LAYER_BLEND_MODE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_blend_mode);
        case HWC2_FUNCTION_SET_LAYER_BUFFER:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_buffer);
        case HWC2_FUNCTION_SET_LAYER_COLOR:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_color);
        case HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_composition_type);
        case HWC2_FUNCTION_SET_LAYER_DATASPACE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_dataspace);
        case HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_display_frame);
        case HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_plane_alpha);
        case HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_sideband_stream);
        case HWC2_FUNCTION_SET_LAYER_SOURCE_CROP:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_source_crop);
        case HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_surface_damage);
        case HWC2_FUNCTION_SET_LAYER_TRANSFORM:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_transform);
        case HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_visible_region);
        case HWC2_FUNCTION_SET_LAYER_Z_ORDER:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_layer_z_order);
        case HWC2_FUNCTION_SET_OUTPUT_BUFFER:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_output_buffer);
        case HWC2_FUNCTION_SET_POWER_MODE:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_power_mode);
        case HWC2_FUNCTION_SET_VSYNC_ENABLED:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_set_vsync_enabled);
        case HWC2_FUNCTION_VALIDATE_DISPLAY:
            return reinterpret_cast<hwc2_function_pointer_t>(hwc2_validate_display);
#ifdef HWC_HDR_METADATA_SUPPORT
        case HWC2_FUNCTION_SET_LAYER_PER_FRAME_METADATA:
            return reinterpret_cast<hwc2_function_pointer_t>(setLayerPerFrameMetadata);
        case HWC2_FUNCTION_GET_PER_FRAME_METADATA_KEYS:
            return reinterpret_cast<hwc2_function_pointer_t>(getPerFrameMetadataKeys);
#endif
        default:
            MESON_LOGE("Unkown function description (%d)", descriptor);
            break;
    };
    return NULL;
}

void hwc2_getCapabilities(struct hwc2_device* device, uint32_t* outCount,
        int32_t* outCapabilities) {
    GET_MESON_HWC();
    mesonhwc->getCapabilities(outCount, outCapabilities);
}

static int hwc2_device_close(struct hw_device_t *dev) {
    MESON_LOGV("close hwc2 (%p)", dev);

    hwc2_impl_t * hwc = reinterpret_cast<hwc2_impl_t *>(dev);
    if (hwc->impl) {
        delete hwc->impl;
    }
    free(hwc);

    return 0;
}

static int hwc2_device_open(
                            const struct hw_module_t* module,
                            const char* name,
                            struct hw_device_t** device) {
    if (!name) {
        return -EINVAL;
    }

    MESON_LOGV("open hwc2 (%s)", name);

    if (strcmp(name, HWC_HARDWARE_COMPOSER) != 0) {
        MESON_LOGE("Not supported composer (%s)", name);
        return -EINVAL;
    }

    /*init hwc device. */
    hwc2_impl_t * hwc = (hwc2_impl_t*)calloc(1, sizeof(hwc2_impl_t));
    hwc->impl = new MesonHwc2();

    hwc->base.common.module = const_cast<hw_module_t*>(module);
    hwc->base.common.version = HWC_DEVICE_API_VERSION_2_0;
    hwc->base.common.close = hwc2_device_close;
    hwc->base.getCapabilities = hwc2_getCapabilities;
    hwc->base.getFunction = hwc2_getFunction;

    *device = reinterpret_cast<hw_device_t*>(hwc);

    return 0;
}

static struct hw_module_methods_t hwc2_module_methods = {
    .open = hwc2_device_open
};

typedef struct hwc2_module {
    struct hw_module_t common;
    /* add extra info below.*/
} hwc2_module_t;

hwc_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 2,
        .version_minor = 0,
        .id = HWC_HARDWARE_MODULE_ID,
        .name = "amlogic hwc module",
        .author = "Amlogic Graphic",
        .methods = &hwc2_module_methods,
        .dso = NULL,
        .reserved = {0},
    }
};


