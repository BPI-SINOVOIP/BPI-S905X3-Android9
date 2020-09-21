#
# Copyright (C) 2015 The Android Open Source Project
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

# Use these prebuilts unless we are actually building from a newly rebased
# LLVM. This variable is set by external/clang/build.py.
ifneq (true,$(FORCE_BUILD_SANITIZER_SHARED_OBJECTS))

libclang_dir := $(LLVM_PREBUILTS_VERSION)/lib64/clang/$(LLVM_RELEASE_VERSION)

# Also build/install the newest asan_test for each arch
# We rename it to asan-test for now to avoid duplicate definitions.

include $(CLEAR_VARS)
LOCAL_MODULE := asan-test
LOCAL_SRC_FILES := $(LLVM_PREBUILTS_VERSION)/test/arm/bin/asan_test
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $($(TARGET_2ND_ARCH_VAR_PREFIX)TARGET_OUT_DATA_NATIVE_TESTS)/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_SANITIZE := never
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_CXX_STL := none
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := asan-test
LOCAL_SRC_FILES := $(LLVM_PREBUILTS_VERSION)/test/aarch64/bin/asan_test
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS)/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_TARGET_ARCH := arm64
LOCAL_SANITIZE := never
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_CXX_STL := none
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := asan-test
LOCAL_SRC_FILES := $(LLVM_PREBUILTS_VERSION)/test/i686/bin/asan_test
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $($(TARGET_2ND_ARCH_VAR_PREFIX)TARGET_OUT_DATA_NATIVE_TESTS)/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_TARGET_ARCH := x86
LOCAL_SANITIZE := never
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_CXX_STL := none
include $(BUILD_PREBUILT)

# There is no x86_64 prebuilt here yet.

include $(CLEAR_VARS)
LOCAL_MODULE := asan-test
LOCAL_SRC_FILES := $(LLVM_PREBUILTS_VERSION)/test/mips/bin/asan_test
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $($(TARGET_2ND_ARCH_VAR_PREFIX)TARGET_OUT_DATA_NATIVE_TESTS)/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_TARGET_ARCH := mips
LOCAL_SANITIZE := never
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_CXX_STL := none
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := asan-test
LOCAL_SRC_FILES := $(LLVM_PREBUILTS_VERSION)/test/mips64/bin/asan_test
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_SUFFIX :=
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS)/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_TARGET_ARCH := mips64
LOCAL_SANITIZE := never
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_CXX_STL := none
include $(BUILD_PREBUILT)

endif
