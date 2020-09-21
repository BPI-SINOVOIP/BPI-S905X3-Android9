#
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
#

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/../Android.clean.mk

test_cflags = \
    -fstack-protector-all \
    -g \
    -Wall -Wextra -Wunused \
    -Werror \
    -fno-builtin \

test_cflags += -D__STDC_LIMIT_MACROS  # For glibc.

test_cppflags := \

test_executable := VtsHalLightsTestCases
list_executable := $(test_executable)_list

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_MODULE := $(test_executable)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/nativetest
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)64

LOCAL_SHARED_LIBRARIES += \
    libdl \
    libhardware \

LOCAL_WHOLE_STATIC_LIBRARIES += \
    libHalLightsTests \
    libVtsGtestMain \

LOCAL_STATIC_LIBRARIES += \
    libbase \
    libtinyxml2 \
    liblog \
    libgtest \

# Tag this module as a vts test artifact
LOCAL_COMPATIBILITY_SUITE := vts

# May need more in CFLAGS.
LOCAL_CFLAGS += -Wall -Wextra -Werror
include $(BUILD_EXECUTABLE)

# VTS testcase isn't designed to be run on host due to its dependencies to
# libhardware or other android runtime.
build_host := false
common_additional_dependencies := $(LOCAL_PATH)/Android.mk $(LOCAL_PATH)/../Android.build.mk

# -----------------------------------------------------------------------------
# All standard tests.
# -----------------------------------------------------------------------------

libHalLightsStandardTests_src_files := \
    hal_lights_basic_test.cpp

libHalLightsStandardTests_cflags := \
    $(test_cflags) \

libHalLightsStandardTests_cppflags := \
    $(test_cppflags) \

libHalLightsStandardTests_c_includes := \
    test/vts/testcases/include \
    external/tinyxml2 \

libHalLightsStandardTests_static_libraries := \
    libbase \

libHalLightsStandardTests_ldlibs_host := \
    -lrt \

module := libHalLightsStandardTests
module_tag := optional
build_type := target
build_target := STATIC_TEST_LIBRARY
include $(LOCAL_PATH)/../Android.build.mk
build_type := host
include $(LOCAL_PATH)/../Android.build.mk

# Library of all tests (excluding the dynamic linker tests).
# -----------------------------------------------------------------------------
libHalLightsTests_whole_static_libraries := \
    libHalLightsStandardTests \


module := libHalLightsTests
module_tag := optional
build_type := target
build_target := STATIC_TEST_LIBRARY
include $(LOCAL_PATH)/../Android.build.mk
build_type := host
include $(LOCAL_PATH)/../Android.build.mk

# -----------------------------------------------------------------------------
# Tests for the device using HAL Lights's .so. Run with:
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests32
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests64
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests-gcc32
#   adb shell /data/nativetest/hallights-unit-tests/hallights-unit-tests-gcc64
# -----------------------------------------------------------------------------
common_hallights-unit-tests_whole_static_libraries := \
    libHalLightsTests \
    libVtsGtestMain \

common_hallights-unit-tests_static_libraries := \
    libtinyxml2 \
    liblog \
    libbase \

# TODO: Include __cxa_thread_atexit_test.cpp to glibc tests once it is upgraded (glibc 2.18+)
common_hallights-unit-tests_src_files := \
    hal_lights_basic_test.cpp \

common_hallights-unit-tests_cflags := $(test_cflags)

common_hallights-unit-tests_conlyflags := \
    -fexceptions \
    -fnon-call-exceptions \

common_hallights-unit-tests_cppflags := \
    $(test_cppflags)

common_hallights-unit-tests_ldflags := \
    -Wl,--export-dynamic

common_hallights-unit-tests_c_includes := \
    bionic/libc \

common_hallights-unit-tests_shared_libraries_target := \
    libdl \
    libhardware \
    libpagemap \
    libdl_preempt_test_1 \
    libdl_preempt_test_2 \
    libdl_test_df_1_global \

# The order of these libraries matters, do not shuffle them.
common_hallights-unit-tests_static_libraries_target := \
    libbase \
    libziparchive \
    libz \
    libutils \

module_tag := optional
build_type := target
build_target := NATIVE_TEST

module := hallights-unit-tests
hallights-unit-tests_clang_target := true
hallights-unit-tests_whole_static_libraries := $(common_hallights-unit-tests_whole_static_libraries)
hallights-unit-tests_static_libraries := $(common_hallights-unit-tests_static_libraries)
hallights-unit-tests_src_files := $(common_hallights-unit-tests_src_files)
hallights-unit-tests_cflags := $(common_hallights-unit-tests_cflags)
hallights-unit-tests_conlyflags := $(common_hallights-unit-tests_conlyflags)
hallights-unit-tests_cppflags := $(common_hallights-unit-tests_cppflags)
hallights-unit-tests_ldflags := $(common_hallights-unit-tests_ldflags)
hallights-unit-tests_c_includes := $(common_hallights-unit-tests_c_includes)
hallights-unit-tests_shared_libraries_target := $(common_hallights-unit-tests_shared_libraries_target)
hallights-unit-tests_static_libraries_target := $(common_hallights-unit-tests_static_libraries_target)
include $(LOCAL_PATH)/../Android.build.mk

module := hallights-unit-tests-gcc
hallights-unit-tests-gcc_clang_target := false
hallights-unit-tests-gcc_whole_static_libraries := $(common_hallights-unit-tests_whole_static_libraries)
hallights-unit-tests-gcc_static_libraries := $(common_hallights-unit-tests_static_libraries)
hallights-unit-tests-gcc_src_files := $(common_hallights-unit-tests_src_files)
hallights-unit-tests-gcc_cflags := $(common_hallights-unit-tests_cflags)
hallights-unit-tests-gcc_conlyflags := $(common_hallights-unit-tests_conlyflags)
hallights-unit-tests-gcc_cppflags := $(common_hallights-unit-tests_cppflags)
hallights-unit-tests-gcc_ldflags := $(common_hallights-unit-tests_ldflags)
hallights-unit-tests-gcc_c_includes := $(common_hallights-unit-tests_c_includes)
hallights-unit-tests-gcc_shared_libraries_target := $(common_hallights-unit-tests_shared_libraries_target)
hallights-unit-tests-gcc_static_libraries_target := $(common_hallights-unit-tests_static_libraries_target)
include $(LOCAL_PATH)/../Android.build.mk
