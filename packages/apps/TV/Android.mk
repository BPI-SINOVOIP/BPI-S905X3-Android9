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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)


LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/version.mk

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_JAVA_LIBRARIES := droidlogic droidlogic-tv
LOCAL_PACKAGE_NAME := LiveTv

LOCAL_CERTIFICATE := platform

# It is required for com.android.providers.tv.permission.ALL_EPG_DATA
LOCAL_PRIVILEGED_MODULE := true

LOCAL_PRIVATE_PLATFORM_APIS := true

#LOCAL_SDK_VERSION := system_current
LOCAL_MIN_SDK_VERSION := 23  # M

LOCAL_USE_AAPT2 := true

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res \

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-annotations \
    lib-exoplayer \
    lib-exoplayer-v2-core \
    jsr330 \

LOCAL_STATIC_ANDROID_LIBRARIES := \
    android-support-compat \
    android-support-core-ui \
    android-support-tv-provider \
    android-support-v4 \
    android-support-v7-appcompat \
    android-support-v7-palette \
    android-support-v7-preference \
    android-support-v7-recyclerview \
    android-support-v14-preference \
    android-support-v17-leanback \
    android-support-v17-preference-leanback \
    live-channels-partner-support \
    live-tv-tuner \
    tv-common \


LOCAL_JAVACFLAGS := -Xlint:deprecation -Xlint:unchecked

LOCAL_AAPT_FLAGS += \
    --version-name "$(version_name_package)" \
    --version-code $(version_code_package) \

LOCAL_JNI_SHARED_LIBRARIES := libtunertvinput_jni
LOCAL_AAPT_FLAGS += --extra-packages com.android.tv.tuner

include $(BUILD_PACKAGE)

#############################################################
# Pre-built dependency jars
#############################################################
prebuilts := \
    lib-exoplayer:libs/exoplayer-r1.5.16.aar \
    lib-exoplayer-v2-core:libs/exoplayer-core-2-SNAPHOT-20180114.aar \
    auto-value-jar:../../../prebuilts/tools/common/m2/repository/com/google/auto/value/auto-value/1.5.2/auto-value-1.5.2.jar \
    javax-annotations-jar:../../../prebuilts/tools/common/m2/repository/javax/annotation/javax.annotation-api/1.2/javax.annotation-api-1.2.jar \
    truth-0-36-prebuilt-jar:../../../prebuilts/tools/common/m2/repository/com/google/truth/truth/0.36/truth-0.36.jar \

define define-prebuilt
  $(eval tw := $(subst :, ,$(strip $(1)))) \
  $(eval include $(CLEAR_VARS)) \
  $(eval LOCAL_MODULE := $(word 1,$(tw))) \
  $(eval LOCAL_MODULE_TAGS := optional) \
  $(eval LOCAL_MODULE_CLASS := JAVA_LIBRARIES) \
  $(eval LOCAL_SRC_FILES := $(word 2,$(tw))) \
  $(eval LOCAL_UNINSTALLABLE_MODULE := true) \
  $(eval LOCAL_SDK_VERSION := current) \
  $(eval include $(BUILD_PREBUILT))
endef

$(foreach p,$(prebuilts),\
  $(call define-prebuilt,$(p)))

prebuilts :=

include $(call all-makefiles-under,$(LOCAL_PATH))
