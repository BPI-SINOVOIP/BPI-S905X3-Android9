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

LOCAL_PATH := $(call my-dir)

# build dagger2 host jar
# ============================================================

include $(CLEAR_VARS)

LOCAL_MODULE := dagger2-host
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-java-files-under, core/src/main/java/)

LOCAL_STATIC_JAVA_LIBRARIES := \
  dagger2-inject-host \

LOCAL_JAVA_LIBRARIES := \
  guavalib

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# build dagger2 producers host jar
# ============================================================

include $(CLEAR_VARS)

LOCAL_MODULE := dagger2-producers-host
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_SRC_FILES := $(call all-java-files-under, producers/src/main/java/)

LOCAL_STATIC_JAVA_LIBRARIES := \
  dagger2-inject-host \

LOCAL_JAVA_LIBRARIES := \
  dagger2-host \
  guavalib

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)

# build dagger2 compiler host jar
# ============================================================

include $(CLEAR_VARS)

LOCAL_MODULE := dagger2-compiler-host
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
# Required for use of javax.annotation.Generated per http://b/62050818
LOCAL_JAVACFLAGS := $(if $(USE_OPENJDK9),-J--add-modules=java.xml.ws.annotation,)
LOCAL_SRC_FILES := $(call all-java-files-under, compiler/src/main/java/)

# Manually include META-INF/services/javax.annotation.processing.Processor
# as the AutoService processor doesn't work properly.
LOCAL_JAVA_RESOURCE_DIRS := resources

LOCAL_STATIC_JAVA_LIBRARIES := \
  dagger2-host \
  dagger2-auto-common-host \
  dagger2-auto-factory-host \
  dagger2-auto-service-host \
  dagger2-auto-value-host \
  dagger2-google-java-format \
  dagger2-inject-host \
  dagger2-producers-host \
  guavalib

LOCAL_ANNOTATION_PROCESSORS := \
  dagger2-auto-common-host \
  dagger2-auto-factory-host \
  dagger2-auto-service-host \
  dagger2-auto-value-host \
  guavalib

LOCAL_ANNOTATION_PROCESSOR_CLASSES := \
  com.google.auto.factory.processor.AutoFactoryProcessor \
  com.google.auto.service.processor.AutoServiceProcessor \
  com.google.auto.value.processor.AutoAnnotationProcessor \
  com.google.auto.value.processor.AutoValueProcessor

LOCAL_JAVA_LANGUAGE_VERSION := 1.7
include $(BUILD_HOST_JAVA_LIBRARY)
