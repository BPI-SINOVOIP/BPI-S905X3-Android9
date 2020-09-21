LOCAL_PATH:= $(call my-dir)

# f2fs-tools depends on Linux kernel headers being in the system include path.
ifneq (,$filter linux darwin,$(HOST_OS))

# The versions depend on $(LOCAL_PATH)/VERSION
version_CFLAGS := -DF2FS_MAJOR_VERSION=1 -DF2FS_MINOR_VERSION=10 -DF2FS_TOOLS_VERSION=\"1.10.0\" -DF2FS_TOOLS_DATE=\"2018-01-30\"
common_CFLAGS := -DWITH_ANDROID $(version_CFLAGS) \
    -Wall -Werror \
    -Wno-format \
    -Wno-macro-redefined \
    -Wno-missing-field-initializers \
    -Wno-pointer-arith \
    -Wno-sign-compare \
    -Wno-unused-function \

# external/e2fsprogs/lib is needed for uuid/uuid.h
common_C_INCLUDES := $(LOCAL_PATH)/include \
    external/e2fsprogs/lib/ \
    system/core/libsparse/include \

#----------------------------------------------------------
include $(CLEAR_VARS)
# The LOCAL_MODULE name is referenced by the code. Don't change it.
LOCAL_MODULE := mkfs.f2fs

# mkfs.f2fs is used in recovery: must be static.
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin

LOCAL_SRC_FILES := \
    lib/libf2fs_io.c \
    mkfs/f2fs_format_main.c
LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    libf2fs_fmt \
    libext2_uuid \
    libsparse \
    libz
LOCAL_WHOLE_STATIC_LIBRARIES := libbase
include $(BUILD_EXECUTABLE)

#----------------------------------------------------------
include $(CLEAR_VARS)
# The LOCAL_MODULE name is referenced by the code. Don't change it.
LOCAL_MODULE := sload.f2fs

# mkfs.f2fs is used in recovery: must be static.
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin

LOCAL_SRC_FILES := \
    fsck/fsck.c \
    fsck/sload.c \
    fsck/dir.c \
    fsck/dict.c \
    fsck/mkquota.c \
    fsck/quotaio.c \
    fsck/quotaio_tree.c \
    fsck/quotaio_v2.c \
    fsck/node.c \
    fsck/segment.c \
    fsck/xattr.c \
    fsck/main.c \
    fsck/mount.c \
    lib/libf2fs.c \
    lib/libf2fs_io.c
LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(common_CFLAGS) -DWITH_SLOAD
LOCAL_STATIC_LIBRARIES := \
    libcutils \
    libselinux \
    libcrypto \
    libsparse \
    liblog \
    libz
LOCAL_WHOLE_STATIC_LIBRARIES := libbase
include $(BUILD_EXECUTABLE)

endif
