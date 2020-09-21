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
LOCAL_PATH := $(call my-dir)

mockftpserver_src_files := $(call all-java-files-under,MockFtpServer/src/main/java)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(mockftpserver_src_files)
LOCAL_JAVA_RESOURCE_DIRS := MockFtpServer/src/main/resources
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mockftpserver
LOCAL_JAVA_LIBRARIES := slf4j-jdk14
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SDK_VERSION := core_current
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(mockftpserver_src_files)
LOCAL_JAVA_RESOURCE_DIRS := MockFtpServer/src/main/resources
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mockftpserver-hostdex
LOCAL_JAVA_LIBRARIES := slf4j-jdk14-hostdex
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_HOST_DALVIK_STATIC_JAVA_LIBRARY)
