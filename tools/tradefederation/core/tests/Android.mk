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

include $(CLEAR_VARS)

# Only compile source java files in this lib.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_JAVA_RESOURCE_DIRS := res

LOCAL_JAVACFLAGS += -g -Xlint
-include tools/tradefederation/core/error_prone_rules.mk

LOCAL_MODULE := tradefed-tests
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := easymock objenesis-host mockito-host truth-host-prebuilt
LOCAL_JAVA_LIBRARIES := tradefed host-libprotobuf-java-full

LOCAL_JAR_MANIFEST := MANIFEST.mf

include $(BUILD_HOST_JAVA_LIBRARY)

# makefile rules to copy jars to HOST_OUT/tradefed
# so tradefed.sh can automatically add to classpath

DEST_JAR := $(HOST_OUT)/tradefed/$(LOCAL_MODULE).jar
$(DEST_JAR): $(LOCAL_BUILT_MODULE)
	$(copy-file-to-new-target)

# this dependency ensure the above rule will be executed if module is built
$(LOCAL_INSTALLED_MODULE) : $(DEST_JAR)

# Build all sub-directories
include $(call all-makefiles-under,$(LOCAL_PATH))
