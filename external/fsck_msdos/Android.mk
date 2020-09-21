LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := boot.c check.c dir.c fat.c main.c

LOCAL_C_INCLUDES := external/fsck_msdos/

LOCAL_CFLAGS := -O2 -g \
    -Wall -Werror \
    -D_BSD_SOURCE \
    -D_LARGEFILE_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    -Wno-unused-variable \
    -Wno-unused-const-variable \
    -Wno-format \
    -Wno-sign-compare

LOCAL_MODULE := fsck_msdos
LOCAL_MODULE_TAGS :=
LOCAL_SYSTEM_SHARED_LIBRARIES := libc

include $(BUILD_EXECUTABLE)
