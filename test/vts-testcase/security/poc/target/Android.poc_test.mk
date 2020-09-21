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

# Compiles a PoC test module into an executable and copies it into
# android-vts/testcases/DATA/nativetest(64)/security/poc/ directory.
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
module_name := $(notdir $(module_testname))
module_path := security/poc/$(dir $(module_testname))

LOCAL_MODULE := $(module_name)
LOCAL_MODULE_STEM := $(module_name)
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS)/$(module_path)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
    $(poc_test_src_files) \
    $(addprefix $(poc_test_dir)/,$(module_src_files)) \

LOCAL_CFLAGS := \
    $(poc_test_cflags) \
    $(module_cflags) \

LOCAL_C_INCLUDES := \
    $(poc_test_c_includes) \
    $(addprefix $(poc_test_dir)/,$(module_c_includes)) \

LOCAL_STATIC_LIBRARIES := \
    $(poc_test_static_libraries) \
    $(module_static_libraries) \

LOCAL_SHARED_LIBRARIES := \
    $(ltp_shared_libraries) \
    $(module_shared_libraries) \

include $(BUILD_EXECUTABLE)

module_name :=
module_path :=
vts_src_file :=
