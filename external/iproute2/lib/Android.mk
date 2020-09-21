LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    color.c utils.c rt_names.c ll_types.c ll_proto.c ll_addr.c inet_proto.c \
    mpls_pton.c namespace.c names.c libgenl.c libnetlink.c bpf.c exec.c fs.c \
    json_print.c json_writer.c
LOCAL_MODULE := libiprouteutil
LOCAL_SYSTEM_SHARED_LIBRARIES := libc

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include $(UAPI_INCLUDES)
LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DCONFDIR=\"/data/misc/net\" \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_SETNS \
	-D_GNU_SOURCE \
	-Wno-pointer-arith \
	-Wno-sign-compare \
	-Wno-unused-parameter \
	-Werror

# This is a work around for b/18403920
LOCAL_LDFLAGS := -Wl,--no-gc-sections

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ll_map.c libnetlink.c
LOCAL_MODULE := libnetlink
LOCAL_SYSTEM_SHARED_LIBRARIES := libc

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include $(UAPI_INCLUDES)
LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-Wno-pointer-arith \
	-Wno-sign-compare \
	-Wno-unused-parameter \
	-Werror

include $(BUILD_SHARED_LIBRARY)
