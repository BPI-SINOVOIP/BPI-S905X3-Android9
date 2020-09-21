#
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
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

# Work around limitations of AAR prebuilts
LOCAL_RESOURCE_DIR += prebuilts/sdk/current/car/car/res

LOCAL_MODULE := car-setup-wizard-lib
LOCAL_MODULE_TAGS := optional

LOCAL_USE_AAPT2 := true

LOCAL_STATIC_ANDROID_LIBRARIES += \
    android-support-car

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_STATIC_JAVA_LIBRARY)

#disable build in PDK, robotests won't build
ifneq ($(TARGET_BUILD_PDK),true)
    # Use the following include to make our test apk.
    ifeq (,$(ONE_SHOT_MAKEFILE))
        include $(call all-makefiles-under,$(LOCAL_PATH))
    endif
endif #TARGET_BUILD_PDK