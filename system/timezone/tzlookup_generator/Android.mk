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

LOCAL_PATH:= $(call my-dir)

# Proto library
include $(CLEAR_VARS)
LOCAL_MODULE := countryzonesprotos
LOCAL_PROTOC_OPTIMIZE_TYPE := full
LOCAL_PROTOC_FLAGS := -Iexternal/protobuf/src
LOCAL_SOURCE_FILES_ALL_GENERATED := true
LOCAL_SRC_FILES := $(call all-proto-files-under, src/main/proto)
include $(BUILD_HOST_JAVA_LIBRARY)

# A static library for the tzlookup_generator host tool.
# The tool can be run with java -jar tzlookup_generator.jar
include $(CLEAR_VARS)
LOCAL_MODULE := tzlookup_generator
LOCAL_MODULE_TAGS := optional
LOCAL_JAR_MANIFEST := src/main/manifest/MANIFEST.mf
LOCAL_SRC_FILES := $(call all-java-files-under, src/main/java)
LOCAL_JAVACFLAGS := -encoding UTF-8
LOCAL_STATIC_JAVA_LIBRARIES := \
    icu4j-host \
    countryzonesprotos \
    libprotobuf-java-full
include $(BUILD_HOST_JAVA_LIBRARY)
