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
LOCAL_PATH:= $(call my-dir)

jack_jar_tools := $(LOCAL_PATH)/jack-jar-tools.jar
jack_eng_jar := $(LOCAL_PATH)/jacks/jack.jar

JACK_STABLE_VERSION := 4.32.CANDIDATE
JACK_DOGFOOD_VERSION := 4.32.CANDIDATE
JACK_SDKTOOL_VERSION := 4.32.CANDIDATE
JACK_LANG_DEV_VERSION := 4.32.CANDIDATE
ifneq ("$(wildcard $(jack_eng_jar))","")
JACK_ENGINEERING_VERSION := $(shell $(JAVA) -jar $(jack_jar_tools) --version-code jack $(jack_eng_jar))
endif

ifneq ($(ANDROID_JACK_DEFAULT_VERSION),)
JACK_DEFAULT_VERSION := $(JACK_$(ANDROID_JACK_DEFAULT_VERSION)_VERSION)
ifeq ($(JACK_DEFAULT_VERSION),)
$(error "$(ANDROID_JACK_DEFAULT_VERSION)" is an invalid value for ANDROID_JACK_DEFAULT_VERSION)
endif
JACK_APPS_VERSION := $(ANDROID_JACK_DEFAULT_VERSION)
else
ifneq (,$(TARGET_BUILD_APPS))
# Unbundled branches
JACK_DEFAULT_VERSION := $(JACK_STABLE_VERSION)
else
# Complete android tree
JACK_DEFAULT_VERSION := $(JACK_STABLE_VERSION)
endif
JACK_APPS_VERSION := STABLE
endif
