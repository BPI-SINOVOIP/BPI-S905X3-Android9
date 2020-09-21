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
#

LOCAL_PATH := $(call my-dir)

# Defines a test module.
#
# The upstream gmock configuration builds each of these as separate executables.
# It's a pain for how we run tests in the platform, but we can handle that with
# a test running script.
#
# $(1): Test name. test/$(1).cc will automatically be added to sources.
# $(2): Additional source files.
# $(3): "libgmock_main" or empty.
# $(4): Variant. Can be "_host", "_ndk", or empty.
define gmock-unit-test
    $(eval include $(CLEAR_VARS)) \
    $(eval LOCAL_MODULE := $(1)$(4)) \
    $(eval LOCAL_CPP_EXTENSION := .cc) \
    $(eval LOCAL_SRC_FILES := test/$(strip $(1)).cc $(2)) \
    $(eval LOCAL_C_INCLUDES := $(LOCAL_PATH)/include) \
    $(eval LOCAL_C_INCLUDES += $(LOCAL_PATH)/../googletest) \
    $(eval LOCAL_CFLAGS += -Wall -Werror -Wno-sign-compare -Wno-unused-parameter) \
    $(eval LOCAL_CFLAGS += -Wno-unused-private-field) \
    $(eval LOCAL_CPP_FEATURES := rtti) \
    $(eval LOCAL_STATIC_LIBRARIES := $(if $(3),$(3)$(4)) libgmock$(4)) \
    $(eval LOCAL_STATIC_LIBRARIES += libgtest$(4)) \
    $(if $(findstring _ndk,$(4)),$(eval LOCAL_SDK_VERSION := 9)) \
    $(eval LOCAL_NDK_STL_VARIANT := c++_static) \
    $(if $(findstring _host,$(4)),,\
        $(eval LOCAL_MODULE_PATH := $(TARGET_OUT_DATA_NATIVE_TESTS))) \
    $(eval $(if $(findstring _host,$(4)), \
        include $(BUILD_HOST_EXECUTABLE), \
        include $(BUILD_EXECUTABLE)))
endef

# Create modules for each test in the suite.
#
# $(1): Variant. Can be "_host", "_ndk", or empty.
define gmock-test-suite
    $(eval $(call gmock-unit-test,gmock-actions_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-cardinalities_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-generated-actions_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-generated-function-mockers_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-generated-internal-utils_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-generated-matchers_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-internal-utils_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-matchers_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-more-actions_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-nice-strict_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-port_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock-spec-builders_test,,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock_link_test,test/gmock_link2_test.cc,libgmock_main,$(1))) \
    $(eval $(call gmock-unit-test,gmock_test,,libgmock_main,$(1)))
endef

# Test is disabled because Android doesn't build gmock with exceptions.
# $(eval $(call gmock-unit-test,gmock_ex_test,,libgmock_main,$(1)))

$(call gmock-test-suite,)
$(call gmock-test-suite,_host)
