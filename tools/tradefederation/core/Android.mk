# Copyright (C) 2010 The Android Open Source Project
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
COMPATIBILITY.tradefed_tests_dir := \
  $(COMPATIBILITY.tradefed_tests_dir) $(LOCAL_PATH)/res/config $(LOCAL_PATH)/tests/res/config $(LOCAL_PATH)/prod-tests/res/config

include $(CLEAR_VARS)
# Module to compile protos for tradefed
LOCAL_MODULE := tradefed-protos
LOCAL_SRC_FILES := $(call all-proto-files-under, proto)
LOCAL_JAVA_LIBRARIES := host-libprotobuf-java-full
LOCAL_PROTOC_OPTIMIZE_TYPE := full
LOCAL_PROTOC_FLAGS := \
    -Iexternal/protobuf/src
LOCAL_SOURCE_FILES_ALL_GENERATED := true
LOCAL_MODULE_TAGS := optional

# If the real one isn't present, use the prebuilt platformprotos library.
ifeq ($(wildcard frameworks/base/Android.mk),)
  LOCAL_STATIC_JAVA_LIBRARIES += platformprotos-prebuilt
else
  LOCAL_STATIC_JAVA_LIBRARIES += platformprotos
endif

include $(BUILD_HOST_JAVA_LIBRARY)

include $(CLEAR_VARS)

# Only compile source java files in this lib.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_JAVA_RESOURCE_DIRS := res

LOCAL_JAVACFLAGS += -g -Xlint
ifdef TARGET_OPENJDK9
LOCAL_JAVACFLAGS += --add-modules=java.xml.bind
endif

-include tools/tradefederation/core/error_prone_rules.mk

LOCAL_MODULE := tradefed

LOCAL_STATIC_JAVA_LIBRARIES := junit-host junit-params-host kxml2-2.3.0 jline-1.0 tf-remote-client commons-compress-prebuilt host-libprotobuf-java-full tradefed-protos error_prone_annotations-2.0.18 longevity-host-lib gson-prebuilt-jar guice

# emmalib is only a runtime dependency if generating code coverage reporters,
# not a compile time dependency
LOCAL_JAVA_LIBRARIES := loganalysis tools-common-prebuilt

LOCAL_JAR_MANIFEST := MANIFEST.mf

include $(BUILD_HOST_JAVA_LIBRARY)

# makefile rules to copy jars to HOST_OUT/tradefed
# so tradefed.sh can automatically add to classpath
DEST_JAR := $(HOST_OUT)/tradefed/$(LOCAL_MODULE).jar
$(DEST_JAR): $(LOCAL_BUILT_MODULE)
	$(copy-file-to-new-target)

$(HOST_OUT)/tradefed/%.jar : $(HOST_OUT_JAVA_LIBRARIES)/%.jar
	$(copy-file-to-new-target)

# this dependency ensure the above rule will be executed if jar is built
$(LOCAL_INSTALLED_MODULE) : $(DEST_JAR)
$(LOCAL_INSTALLED_MODULE) : $(foreach m, $(LOCAL_JAVA_LIBRARIES), $(HOST_OUT)/tradefed/$(m).jar)

#######################################################
# intentionally skipping CLEAR_VARS
# Enable the build process to generate javadoc
# We need to reference symbols in the jar built above.

# ==== tradefed-docs: devsite-compatible reference docs for source.android.com
LOCAL_JAVA_LIBRARIES += tradefed
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_ADDITIONAL_DEPENDENCIES := tradefed
LOCAL_DROIDDOC_CUSTOM_TEMPLATE_DIR := external/doclava/res/assets/templates-sdk
LOCAL_DROIDDOC_OPTIONS := \
        -hdf sac true \
        -hdf devices true \
        -hdf android.whichdoc online \
        -hdf css.path /reference/assets/css/doclava-devsite.css \
        -hdf book.root toc \
        -hdf book.path /reference/tradefed/_book.yaml \
        -yaml _book.yaml \
        -apidocsdir reference/tradefed/ \
        -werror \
        -package \
        -devsite \

include $(BUILD_DROIDDOC)

#######################################################
include $(CLEAR_VARS)

# Create a simple alias to build all the TF-related targets
# Note that this is incompatible with `make dist`.  If you want to make
# the distribution, you must run `tapas` with the individual target names.
.PHONY: tradefed-core
tradefed-core: tradefed atest_tradefed tf-prod-tests tf-prod-metatests tradefed-contrib script_help

.PHONY: tradefed-all
tradefed-all: tradefed-core tradefed-tests tradefed_win verify loganalysis-tests

# ====================================
include $(CLEAR_VARS)
# copy tradefed.sh script to host dir

LOCAL_MODULE_TAGS := optional

LOCAL_PREBUILT_EXECUTABLES := tradefed.sh tradefed_win.bat script_help.sh verify.sh run_tf_cmd.sh atest_tradefed.sh
include $(BUILD_HOST_PREBUILT)

# Build all sub-directories
include $(call all-makefiles-under,$(LOCAL_PATH))

########################################################
# Zip up the built files and dist it as tradefed.zip
ifneq (,$(filter tradefed tradefed-all, $(TARGET_BUILD_APPS)))

tradefed_dist_host_jars := tradefed tradefed-tests tf-prod-tests tf-prod-metatests emmalib jack-jacoco-reporter loganalysis loganalysis-tests tf-remote-client tradefed-contrib
tradefed_dist_host_jar_files := $(foreach m, $(tradefed_dist_host_jars), $(HOST_OUT_JAVA_LIBRARIES)/$(m).jar)

tradefed_dist_host_exes := tradefed.sh tradefed_win.bat script_help.sh verify.sh run_tf_cmd.sh atest_tradefed.sh
tradefed_dist_host_exe_files := $(foreach m, $(tradefed_dist_host_exes), $(BUILD_OUT_EXECUTABLES)/$(m))

tradefed_dist_test_apks := TradeFedUiTestApp TradeFedTestApp DeviceSetupUtil
tradefed_dist_test_apk_files := $(foreach m, $(tradefed_dist_test_apks), $(TARGET_OUT_DATA_APPS)/$(m)/$(m).apk)

tradefed_dist_files := \
    $(tradefed_dist_host_jar_files) \
    $(tradefed_dist_test_apk_files) \
    $(tradefed_dist_host_exe_files)

tradefed_dist_intermediates := $(call intermediates-dir-for,PACKAGING,tradefed_dist,HOST,COMMON)
tradefed_dist_zip := $(tradefed_dist_intermediates)/tradefed.zip
$(tradefed_dist_zip) : $(tradefed_dist_files)
	@echo "Package: $@"
	$(hide) rm -rf $(dir $@) && mkdir -p $(dir $@)
	$(hide) cp -f $^ $(dir $@)
	$(hide) cd $(dir $@) && zip -q $(notdir $@) $(notdir $^)

$(call dist-for-goals, apps_only, $(tradefed_dist_zip))

endif  # tradefed in $(TARGET_BUILD_APPS)
