##############################################################################
#
#    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(PLATFORM_VENDOR),1)
LOCAL_VENDOR_MODULE  := true
endif
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ovx_inc \
                    $(LOCAL_PATH)/../applib/nnapi \
		    $(LOCAL_PATH)/../applib/ovxinc/include \
		    $(LOCAL_PATH)/../applib/nnrt \
		    $(LOCAL_PATH)/../applib/ \
		    $(LOCAL_PATH)/../applib/nnrt/boost/libs/preprocessor/include \
                    $(LOCAL_PATH)/../../../../frameworks/ml/nn/common/include/ \
                    $(LOCAL_PATH)/../../../../frameworks/ml/nn/runtime/include/    \


LOCAL_SHARED_LIBRARIES := \
    libbase \
    libdl   \
    libhardware \
    libhidlbase \
    libhidlmemory   \
    libhidltransport    \
    liblog  \
    libutils    \
    libcutils    \
    android.hardware.neuralnetworks@1.0 \
    android.hidl.allocator@1.0  \
    android.hidl.memory@1.0 \
    libneuralnetworks   \
    libovxlib\
    libnnrt

LOCAL_SRC_FILES:= \
    VsiRTInfo.cpp \
    VsiDriver.cpp \
    1.0/VsiDriver1_0.cpp \
    1.0/VsiDevice1_0.cpp \
    VsiPreparedModel.cpp

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 28),1)
LOCAL_SRC_FILES += 1.1/VsiDevice1_1.cpp \
                   1.1/VsiDriver1_1.cpp
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_SRC_FILES += \
    1.2/VsiDevice1_2.cpp\
    1.2/VsiPreparedModel1_2.cpp\
    1.2/VsiDriver1_2.cpp \
    1.2/VsiBurstExecutor.cpp    \
    hal_limitation/nnapi_limitation.cpp \
    hal_limitation/support.cpp

endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 28),1)

LOCAL_SHARED_LIBRARIES += android.hardware.neuralnetworks@1.1
LOCAL_STATIC_LIBRARIES += libneuralnetworks_common

LOCAL_CFLAGS += -Wno-error=unused-variable -Wno-error=unused-function -Wno-error=return-type \
                -Wno-unused-parameter

LOCAL_C_INCLUDES += frameworks/native/libs/nativewindow/include \
                    frameworks//native/libs/arect/include

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 29),1)
LOCAL_C_INCLUDES += frameworks/ml/nn/runtime/include \
                    frameworks/native/libs/ui/include \
                    frameworks/native/libs/nativebase/include \
                    system/libfmq/include   \
                    $(LOCAL_PATH)/hal_limitation \
                    $(LOCAL_PATH)/op_validate

LOCAL_SHARED_LIBRARIES += libfmq \
                          libui \
                          android.hardware.neuralnetworks@1.2

ifeq ($(shell expr $(PLATFORM_VERSION) ">=" R),1)
LOCAL_SHARED_LIBRARIES += \
                          android.hardware.neuralnetworks@1.3

LOCAL_CFLAGS += -DANDROID_NN_API=30
endif

LOCAL_MODULE      := android.hardware.neuralnetworks@1.2-service-ovx-driver
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.2-service-ovx-driver.rc
else
LOCAL_SHARED_LIBRARIES += libneuralnetworks
LOCAL_MODULE      := android.hardware.neuralnetworks@1.1-service-ovx-driver
LOCAL_INIT_RC := android.hardware.neuralnetworks@1.1-service-ovx-driver.rc
endif


else

LOCAL_MODULE      := android.hardware.neuralnetworks@1.0-service-ovx-driver
endif

LOCAL_CFLAGS += -DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)  -Wno-error=unused-parameter
LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin/hw
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif

LOCAL_MODULE_RELATIVE_PATH := hw
#LOCAL_INIT_RC := android.hardware.neuralnetworks@1.1-service-ovx-driver.rc

LOCAL_CFLAGS += -DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)  -Wno-error=unused-parameter\
                -Wno-unused-private-field \
                -Wno-unused-parameter \
                -Wno-delete-non-virtual-dtor -Wno-non-virtual-dtor\

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
