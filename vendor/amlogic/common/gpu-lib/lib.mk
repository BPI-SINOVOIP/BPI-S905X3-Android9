
MALI_LIB_PREBUILT=true
#build in hardware/amlogic/ddk
ifneq (,$(wildcard vendor/arm))
MALI_LIB_PREBUILT=false
endif
ifneq (,$(wildcard hardware/amlogic/ddk))
MALI_LIB_PREBUILT=false
endif
#build in hardware/arm/gpu/ddk
ifneq (,$(wildcard hardware/arm/gpu/ddk))
MALI_LIB_PREBUILT=false
endif
ifneq (,$(wildcard vendor/arm/t83x))
MALI_LIB_PREBUILT=false
endif
ifneq (,$(wildcard vendor/amlogic/meson_mali))
MALI_LIB_PREBUILT=false
endif
#already in hardware/arm/gpu/lib

ifeq ($(MALI_LIB_PREBUILT),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TARGET:=$(GPU_TYPE)
TARGET?=mali400
ifeq ($(USING_MALI450), true)
TARGET=mali450
endif

TARGET:=$(TARGET)_ion
GPU_TARGET_PLATFORM ?= default_7a
ifeq ($(GPU_ARCH),midgard)
GPU_DRV_VERSION?=r11p0
else
GPU_DRV_VERSION?=r6p1
endif

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 29 && echo OK),OK)
LOCAL_ANDROID_VERSION_NUM:=q-${GPU_DRV_VERSION}
else
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 28 && echo OK),OK)
LOCAL_ANDROID_VERSION_NUM:=p-${GPU_DRV_VERSION}
else
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_ANDROID_VERSION_NUM:=o-${GPU_DRV_VERSION}
else
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 24 && echo OK),OK)
LOCAL_ANDROID_VERSION_NUM:=n-${GPU_DRV_VERSION}
else
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 23 && echo OK),OK)
LOCAL_ANDROID_VERSION_NUM:=m-${GPU_DRV_VERSION}
else
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 22 && echo OK),OK)
LOCAL_ANDROID_VERSION_NUM:=l-${GPU_DRV_VERSION}
else
LOCAL_ANDROID_VERSION_NUM:=k-${GPU_DRV_VERSION}
endif
endif
endif
endif
endif
endif

ifeq ($(GRALLOC_USE_GRALLOC1_API),1)
LOCAL_ANDROID_VERSION_NUM:=${LOCAL_ANDROID_VERSION_NUM}gralloc1
endif

LOCAL_MODULE := libGLES_mali
LOCAL_MULTILIB := both
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH    := $(TARGET_OUT_VENDOR)/egl
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib/egl
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64/egl
else
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/egl
LOCAL_MODULE_PATH_32 := $(TARGET_OUT)/lib/egl
LOCAL_MODULE_PATH_64 := $(TARGET_OUT)/lib64/egl
endif
ifeq ($(TARGET_2ND_ARCH),)
LOCAL_SRC_FILES    	 := $(TARGET)/libGLES_mali_$(GPU_TARGET_PLATFORM)_32-$(LOCAL_ANDROID_VERSION_NUM).so
else
LOCAL_SRC_FILES_32   := $(TARGET)/libGLES_mali_$(GPU_TARGET_PLATFORM)_32-$(LOCAL_ANDROID_VERSION_NUM).so
LOCAL_SRC_FILES_64	 := $(TARGET)/libGLES_mali_$(GPU_TARGET_PLATFORM)_64-$(LOCAL_ANDROID_VERSION_NUM).so
endif

#BOARD_INSTALL_VULKAN default is false
#It should defined in $(TARGET_PRODUCT).mk if Vulkan is needed.
$(info "the value of BOARD_INSTALL_VULKAN is $(BOARD_INSTALL_VULKAN)")
ifneq ($(BOARD_INSTALL_VULKAN),false)
$(info TARGET_PRODUCT is $(TARGET_PRODUCT))
LOCAL_POST_INSTALL_CMD = \
	if [ ! -d $(dir $(LOCAL_INSTALLED_MODULE))/../hw ]; then \
		mkdir -p $(dir $(LOCAL_INSTALLED_MODULE))/../hw; \
	fi;\
	cd $(dir $(LOCAL_INSTALLED_MODULE))/../hw;\
	pwd; \
	ln -sf ../egl/$(notdir $(LOCAL_INSTALLED_MODULE)) ./vulkan.amlogic.so;
endif

ifeq ($(BOARD_INSTALL_OPENCL),true)
#related to vulkan.amlogic.so
LOCAL_POST_INSTALL_CMD += \
	cd ..; \
	ln -sf egl/$(notdir $(LOCAL_INSTALLED_MODULE)) libOpenCL.so.1.1;\
	ln -sf libOpenCL.so.1.1 libOpenCL.so.1;\
	ln -sf libOpenCL.so.1 libOpenCL.so;
endif

include $(BUILD_PREBUILT)

endif
