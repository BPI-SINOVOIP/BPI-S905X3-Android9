# Copyright (C) 2016 The Android Open Source Project
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

LOCAL_MODULE := Magick_filters

LOCAL_SDK_VERSION := 24

LOCAL_SRC_FILES := analyze.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/..

LOCAL_CFLAGS += -DHAVE_CONFIG_H -Wall -Werror

LOCAL_STATIC_LIBRARIES += libbz
LOCAL_SHARED_LIBRARIES += libft2 liblzma libxml2 libicuuc libpng libjpeg

include $(BUILD_STATIC_LIBRARY)
