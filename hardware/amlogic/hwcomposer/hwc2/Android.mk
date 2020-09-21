# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(USE_HWC2), true)

$(info "Build HWC 2.0")

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_CPPFLAGS := $(HWC_CPP_FLAGS)
LOCAL_CFLAGS := $(HWC_C_FLAGS)
LOCAL_SHARED_LIBRARIES := $(HWC_SHARED_LIBS)

# hwc 2.2 interface enable
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_CFLAGS += -DHWC_HDR_METADATA_SUPPORT
endif

ifeq ($(HWC_SUPPORT_MODES_LIST), true)
LOCAL_CFLAGS += -DHWC_SUPPORT_MODES_LIST
endif

LOCAL_SRC_FILES := \
    Hwc2Base.cpp \
    Hwc2Display.cpp \
    Hwc2Layer.cpp \
    Hwc2Module.cpp \
    HwcModeMgr.cpp \
    FixedSizeModeMgr.cpp \
    VariableModeMgr.cpp \
    ActiveModeMgr.cpp \
    RealModeMgr.cpp \
    MesonHwc2.cpp

LOCAL_C_INCLUDES := \
    hardware/libhardware/include \
    $(LOCAL_PATH)/include

# !!! static lib sequence is serious, donot change it.
LOCAL_STATIC_LIBRARIES := \
    hwc.common_static \
    hwc.composition_static \
    hwc.postprocessor_static \
    hwc.display_static \
    hwc.base_static \
    hwc.utils_static \
    hwc.debug_static \
    libomxutil

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := hwcomposer.amlogic

include $(BUILD_SHARED_LIBRARY)

endif
