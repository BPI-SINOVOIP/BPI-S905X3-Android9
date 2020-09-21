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

all_wycheproof_files := $(call all-java-files-under,java)
supported_wycheproof_files := $(filter-out %SpongyCastleTest.java %SpongyCastleAllTests.java,$(all_wycheproof_files))

include $(CLEAR_VARS)
LOCAL_MODULE := wycheproof
LOCAL_MODULE_TAGS := optional
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_SRC_FILES := $(supported_wycheproof_files)
LOCAL_JAVA_LIBRARIES := core-oj core-libart conscrypt bouncycastle junit
include $(BUILD_STATIC_JAVA_LIBRARY)

all_wycheproof_files :=
