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

LOCAL_PATH := $(call my-dir)

# Common variables
# ========================================================
defaultCppExtension := .cc
defaultCFlags := \
  -Wall -Werror \
  -Wno-unused-parameter \
  -fvisibility=hidden \
  -DUSE_TPM2 \
  -DUSE_BINDER_IPC
defaultIncludes := $(LOCAL_PATH)/..
defaultSharedLibraries := \
  libbase \
  libbinder \
  libbinderwrapper \
  libbrillo \
  libbrillo-binder \
  libchrome \
  libchrome-crypto \
  libcrypto \
  libprotobuf-cpp-lite \
  libtrunks \
  libutils

# libtpm_manager_generated
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtpm_manager_generated
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags) -fvisibility=default
LOCAL_CLANG := true
proto_include := $(call local-generated-sources-dir)/proto/$(LOCAL_PATH)/..
aidl_include := $(call local-generated-sources-dir)/aidl-generated/include
LOCAL_C_INCLUDES := $(proto_include) $(aidl_include) $(defaultIncludes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(proto_include) $(aidl_include)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries)
LOCAL_SRC_FILES := \
  common/tpm_manager.proto \
  aidl/android/tpm_manager/ITpmManagerClient.aidl \
  aidl/android/tpm_manager/ITpmNvram.aidl \
  aidl/android/tpm_manager/ITpmOwnership.aidl
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/aidl
include $(BUILD_STATIC_LIBRARY)

# libtpm_manager_common
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtpm_manager_common
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags) -fvisibility=default
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(defaultIncludes)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries)
LOCAL_STATIC_LIBRARIES := libtpm_manager_generated
LOCAL_SRC_FILES := \
  common/print_tpm_manager_proto.cc
include $(BUILD_STATIC_LIBRARY)

# libtpm_manager_server
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtpm_manager_server
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(defaultIncludes)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries)
LOCAL_STATIC_LIBRARIES := libtpm_manager_generated
LOCAL_SRC_FILES := \
  server/binder_service.cc \
  server/local_data_store_impl.cc \
  server/openssl_crypto_util_impl.cc \
  server/tpm2_initializer_impl.cc \
  server/tpm2_nvram_impl.cc \
  server/tpm2_status_impl.cc \
  server/tpm_manager_service.cc
include $(BUILD_STATIC_LIBRARY)

# tpm_managerd
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := tpm_managerd
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags)
LOCAL_CLANG := true
LOCAL_INIT_RC := server/tpm_managerd.rc
LOCAL_C_INCLUDES := $(defaultIncludes)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries)
LOCAL_STATIC_LIBRARIES := \
  libtpm_manager_server \
  libtpm_manager_common \
  libtpm_manager_generated
LOCAL_SRC_FILES := \
  server/main.cc

include $(BUILD_EXECUTABLE)

# libtpm_manager
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libtpm_manager
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(defaultIncludes)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries)
LOCAL_WHOLE_STATIC_LIBRARIES := \
  libtpm_manager_common \
  libtpm_manager_generated
LOCAL_SRC_FILES := \
  client/tpm_nvram_binder_proxy.cc \
  client/tpm_ownership_binder_proxy.cc
include $(BUILD_SHARED_LIBRARY)

# tpm_manager_client
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := tpm_manager_client
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(defaultIncludes)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries) libtpm_manager
LOCAL_SRC_FILES := \
  client/main.cc
include $(BUILD_EXECUTABLE)

# Target unit tests
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := tpm_manager_test
LOCAL_MODULE_TAGS := eng
LOCAL_CPP_EXTENSION := $(defaultCppExtension)
LOCAL_CFLAGS := $(defaultCFlags) -Wno-sign-compare
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(defaultIncludes)
LOCAL_SHARED_LIBRARIES := $(defaultSharedLibraries) libtpm_manager
LOCAL_SRC_FILES := \
  common/mock_tpm_nvram_interface.cc \
  common/mock_tpm_ownership_interface.cc \
  server/binder_service_test.cc \
  server/mock_local_data_store.cc \
  server/mock_openssl_crypto_util.cc \
  server/mock_tpm_initializer.cc \
  server/mock_tpm_nvram.cc \
  server/mock_tpm_status.cc \
  server/tpm2_initializer_test.cc \
  server/tpm2_nvram_test.cc \
  server/tpm2_status_test.cc \
  server/tpm_manager_service_test.cc
LOCAL_STATIC_LIBRARIES := \
  libBionicGtestMain \
  libgmock \
  libtpm_manager_common \
  libtpm_manager_generated \
  libtpm_manager_server \
  libtrunks_test
include $(BUILD_NATIVE_TEST)
