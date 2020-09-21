LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := checkseapp
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLINK_SEPOL_STATIC -Wall -Werror
LOCAL_SRC_FILES := check_seapp.c
LOCAL_STATIC_LIBRARIES := libsepol
LOCAL_WHOLE_STATIC_LIBRARIES := libpcre2
LOCAL_CXX_STL := none

include $(BUILD_HOST_EXECUTABLE)

###################################
include $(CLEAR_VARS)

LOCAL_MODULE := checkfc
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall -Werror
LOCAL_SRC_FILES := checkfc.c
LOCAL_STATIC_LIBRARIES := libsepol libselinux
LOCAL_CXX_STL := none

include $(BUILD_HOST_EXECUTABLE)

##################################
include $(CLEAR_VARS)

LOCAL_MODULE := insertkeys.py
LOCAL_SRC_FILES := insertkeys.py
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional

include $(BUILD_PREBUILT)
###################################
include $(CLEAR_VARS)

LOCAL_MODULE := sepolicy-check
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall -Werror
LOCAL_SRC_FILES := sepolicy-check.c
LOCAL_STATIC_LIBRARIES := libsepol
LOCAL_CXX_STL := none

include $(BUILD_HOST_EXECUTABLE)

###################################
include $(CLEAR_VARS)

LOCAL_MODULE := version_policy
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall -Werror
LOCAL_SRC_FILES := version_policy.c
LOCAL_SHARED_LIBRARIES := libsepol
LOCAL_CXX_STL := none

include $(BUILD_HOST_EXECUTABLE)


include $(call all-makefiles-under,$(LOCAL_PATH))
