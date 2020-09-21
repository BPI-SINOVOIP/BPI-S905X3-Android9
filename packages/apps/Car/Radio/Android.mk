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

ifneq ($(TARGET_BUILD_PDK), true)

LOCAL_PATH := $(call my-dir)
CAR_BROADCASTRADIO_SUPPORTLIB_PATH := packages/apps/Car/libs/car-broadcastradio-support

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src) $(call all-Iaidl-files-under, src)
LOCAL_AIDL_INCLUDES := \
    $(LOCAL_PATH)/src \
    $(CAR_BROADCASTRADIO_SUPPORTLIB_PATH)/src

LOCAL_PACKAGE_NAME := CarRadioApp
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := optional

LOCAL_PRIVILEGED_MODULE := true

LOCAL_USE_AAPT2 := true

LOCAL_JAVA_LIBRARIES += android.car

LOCAL_STATIC_ANDROID_LIBRARIES += \
    android-support-car \
    android-support-constraint-layout \
    car-apps-common \
    car-broadcastradio-support \
    car-stream-ui-lib

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-arch-lifecycle-livedata \
    android-arch-persistence-db-framework \
    android-arch-persistence-db \
    android-support-constraint-layout-solver \
    bcradio-android-arch-room-common-nodeps \
    bcradio-android-arch-room-runtime-nodeps

LOCAL_ANNOTATION_PROCESSORS := \
    bcradio-android-arch-room-common-nodeps \
    bcradio-android-arch-room-compiler-nodeps \
    bcradio-android-arch-room-migration-nodeps \
    bcradio-android-support-annotations-nodeps \
    bcradio-antlr4-nodeps \
    bcradio-apache-commons-codec-nodeps \
    bcradio-auto-common-nodeps \
    bcradio-javapoet-nodeps \
    bcradio-kotlin-metadata-nodeps \
    bcradio-sqlite-jdbc-nodeps \
    guava-21.0 \
    kotlin-stdlib

LOCAL_ANNOTATION_PROCESSOR_CLASSES := \
    android.arch.persistence.room.RoomProcessor

LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_DEX_PREOPT := false

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
    bcradio-android-arch-room-runtime-nodeps:libs/android-arch/room/runtime-1.1.0-beta3.aar \
    bcradio-android-arch-room-common-nodeps:libs/android-arch/room/common-1.1.0-beta3.jar

include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)

COMMON_LIBS_PATH := ../../../../prebuilts/tools/common/m2/repository
MAVEN_LIBS_PATH := ../../../../prebuilts/maven_repo/android

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
    bcradio-android-arch-room-common-nodeps:libs/android-arch/room/common-1.1.0-beta3.jar \
    bcradio-android-arch-room-compiler-nodeps:libs/android-arch/room/compiler-1.1.0-beta3.jar \
    bcradio-android-arch-room-migration-nodeps:libs/android-arch/room/migration-1.1.0-beta3.jar \
    bcradio-android-support-annotations-nodeps:$(MAVEN_LIBS_PATH)/com/android/support/support-annotations/27.1.0/support-annotations-27.1.0.jar \
    bcradio-antlr4-nodeps:$(COMMON_LIBS_PATH)/org/antlr/antlr4/4.5.3/antlr4-4.5.3.jar \
    bcradio-apache-commons-codec-nodeps:$(COMMON_LIBS_PATH)/org/eclipse/tycho/tycho-bundles-external/0.18.1/eclipse/plugins/org.apache.commons.codec_1.4.0.v201209201156.jar \
    bcradio-auto-common-nodeps:$(COMMON_LIBS_PATH)/com/google/auto/auto-common/0.9/auto-common-0.9.jar \
    bcradio-javapoet-nodeps:$(COMMON_LIBS_PATH)/com/squareup/javapoet/1.8.0/javapoet-1.8.0.jar \
    bcradio-kotlin-metadata-nodeps:$(COMMON_LIBS_PATH)/me/eugeniomarletti/kotlin-metadata/1.2.1/kotlin-metadata-1.2.1.jar \
    bcradio-sqlite-jdbc-nodeps:$(COMMON_LIBS_PATH)/org/xerial/sqlite-jdbc/3.20.1/sqlite-jdbc-3.20.1.jar

include $(BUILD_HOST_PREBUILT)

endif
