LOCAL_PATH := $(call my-dir)

### shared library

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    org_droidlogic_dtvkit_DtvkitGlueClient.cpp

$(warning $(JNI_H_INCLUDE))
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) \
                    libnativehelper/include/nativehelper \
                    frameworks/base/core/jni/include \
                    frameworks/base/libs/hwui \
                    frameworks/native/libs/nativewindow \
                    external/skia/include/core \
                    hardware/amlogic/gralloc \

DVBCORE_DIRECTORY := $(LOCAL_PATH)/../../../../DVBCore/
HAVE_DVBCORE_DIRECTORY := $(shell test -d $(DVBCORE_DIRECTORY) && echo yes)
ifeq ($(HAVE_DVBCORE_DIRECTORY),yes)
$(warning "DVBCore directory exist")
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../android-rpcservice/apps/binder/inc
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../releaseDTVKit/rpcservice
endif

LOCAL_MODULE := libdtvkit_jni
LOCAL_SHARED_LIBRARIES :=  \
    libcutils \
    libutils \
    libgui \
    libandroid_runtime \
    liblog \
    libhardware \
    libhardware_legacy \
    libnativehelper \
    libdtvkitserver

LOCAL_STATIC_LIBRARIES := libamgralloc_ext_static@2
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_PRODUCT_MODULE := true

LOCAL_PRIVATE_PLATFORM_APIS := true
include $(BUILD_SHARED_LIBRARY)
