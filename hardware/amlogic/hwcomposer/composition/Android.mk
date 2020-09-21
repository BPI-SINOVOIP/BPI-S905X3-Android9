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

#For Android p,and later
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CPPFLAGS := $(HWC_CPP_FLAGS)
LOCAL_CFLAGS := $(HWC_C_FLAGS)

LOCAL_SRC_FILES := \
    Composition.cpp \
    CompositionStrategyFactory.cpp \
    composer/ComposerFactory.cpp \
    composer/ClientComposer.cpp \
    composer/DummyComposer.cpp \
    simplestrategy/SingleplaneComposition/SingleplaneComposition.cpp \
    simplestrategy/MultiplanesComposition/MultiplanesComposition.cpp

ifeq ($(TARGET_SUPPORT_GE2D_COMPOSITION),true)
LOCAL_SRC_FILES += \
    composer/GE2DComposer.cpp
endif

LOCAL_C_INCLUDES := \
    hardware/libhardware/include \
    $(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES := \
    hwc.utils_static \
    hwc.base_static \
    hwc.display_static

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/include

LOCAL_MODULE := hwc.composition_static

include $(BUILD_STATIC_LIBRARY)
