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

LOCAL_PATH:= $(call my-dir)

###################################################

include $(CLEAR_VARS)

LOCAL_MODULE := mkdtimg
LOCAL_CFLAGS := -Wall -Werror
ifeq ($(HOST_OS),darwin)
LOCAL_CFLAGS += -Wno-error=format
endif
LOCAL_SRC_FILES := \
	mkdtimg.c \
	mkdtimg_cfg_create.c \
	mkdtimg_core.c \
	mkdtimg_create.c \
	mkdtimg_dump.c \
	dt_table.c
LOCAL_STATIC_LIBRARIES := \
	libfdt \
	libufdt_sysdeps
LOCAL_REQUIRED_MODULES := dtc
LOCAL_CXX_STL := none

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals, dist_files, $(ALL_MODULES.mkdtimg.BUILT):libufdt/mkdtimg)
###################################################
include $(CLEAR_VARS)

LOCAL_MODULE := mkdtboimg.py
LOCAL_SRC_FILES := mkdtboimg.py
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional

include $(BUILD_PREBUILT)

