# Copyright (C) 2017 The Android Open Source Project
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

# Useful environment variables that can be set on the mmma command line, as
# <key>=<value> pairs:
#
# LIBTEXTCLASSIFIER_STRIP_OPTS: (optional) value for LOCAL_STRIP_MODULE (for all
#   modules we build).  NOT for prod builds.  Can be set to keep_symbols for
#   debug / treemap purposes.


LOCAL_PATH := $(call my-dir)

# Custom C/C++ compilation flags:
MY_LIBTEXTCLASSIFIER_WARNING_CFLAGS := \
    -Wall \
    -Werror \
    -Wno-ignored-qualifiers \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-tautological-constant-out-of-range-compare \
    -Wno-undefined-var-template \
    -Wno-unused-function \
    -Wno-unused-parameter \

MY_LIBTEXTCLASSIFIER_CFLAGS := \
    $(MY_LIBTEXTCLASSIFIER_WARNING_CFLAGS) \
    -fvisibility=hidden \
    -DLIBTEXTCLASSIFIER_UNILIB_ICU \
    -DZLIB_CONST

# Only enable debug logging in userdebug/eng builds.
ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
  MY_LIBTEXTCLASSIFIER_CFLAGS += -DTC_DEBUG_LOGGING=1
endif

# -----------------
# flatbuffers
# -----------------

# Empty static library so that other projects can include just the basic
# FlatBuffers headers as a module.
include $(CLEAR_VARS)
LOCAL_MODULE := flatbuffers
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_CPPFLAGS := -std=c++11 -fexceptions -Wall \
    -DFLATBUFFERS_TRACK_VERIFIER_BUFFER_SIZE

include $(BUILD_STATIC_LIBRARY)

# -----------------
# libtextclassifier
# -----------------

include $(CLEAR_VARS)
LOCAL_MODULE := libtextclassifier

LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += $(MY_LIBTEXTCLASSIFIER_CFLAGS)
LOCAL_STRIP_MODULE := $(LIBTEXTCLASSIFIER_STRIP_OPTS)

LOCAL_SRC_FILES := $(filter-out tests/% %_test.cc test-util.%,$(call all-subdir-cpp-files))

LOCAL_C_INCLUDES := $(TOP)/external/zlib
LOCAL_C_INCLUDES += $(TOP)/external/tensorflow
LOCAL_C_INCLUDES += $(TOP)/external/flatbuffers/include

LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libicuuc
LOCAL_SHARED_LIBRARIES += libicui18n
LOCAL_SHARED_LIBRARIES += libtflite
LOCAL_SHARED_LIBRARIES += libz

LOCAL_STATIC_LIBRARIES += flatbuffers

LOCAL_REQUIRED_MODULES := textclassifier.en.model
LOCAL_REQUIRED_MODULES += textclassifier.universal.model

LOCAL_ADDITIONAL_DEPENDENCIES += $(LOCAL_PATH)/jni.lds
LOCAL_LDFLAGS += -Wl,-version-script=$(LOCAL_PATH)/jni.lds
LOCAL_CPPFLAGS_32 += -DLIBTEXTCLASSIFIER_TEST_DATA_DIR="\"/data/nativetest/libtextclassifier_tests/test_data/\""
LOCAL_CPPFLAGS_64 += -DLIBTEXTCLASSIFIER_TEST_DATA_DIR="\"/data/nativetest64/libtextclassifier_tests/test_data/\""

include $(BUILD_SHARED_LIBRARY)

# -----------------------
# libtextclassifier_tests
# -----------------------

include $(CLEAR_VARS)

LOCAL_MODULE := libtextclassifier_tests
LOCAL_COMPATIBILITY_SUITE := device-tests
LOCAL_MODULE_TAGS := tests

LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS += $(MY_LIBTEXTCLASSIFIER_CFLAGS)
LOCAL_STRIP_MODULE := $(LIBTEXTCLASSIFIER_STRIP_OPTS)

LOCAL_TEST_DATA := $(call find-test-data-in-subdirs, $(LOCAL_PATH), *, test_data)

LOCAL_CPPFLAGS_32 += -DLIBTEXTCLASSIFIER_TEST_DATA_DIR="\"/data/nativetest/libtextclassifier_tests/test_data/\""
LOCAL_CPPFLAGS_64 += -DLIBTEXTCLASSIFIER_TEST_DATA_DIR="\"/data/nativetest64/libtextclassifier_tests/test_data/\""

LOCAL_SRC_FILES := $(call all-subdir-cpp-files)

LOCAL_C_INCLUDES := $(TOP)/external/zlib
LOCAL_C_INCLUDES += $(TOP)/external/tensorflow
LOCAL_C_INCLUDES += $(TOP)/external/flatbuffers/include

LOCAL_STATIC_LIBRARIES += libgmock
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libicuuc
LOCAL_SHARED_LIBRARIES += libicui18n
LOCAL_SHARED_LIBRARIES += libtflite
LOCAL_SHARED_LIBRARIES += libz

LOCAL_STATIC_LIBRARIES += flatbuffers

include $(BUILD_NATIVE_TEST)

# ----------------------
# Smart Selection models
# ----------------------

include $(CLEAR_VARS)
LOCAL_MODULE        := textclassifier.en.model
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_OWNER  := google
LOCAL_SRC_FILES     := ./models/textclassifier.en.model
LOCAL_MODULE_PATH   := $(TARGET_OUT_ETC)/textclassifier
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE        := textclassifier.universal.model
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_OWNER  := google
LOCAL_SRC_FILES     := ./models/textclassifier.universal.model
LOCAL_MODULE_PATH   := $(TARGET_OUT_ETC)/textclassifier
include $(BUILD_PREBUILT)

# -----------------------
# Smart Selection bundles
# -----------------------

include $(CLEAR_VARS)
LOCAL_MODULE           := textclassifier.bundle1
LOCAL_REQUIRED_MODULES := textclassifier.en.model
LOCAL_CFLAGS := $(MY_LIBTEXTCLASSIFIER_WARNING_CFLAGS)
include $(BUILD_STATIC_LIBRARY)
