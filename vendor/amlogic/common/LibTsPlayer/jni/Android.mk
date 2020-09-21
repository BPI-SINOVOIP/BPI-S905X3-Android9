LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
# Also build all of the sub-targets under this one: the shared library.
include $(call all-makefiles-under,$(LOCAL_PATH))
