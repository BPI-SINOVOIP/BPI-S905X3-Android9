#
# Copyright (C) 2014 The Android Open Source Project
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

#########################################
# The prebuilt support libraries.

include $(CLEAR_VARS)

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

# For apps (unbundled) build, replace the typical
# make target artifacts with prebuilts.
ifneq (,$(TARGET_BUILD_APPS)$(filter true,$(TARGET_BUILD_PDK)))
    # Set up prebuilts for Multidex library artifacts.
    multidex_jars := \
      $(patsubst $(LOCAL_PATH)/%,%,\
        $(shell find $(LOCAL_PATH)/multidex -name "*.jar"))
    prebuilts := $(foreach jar,$(multidex_jars),\
        $(basename $(notdir $(jar))):$(jar))

    # Set up prebuilts for optional libraries. Need to specify them explicitly
    # as the target name does not match the JAR name.
    prebuilts += \
        android.test.base.stubs:optional/android.test.base.jar \
        android.test.mock.stubs:optional/android.test.mock.jar \
        android.test.runner.stubs:optional/android.test.runner.jar

    $(foreach p,$(prebuilts),\
        $(call define-prebuilt,$(p)))

    prebuilts :=
endif  # TARGET_BUILD_APPS not empty or TARGET_BUILD_PDK set to True


# Include all Support Library modules as prebuilts.
include $(call all-makefiles-under,$(LOCAL_PATH))
