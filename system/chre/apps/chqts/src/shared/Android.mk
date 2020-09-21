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

# While it's arguably overkill to have unit test for our testing code,
# this testing code runs under an embedded environment which is much more
# difficult to test.  To the extent that we have platform-independent
# code here, it seems a long-term time saver to have Linux tests for
# what we can easily test.

LOCAL_PATH := $(call my-dir)

FILES_UNDER_TEST := \
    dumb_allocator.cc \
    nano_endian.cc \
    nano_string.cc \

TESTING_SOURCE := \
    dumb_allocator_test.cc \
    nano_endian_be_test.cc \
    nano_endian_le_test.cc \
    nano_endian_test.cc \
    nano_string_test.cc \

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := $(LOCAL_PATH)/..

LOCAL_SRC_FILES := $(FILES_UNDER_TEST) $(TESTING_SOURCE)

LOCAL_MODULE := nanoapp_chqts_shared_tests
LOCAL_MODULE_TAGS := tests

LOCAL_CPPFLAGS += -Wall -Wextra -Werror
LOCAL_CPPFLAGS += -DINTERNAL_TESTING

include $(BUILD_HOST_NATIVE_TEST)

endif  # HOST_OS == linux
