# Explicitly list the bionic UAPI includes so we don't pick up stray
# vendor copies of the UAPI includes that are too old for us to build.
UAPI_INCLUDES := bionic/libc/kernel/uapi

include $(call all-subdir-makefiles)
