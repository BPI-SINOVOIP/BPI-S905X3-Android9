#
# Copyright (C) 2016 The Android Open Source Project
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

LOCAL_PATH := $(call my-dir)
local_ltp_root := $(LOCAL_PATH)/..

ltp_build_prebuilt := $(LOCAL_PATH)/Android.prebuilt.mk

include $(LOCAL_PATH)/Android.ltp.mk

local_ltp_root :=
ltp_build_prebuilt :=

include $(CLEAR_VARS)
LOCAL_MODULE := ltp
LOCAL_MODULE_STEM := disabled_tests.txt
LOCAL_PREBUILT_MODULE_FILE := $(LOCAL_PATH)/tools/disabled_tests.txt
LOCAL_MODULE_RELATIVE_PATH := ltp
LOCAL_MODULE_CLASS := NATIVE_TESTS
LOCAL_MULTILIB := both
LOCAL_TEST_DATA := $(call find-test-data-in-subdirs,external/ltp,"*",runtest)

include $(LOCAL_PATH)/ltp_package_list.mk
LOCAL_REQUIRED_MODULES := $(ltp_packages)
ltp_packages :=

include $(BUILD_PREBUILT)
