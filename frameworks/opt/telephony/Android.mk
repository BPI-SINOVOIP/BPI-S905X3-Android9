# Copyright (C) 2011 The Android Open Source Project
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

# enable this build only when platform library is available
ifneq ($(TARGET_BUILD_PDK), true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/src/java
LOCAL_SRC_FILES := $(call all-java-files-under, src/java) \
	$(call all-Iaidl-files-under, src/java) \
	$(call all-logtags-files-under, src/java)

LOCAL_JAVA_LIBRARIES := voip-common ims-common services bouncycastle
LOCAL_STATIC_JAVA_LIBRARIES := \
    telephony-protos \
    android.hardware.radio-V1.0-java \
    android.hardware.radio-V1.1-java \
    android.hardware.radio-V1.2-java \
    android.hardware.radio.config-V1.0-java \
    android.hardware.radio.deprecated-V1.0-java \
    android.hidl.base-V1.0-java

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := telephony-common

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

ifeq ($(EMMA_INSTRUMENT_FRAMEWORK),true)
LOCAL_EMMA_INSTRUMENT := true
endif

include $(BUILD_JAVA_LIBRARY)

# Include subdirectory makefiles
# ============================================================
include $(call all-makefiles-under,$(LOCAL_PATH))

endif # non-PDK build
