LOCAL_PATH:= $(call my-dir)

########################
# Complete DocumentsUI app:
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := DocumentsUI
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res
LOCAL_FULL_MANIFEST_FILE := $(LOCAL_PATH)/AndroidManifest.xml

include $(LOCAL_PATH)/build_apk.mk

########################
# Minimal DocumentsUI app (supports Scoped Directory Access only):
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        src/com/android/documentsui/ScopedAccessActivity.java \
        src/com/android/documentsui/ScopedAccessPackageReceiver.java \
        src/com/android/documentsui/ScopedAccessProvider.java \
        src/com/android/documentsui/ScopedAccessMetrics.java \
        src/com/android/documentsui/archives/Archive.java \
        src/com/android/documentsui/archives/ArchiveId.java \
        src/com/android/documentsui/archives/ArchivesProvider.java \
        src/com/android/documentsui/archives/Loader.java \
        src/com/android/documentsui/archives/Proxy.java \
        src/com/android/documentsui/archives/ReadableArchive.java \
        src/com/android/documentsui/archives/WriteableArchive.java \
        src/com/android/documentsui/base/Providers.java \
        src/com/android/documentsui/base/SharedMinimal.java \
        src/com/android/documentsui/prefs/ScopedAccessLocalPreferences.java

LOCAL_PACKAGE_NAME := DocumentsUIMinimal
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/minimal/res
LOCAL_FULL_MANIFEST_FILE := $(LOCAL_PATH)/minimal/AndroidManifest.xml

include $(LOCAL_PATH)/build_apk.mk

# Include makefiles for tests and libraries under the current path
include $(call all-makefiles-under, $(LOCAL_PATH))
