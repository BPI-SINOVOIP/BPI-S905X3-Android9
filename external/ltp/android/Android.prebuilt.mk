#
# Copyright (C) 2016 The Android Open Source Project
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

include $(CLEAR_VARS)

module_name := ltp_$(subst /,_,$(module_prebuilt))
module_stem := $(notdir $(module_prebuilt))
module_path := $(patsubst %/,%,$(dir $(module_prebuilt)))

LOCAL_MODULE := $(module_name)
LOCAL_INSTALLED_MODULE_STEM := $(module_stem)
LOCAL_PREBUILT_MODULE_FILE := $(local_ltp_root)/$(module_src_files)
LOCAL_MODULE_RELATIVE_PATH := ltp/$(module_path)
LOCAL_MODULE_CLASS := NATIVE_TESTS
LOCAL_MULTILIB := both

include $(BUILD_PREBUILT)

module_name :=
module_stem :=
module_path :=
