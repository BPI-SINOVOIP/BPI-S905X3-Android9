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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_HOST_OS := linux
LOCAL_SRC_FILES := ioshark_bench.c ioshark_bench_subr.c ioshark_bench_mmap.c
LOCAL_CFLAGS := -g -O2 -Wall  -Werror
LOCAL_MODULE := ioshark_bench
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_HOST_OS := linux
LOCAL_SRC_FILES := compile_ioshark.c compile_ioshark_subr.c
LOCAL_CFLAGS := -g -O2 -Wall -Werror -D_GNU_SOURCE
LOCAL_MODULE := compile_ioshark
LOCAL_MODULE_TAGS := debug
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_HOST_OS := linux
LOCAL_SRC_FILES := dump_ioshark_filenames.c
LOCAL_CFLAGS := -g -O2 -Wall -Werror -D_GNU_SOURCE
LOCAL_MODULE := dump_ioshark_filenames
LOCAL_MODULE_TAGS := debug
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_HOST_OS := linux
LOCAL_SRC_FILES := convert_format.c
LOCAL_CFLAGS := -g -O2 -Wall -Werror -D_GNU_SOURCE
LOCAL_MODULE := convert_format
LOCAL_MODULE_TAGS := debug
include $(BUILD_HOST_EXECUTABLE)
