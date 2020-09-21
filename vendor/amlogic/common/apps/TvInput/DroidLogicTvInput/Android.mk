LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_SRC_FILES += src/com/droidlogic/tvinput/services/ITvScanService.aidl
LOCAL_SRC_FILES += src/com/droidlogic/tvinput/services/IUpdateUiCallbackListener.aidl
LOCAL_SRC_FILES += src/com/droidlogic/tvinput/services/IAudioEffectsService.aidl

LOCAL_CERTIFICATE := platform
LOCAL_PACKAGE_NAME := DroidLogicTvInput

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_JAVA_LIBRARIES := droidlogic droidlogic-tv

LOCAL_JNI_SHARED_LIBRARIES := \
    libjnidtvsubtitle \
    libjnidtvepgscanner \
    libjnifont

LOCAL_PRIVATE_PLATFORM_APIS := true

#LOCAL_PRIVILEGED_MODULE := true
ifndef PRODUCT_SHIPPING_API_LEVEL
LOCAL_PRIVATE_PLATFORM_APIS := true
endif
LOCAL_PRODUCT_MODULE := true

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))

