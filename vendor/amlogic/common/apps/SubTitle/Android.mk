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

LOCAL_SRC_FILES := $(call all-java-files-under, src) \
    src/com/droidlogic/app/ISubTitleService.aidl  \
    src/com/droidlogic/app/ISubTitleServiceCallback.aidl

#ifndef PRODUCT_SHIPPING_API_LEVEL
#LOCAL_PRIVATE_PLATFORM_APIS := true
#endif
LOCAL_JAVA_LIBRARIES := \
	android.hidl.base-V1.0-java \
	android.hidl.manager-V1.0-java

LOCAL_STATIC_JAVA_LIBRARIES := \
	vendor.amlogic.hardware.subtitleserver-V1.0-java

LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
LOCAL_PROGUARD_ENABLED := disabled

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_PACKAGE_NAME := SubTitle
LOCAL_CERTIFICATE := platform
LOCAL_JAVA_LIBRARIES := droidlogic
LOCAL_REQUIRED_MODULES := libsubjni libtvsubtitle_tv libjnifont_tv

include $(BUILD_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
