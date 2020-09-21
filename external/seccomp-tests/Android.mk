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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := external_seccomp_tests

# mips and mips64 don't support Seccomp.
LOCAL_MODULE_TARGET_ARCH := arm arm64 x86 x86_64

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := linux/seccomp_bpf.c

LOCAL_CFLAGS := \
		-Wall -Werror \
		-Wno-gnu-designator \
		-Wno-unused-parameter \
		-Wno-literal-conversion \
		-Wno-incompatible-pointer-types-discards-qualifiers \
		-Wno-sign-compare \
		-Wno-empty-body \
		-D__ARCH_WANT_SYSCALL_DEPRECATED  # TODO(rsesek): Remove after syncing in upstream.

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
