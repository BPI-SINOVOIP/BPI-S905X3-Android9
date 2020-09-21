# Copyright (c) 2016, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): MY_TOOL := $(HOST_OUT)/bin/domainGenerator.py
$(LOCAL_BUILT_MODULE): MY_TOPLEVEL_FILE := $(PFW_TOPLEVEL_FILE)
$(LOCAL_BUILT_MODULE): MY_CRITERIA_FILE := $(PFW_CRITERIA_FILE)
$(LOCAL_BUILT_MODULE): MY_TUNING_FILE := $(PFW_TUNING_FILE)
$(LOCAL_BUILT_MODULE): MY_EDD_FILES := $(PFW_EDD_FILES)
$(LOCAL_BUILT_MODULE): MY_DOMAIN_FILES := $(PFW_DOMAIN_FILES)
$(LOCAL_BUILT_MODULE): MY_SCHEMAS_DIR := $(PFW_SCHEMAS_DIR)

$(LOCAL_BUILT_MODULE): $(LOCAL_ADDITIONAL_DEPENDENCIES) $(HOST_OUT)/bin/domainGenerator.py
	$(hide) mkdir -p "$(dir $@)"

	"$(MY_TOOL)" --validate \
		--toplevel-config "$(MY_TOPLEVEL_FILE)" \
		--criteria "$(MY_CRITERIA_FILE)" \
		--initial-settings $(MY_TUNING_FILE) \
		--add-edds $(MY_EDD_FILES) \
		--add-domains $(MY_DOMAIN_FILES) \
		--schemas-dir $(MY_SCHEMAS_DIR) > "$@"

# Clear variables for further use
PFW_TOPLEVEL_FILE :=
PFW_CRITERIA_FILE :=
PFW_TUNING_FILE :=
PFW_EDD_FILES :=
PFW_DOMAIN_FILES :=
PFW_SCHEMAS_DIR := $(PFW_DEFAULT_SCHEMAS_DIR)
