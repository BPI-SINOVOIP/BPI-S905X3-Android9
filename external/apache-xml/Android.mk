#
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
#
LOCAL_PATH := $(call my-dir)

apache-xml_src_files := $(call all-java-files-under,src/main/java)

include $(CLEAR_VARS)
LOCAL_MODULE := apache-xml
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(apache-xml_src_files)
LOCAL_JAVA_LIBRARIES := core-oj core-libart
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_RESOURCE_DIRS := src/main/java
LOCAL_ERROR_PRONE_FLAGS := -Xep:MissingOverride:OFF
include $(BUILD_JAVA_LIBRARY)

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_MODULE := apache-xml-hostdex
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(apache-xml_src_files)
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_RESOURCE_DIRS := src/main/java
LOCAL_ERROR_PRONE_FLAGS := -Xep:MissingOverride:OFF
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif  # HOST_OS == linux
