#
# Copyright (C) 2015 The Android Open Source Project
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

# -O0 is need to prevent optimizations like function inlining in runtest executables.
simpleperf_runtest_cppflags := -Wall -Wextra -Werror -Wunused \
                               -O0 \

include $(CLEAR_VARS)
LOCAL_CPPFLAGS := $(simpleperf_runtest_cppflags)
LOCAL_SRC_FILES := $(module_src_files)
LOCAL_SHARED_LIBRARIES := libsimpleperf_inplace_sampler
LOCAL_MODULE := $(module)
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(module)32
LOCAL_MODULE_STEM_64 := $(module)64
LOCAL_STRIP_MODULE := false
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.build.mk
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS := $(simpleperf_runtest_cppflags)
LOCAL_SRC_FILES := $(module_src_files)
LOCAL_SHARED_LIBRARIES := libsimpleperf_inplace_sampler
LOCAL_MODULE := $(module)
LOCAL_MODULE_HOST_OS := linux
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(module)32
LOCAL_MODULE_STEM_64 := $(module)64
LOCAL_LDLIBS_linux := -lrt
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.build.mk
include $(BUILD_HOST_EXECUTABLE)