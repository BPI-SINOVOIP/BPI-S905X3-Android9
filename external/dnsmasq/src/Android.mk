LOCAL_PATH := $(call my-dir)

#########################

include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  cache.c dhcp.c dnsmasq.c forward.c helper.c lease.c log.c \
                    netlink.c network.c option.c rfc1035.c rfc2131.c util.c

LOCAL_MODULE := dnsmasq

LOCAL_C_INCLUDES := external/dnsmasq/src

LOCAL_CFLAGS := -O2 -g -W -Wall -D__ANDROID__ -DIPV6 -DNO_SCRIPT -D_BSD_SOURCE \
                -Wno-unused-variable -Wno-unused-parameter -Werror
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_SHARED_LIBRARIES := libcutils liblog

include $(BUILD_EXECUTABLE)
