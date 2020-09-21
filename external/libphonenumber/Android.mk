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

LOCAL_PATH := $(call my-dir)

libphonenumber_test_files := \
    $(call all-java-files-under, carrier/test) \
    $(call all-java-files-under, geocoder/test) \
    $(call all-java-files-under, internal/prefixmapper/test) \
    $(call all-java-files-under, libphonenumber/test)

libphonenumber_test_resource_dirs := \
    carrier/test \
    geocoder/test \
    libphonenumber/test

# Tests for unbundled use.
# vogar --timeout 0 \
    --classpath out/target/common/obj/JAVA_LIBRARIES/libphonenumber-test_intermediates/classes.jack \
    com.google.i18n.phonenumbers
include $(CLEAR_VARS)
LOCAL_MODULE := libphonenumber-test
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(libphonenumber_test_files)
LOCAL_JAVA_RESOURCE_DIRS := $(libphonenumber_test_resource_dirs)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_SDK_VERSION := current
LOCAL_STATIC_JAVA_LIBRARIES := libphonenumber junit
LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_STATIC_JAVA_LIBRARY)
