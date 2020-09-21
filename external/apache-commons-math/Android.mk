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

apache-commons-math_src_files := $(call all-java-files-under,src/main/java)

# build target jar
# ============================================================

include $(CLEAR_VARS)
LOCAL_MODULE := apache-commons-math
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(apache-commons-math_src_files)
LOCAL_JAVACFLAGS := -encoding UTF-8
LOCAL_SDK_VERSION := current
LOCAL_ERROR_PRONE_FLAGS := -Xep:MissingOverride:OFF
include $(BUILD_STATIC_JAVA_LIBRARY)

# build host jar
# ============================================================

include $(CLEAR_VARS)
LOCAL_MODULE := apache-commons-math-host
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(apache-commons-math_src_files)
LOCAL_JAVACFLAGS := -encoding UTF-8
LOCAL_SDK_VERSION := current
LOCAL_JAVA_LANGUAGE_VERSION := 1.7
LOCAL_ERROR_PRONE_FLAGS := -Xep:MissingOverride:OFF
include $(BUILD_HOST_JAVA_LIBRARY)
