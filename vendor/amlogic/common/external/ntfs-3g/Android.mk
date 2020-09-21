ifneq ($(TARGET_SIMULATOR), true)

LOCAL_PATH := $(call my-dir)
MY_CFLAG:=  -O2 -g -W -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CONFIG_H -DANDROID
###################################################################
## For stage1, we have to make  libfuse
###################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  \
	libfuse-lite/fuse.c \
	libfuse-lite/fusermount.c \
	libfuse-lite/fuse_kern_chan.c \
	libfuse-lite/fuse_loop.c \
	libfuse-lite/fuse_lowlevel.c \
	libfuse-lite/fuse_opt.c \
	libfuse-lite/fuse_session.c \
	libfuse-lite/fuse_signals.c \
	libfuse-lite/helper.c \
	libfuse-lite/mount.c \
	libfuse-lite/mount_util.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/fuse-lite

LOCAL_CFLAGS := $(MY_CFLAG)

LOCAL_MODULE := libfuse
LOCAL_MODULE_TAGS := optional
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libcutils liblog

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif

include $(BUILD_STATIC_LIBRARY)

###################################################################
## For stage2, we have to make  libntfs-3g
###################################################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	libntfs-3g/acls.c \
	libntfs-3g/attrib.c \
	libntfs-3g/attrlist.c \
	libntfs-3g/bitmap.c \
	libntfs-3g/bootsect.c \
	libntfs-3g/cache.c \
	libntfs-3g/collate.c \
	libntfs-3g/compat.c \
	libntfs-3g/compress.c \
	libntfs-3g/debug.c \
	libntfs-3g/device.c \
	libntfs-3g/dir.c \
	libntfs-3g/efs.c \
	libntfs-3g/index.c \
	libntfs-3g/inode.c \
	libntfs-3g/lcnalloc.c \
	libntfs-3g/logfile.c \
	libntfs-3g/logging.c \
	libntfs-3g/mft.c \
	libntfs-3g/misc.c \
	libntfs-3g/mst.c \
	libntfs-3g/object_id.c \
	libntfs-3g/reparse.c \
	libntfs-3g/realpath.c \
	libntfs-3g/runlist.c \
	libntfs-3g/security.c \
	libntfs-3g/unistr.c \
	libntfs-3g/unix_io.c \
	libntfs-3g/volume.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/fuse-lite \
	$(LOCAL_PATH)/include/ntfs-3g

LOCAL_CFLAGS := $(MY_CFLAG)

LOCAL_MODULE := libntfs-3g
LOCAL_MODULE_TAGS := optional
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libcutils liblog

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif

include $(BUILD_STATIC_LIBRARY)


###################################################################
## For stage3, we make ntfs-3g
###################################################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	src/ntfs-3g.c \
	src/ntfs-3g_common.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/fuse-lite  \
	$(LOCAL_PATH)/include/ntfs-3g \
	$(LOCAL_PATH)/src

LOCAL_CFLAGS := $(MY_CFLAG)

LOCAL_MODULE := ntfs-3g
LOCAL_MODULE_TAGS := optional
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libcutils liblog
LOCAL_STATIC_LIBRARIES := libfuse libntfs-3g

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif

include $(BUILD_EXECUTABLE)



###################################################################
##
###################################################################
#include $(CLEAR_VARS)
#LOCAL_SRC_FILES := src/test.c

#LOCAL_C_INCLUDES :=

#LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/fuse-lite  $(LOCAL_PATH)/include/ntfs-3g \
	$(LOCAL_PATH)/src

#LOCAL_CFLAGS := $(MY_CFLAG)

#LOCAL_MODULE := ntfstest
#LOCAL_MODULE_TAGS := optional
#LOCAL_SYSTEM_SHARED_LIBRARIES := libc libcutils


#include $(BUILD_EXECUTABLE)


###################################################################
##
###################################################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	ntfsprogs/attrdef.c \
	ntfsprogs/boot.c \
	ntfsprogs/sd.c \
        ntfsprogs/mkntfs.c \
	ntfsprogs/utils.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/fuse-lite  \
	$(LOCAL_PATH)/include/ntfs-3g \
        $(LOCAL_PATH)/ntfsprogs

LOCAL_CFLAGS := $(MY_CFLAG)

LOCAL_MODULE := mkntfs
LOCAL_MODULE_TAGS := optional
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libcutils liblog
LOCAL_STATIC_LIBRARIES:= libntfs-3g libfuse

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif

include $(BUILD_EXECUTABLE)

###################################################################
##
###################################################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	ntfsprogs/ntfsfix.c \
	ntfsprogs/sd.c \
	ntfsprogs/utils.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/fuse-lite  \
	$(LOCAL_PATH)/include/ntfs-3g \
        $(LOCAL_PATH)/ntfsprogs

LOCAL_CFLAGS := $(MY_CFLAG)

LOCAL_MODULE := ntfsfix
LOCAL_MODULE_TAGS := optional
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libcutils liblog
LOCAL_STATIC_LIBRARIES:= libntfs-3g libfuse

#ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
#LOCAL_PROPRIETARY_MODULE := true
#endif

include $(BUILD_EXECUTABLE)

endif
