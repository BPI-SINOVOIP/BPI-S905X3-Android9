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
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAVA_LIBRARIES := droidlogic
#LOCAL_SDK_VERSION := current

LOCAL_PACKAGE_NAME := Miracast
LOCAL_CERTIFICATE := platform

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

#ifndef PRODUCT_SHIPPING_API_LEVEL
LOCAL_PRIVATE_PLATFORM_APIS := true
#endif
#LOCAL_JNI_SHARED_LIBRARIES := libwfd_jni
LOCAL_REQUIRED_MODULES := libwfd_jni

LOCAL_JAVA_LIBRARIES += \
    android.hidl.base-V1.0-java \
    android.hidl.manager-V1.0-java

#LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PROGUARD_ENABLED := full
LOCAL_PROGUARD_FLAG_FILES := proguard.flags
LOCAL_DEX_PREOPT := false
LOCAL_ARM_MODE := arm
LOCAL_PRODUCT_MODULE := true
#WITH_DEXPREOPT = false
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
