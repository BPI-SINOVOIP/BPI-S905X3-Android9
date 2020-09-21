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

include $(LOCAL_PATH)/../../../testcases/target/Android.clean.mk

test_cflags = \
    -fstack-protector-all \
    -g \
    -Wall -Wextra -Wunused \
    -Werror \
    -fno-builtin \

test_cflags += -D__STDC_LIMIT_MACROS  # For glibc.

test_cppflags := \

# Library of bionic customized gtest main function, with normal gtest output format,
# which is needed by bionic cts test.
libVtsGtestMain_src_files := \
    gtest_main.cpp
libVtsGtestMain_cflags := $(test_cflags)
libVtsGtestMain_cppflags := $(test_cppflags) \

module := libVtsGtestMain
module_tag := optional
build_type := target
build_target := STATIC_TEST_LIBRARY
include $(LOCAL_PATH)/../../../testcases/target/Android.build.mk
build_type := host
include $(LOCAL_PATH)/../../../testcases/target/Android.build.mk
