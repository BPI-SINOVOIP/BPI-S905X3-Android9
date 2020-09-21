LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    main_hdmicec.cpp \
    DroidHdmiCec.cpp

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/../libhdmi_cec \
   $(LOCAL_PATH)/../binder \
   external/libcxx/include \
   hardware/libhardware/include \
   system/libhidl/transport/include/hidl \
   system/core/libutils/include \
   system/core/liblog/include \
   vendor/amlogic/common/frameworks/services/systemcontrol \
   vendor/amlogic/common/frameworks/services/systemcontrol/PQ/include \
   $(LOCAL_PATH)/../../../../tv/frameworks/libtvbinder/include

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.hdmicec@1.0 \
    vendor.amlogic.hardware.tvserver@1.0 \
    vendor.amlogic.hardware.systemcontrol@1.1 \
    libsystemcontrolservice \
    libbase \
    libhidlbase \
    libhidltransport \
    libcutils \
    libutils \
    libbinder \
    libhdmicec \
    libtvbinder \
    liblog

LOCAL_STATIC_LIBRARIES := \
    libhdmi_cec_static

LOCAL_CPPFLAGS += -std=c++14
LOCAL_INIT_RC := hdmicecd.rc

LOCAL_MODULE:= hdmicecd


ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)
