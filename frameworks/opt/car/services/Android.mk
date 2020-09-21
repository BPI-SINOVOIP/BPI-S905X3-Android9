LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src) \
    src/com/android/internal/car/ICarServiceHelper.aidl \

LOCAL_JNI_SHARED_LIBRARIES := libcar-framework-service-jni
LOCAL_REQUIRED_MODULES := libcar-framework-service-jni

LOCAL_JAVA_LIBRARIES := services

LOCAL_MODULE := car-frameworks-service

include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/jni/com_android_internal_car_CarServiceHelperService.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \
    system/core/libsuspend/include

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libhidlbase \
    liblog \
    libnativehelper \
    libsuspend \
    libutils \

LOCAL_MODULE := libcar-framework-service-jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Include subdirectory makefiles
# ============================================================
include $(call all-makefiles-under,$(LOCAL_PATH))
