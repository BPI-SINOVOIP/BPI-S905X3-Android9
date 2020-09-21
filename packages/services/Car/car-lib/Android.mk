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
#

#disble build in PDK, missing aidl import breaks build
ifneq ($(TARGET_BUILD_PDK),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := android.car
LOCAL_MODULE_TAGS := optional

ifneq ($(TARGET_USES_CAR_FUTURE_FEATURES),true)
#TODO need a tool to generate proguard rule to drop all items under @FutureFeature
#LOCAL_PROGUARD_ENABLED := custom
#LOCAL_PROGUARD_FLAG_FILES := proguard_drop_future.flags
endif

car_lib_sources := $(call all-java-files-under, src)
ifeq ($(TARGET_USES_CAR_FUTURE_FEATURES),true)
car_lib_sources += $(call all-java-files-under, src_feature_future)
else
car_lib_sources += $(call all-java-files-under, src_feature_current)
endif

car_lib_sources += $(call all-Iaidl-files-under, src)

# IoStats* are parcelable types (vs. interface types), but the build system uses an initial
# I as a magic marker to mean "interface", and due to this ends up refusing to compile
# these files as part of the build process.
# A clean solution to this is actively being worked on by the build team, but is not yet
# available, so for now we just filter the files out by hand.
car_lib_sources := $(filter-out src/android/car/storagemonitoring/IoStats.aidl,$(car_lib_sources))
car_lib_sources := $(filter-out src/android/car/storagemonitoring/IoStatsEntry.aidl,$(car_lib_sources))

LOCAL_AIDL_INCLUDES += system/bt/binder

LOCAL_SRC_FILES := $(car_lib_sources)

ifeq ($(EMMA_INSTRUMENT_FRAMEWORK),true)
LOCAL_EMMA_INSTRUMENT := true
endif

include $(BUILD_JAVA_LIBRARY)

ifeq ($(BOARD_IS_AUTOMOTIVE), true)
$(call dist-for-goals,dist_files,$(full_classes_jar):$(LOCAL_MODULE).jar)
endif

# API Check
# ---------------------------------------------
car_module := $(LOCAL_MODULE)
car_module_src_files := $(LOCAL_SRC_FILES)
car_module_api_dir := $(LOCAL_PATH)/api
car_module_java_libraries := framework
car_module_include_systemapi := true
car_module_java_packages := android.car*
include $(CAR_API_CHECK)

# Build stubs jar for target android-support-car
# ---------------------------------------------
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_JAVA_LIBRARIES := android.car

LOCAL_ADDITIONAL_JAVA_DIR := $(call intermediates-dir-for,JAVA_LIBRARIES,android.car,,COMMON)/src

android_car_stub_packages := \
    android.car*

android_car_api := \
    $(TARGET_OUT_COMMON_INTERMEDIATES)/PACKAGING/android.car_api.txt

# Note: The make target is android.car-stub-docs
LOCAL_MODULE := android.car-stub
LOCAL_DROIDDOC_OPTIONS := \
    -stubs $(call intermediates-dir-for,JAVA_LIBRARIES,android.car-stubs,,COMMON)/src \
    -stubpackages $(subst $(space),:,$(android_car_stub_packages)) \
    -api $(android_car_api) \
    -nodocs

LOCAL_DROIDDOC_SOURCE_PATH := $(LOCAL_PATH)/java/
LOCAL_DROIDDOC_HTML_DIR :=

LOCAL_MODULE_CLASS := JAVA_LIBRARIES

LOCAL_UNINSTALLABLE_MODULE := true

include $(BUILD_DROIDDOC)

$(android_car_api): $(full_target)

android.car-stubs_stamp := $(full_target)

###############################################
# Build the stubs java files into a jar. This build rule relies on the
# stubs_stamp make variable being set from the droiddoc rule.

include $(CLEAR_VARS)

# CAR_API_CHECK uses the same name to generate a module, but BUILD_DROIDDOC
# appends "-docs" to module name.
LOCAL_MODULE := android.car-stubs
LOCAL_SOURCE_FILES_ALL_GENERATED := true

# Make sure to run droiddoc first to generate the stub source files.
LOCAL_ADDITIONAL_DEPENDENCIES := $(android.car-stubs_stamp)

include $(BUILD_STATIC_JAVA_LIBRARY)

android.car-stubs_stamp :=
android_car_stub_packages :=
android_car_api :=

include $(call all-makefiles-under,$(LOCAL_PATH))

endif #TARGET_BUILD_PDK
