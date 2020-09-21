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
poc_target_dir := $(LOCAL_PATH)

include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

poc_test_src_files := \
    poc_test.cpp \

# TODO(trong): tests should not emit warnings.
poc_test_cflags := \
    -Wall \
    -Werror \
    -Wno-int-conversion \
    -Wno-pointer-sign \
    -Wno-unused-parameter \
    -Wno-unused-variable \

poc_test_c_includes := \
    $(poc_target_dir) \

build_poc_test := $(poc_target_dir)/Android.poc_test.mk
include $(poc_target_dir)/Android.poc_test_list.mk
