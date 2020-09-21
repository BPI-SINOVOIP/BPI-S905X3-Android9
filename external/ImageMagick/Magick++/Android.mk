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

define define-magick++-vars
  $(eval LOCAL_SRC_FILES := lib/Blob.cpp\
    lib/BlobRef.cpp\
    lib/CoderInfo.cpp\
    lib/Color.cpp\
    lib/Drawable.cpp\
    lib/Exception.cpp\
    lib/Functions.cpp\
    lib/Geometry.cpp\
    lib/Image.cpp\
    lib/ImageRef.cpp\
    lib/Montage.cpp\
    lib/Options.cpp\
    lib/Pixels.cpp\
    lib/ResourceLimits.cpp\
    lib/STL.cpp\
    lib/Statistic.cpp\
    lib/Thread.cpp\
    lib/TypeMetric.cpp) \
  $(eval LOCAL_C_INCLUDES += $(LOCAL_PATH)/.. \
    $(LOCAL_PATH)/lib) \
  $(eval LOCAL_CFLAGS += -DHAVE_CONFIG_H \
    -Wall -Werror \
    -Wno-deprecated-register \
    -Wno-unused-private-field) \
  $(eval LOCAL_CPPFLAGS += -fexceptions) \
  $(eval LOCAL_EXPORT_C_INCLUDE_DIRS := \
    external/ImageMagick/Magick++/lib \
    external/ImageMagick)
endef

include $(CLEAR_VARS)

$(call define-magick++-vars)
LOCAL_MODULE := Magick++_platform
LOCAL_CXX_STL := libc++_static

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

$(call define-magick++-vars)
LOCAL_MODULE := Magick++
LOCAL_SDK_VERSION := 24
LOCAL_NDK_STL_VARIANT := c++_static

include $(BUILD_STATIC_LIBRARY)
