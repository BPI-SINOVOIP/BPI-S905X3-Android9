####busybox #######
LOCAL_PATH := $(call my-dir)
BB_PATH := $(LOCAL_PATH)

# Bionic Branches Switches (GB/ICS/L)
BIONIC_ICS := false
BIONIC_L := true

# Make a static library for regex.
include $(CLEAR_VARS)
LOCAL_SRC_FILES := android/regex/bb_regex.c
LOCAL_C_INCLUDES := $(BB_PATH)/android/regex
LOCAL_CFLAGS := -Wno-sign-compare
LOCAL_MODULE := libclearsilverregex

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_STATIC_LIBRARY)

# Make a static library for RPC library (coming from uClibc).
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(shell cat $(BB_PATH)/android/librpc.sources)
LOCAL_C_INCLUDES := $(BB_PATH)/android/librpc
LOCAL_MODULE := libuclibcrpc
LOCAL_CFLAGS += -fno-strict-aliasing

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(BIONIC_L),true)
LOCAL_CFLAGS += -DBIONIC_ICS -DBIONIC_L
endif
include $(BUILD_STATIC_LIBRARY)

#####################################################################

# Execute make prepare for normal config & static lib (recovery)

LOCAL_PATH := $(BB_PATH)
include $(CLEAR_VARS)

# Explicitly set an architecture specific CONFIG_CROSS_COMPILER_PREFIX
ifneq ($(filter arm arm64,$(TARGET_ARCH)),)
    BUSYBOX_CROSS_COMPILER_PREFIX := arm-linux-androideabi-
endif
ifneq ($(filter x86 x86_64,$(TARGET_ARCH)),)
    BUSYBOX_CROSS_COMPILER_PREFIX := $(if $(filter x86_64,$(HOST_ARCH)),x86_64,i686)-linux-android-
endif
ifeq ($(TARGET_ARCH),mips)
    BUSYBOX_CROSS_COMPILER_PREFIX := mipsel-linux-android-
endif

BB_PREPARE_FLAGS:=
ifeq ($(HOST_OS),darwin)
    BB_HOSTCC := $(ANDROID_BUILD_TOP)/prebuilts/gcc/darwin-x86/host/i686-apple-darwin-4.2.1/bin/i686-apple-darwin11-gcc
    BB_PREPARE_FLAGS := HOSTCC=$(BB_HOSTCC)
endif

# On aosp (master), path is relative, not on cm (kitkat)
bb_gen := $(abspath $(TARGET_OUT_INTERMEDIATES)/busybox)

busybox_prepare_full := $(bb_gen)/full/.config
$(busybox_prepare_full): $(BB_PATH)/busybox-full.config
	@echo -e ${CL_YLW}"Prepare config for busybox binary"${CL_RST}
	@rm -rf $(bb_gen)/full
	@rm -f $(addsuffix /*.o, $(abspath $(call intermediates-dir-for,EXECUTABLES,busybox)))
	@mkdir -p $(@D)
	@cat $^ > $@ && echo "CONFIG_CROSS_COMPILER_PREFIX=\"$(BUSYBOX_CROSS_COMPILER_PREFIX)\"" >> $@
	$(MAKE) -C $(BB_PATH) prepare O=$(@D) $(BB_PREPARE_FLAGS)

busybox_prepare_minimal := $(bb_gen)/minimal/.config
$(busybox_prepare_minimal): $(BB_PATH)/busybox-minimal.config
	@echo -e ${CL_YLW}"Prepare config for libbusybox"${CL_RST}
	@rm -rf $(bb_gen)/minimal
	@rm -f $(addsuffix /*.o, $(abspath $(call intermediates-dir-for,STATIC_LIBRARIES,libbusybox)))
	@mkdir -p $(@D)
	@cat $^ > $@ && echo "CONFIG_CROSS_COMPILER_PREFIX=\"$(BUSYBOX_CROSS_COMPILER_PREFIX)\"" >> $@
	$(MAKE) -C $(BB_PATH) prepare O=$(@D) $(BB_PREPARE_FLAGS)

KERNEL_MODULES_DIR ?= /system/lib/modules
BUSYBOX_CONFIG := minimal full
$(BUSYBOX_CONFIG):
	@echo -e ${CL_PFX}"prepare config for busybox $@ profile"${CL_RST}
	@cd $(BB_PATH) && make clean
	@cd $(BB_PATH) && git clean -f -- ./include-$@/
	cp $(BB_PATH)/.config-$@ $(BB_PATH)/.config
	cd $(BB_PATH) && make prepare
	@#cp $(BB_PATH)/.config $(BB_PATH)/.config-$@
	@mkdir -p $(BB_PATH)/include-$@
	cp $(BB_PATH)/include/*.h $(BB_PATH)/include-$@/
	@rm $(BB_PATH)/include/usage_compressed.h
	@rm $(BB_PATH)/include/autoconf.h
	@rm -f $(BB_PATH)/.config-old

busybox_prepare: $(BUSYBOX_CONFIG)
LOCAL_MODULE := busybox_prepare
LOCAL_MODULE_TAGS := eng debug

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
#include $(BUILD_STATIC_LIBRARY)

#####################################################################

LOCAL_PATH := $(BB_PATH)
include $(CLEAR_VARS)

KERNEL_MODULES_DIR ?= /system/lib/modules

SUBMAKE := make -s -C $(BB_PATH) CC=$(CC)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
BUSYBOX_SRC_FILES = \
	$(shell cat $(BB_PATH)/busybox-$(BUSYBOX_CONFIG).sources) \
	android/libc/mktemp.c \
	android/libc/pty.c \
	shell/ash_80.c \
	android/android.c
else
BUSYBOX_SRC_FILES = \
	$(shell cat $(BB_PATH)/busybox-$(BUSYBOX_CONFIG).sources) \
	android/libc/mktemp.c \
	android/libc/pty.c \
	shell/ash.c \
	android/android.c
endif

BUSYBOX_ASM_FILES =
ifneq ($(BIONIC_L),true)
    BUSYBOX_ASM_FILES += swapon.S swapoff.S sysinfo.S
endif

ifneq ($(filter arm x86 mips,$(TARGET_ARCH)),)
    BUSYBOX_SRC_FILES += \
        $(addprefix android/libc/arch-$(TARGET_ARCH)/syscalls/,$(BUSYBOX_ASM_FILES))
endif

BUSYBOX_C_INCLUDES = \
	$(BB_PATH)/include $(BB_PATH)/libbb \
	bionic/libc/private \
	bionic/libm/include \
	bionic/libc \
	bionic/libm \
	libc/kernel/common \
	external/libselinux/include \
	external/selinux/libsepol/include \
	$(BB_PATH)/android/regex \
	$(BB_PATH)/android/librpc

BUSYBOX_CFLAGS = \
	-Werror=implicit -Wno-clobbered \
	-DNDEBUG \
	-DANDROID \
	-fno-strict-aliasing \
	-fno-builtin-stpcpy \
	-include $(BB_PATH)/$(BUSYBOX_CONFIG)/include/autoconf.h \
	-D'CONFIG_DEFAULT_MODULES_DIR="$(KERNEL_MODULES_DIR)"' \
	-D'BB_VER="$(strip $(shell $(SUBMAKE) kernelversion)) $(BUSYBOX_SUFFIX)"' -DBB_BT=AUTOCONF_TIMESTAMP

ifeq ($(BIONIC_L),true)
    BUSYBOX_CFLAGS += -DBIONIC_L
    BUSYBOX_AFLAGS += -DBIONIC_L
    # include changes for ICS/JB/KK
    BIONIC_ICS := true
endif

ifeq ($(BIONIC_ICS),true)
    BUSYBOX_CFLAGS += -DBIONIC_ICS
endif


# Build the static lib for the recovery tool

BUSYBOX_CONFIG:=minimal
BUSYBOX_SUFFIX:=static
LOCAL_SRC_FILES := $(BUSYBOX_SRC_FILES)
LOCAL_C_INCLUDES := $(BB_PATH)/minimal/include $(BUSYBOX_C_INCLUDES)
LOCAL_CFLAGS := -Dmain=busybox_driver $(BUSYBOX_CFLAGS)
LOCAL_CFLAGS += \
  -DRECOVERY_VERSION \
  -Dgetusershell=busybox_getusershell \
  -Dsetusershell=busybox_setusershell \
  -Dendusershell=busybox_endusershell \
  -Dgetmntent=busybox_getmntent \
  -Dgetmntent_r=busybox_getmntent_r \
  -Dgenerate_uuid=busybox_generate_uuid
LOCAL_ASFLAGS := $(BUSYBOX_AFLAGS)
LOCAL_MODULE := libbusybox
LOCAL_MODULE_TAGS := eng debug
#$(LOCAL_MODULE): busybox_prepare
LOCAL_STATIC_LIBRARIES := libcutils libselinux
LOCAL_ADDITIONAL_DEPENDENCIES := $(busybox_prepare_minimal)

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_STATIC_LIBRARY)


# Bionic Busybox /system/xbin

LOCAL_PATH := $(BB_PATH)
include $(CLEAR_VARS)

BUSYBOX_CONFIG:=full
BUSYBOX_SUFFIX:=bionic
LOCAL_SRC_FILES := $(BUSYBOX_SRC_FILES)
LOCAL_C_INCLUDES := $(BB_PATH)/full/include $(BUSYBOX_C_INCLUDES)
LOCAL_CFLAGS := $(BUSYBOX_CFLAGS)
LOCAL_ASFLAGS := $(BUSYBOX_AFLAGS)
LOCAL_MODULE := busybox
LOCAL_MODULE_TAGS := eng debug

LOCAL_SHARED_LIBRARIES := libcutils
#$(LOCAL_MODULE): busybox_prepare
LOCAL_STATIC_LIBRARIES += libclearsilverregex libuclibcrpc libselinux
LOCAL_ADDITIONAL_DEPENDENCIES := $(busybox_prepare_full)

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/xbin
else
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
endif

include $(BUILD_EXECUTABLE)

BUSYBOX_LINKS := $(shell cat $(BB_PATH)/busybox-$(BUSYBOX_CONFIG).links)
# nc is provided by external/netcat
exclude := nc which

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
SYMLINKS := $(addprefix $(TARGET_OUT_VENDOR)/xbin/,$(filter-out $(exclude),$(notdir $(BUSYBOX_LINKS))))
else
SYMLINKS := $(addprefix $(TARGET_OUT_OPTIONAL_EXECUTABLES)/,$(filter-out $(exclude),$(notdir $(BUSYBOX_LINKS))))
endif

$(SYMLINKS): BUSYBOX_BINARY := $(LOCAL_MODULE)
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo -e ${CL_CYN}"Symlink:"${CL_RST}" $@ -> $(BUSYBOX_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(BUSYBOX_BINARY) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)

# We need this so that the installed files could be picked up based on the
# local module name
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
    $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)


# Static Busybox

LOCAL_PATH := $(BB_PATH)
include $(CLEAR_VARS)

BUSYBOX_CONFIG:=full
BUSYBOX_SUFFIX:=static
LOCAL_SRC_FILES := $(BUSYBOX_SRC_FILES)
LOCAL_C_INCLUDES := $(BB_PATH)/full/include $(BUSYBOX_C_INCLUDES)
LOCAL_CFLAGS := $(BUSYBOX_CFLAGS)
LOCAL_CFLAGS += \
  -Dgetusershell=busybox_getusershell \
  -Dsetusershell=busybox_setusershell \
  -Dendusershell=busybox_endusershell \
  -Dgetmntent=busybox_getmntent \
  -Dgetmntent_r=busybox_getmntent_r \
  -Dgenerate_uuid=busybox_generate_uuid
LOCAL_ASFLAGS := $(BUSYBOX_AFLAGS)
# LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE := static_busybox
LOCAL_MODULE_STEM := busybox
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libclearsilverregex libcutils libuclibcrpc libselinux
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/utilities
LOCAL_UNSTRIPPED_PATH := $(PRODUCT_OUT)/symbols/utilities
#$(LOCAL_MODULE): busybox_prepare
LOCAL_ADDITIONAL_DEPENDENCIES := $(busybox_prepare_full)

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)

$(LOCAL_MODULE_PATH):
	@mkdir -p $@

BUSYBOX_LINKS := $(shell cat $(BB_PATH)/busybox-$(BUSYBOX_CONFIG).links)
# nc is provided by external/netcat
exclude := nc which
SYMLINKS := $(addprefix $(TARGET_RECOVERY_ROOT_OUT)/sbin/,$(filter-out $(exclude),$(notdir $(BUSYBOX_LINKS))))
$(SYMLINKS): BUSYBOX_BINARY := $(LOCAL_MODULE_STEM)
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo -e ${CL_CYN}"Symlink:"${CL_RST}" $@ -> $(BUSYBOX_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(BUSYBOX_BINARY) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)

# We need this so that the installed files could be picked up based on the
# local module name
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
    $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)
