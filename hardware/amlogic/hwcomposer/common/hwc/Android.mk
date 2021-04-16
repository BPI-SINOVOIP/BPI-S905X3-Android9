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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CPPFLAGS := $(HWC_CPP_FLAGS)
LOCAL_CFLAGS := $(HWC_C_FLAGS)
LOCAL_SHARED_LIBRARIES := $(HWC_SHARED_LIBS)

#*********************************HWC DISPLAY Config************************
ifneq ($(HWC_DISPLAY_NUM),)
    LOCAL_CFLAGS += -DHWC_DISPLAY_NUM=$(HWC_DISPLAY_NUM)
else
$(error "need config hwc crtc num")
endif

#PIPELIEN CONFIG
#Primary
ifneq ($(HWC_PIPELINE),)
    LOCAL_CFLAGS += -DHWC_PIPELINE=\"$(HWC_PIPELINE)\"
else
    $(info "no pipeline configed, will use default")
endif

#Vdin fb postprocessor type
ifneq ($(HWC_VDIN_FBPROCESSOR),)
    LOCAL_CFLAGS += -DHWC_VDIN_FBPROCESSOR=\"$(HWC_VDIN_FBPROCESSOR)\"
endif

#FRAMEBUFFER CONFIG
#Primary
ifneq ($(HWC_PRIMARY_FRAMEBUFFER_WIDTH)$(HWC_PRIMARY_FRAMEBUFFER_HEIGHT),)
    LOCAL_CFLAGS += -DHWC_PRIMARY_FRAMEBUFFER_WIDTH=$(HWC_PRIMARY_FRAMEBUFFER_WIDTH)
    LOCAL_CFLAGS += -DHWC_PRIMARY_FRAMEBUFFER_HEIGHT=$(HWC_PRIMARY_FRAMEBUFFER_HEIGHT)
endif
#Extend, if needed.
ifneq ($(HWC_EXTEND_FRAMEBUFFER_WIDTH)$(HWC_EXTEND_FRAMEBUFFER_HEIGHT),)
    LOCAL_CFLAGS += -DHWC_EXTEND_FRAMEBUFFER_WIDTH=$(HWC_EXTEND_FRAMEBUFFER_WIDTH)
    LOCAL_CFLAGS += -DHWC_EXTEND_FRAMEBUFFER_HEIGHT=$(HWC_EXTEND_FRAMEBUFFER_HEIGHT)
endif

#CONNECTOR
#Primary
ifneq ($(HWC_PRIMARY_CONNECTOR_TYPE),)
    LOCAL_CFLAGS += -DHWC_PRIMARY_CONNECTOR_TYPE=\"$(HWC_PRIMARY_CONNECTOR_TYPE)\"
else
$(error "need config hwc primary connector type")
endif
#Extend, if needed.
ifneq ($(HWC_EXTEND_CONNECTOR_TYPE),)
    LOCAL_CFLAGS += -DHWC_EXTEND_CONNECTOR_TYPE=\"$(HWC_EXTEND_CONNECTOR_TYPE)\"
endif

#HEADLESS MODE
ifeq ($(HWC_ENABLE_HEADLESS_MODE), true)
LOCAL_CFLAGS += -DHWC_ENABLE_HEADLESS_MODE
LOCAL_CFLAGS += -DHWC_HEADLESS_REFRESHRATE=5
endif

#Active Mode
ifeq ($(HWC_ENABLE_ACTIVE_MODE), true)
LOCAL_CFLAGS += -DHWC_ENABLE_ACTIVE_MODE
endif

#Real Mode
ifeq ($(HWC_ENABLE_REAL_MODE), true)
LOCAL_CFLAGS += -DHWC_ENABLE_REAL_MODE
endif

#Display Calibrate
ifeq ($(HWC_ENABLE_PRE_DISPLAY_CALIBRATE), true)
#pre display calibrate means calibrate in surfacefligner,
#all the coordinates got by hwc already calibrated.
LOCAL_CFLAGS += -DHWC_ENABLE_PRE_DISPLAY_CALIBRATE
endif

#HWC Feature Config
ifeq ($(HWC_ENABLE_SOFTWARE_VSYNC), true)
LOCAL_CFLAGS += -DHWC_ENABLE_SOFTWARE_VSYNC
endif
ifeq ($(ENABLE_PRIMARY_DISPLAY_HOTPLUG), true) #Used for NTS test
HWC_ENABLE_PRIMARY_HOTPLUG := true
endif
ifeq ($(HWC_ENABLE_PRIMARY_HOTPLUG), true) #need surfaceflinger modifications
LOCAL_CFLAGS += -DHWC_ENABLE_PRIMARY_HOTPLUG
endif
ifeq ($(HWC_ENABLE_SECURE_LAYER_PROCESS), true)
LOCAL_CFLAGS += -DHWC_ENABLE_SECURE_LAYER_PROCESS
endif
ifeq ($(HWC_DISABLE_CURSOR_PLANE), true)
LOCAL_CFLAGS += -DHWC_DISABLE_CURSOR_PLANE
endif
ifeq ($(TARGET_USE_DEFAULT_HDR_PROPERTY),true)
LOCAL_CFLAGS += -DHWC_ENABLE_DEFAULT_HDR_CAPABILITIES
endif
ifeq ($(TARGET_APP_LAYER_USE_CONTINUOUS_BUFFER),false)
LOCAL_CFLAGS += -DHWC_FORCE_CLIENT_COMPOSITION
endif
ifeq ($(HWC_PIPE_VIU1VDINVIU2_ALWAYS_LOOPBACK), true)
LOCAL_CFLAGS += -DHWC_PIPE_VIU1VDINVIU2_ALWAYS_LOOPBACK
endif

LOCAL_CFLAGS += -DODROID
ifeq ($(HWC_DYNAMIC_SWITCH_CONNECTOR), true)
LOCAL_CFLAGS += -DHWC_DYNAMIC_SWITCH_CONNECTOR
endif
ifeq ($(HWC_DYNAMIC_SWITCH_VIU), true)
LOCAL_CFLAGS += -DHWC_DYNAMIC_SWITCH_VIU
endif
#*********************************HWC CONFIGS END************************

LOCAL_SRC_FILES := \
    HwcVsync.cpp \
    HwcConfig.cpp \
    HwcPowerMode.cpp \
    HwcDisplayPipe.cpp \
    FixedDisplayPipe.cpp \
    LoopbackDisplayPipe.cpp \
    DualDisplayPipe.cpp

LOCAL_C_INCLUDES := \
    hardware/libhardware/include \
    $(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES := \
    hwc.utils_static \
    hwc.base_static \
    hwc.display_static \
    hwc.postprocessor_static

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/include

LOCAL_MODULE := hwc.common_static

include $(BUILD_STATIC_LIBRARY)
