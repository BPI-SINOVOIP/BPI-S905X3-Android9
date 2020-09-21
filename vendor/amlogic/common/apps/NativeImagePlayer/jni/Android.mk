LOCAL_PATH := $(call my-dir)

### shared library

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	com_droidlogic_imageplayer_SurfaceOverlay.cpp

$(warning $(JNI_H_INCLUDE))
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) \
                    libnativehelper/include/nativehelper \
                    frameworks/base/core/jni/include \
                    frameworks/base/libs/hwui \
                    frameworks/native/libs/nativewindow \
                    external/skia/include/core \
                    hardware/amlogic/gralloc \
                    $(call include-path-for, libhardware)/hardware \
                    $(call include-path-for, libhardware_legacy)/hardware_legacy
LOCAL_MODULE := libsurfaceoverlay_jni
LOCAL_SHARED_LIBRARIES := libcutils \
                          libutils \
                          libgui \
                          libandroid_runtime \
                          liblog \
                          libhardware \
                          libhardware_legacy \
                          libnativehelper

LOCAL_STATIC_LIBRARIES := libamgralloc_ext_static
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_PRODUCT_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true
include $(BUILD_SHARED_LIBRARY)
