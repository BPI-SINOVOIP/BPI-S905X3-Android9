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
#
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    main.cpp \
    eventgatherer.cpp \
    inputsource.cpp \
    keymap.cpp \
    event.cpp \
    eventprovider.cpp \
    ../common/com/android/car/keventreader/IEventCallback.aidl \
    ../common/com/android/car/keventreader/IEventProvider.aidl \

LOCAL_C_INCLUDES += \
    frameworks/base/include

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    liblog \
    libutils

LOCAL_AIDL_INCLUDES = $(LOCAL_PATH)/../common

LOCAL_MODULE := com.android.car.keventreader
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Wall -Werror
LOCAL_CPPFLAGS += -std=c++17

include $(BUILD_EXECUTABLE)
