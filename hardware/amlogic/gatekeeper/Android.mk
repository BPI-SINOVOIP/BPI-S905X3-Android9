#
# Copyright (C) 2015 The Android Open-Source Project
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

# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.panda), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := gatekeeper.amlogic

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SRC_FILES := \
	GKPModule.cpp \
	soft/SoftGateKeeperDevice.cpp

#	trusty/trusty_gatekeeper_ipc.c \
#	trusty/trusty_gatekeeper.cpp \

LOCAL_STATIC_LIBRARIES := libscrypt_static
LOCAL_C_INCLUDES := \
  external/boringssl/src/include/ \
  external/scrypt/lib/crypto \
  system/core/base/include \
  system/core/libsystem/include \
  hardware/libhardware/include \
  system/gatekeeper/include/gatekeeper

LOCAL_CLFAGS = -fvisibility=hidden -Wall -Werror

LOCAL_SHARED_LIBRARIES := \
	libgatekeeper \
	liblog \
	libcutils \
	libcrypto

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
