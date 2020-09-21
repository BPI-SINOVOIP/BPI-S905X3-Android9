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

spec_copy_pairs :=
$(foreach m,$(VTS_SPEC_FILE_LIST),\
  $(eval spec_copy_dir :=\
    spec/$(word 2,$(subst android/frameworks/, ,\
                    $(subst android/hardware/, ,\
                      $(subst android/hidl/, ,\
                        $(subst android/system/, ,$(dir $(m))))))))\
  $(eval spec_copy_file := $(notdir $(m)))\
  $(eval spec_copy_dest := $(spec_copy_dir)/$(spec_copy_file))\
  $(eval spec_copy_pairs += $(m):$(TARGET_OUT_DATA)/vts_specs/$(spec_copy_dest)))

.PHONY: vts_specs
vts_specs: \
  $(call copy-many-files,$(spec_copy_pairs)) \
