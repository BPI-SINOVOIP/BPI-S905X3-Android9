#
# Copyright (C) 2016 The Android Open Source Project
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

archive-patcher_shared_src_files := $(call all-java-files-under,shared/src/main/java)
archive-patcher_applier_src_files := $(call all-java-files-under,applier/src/main/java)

include $(CLEAR_VARS)
LOCAL_MODULE := archive-patcher
LOCAL_SRC_FILES := $(archive-patcher_shared_src_files) $(archive-patcher_applier_src_files)
# archive-patcher should be compatible with all versions of Android
LOCAL_SDK_VERSION := 4
LOCAL_JAVA_LANGUAGE_VERSION := 1.6
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(archive-patcher_shared_src_files) $(archive-patcher_applier_src_files)
LOCAL_MODULE := archive-patcher-hostdex
# archive-patcher should be compatible with all versions of Android
LOCAL_SDK_VERSION := 4
LOCAL_JAVA_LANGUAGE_VERSION := 1.6
include $(BUILD_HOST_DALVIK_STATIC_JAVA_LIBRARY)
