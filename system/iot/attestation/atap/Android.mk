#
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

LOCAL_PATH := $(my-dir)

atap_common_cflags := \
    -D_FILE_OFFSET_BITS=64 \
    -D_POSIX_C_SOURCE=199309L \
    -Wa,--noexecstack \
    -Werror \
    -Wall \
    -Wextra \
    -Wformat=2 \
    -Wno-psabi \
    -Wno-unused-parameter \
    -ffunction-sections \
    -fstack-protector-strong \
    -g
atap_common_cppflags := \
    -Wnon-virtual-dtor \
    -fno-strict-aliasing
atap_common_ldflags := \
    -Wl,--gc-sections \
    -rdynamic

# Build libatap for the host (for unit tests).
include $(CLEAR_VARS)
LOCAL_MODULE := libatap_host
LOCAL_MODULE_HOST_OS := linux
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CFLAGS := $(atap_common_cflags) -fno-stack-protector -DATAP_ENABLE_DEBUG
LOCAL_LDFLAGS := $(atap_common_ldflags)
LOCAL_SRC_FILES := \
    libatap/atap_commands.c \
    libatap/atap_util.c
include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libatap_host_unittest
LOCAL_MODULE_HOST_OS := linux
LOCAL_CFLAGS := $(atap_common_cflags) -DATAP_ENABLE_DEBUG
LOCAL_CPPFLAGS := $(atap_comman_cppflags)
LOCAL_LDFLAGS := $(atap_common_ldflags)
LOCAL_STATIC_LIBRARIES := \
    libbase \
    libatap_host \
    liblog \
    libgmock_host \
    libgtest_host
LOCAL_SHARED_LIBRARIES := \
    libchrome \
    libcrypto
LOCAL_SRC_FILES := \
    ops/atap_ops_provider.cpp \
    ops/openssl_ops.cpp \
    test/atap_util_unittest.cpp \
    test/atap_command_unittest.cpp \
    test/atap_sysdeps_posix_testing.cpp \
    test/fake_atap_ops.cpp
LOCAL_LDLIBS_linux := -lrt
include $(BUILD_HOST_NATIVE_TEST)
