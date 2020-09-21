# Copyright (C) 2012 Amlogic Inc.
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

define PREBUILT_template
    LOCAL_MODULE:= $(1)
    LOCAL_MODULE_CLASS := APPS
    LOCAL_MODULE_SUFFIX := $$(COMMON_ANDROID_PACKAGE_SUFFIX)
    LOCAL_CERTIFICATE := platform
    LOCAL_SRC_FILES := $$(LOCAL_MODULE).apk
    LOCAL_REQUIRED_MODULES := $(2)
    include $$(BUILD_PREBUILT)
endef

define PREBUILT_APP_template
    include $$(CLEAR_VARS)
    LOCAL_MODULE_TAGS := optional
    ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
    LOCAL_PROPRIETARY_MODULE := true
    endif
    $(call PREBUILT_template, $(1), $(2))
endef

prebuilt_apps := \
    FactoryTest \
    RC_Service \
    ReadLog \
    BluetoothRemote

ifneq ($(wildcard packages/apps/TV/Android.mk),packages/apps/TV/Android.mk)
prebuilt_apps += \
    LiveTv
endif


$(foreach app,$(prebuilt_apps), \
    $(eval $(call PREBUILT_APP_template, $(app),)))

include $(call all-makefiles-under,$(LOCAL_PATH))

