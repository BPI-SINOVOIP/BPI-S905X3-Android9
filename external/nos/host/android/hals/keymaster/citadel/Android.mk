#
# Copyright (C) 2018 The Android Open Source Project
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

LOCAL_MODULE := android.hardware.keymaster@4.0-service.citadel
LOCAL_INIT_RC := android.hardware.keymaster@4.0-service.citadel.rc

LOCAL_SRC_FILES := service.cpp

LOCAL_REQUIRED_MODULES := citadeld
LOCAL_HEADER_LIBRARIES := nos_headers
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    libhidltransport \
    libnos \
    libnosprotos \
    libutils \
    libprotobuf-cpp-full \
    android.hardware.keymaster@4.0 \
    android.hardware.keymaster@4.0-impl.nos \
    libnos_citadeld_proxy \
    nos_app_keymaster


LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_CFLAGS := -pedantic -Wall -Wextra -Werror -Wno-zero-length-array
LOCAL_CONLYFLAGS := -std=c11
LOCAL_CLANG := true
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_OWNER := google

ifneq ($(BUILD_WITHOUT_VENDOR),true)
ifeq ($(call is-board-platform-in-list, sdm845),true)
LOCAL_SHARED_LIBRARIES += \
    libkeymasterprovision \
    libkeymasterutils \
    libkeymasterdeviceutils \
    libQSEEComAPI

LOCAL_CFLAGS += -DENABLE_QCOM_OTF_PROVISIONING=1
endif
endif

include $(BUILD_EXECUTABLE)

#cc_binary {
#    name: "android.hardware.keymaster@4.0-service.citadel",
#    init_rc: ["android.hardware.keymaster@4.0-service.citadel.rc"],
#    required: ["citadeld"],
#    srcs: [
#        "service.cpp",
#    ],
#    defaults: ["nos_hal_service_defaults"],
#    shared_libs: [
#        "android.hardware.keymaster@4.0",
#        "android.hardware.keymaster@4.0-impl.nos",
#        "libnos_citadeld_proxy",
#        "nos_app_keymaster",
#        "libkeymasterprovision",
#        "libkeymasterutils",
#        "libkeymasterdeviceutils",
#        "libQSEEComAPI",
#    ],
#}
