# -*- mode: makefile -*-
# Copyright (C) 2009 The Android Open Source Project
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

#
# Subprojects with separate makefiles
#

subdirs := benchmarks tzdata ojluni tools/upstream metrictests
subdir_makefiles := $(call all-named-subdir-makefiles,$(subdirs))

#
# Include the definitions to build the Java code.
#

include $(LOCAL_PATH)/JavaLibrary.mk

#
# Disable test modules if LIBCORE_SKIP_TESTS environment variable is set.
#

ifneq ($(LIBCORE_SKIP_TESTS),)
$(info ********************************************************************************)
$(info * libcore tests are skipped because environment variable LIBCORE_SKIP_TESTS=$(LIBCORE_SKIP_TESTS))
$(info ********************************************************************************)
endif

include $(subdir_makefiles)
