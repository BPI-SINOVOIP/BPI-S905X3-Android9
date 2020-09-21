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
#

#disble build in PDK, missing aidl import breaks build
ifneq ($(TARGET_BUILD_PDK),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE := com.android.car.obd2
LOCAL_JAVA_LANGUAGE_VERSION := 1.8

LOCAL_STATIC_JAVA_LIBRARIES += vehicle-hal-support-lib \

include $(BUILD_STATIC_JAVA_LIBRARY)

endif #TARGET_BUILD_PDK
