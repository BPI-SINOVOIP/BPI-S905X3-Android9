LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbackup
LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current

EXCLUDE_BACKUP_LIB_SRCS := $(call all-java-files-under, src/com/google/android/libraries/backup/shadow/)

LOCAL_SRC_FILES := $(call all-java-files-under,.)
LOCAL_SRC_FILES := $(filter-out $(EXCLUDE_BACKUP_LIB_SRCS),$(LOCAL_SRC_FILES))

LOCAL_STATIC_ANDROID_LIBRARIES := android-support-v4 \

include $(BUILD_STATIC_JAVA_LIBRARY)

EXCLUDE_BACKUP_LIB_SRCS :=
