# Copyright (C) 2018 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

# DynamicsProcessing library
include $(CLEAR_VARS)

LOCAL_VENDOR_MODULE := true

EIGEN_PATH := external/eigen
LOCAL_C_INCLUDES += $(EIGEN_PATH)

LOCAL_SRC_FILES:= \
    EffectDynamicsProcessing.cpp \
    dsp/DPBase.cpp \
    dsp/DPFrequency.cpp

LOCAL_CFLAGS+= -O2 -fvisibility=hidden
LOCAL_CFLAGS += -Wall -Werror

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \

LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE:= libdynproc

LOCAL_HEADER_LIBRARIES := \
    libaudioeffects

include $(BUILD_SHARED_LIBRARY)
