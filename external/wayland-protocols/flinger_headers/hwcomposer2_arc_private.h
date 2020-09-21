/*
 * Copyright 2017 The Chromium Authors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef ANDROID_SF_PRIVATE_HWCOMPOSER2_ARC_PRIVATE_H
#define ANDROID_SF_PRIVATE_HWCOMPOSER2_ARC_PRIVATE_H

__BEGIN_DECLS

/* Optional ARC private capabilities. The particular set of supported private
 * capabilities for a given device may be retrieved using
 * getArcPrivateCapabilities. */
typedef enum {
    HWC2_ARC_PRIVATE_CAPABILITY_INVALID = 0,

    /* Specifies that the device supports ARC attribute data. Decoding the data
     * is an implementation detail for the device. Note that ordinarily the
     * Android framework does not send this data. It is assumed that a vendor
     * that wants this data has also modified the framework to send it. */
    HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES = 1,

    /* Specifies that the device is an ARC windowing composer. A windowing
     * composer generates windowed output inside some external
     * implementation-defined windowing environment. It means that there is no
     * longer a single output frame buffer being used. The device must handle
     * all composition, and the client must not do so. The client cannot do any
     * culling of layers either -- it may not have full knowledge of what is
     * actually visible or not. */
    HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER = 2,
} hwc2_arc_private_capability_t;

/* ARC private function descriptors for use with getFunction.
 * The first entry needs to be maintained so there is no overlap with the
 * constants there. */
typedef enum {
    HWC2_ARC_PRIVATE_FUNCTION_GET_CAPABILITIES = 0x10000,

    // For HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER
    HWC2_ARC_PRIVATE_FUNCTION_GET_DISPLAY_ATTRIBUTE,

    // For HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES
    HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_ATTRIBUTES,

    // For HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER
    HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_HIDDEN,

    // For HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES
    HWC2_ARC_PRIVATE_FUNCTION_ATTRIBUTES_SHOULD_FORCE_UPDATE,
} hwc2_arc_private_function_descriptor_t;

typedef enum {
    HWC2_ARC_PRIVATE_DISPLAY_ATTRIBUTE_INVALID = 0,
    HWC2_ARC_PRIVATE_DISPLAY_ATTRIBUTE_OUTPUT_ROTATION = 1,
} hwc2_arc_private_display_attribute_t;

typedef enum {
    HWC2_ARC_PRIVATE_HIDDEN_INVALID = 0,
    HWC2_ARC_PRIVATE_HIDDEN_ENABLE = 1,
    HWC2_ARC_PRIVATE_HIDDEN_DISABLE = 2,
} hwc2_arc_private_hidden_t;

/*
 * Stringification Functions
 */

#ifdef HWC2_INCLUDE_STRINGIFICATION

static inline const char* getArcPrivateCapabilityName(hwc2_arc_private_capability_t capability)
{
    switch (capability) {
    case HWC2_ARC_PRIVATE_CAPABILITY_INVALID:
        return "Invalid";
    case HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES:
        return "ArcAttributes";
    case HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER:
        return "ArcWindowingComposer";
    default:
        return "Unknown";
    }
}

static inline const char* getArcPrivateFunctionDescriptorName(
        hwc2_arc_private_function_descriptor_t desc)
{
    switch (desc) {
    case HWC2_ARC_PRIVATE_FUNCTION_GET_CAPABILITIES:
        return "ArcGetCapabilities";
    case HWC2_ARC_PRIVATE_FUNCTION_GET_DISPLAY_ATTRIBUTE:
        return "ArcGetDisplayAttribute";
    case HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_ATTRIBUTES:
        return "ArcSetLayerAttributes";
    case HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_HIDDEN:
        return "ArcSetLayerHidden";
    case HWC2_ARC_PRIVATE_FUNCTION_ATTRIBUTES_SHOULD_FORCE_UPDATE:
        return "ArcAttributesShouldForceUpdate";
    default:
        return "Unknown";
    }
}

static inline const char* getArcPrivateDisplayAttributeName(hwc2_arc_private_display_attribute_t attribute)
{
    switch (attribute) {
    case HWC2_ARC_PRIVATE_DISPLAY_ATTRIBUTE_INVALID:
        return "Invalid";
    case HWC2_ARC_PRIVATE_DISPLAY_ATTRIBUTE_OUTPUT_ROTATION:
        return "OutputRotation";
    default:
        return "Unknown";
    }
}

static inline const char* getArcPrivateHiddenName(hwc2_arc_private_hidden_t hidden)
{
    switch (hidden) {
    case HWC2_ARC_PRIVATE_HIDDEN_INVALID:
        return "Invalid";
    case HWC2_ARC_PRIVATE_HIDDEN_ENABLE:
        return "Enable";
    case HWC2_ARC_PRIVATE_HIDDEN_DISABLE:
        return "Disable";
    default:
        return "Unknown";
    }
}

#endif  // HWC2_INCLUDE_STRINGIFICATION

/*
 * C++11 features
 */

#ifdef HWC2_USE_CPP11
__END_DECLS

#ifdef HWC2_INCLUDE_STRINGIFICATION
#include <string>
#endif

namespace HWC2 {

enum class ArcPrivateCapability : int32_t {
    Invalid = HWC2_ARC_PRIVATE_CAPABILITY_INVALID,
    Attributes = HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES,
    WindowingComposer = HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER,
};
TO_STRING(hwc2_arc_private_capability_t, ArcPrivateCapability, getArcPrivateCapabilityName)

enum class ArcPrivateFunctionDescriptor : int32_t {
    // Since we are extending the HWC2 FunctionDescriptor, we duplicate
    // all of its
    GetCapabilities = HWC2_ARC_PRIVATE_FUNCTION_GET_CAPABILITIES,
    GetDisplayAttribute = HWC2_ARC_PRIVATE_FUNCTION_GET_DISPLAY_ATTRIBUTE,
    SetLayerAttributes = HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_ATTRIBUTES,
    SetLayerHidden = HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_HIDDEN,
    AttributesShouldForceUpdate = HWC2_ARC_PRIVATE_FUNCTION_ATTRIBUTES_SHOULD_FORCE_UPDATE,
};
TO_STRING(hwc2_arc_private_function_descriptor_t, ArcPrivateFunctionDescriptor,
        getArcPrivateFunctionDescriptorName)

enum class ArcPrivateDisplayAttribute : int32_t {
    Invalid = HWC2_ARC_PRIVATE_DISPLAY_ATTRIBUTE_INVALID,
    OutputRotation = HWC2_ARC_PRIVATE_DISPLAY_ATTRIBUTE_OUTPUT_ROTATION,
};
TO_STRING(hwc2_arc_private_display_attribute_t, ArcPrivateDisplayAttribute,
        getArcPrivateDisplayAttributeName)

enum class ArcPrivateHidden : int32_t {
    Invalid = HWC2_ARC_PRIVATE_HIDDEN_INVALID,
    Enable = HWC2_ARC_PRIVATE_HIDDEN_ENABLE,
    Disable = HWC2_ARC_PRIVATE_HIDDEN_DISABLE,
};
TO_STRING(hwc2_arc_private_hidden_t, ArcPrivateHidden, getArcPrivateHiddenName)

}  // namespace HWC2

__BEGIN_DECLS
#endif  // HWC2_USE_CPP11

/*
 * ARC Private device Functions
 *
 * All of these functions take as their first parameter a device pointer, so
 * this parameter is omitted from the described parameter lists.
 */

/* arcGetCapabilities(..., outCount, outCapabilities)
 * Descriptor: HWC2_ARC_PRIVATE_FUNCTION_GET_CAPABILITIES
 *
 * Gets additional capabilities supported by the device.
 *
 * Parameters:
 *   outCount - if outCapabilities was NULL, the number of capabilities which
 *       would have been returned; if outCapabilities was not NULL, the number
 *       of capabilities returned, which must not exceed the value stored in
 *       outCount priort to the call; pointer will be non-NULL
 *   outCapabilities - an array of capabilities
 * */
typedef void (*HWC2_ARC_PRIVATE_PFN_GET_CAPABILITIES)(hwc2_device_t* device, uint32_t* outCount,
        int32_t* /*hwc2_arc_private_capability_t*/ outCapabilities);

/*
 * ARC Private display functions
 *
 * All of these functions take as their first two parameters a device pointer,
 * and a display handle for the display. These parameters are omitted from the
 * described parameter lists.
 */

/* arcGetDisplayAttributes(..., attribute, outValue)
 * Descriptor: HWC2_ARC_PRIVATE_FUNCTION_GET_DISPLAY_ATTRIBUTE
 * Provided by HWC2 devices which support
 * HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER
 *
 * Gets additional display attribute data.
 *
 * Parameters:
 *   attribute - The attribute to get
 *   outValue - A location to store the value of the attribute
 *
 * Returns HWC2_ERROR_NONE if a value was set, or HWC2_ERROR_BAD_DISPLAY, or
 * HWC2_ERROR_BAD_PARAMETER
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_ARC_PRIVATE_PFN_GET_DISPLAY_ATTRIBUTE)(
        hwc2_device_t* device, hwc2_display_t display, const int32_t attribute, int32_t* outValue);

/*
 * ARC Private layer Functions
 *
 * These are functions which operate on layers, but which do not modify state
 * that must be validated before use. See also 'Layer State Functions' below.
 *
 * All of these functions take as their first three parameters a device pointer,
 * a display handle for the display which contains the layer, and a layer
 * handle, so these parameters are omitted from the described parameter lists.
 */

/* arcSetLayerAttributes(..., numElements, ids, sizes, values)
 * Descriptor: HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_ATTRIBUTES
 * Provided by HWC2 devices which support
 * HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES
 *
 * Sets additional surface data in the form of id/value pairs sent down by the
 * framework. The meaning of each attribute is opaque to the client, and is
 * assumed to be understood by the device implementation.
 *
 * Parameters:
 *   numElements - the number of elements in each array.
 *   ids - an array of surface attribute ids
 *   sizes - an array of sizes, giving the size in bytes of each value
 *   values - an array of pointers to the data for each value
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_LAYER - an invalid layer handle was passed in
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_ARC_PRIVATE_PFN_SET_LAYER_ATTRIBUTES)(hwc2_device_t* device,
        hwc2_display_t display, hwc2_layer_t layer, uint32_t numElements, const int32_t* ids,
        const uint32_t* sizes, const uint8_t** values);

/* arcSetLayerHidden(..., hidden)
 * Descriptor: HWC2_ARC_PRIVATE_FUNCTION_SET_LAYER_HIDDEN
 * Provided by HWC2 devices which support
 * HWC2_ARC_PRIVATE_CAPABILITY_WINDOWING_COMPOSER
 *
 * Indicates whether the layer should be hidden or not. This flag is set by the
 * window manager.
 *
 * Parameters:
 *   hidden - the setting to use.
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_LAYER - an invalid layer handle was passed in
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_ARC_PRIVATE_PFN_SET_LAYER_HIDDEN)(
        hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
        int32_t /* hwc2_arc_private_hidden_t */ hidden);

/* arcAttributesShouldForceUpdate(..., numElements, ids, sizes, values, outShouldForceUpdate)
 * Descriptor: HWC2_ARC_PRIVATE_FUNCTION_ATTRIBUTES_SHOULD_FORCE_UPDATE
 * Provided by HWC2 devices which support
 * HWC2_ARC_PRIVATE_CAPABILITY_ATTRIBUTES
 *
 * Outputs to |outShouldForceUpdate| whether to send geometry updates without
 * waiting for a matching buffer, given the specified layer attributes.
 *
 * Parameters:
 *   numElements - the number of elements in each array.
 *   ids - an array of surface attribute ids
 *   sizes - an array of sizes, giving the size in bytes of each value
 *   values - an array of pointers to the data for each value
 *   outShouldForceUpdate - whether to send geometry updates without waiting
 *      for a matching buffer
 *
 * Returns HWC2_ERROR_NONE or one of the following errors:
 *   HWC2_ERROR_BAD_LAYER - an invalid layer handle was passed in
 */
typedef int32_t /*hwc2_error_t*/ (*HWC2_ARC_PRIVATE_PFN_ATTRIBUTES_SHOULD_FORCE_UPDATE)(
    hwc2_device_t* device, hwc2_display_t display, hwc2_layer_t layer,
    uint32_t numElements, const int32_t* ids, const uint32_t* sizes,
    const uint8_t** values, bool* outShouldForceUpdate);

__END_DECLS

#endif
