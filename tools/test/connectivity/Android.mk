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

LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)

ifeq ($(HOST_OS),linux)

# general Android Conntectivity Test Suite
ACTS_DISTRO := $(HOST_OUT)/acts-dist/acts.zip

$(ACTS_DISTRO): $(sort $(shell find $(LOCAL_PATH)/acts))
	@echo "Packaging ACTS into $(ACTS_DISTRO)"
	@mkdir -p $(HOST_OUT)/acts-dist/
	@rm -f $(HOST_OUT)/acts-dist/acts.zip
	$(hide) zip $(HOST_OUT)/acts-dist/acts.zip $(shell find tools/test/connectivity/acts/* ! -wholename "*__pycache__*")
acts: $(ACTS_DISTRO)

$(call dist-for-goals,tests,$(ACTS_DISTRO))

# Wear specific Android Connectivity Test Suite
WTS_ACTS_DISTRO_DIR := $(HOST_OUT)/wts-acts-dist
WTS_ACTS_DISTRO := $(WTS_ACTS_DISTRO_DIR)/wts-acts
WTS_ACTS_DISTRO_ARCHIVE := $(WTS_ACTS_DISTRO_DIR)/wts-acts.zip
WTS_LOCAL_ACTS_DIR := tools/test/connectivity/acts/framework/acts/

$(WTS_ACTS_DISTRO): $(SOONG_ZIP)
	@echo "Packaging WTS-ACTS into $(WTS_ACTS_DISTRO)"
	# clean-up and mkdir for dist
	@rm -Rf $(WTS_ACTS_DISTRO_DIR)
	@mkdir -p $(WTS_ACTS_DISTRO_DIR)
	# grab the files from local acts framework and zip them up
	$(hide) find $(WTS_LOCAL_ACTS_DIR) | sort >$@.list
	$(hide) $(SOONG_ZIP) -d -P acts -o $(WTS_ACTS_DISTRO_ARCHIVE) -C tools/test/connectivity/acts/framework/acts/ -l $@.list
	# add in the local wts py files for use with the prebuilt
	$(hide) zip -r $(WTS_ACTS_DISTRO_ARCHIVE) -j tools/test/connectivity/wts-acts/*.py
	# create executable tool from the archive
	$(hide) echo '#!/usr/bin/env python' | cat - $(WTS_ACTS_DISTRO_DIR)/wts-acts.zip > $(WTS_ACTS_DISTRO_DIR)/wts-acts
	$(hide) chmod 755 $(WTS_ACTS_DISTRO)

wts-acts: $(WTS_ACTS_DISTRO)

$(call dist-for-goals,tests,$(WTS_ACTS_DISTRO))



endif
