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

# Rules to generate a zip file that included test modules
# based on the configuration.
#

include $(call my-dir)/tests/platform_test_list.mk
-include $(wildcard vendor/*/build/tasks/tests/platform_test_list.mk)

my_modules := $(platform_tests)

my_package_name := platform_tests

include $(BUILD_SYSTEM)/tasks/tools/package-modules.mk

.PHONY: platform_tests
platform_tests : $(my_package_zip)

name := $(TARGET_PRODUCT)-tests-$(FILE_NAME_TAG)
$(call dist-for-goals, platform_tests, $(my_package_zip):$(name).zip)
