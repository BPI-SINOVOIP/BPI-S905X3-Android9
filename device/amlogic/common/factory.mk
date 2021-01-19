IMGPACK := $(BUILD_OUT_EXECUTABLES)/logo_img_packer$(BUILD_EXECUTABLE_SUFFIX)
PRODUCT_UPGRADE_OUT := $(PRODUCT_OUT)/upgrade
AML_EMMC_BIN_GENERATOR := $(BOARD_AML_VENDOR_PATH)/tools/aml_upgrade/amlogic_emmc_bin_maker.sh
PRODUCT_COMMON_DIR := device/amlogic/common/products/$(PRODUCT_TYPE)

ifeq ($(TARGET_NO_RECOVERY),true)
BUILT_IMAGES := boot.img bootloader.img dt.img
else
BUILT_IMAGES := boot.img recovery.img bootloader.img dt.img
endif

VB_CHECK_IMAGES := vendor.img system.img vbmeta.img boot.img product.img

ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	BUILT_IMAGES := $(addsuffix .encrypt, $(BUILT_IMAGES))
endif#ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)

BUILT_IMAGES += system.img #userdata.img

ifneq ($(AB_OTA_UPDATER),true)
#BUILT_IMAGES += cache.img
endif

ifdef BOARD_PREBUILT_DTBOIMAGE
BUILT_IMAGES += dtbo.img
endif

BUILT_IMAGES += vendor.img
ifeq ($(BOARD_USES_ODMIMAGE),true)
BUILT_IMAGES += odm.img
# Adds the image and the matching map file to <product>-target_files-<build number>.zip, to allow
# generating OTA from target_files.zip (b/111128214).
INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_OUT)/odm.img $(PRODUCT_OUT)/odm.map
# Adds to <product name>-img-<build number>.zip so can be flashed.  b/110831381
BOARD_PACK_RADIOIMAGES += odm.img odm.map
endif

ifeq ($(BOARD_USES_PRODUCTIMAGE),true)
BUILT_IMAGES += product.img
endif

ifeq ($(BUILD_WITH_AVB),true)
BUILT_IMAGES += vbmeta.img
endif

ifeq ($(BOARD_USES_SYSTEM_OTHER_ODEX),true)
BUILT_IMAGES += system_other.img
endif

ifeq ($(strip $(HAS_BUILD_NUMBER)),false)
  # BUILD_NUMBER has a timestamp in it, which means that
  # it will change every time.  Pick a stable value.
  FILE_NAME := eng.$(USER)
else
  FILE_NAME := $(file <$(BUILD_NUMBER_FILE))
endif

AML_TARGET := $(PRODUCT_OUT)/obj/PACKAGING/target_files_intermediates/$(TARGET_PRODUCT)-target_files-$(FILE_NAME)

# -----------------------------------------------------------------
# odm partition image
ifdef BOARD_ODMIMAGE_FILE_SYSTEM_TYPE
INTERNAL_ODMIMAGE_FILES := \
    $(filter $(TARGET_OUT_ODM)/%,$(ALL_DEFAULT_INSTALLED_MODULES))

odmimage_intermediates := \
    $(call intermediates-dir-for,PACKAGING,odm)
BUILT_ODMIMAGE_TARGET := $(PRODUCT_OUT)/odm.img
# We just build this directly to the install location.
INSTALLED_ODMIMAGE_TARGET := $(BUILT_ODMIMAGE_TARGET)

# odm.img currently is a stub impl
$(INSTALLED_ODMIMAGE_TARGET) : $(INTERNAL_ODMIMAGE_FILES) $(PRODUCT_OUT)/system.img
	$(call pretty,"Target odm fs image: $(INSTALLED_ODMIMAGE_TARGET)")
	@mkdir -p $(TARGET_OUT_ODM)
	@mkdir -p $(odmimage_intermediates) && rm -rf $(odmimage_intermediates)/odm_image_info.txt
	$(call generate-userimage-prop-dictionary, $(odmimage_intermediates)/odm_image_info.txt, skip_fsck=true)
	PATH=$(HOST_OUT_EXECUTABLES):$$PATH \
	 mkuserimg_mke2fs.sh -s $(PRODUCT_OUT)/odm $(INSTALLED_ODMIMAGE_TARGET) $(BOARD_ODMIMAGE_FILE_SYSTEM_TYPE) \
	 odm $(BOARD_ODMIMAGE_PARTITION_SIZE) -j 0 -T 1230739200 -B $(PRODUCT_OUT)/odm.map -L odm -M 0 \
	 $(PRODUCT_OUT)/obj/ETC/file_contexts.bin_intermediates/file_contexts.bin
	#mke2fs -s -T -1 -S $(PRODUCT_OUT)/obj/ETC/file_contexts.bin_intermediates/file_contexts.bin -L odm -l $(BOARD_ODMIMAGE_PARTITION_SIZE) -a odm $(INSTALLED_ODMIMAGE_TARGET) $(PRODUCT_OUT)/odm $(PRODUCT_OUT)/system
	$(hide) $(call assert-max-image-size,$(INSTALLED_ODMIMAGE_TARGET),$(BOARD_ODMIMAGE_PARTITION_SIZE))
	-cp $(PRODUCT_OUT)/odm.map $(AML_TARGET)/IMAGES/
	-cp $(PRODUCT_OUT)/odm.img $(AML_TARGET)/IMAGES/

# We need a (implicit) rule for odm.map, in order to support the INSTALLED_RADIOIMAGE_TARGET above.
$(INSTALLED_ODMIMAGE_TARGET): .KATI_IMPLICIT_OUTPUTS := $(PRODUCT_OUT)/odm.map

.PHONY: odm_image
odm_image : $(INSTALLED_ODMIMAGE_TARGET)
$(call dist-for-goals, odm_image, $(INSTALLED_ODMIMAGE_TARGET))

endif

ifdef KERNEL_DEVICETREE
DTBTOOL := $(BOARD_AML_VENDOR_PATH)/tools/dtbTool

DTCTOOL := out/host/linux-x86/bin/dtc
DTIMGTOOL := out/host/linux-x86/bin/mkdtimg

ifdef KERNEL_DEVICETREE_CUSTOMER_DIR
KERNEL_DEVICETREE_DIR := $(KERNEL_DEVICETREE_CUSTOMER_DIR)
else
KERNEL_DEVICETREE_DIR := arch/$(KERNEL_ARCH)/boot/dts/amlogic/
endif

KERNEL_DEVICETREE_SRC := $(addprefix $(KERNEL_ROOTDIR)/$(KERNEL_DEVICETREE_DIR), $(KERNEL_DEVICETREE) )
KERNEL_DEVICETREE_SRC := $(wildcard $(addsuffix .dtd, $(KERNEL_DEVICETREE_SRC)) $(addsuffix .dts, $(KERNEL_DEVICETREE_SRC)))

KERNEL_DEVICETREE_BIN := $(addprefix $(KERNEL_OUT)/$(KERNEL_DEVICETREE_DIR), $(KERNEL_DEVICETREE))
KERNEL_DEVICETREE_BIN := $(addsuffix .dtb, $(KERNEL_DEVICETREE_BIN))

INSTALLED_BOARDDTB_TARGET := $(PRODUCT_OUT)/dt.img
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	INSTALLED_BOARDDTB_TARGET := $(INSTALLED_BOARDDTB_TARGET).encrypt
endif# ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)

ifeq ($(BUILD_WITH_AVB),true)
INSTALLED_AVB_DTBIMAGE_TARGET := $(PRODUCT_OUT)/dtb-avb.img
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += \
    --include_descriptors_from_image $(INSTALLED_AVB_DTBIMAGE_TARGET)

# Add a dependency of AVBTOOL to INSTALLED_BOARDDTB_TARGET
$(INSTALLED_BOARDDTB_TARGET): $(AVBTOOL)

# Add a dependency of dtb.img to vbmeta.img
$(INSTALLED_VBMETAIMAGE_TARGET): $(INSTALLED_BOARDDTB_TARGET)
vbmetaimage: $(INSTALLED_BOARDDTB_TARGET)
endif


$(INSTALLED_BOARDDTB_TARGET) : $(KERNEL_DEVICETREE_SRC) $(DTCTOOL) $(DTIMGTOOL) | $(MINIGZIP)
	$(foreach aDts, $(KERNEL_DEVICETREE), \
		sed -i 's/^#include \"partition_.*/#include \"$(TARGET_PARTITION_DTSI)\"/' $(KERNEL_ROOTDIR)/$(KERNEL_DEVICETREE_DIR)/$(strip $(aDts)).dts; \
		sed -i 's/^#include \"firmware_.*/#include \"$(TARGET_FIRMWARE_DTSI)\"/' $(KERNEL_ROOTDIR)/$(KERNEL_DEVICETREE_DIR)/$(TARGET_PARTITION_DTSI); \
		if [ -f "$(KERNEL_ROOTDIR)/$(KERNEL_DEVICETREE_DIR)/$(aDts).dtd" ]; then \
			$(MAKE) -C $(KERNEL_ROOTDIR) O=../$(KERNEL_OUT) ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) $(strip $(aDts)).dtd; \
		fi;\
		$(MAKE) -C $(KERNEL_ROOTDIR) O=../$(KERNEL_OUT) ARCH=$(KERNEL_ARCH) CROSS_COMPILE=$(PREFIX_CROSS_COMPILE) $(strip $(aDts)).dtb; \
	)
	cp device/amlogic/common/common_partition.xml $(PRODUCT_OUT)/emmc_burn.xml
	./device/amlogic/common/dtsi2xml.sh $(KERNEL_ROOTDIR)/$(KERNEL_DEVICETREE_DIR)/$(TARGET_PARTITION_DTSI)  $(PRODUCT_OUT)/emmc_burn.xml
ifneq ($(strip $(word 2, $(KERNEL_DEVICETREE)) ),)
	$(hide) $(DTBTOOL) -o $@ -p $(KERNEL_OUT)/scripts/dtc/ $(KERNEL_OUT)/$(KERNEL_DEVICETREE_DIR)
else# elif dts num == 1
	cp -f $(KERNEL_DEVICETREE_BIN) $@
endif
	if [ -n "$(shell find $@ -size +200k)" ]; then \
		echo "$@ > 200k will be gziped"; \
		mv $@ $@.orig && $(MINIGZIP) -c $@.orig > $@; \
	fi;
	$(hide) $(call aml-secureboot-sign-bin, $@)
	@echo "Instaled $@"
ifeq ($(BOARD_AVB_ENABLE),true)
	cp $@ $(INSTALLED_AVB_DTBIMAGE_TARGET)
	$(AVBTOOL) add_hash_footer \
	  --image $(INSTALLED_AVB_DTBIMAGE_TARGET) \
	  --partition_size $(BOARD_DTBIMAGE_PARTITION_SIZE) \
	  --partition_name dtb
endif

$(BOARD_PREBUILT_DTBOIMAGE): $(INSTALLED_BOARDDTB_TARGET) | $(DTCTOOL) $(DTIMGTOOL)
	$(hide) $(foreach file,$(DTBO_DEVICETREE), \
		$(DTCTOOL) -@ -O dtb -o $(PRODUCT_OUT)/$(file).dtbo $(KERNEL_ROOTDIR)/$(KERNEL_DEVICETREE_DIR)/overlay/$(TARGET_PRODUCT)/$(file).dts; \
		)
	$(DTIMGTOOL) create $@ \
	$(foreach file,$(DTBO_DEVICETREE), \
		$(PRODUCT_OUT)/$(file).dtbo \
	)

.PHONY: dtbimage
dtbimage: $(INSTALLED_BOARDDTB_TARGET)

.PHONY: dtboimage
dtboimage: $(PRODUCT_OUT)/dtbo.img

endif # ifdef KERNEL_DEVICETREE

# Adds to <product name>-img-<build number>.zip so can be flashed.  b/110831381
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	#using signed boot/recovery directly if 'PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY true'
INSTALLED_AML_ENC_RADIOIMAGE_TARGET = $(addprefix $(PRODUCT_OUT)/,$(filter %.img.encrypt,$(BUILT_IMAGES)))
BOARD_PACK_RADIOIMAGES += $(basename $(filter %.img.encrypt,$(BUILT_IMAGES)))
$(warning echo "radio add $(filter %.img.encrypt,$(BUILT_IMAGES))")
else
INSTALLED_RADIOIMAGE_TARGET += $(addprefix $(PRODUCT_OUT)/,$(filter dt.img bootloader.img,$(BUILT_IMAGES)))
BOARD_PACK_RADIOIMAGES += $(filter dt.img bootloader.img,$(BUILT_IMAGES))
$(warning echo "radio add dt and bootloader")
endif#ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)

BOARD_PACK_RADIOIMAGES += $(filter system.img vendor.img,$(BUILT_IMAGES))

UPGRADE_FILES := \
        aml_sdc_burn.ini \
        ddr_init.bin \
	u-boot.bin.sd.bin  u-boot.bin.usb.bl2 u-boot.bin.usb.tpl \
        u-boot-comp.bin
TOOL_ITEMS := usb_flow.aml keys.conf
UPGRADE_FILES += $(TOOL_ITEMS)

ifneq ($(TARGET_USE_SECURITY_MODE),true)
UPGRADE_FILES += \
        platform.conf
else # secureboot mode
UPGRADE_FILES += \
        u-boot-usb.bin.aml \
        platform_enc.conf
endif

UPGRADE_FILES := $(addprefix $(TARGET_DEVICE_DIR)/upgrade/,$(UPGRADE_FILES))
UPGRADE_FILES := $(wildcard $(UPGRADE_FILES)) #extract only existing files for burnning

PACKAGE_CONFIG_FILE := aml_upgrade_package
ifeq ($(AB_OTA_UPDATER),true)
	PACKAGE_CONFIG_FILE := $(PACKAGE_CONFIG_FILE)_AB
endif # ifeq ($(AB_OTA_UPDATER),true)
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	PACKAGE_CONFIG_FILE := $(PACKAGE_CONFIG_FILE)_enc
endif # ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
ifeq ($(BUILD_WITH_AVB),true)
	PACKAGE_CONFIG_FILE := $(PACKAGE_CONFIG_FILE)_avb
endif
PACKAGE_CONFIG_FILE := $(TARGET_DEVICE_DIR)/upgrade/$(PACKAGE_CONFIG_FILE).conf

ifeq ($(wildcard $(PACKAGE_CONFIG_FILE)),)
	PACKAGE_CONFIG_FILE := $(PRODUCT_COMMON_DIR)/upgrade_4.9/$(notdir $(PACKAGE_CONFIG_FILE))
endif ## ifeq ($(wildcard $(TARGET_DEVICE_DIR)/upgrade/$(PACKAGE_CONFIG_FILE)))
UPGRADE_FILES += $(PACKAGE_CONFIG_FILE)

ifneq ($(TARGET_AMLOGIC_RES_PACKAGE),)
INSTALLED_AML_LOGO := $(PRODUCT_UPGRADE_OUT)/logo.img
$(INSTALLED_AML_LOGO): $(wildcard $(TARGET_AMLOGIC_RES_PACKAGE)/*) | $(IMGPACK) $(MINIGZIP)
	@echo "generate $(INSTALLED_AML_LOGO)"
	$(hide) mkdir -p $(PRODUCT_UPGRADE_OUT)/logo
	$(hide) rm -rf $(PRODUCT_UPGRADE_OUT)/logo/*
	@cp -rf $(TARGET_AMLOGIC_RES_PACKAGE)/* $(PRODUCT_UPGRADE_OUT)/logo
	$(foreach bmpf, $(filter %.bmp,$^), \
		if [ -n "$(shell find $(bmpf) -type f -size +256k)" ]; then \
			echo "logo pic $(bmpf) >256k gziped"; \
			$(MINIGZIP) -c $(bmpf) > $(PRODUCT_UPGRADE_OUT)/logo/$(notdir $(bmpf)); \
		else cp $(bmpf) $(PRODUCT_UPGRADE_OUT)/logo; \
		fi;)
	$(hide) $(IMGPACK) -r $(PRODUCT_UPGRADE_OUT)/logo $@
	@echo "Installed $@"
# Adds to <product name>-img-<build number>.zip so can be flashed.  b/110831381
INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_UPGRADE_OUT)/logo.img
BOARD_PACK_RADIOIMAGES += logo.img

else
INSTALLED_AML_LOGO :=
endif

.PHONY: logoimg
logoimg: $(INSTALLED_AML_LOGO)

BOARD_AUTO_COLLECT_MANIFEST := false
ifneq ($(BOARD_AUTO_COLLECT_MANIFEST),false)
BUILD_TIME := $(shell date +%Y-%m-%d--%H-%M)
INSTALLED_MANIFEST_XML := $(PRODUCT_OUT)/manifests/manifest-$(BUILD_TIME).xml
$(INSTALLED_MANIFEST_XML):
	$(hide) mkdir -p $(PRODUCT_OUT)/manifests
	$(hide) mkdir -p $(PRODUCT_OUT)/upgrade
	# Below fails on google build servers, perhaps because of older version of repo installed
	repo manifest -r -o $(INSTALLED_MANIFEST_XML)
	$(hide) cp $(INSTALLED_MANIFEST_XML) $(PRODUCT_OUT)/upgrade/manifest.xml

.PHONY:build_manifest
build_manifest:$(INSTALLED_MANIFEST_XML)
else
INSTALLED_MANIFEST_XML :=
endif

INSTALLED_AML_USER_IMAGES :=
ifeq ($(TARGET_BUILD_USER_PARTS),true)
define aml-mk-user-img-template
INSTALLED_AML_USER_IMAGES += $(2)
INSTALLED_RADIOIMAGE_TARGET += $(2) $(3)
BOARD_PACK_RADIOIMAGES += $(strip $(1)).img $(strip $(1)).map
$(eval tempUserSrcDir := $$($(strip $(1))_PART_DIR))
$(2): $(call intermediates-dir-for,ETC,file_contexts.bin)/file_contexts.bin $(MAKE_EXT4FS) $(shell find $(tempUserSrcDir) -type f)
	@out/host/linux-x86/bin/mkuserimg_mke2fs.sh -s $(tempUserSrcDir) $$@ ext4  $(1) $$($(strip $(1))_PART_SIZE) -T 1230739200 -B $(strip $(PRODUCT_OUT))/$(strip $(1)).map -L $(1) -M 0 $$< && \
	out/host/linux-x86/bin/mkuserimg_mke2fs.sh -s $(tempUserSrcDir) $$@ ext4  $(1) $$($(strip $(1))_PART_SIZE) -T 1230739200 -B $(strip $(PRODUCT_OUT))/$(strip $(1)).map -L $(1) -M 0 $$<
$(3):$(2)
	echo "already build"
endef
.PHONY:contexts_add
contexts_add:$(TARGET_ROOT_OUT)/file_contexts
	$(foreach userPartName, $(BOARD_USER_PARTS_NAME), \
		$(shell sed -i "/\/$(strip $(userPartName))/d" $< && \
		echo -e "/$(strip $(userPartName))(/.*)?      u:object_r:system_file:s0" >> $<))
$(foreach userPartName, $(BOARD_USER_PARTS_NAME), \
	$(eval $(call aml-mk-user-img-template, $(userPartName),$(PRODUCT_OUT)/$(userPartName).img, $(PRODUCT_OUT)/$(userPartName).map)))

define aml-user-img-update-pkg
	ln -sf $(shell readlink -f $(PRODUCT_OUT)/$(1).img) $(PRODUCT_UPGRADE_OUT)/$(1).img && \
	sed -i "/file=\"$(1)\.img\"/d" $(2) && \
	echo -e "file=\"$(1).img\"\t\tmain_type=\"PARTITION\"\t\tsub_type=\"$(1)\"" >> $(2) ;
endef

.PHONY: aml_usrimg
aml_usrimg :$(INSTALLED_AML_USER_IMAGES)
endif # ifeq ($(TARGET_BUILD_USER_PARTS),true)

INSTALLED_AMLOGIC_BOOTLOADER_TARGET := $(PRODUCT_OUT)/bootloader.img
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	INSTALLED_AMLOGIC_BOOTLOADER_TARGET := $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET).encrypt
endif# ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
.PHONY: aml_bootloader
aml_bootloader : $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET)

BOOTLOADER_INPUT := $(TARGET_DEVICE_DIR)/bootloader.img
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
ifeq ($(PRODUCT_AML_SECURE_BOOT_VERSION3),true)
	BOOTLOADER_INPUT := $(BOOTLOADER_INPUT).zip
endif #ifeq ($(PRODUCT_AML_SECURE_BOOT_VERSION3),true)
endif # ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
$(INSTALLED_AMLOGIC_BOOTLOADER_TARGET) : $(BOOTLOADER_INPUT)
	$(hide) cp $< $(PRODUCT_OUT)/$(notdir $<)
	$(hide) $(call aml-secureboot-sign-bootloader, $@,$(PRODUCT_OUT)/$(notdir $<))
	@echo "make $@: bootloader installed end"

$(call dist-for-goals, droidcore, $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET))

ifeq ($(TARGET_SUPPORT_USB_BURNING_V2),true)
INSTALLED_AML_UPGRADE_PACKAGE_TARGET := $(PRODUCT_OUT)/aml_upgrade_package.img
$(warning will keep $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET))
$(call dist-for-goals, droidcore, $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET))

PACKAGE_CONFIG_FILE := $(PRODUCT_UPGRADE_OUT)/$(notdir $(PACKAGE_CONFIG_FILE))

ifeq ($(TARGET_USE_SECURITY_DM_VERITY_MODE_WITH_TOOL),true)
  SYSTEMIMG_INTERMEDIATES := $(PRODUCT_OUT)/obj/PACKAGING/systemimage_intermediates/system.img.
  SYSTEMIMG_INTERMEDIATES := $(SYSTEMIMG_INTERMEDIATES)verity_table.bin $(SYSTEMIMG_INTERMEDIATES)verity.img
  define security_dm_verity_conf
	  @echo "copy verity.img and verity_table.bin"
	  @sed -i "/verity_table.bin/d" $(PACKAGE_CONFIG_FILE)
	  @sed -i "/verity.img/d" $(PACKAGE_CONFIG_FILE)
	  $(hide) \
		sed -i "/aml_sdc_burn\.ini/ s/.*/&\nfile=\"system.img.verity.img\"\t\tmain_type=\"img\"\t\tsub_type=\"verity\"/" $(PACKAGE_CONFIG_FILE); \
		sed -i "/aml_sdc_burn\.ini/ s/.*/&\nfile=\"system.img.verity_table.bin\"\t\tmain_type=\"bin\"\t\tsub_type=\"verity_table\"/" $(PACKAGE_CONFIG_FILE);
	  cp $(SYSTEMIMG_INTERMEDIATES) $(PRODUCT_UPGRADE_OUT)/
  endef #define security_dm_verity_conf
endif # ifeq ($(TARGET_USE_SECURITY_DM_VERITY_MODE_WITH_TOOL),true)

define update-aml_upgrade-conf
	$(foreach f, $(TOOL_ITEMS), \
	if [ -f $(PRODUCT_UPGRADE_OUT)/$(f) ]; then \
		echo exist item $(f); \
		awk -v file="$(f)" \
		-v main="$(lastword $(subst ., ,$(f)))" \
		-v subtype="$(basename $(f))" \
		'BEGIN{printf("file=\"%s\"\t\tmain_type=\"%s\"\t\tsub_type=\"%s\"\n", file, main, subtype)}' >> $(PACKAGE_CONFIG_FILE) ; \
		sed -i '$$!H;$$!d;$$G' $(PACKAGE_CONFIG_FILE); \
		sed -i '1H;1d;/\[LIST_NORMAL\]/G' $(PACKAGE_CONFIG_FILE); \
	fi;)
endef #define update-aml_upgrade-conf

ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
ifeq ($(PRODUCT_AML_SECURE_BOOT_VERSION3),true)
PRODUCT_AML_FIRMWARE_ANTIROLLBACK_CONFIG := ./device/amlogic/$(PRODUCT_DIR)/fw_arb.txt
define aml-secureboot-sign-bootloader
	@echo -----aml-secureboot-sign-bootloader ------
	rm $(PRODUCT_OUT)/bl_tmp -rf
	unzip $(2) -d $(PRODUCT_OUT)/bl_tmp
	mkdir -p $(PRODUCT_UPGRADE_OUT)
	bash $(PRODUCT_SBV3_SIGBL_TOOL) -p $(PRODUCT_OUT)/bl_tmp \
		-r $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR) -a $(PRODUCT_AML_SECUREBOOT_AESKEY_DIR) \
		-b $(PRODUCT_AML_FIRMWARE_ANTIROLLBACK_CONFIG) -o $(PRODUCT_OUT)
	mv $(PRODUCT_OUT)/u-boot.bin.unsigned $(basename $(1))
	mv $(PRODUCT_OUT)/u-boot.bin.signed.encrypted $(1)
	mv $(PRODUCT_OUT)/u-boot.bin.signed.encrypted.sd.bin $(1).sd.bin
	mv $(PRODUCT_OUT)/u-boot.bin.usb.bl2.signed.encrypted $(1).usb.bl2
	mv $(PRODUCT_OUT)/u-boot.bin.usb.tpl.signed.encrypted $(1).usb.tpl
	mv $(PRODUCT_OUT)/pattern.efuse $(1).encrypt.efuse
	@echo ----- Made aml secure-boot singed bootloader: $(1) --------
endef #define aml-secureboot-sign-bootloader
define aml-secureboot-sign-kernel
	@echo -----aml-secureboot-sign-kernel V3------
	$(hide) mv -f $(1) $(basename $(1))
	bash $(PRODUCT_SBV3_SIGIMG_TOOL) -i $(basename $(1)) \
		-k $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR)/kernelkey.pem \
		-a $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR)/kernelaeskey \
		--iv $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR)/kernelaesiv -o $(1)
	@echo ----- Made aml secure-boot singed kernel v3: $(1) --------
endef #define aml-secureboot-sign-kernel
define aml-secureboot-sign-bin
	@echo -----aml-secureboot-sign-bin v3------
	$(hide) mv -f $(1) $(basename $(1))
	bash $(PRODUCT_SBV3_SIGIMG_TOOL) -i $(basename $(1)) \
		-k $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR)/kernelkey.pem \
		-a $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR)/kernelaeskey \
		--iv $(PRODUCT_AML_SECUREBOOT_RSAKEY_DIR)/kernelaesiv -o $(1)
	@echo ----- Made aml secure-boot singed bin v3: $(1) --------
endef #define aml-secureboot-sign-bin
else #follows secureboot v2
define aml-secureboot-sign-bootloader
	@echo -----aml-secureboot-sign-bootloader ------
	$(hide) $(PRODUCT_AML_SECUREBOOT_SIGNBOOTLOADER) --input $(basename $(1)) --output $(1)
	@echo ----- Made aml secure-boot singed bootloader: $(1) --------
endef #define aml-secureboot-sign-bootloader
define aml-secureboot-sign-kernel
	@echo -----aml-secureboot-sign-kernel ------
	$(hide) mv -f $(1) $(basename $(1))
	$(hide) $(PRODUCT_AML_SECUREBOOT_SIGNIMAGE) --input $(basename $(1)) --output $(1)
	@echo ----- Made aml secure-boot singed kernel: $(1) --------
endef #define aml-secureboot-sign-kernel
define aml-secureboot-sign-bin
	@echo -----aml-secureboot-sign-bin------
	$(hide) mv -f $(1) $(basename $(1))
	$(hide) $(PRODUCT_AML_SECUREBOOT_SIGBIN) --input $(basename $(1)) --output $(1)
	@echo ----- Made aml secure-boot singed bin: $(1) --------
endef #define aml-secureboot-sign-bin
endif#ifeq ($(PRODUCT_AML_SECURE_BOOT_VERSION3),true)
endif# ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)

TARGET_USB_BURNING_V2_DEPEND_MODULES := $(AML_TARGET).zip #copy xx.img to $(AML_TARGET)/IMAGES for diff upgrade

.PHONY:aml_upgrade
aml_upgrade:$(INSTALLED_AML_UPGRADE_PACKAGE_TARGET)
$(INSTALLED_AML_UPGRADE_PACKAGE_TARGET): \
	$(addprefix $(PRODUCT_OUT)/,$(BUILT_IMAGES)) \
	$(UPGRADE_FILES) \
	$(INSTALLED_AML_USER_IMAGES) \
	$(INSTALLED_AML_LOGO) \
	$(INSTALLED_MANIFEST_XML) \
	$(TARGET_USB_BURNING_V2_DEPEND_MODULES)
	mkdir -p $(PRODUCT_UPGRADE_OUT)
	$(hide) $(foreach file,$(UPGRADE_FILES), \
		echo cp $(file) $(PRODUCT_UPGRADE_OUT)/$(notdir $(file)); \
		cp -f $(file) $(PRODUCT_UPGRADE_OUT)/$(notdir $(file)); \
		)
	$(hide) $(foreach file,$(BOARD_USER_PARTS_NAME), \
		echo cp $(PRODUCT_OUT)/$(file).* $(AML_TARGET)/IMAGES/; \
		cp $(PRODUCT_OUT)/$(file).* $(AML_TARGET)/IMAGES/; \
		)
	$(hide) $(foreach file,$(BUILT_IMAGES), \
		echo "ln -sf $(shell readlink -f $(PRODUCT_OUT)/$(file)) $(PRODUCT_UPGRADE_OUT)/$(file)"; \
		ln -sf $(shell readlink -f $(PRODUCT_OUT)/$(file)) $(PRODUCT_UPGRADE_OUT)/$(file); \
		)
	@echo $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET)
	$(hide) $(foreach file,$(VB_CHECK_IMAGES), \
		rm $(PRODUCT_UPGRADE_OUT)/$(file);\
		ln -sf $(shell readlink -f $(AML_TARGET)/IMAGES/$(file)) $(PRODUCT_UPGRADE_OUT)/$(file); \
		)
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	$(hide) rm -f $(PRODUCT_UPGRADE_OUT)/bootloader.img.encrypt.*
	$(hide) $(ACP) $(PRODUCT_OUT)/bootloader.img.encrypt.* $(PRODUCT_UPGRADE_OUT)/
	ln -sf $(shell readlink -f $(PRODUCT_OUT)/dt.img) $(PRODUCT_UPGRADE_OUT)/dt.img
	ln -sf $(shell readlink -f $(basename $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET))) \
		$(PRODUCT_UPGRADE_OUT)/$(notdir $(basename $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET)))
	ln -sf $(shell readlink -f $(PRODUCT_OUT)/bootloader.img.encrypt.efuse) $(PRODUCT_UPGRADE_OUT)/SECURE_BOOT_SET
endif# ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	$(security_dm_verity_conf)
	$(update-aml_upgrade-conf)
	$(hide) $(foreach userPartName, $(BOARD_USER_PARTS_NAME), \
		$(call aml-user-img-update-pkg,$(userPartName),$(PACKAGE_CONFIG_FILE)))
	@echo "Package: $@"
	@echo ./$(BOARD_AML_VENDOR_PATH)/tools/aml_upgrade/aml_image_v2_packer -r \
		$(PACKAGE_CONFIG_FILE)  $(PRODUCT_UPGRADE_OUT)/ $@
	./$(BOARD_AML_VENDOR_PATH)/tools/aml_upgrade/aml_image_v2_packer -r \
		$(PACKAGE_CONFIG_FILE)  $(PRODUCT_UPGRADE_OUT)/ $@
	@echo " $@ installed"
else
#none
INSTALLED_AML_UPGRADE_PACKAGE_TARGET :=
endif

INSTALLED_AML_FASTBOOT_ZIP := $(PRODUCT_OUT)/$(TARGET_PRODUCT)-fastboot-flashall-$(BUILD_NUMBER).zip
$(warning will keep $(INSTALLED_AML_FASTBOOT_ZIP))
$(call dist-for-goals, droidcore, $(INSTALLED_AML_FASTBOOT_ZIP))

FASTBOOT_IMAGES := boot.img dt.img
ifneq ($(TARGET_NO_RECOVERY),true)
	FASTBOOT_IMAGES += recovery.img
endif
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	FASTBOOT_IMAGES := $(addsuffix .encrypt, $(FASTBOOT_IMAGES))
endif#ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)

FASTBOOT_IMAGES += android-info.txt system.img

FASTBOOT_IMAGES += vendor.img

ifeq ($(BOARD_USES_PRODUCTIMAGE),true)
FASTBOOT_IMAGES += product.img
endif

ifdef BOARD_PREBUILT_DTBOIMAGE
FASTBOOT_IMAGES += dtbo.img
endif

ifeq ($(BUILD_WITH_AVB),true)
	FASTBOOT_IMAGES += vbmeta.img
endif

ifeq ($(BOARD_USES_SYSTEM_OTHER_ODEX),true)
FASTBOOT_IMAGES += system_other.img
endif

.PHONY:aml_fastboot_zip
aml_fastboot_zip:$(INSTALLED_AML_FASTBOOT_ZIP)
$(INSTALLED_AML_FASTBOOT_ZIP): $(addprefix $(PRODUCT_OUT)/,$(FASTBOOT_IMAGES)) \
                               $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET) \
                               $(INSTALLED_AML_LOGO) \
                               $(BUILT_ODMIMAGE_TARGET)
	echo "install $@"
	rm -rf $(PRODUCT_OUT)/fastboot
	mkdir -p $(PRODUCT_OUT)/fastboot
	cd $(PRODUCT_OUT); cp $(FASTBOOT_IMAGES) fastboot/;
ifeq ($(TARGET_PRODUCT),ampere)
	echo "board=p212" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),braun)
	echo "board=p230" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),curie)
	echo "board=p241" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),darwin)
	echo "board=txlx_skt" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),einstein)
	echo "board=txlx_r311" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),franklin)
	echo "board=u212" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),galilei)
	echo "board=g12b_w400" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
ifeq ($(TARGET_PRODUCT),atom)
	echo "board=atom" > $(PRODUCT_OUT)/fastboot/android-info.txt
endif
	cd $(PRODUCT_OUT)/fastboot; zip -r ../$(TARGET_PRODUCT)-fastboot-image-$(BUILD_NUMBER).zip $(FASTBOOT_IMAGES)
	rm -rf $(PRODUCT_OUT)/fastboot_auto
	mkdir -p $(PRODUCT_OUT)/fastboot_auto
	cd $(PRODUCT_OUT); cp $(FASTBOOT_IMAGES) fastboot_auto/
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY),true)
	cp $(PRODUCT_OUT)/bootloader.img.encrypt $(PRODUCT_OUT)/fastboot_auto/
	cp $(PRODUCT_OUT)/dt.img.encrypt $(PRODUCT_OUT)/fastboot_auto/
else
	cp $(PRODUCT_OUT)/bootloader.img $(PRODUCT_OUT)/fastboot_auto/
	cp $(PRODUCT_OUT)/dt.img $(PRODUCT_OUT)/fastboot_auto/
endif
	cp $(PRODUCT_OUT)/odm.img $(PRODUCT_OUT)/fastboot_auto/
	cp $(PRODUCT_OUT)/upgrade/logo.img $(PRODUCT_OUT)/fastboot_auto/
ifeq ($(AB_OTA_UPDATER),true)
	cp device/amlogic/common/flash-all-ab.sh $(PRODUCT_OUT)/fastboot_auto/flash-all.sh
	cp device/amlogic/common/flash-all-ab.bat $(PRODUCT_OUT)/fastboot_auto/flash-all.bat
else
	cp device/amlogic/common/flash-all.sh $(PRODUCT_OUT)/fastboot_auto/
	cp device/amlogic/common/flash-all.bat $(PRODUCT_OUT)/fastboot_auto/
endif
	sed -i 's/fastboot update fastboot.zip/fastboot update $(TARGET_PRODUCT)-fastboot-image-$(BUILD_NUMBER).zip/' $(PRODUCT_OUT)/fastboot_auto/flash-all.sh
	sed -i 's/fastboot update fastboot.zip/fastboot update $(TARGET_PRODUCT)-fastboot-image-$(BUILD_NUMBER).zip/' $(PRODUCT_OUT)/fastboot_auto/flash-all.bat
	cd $(PRODUCT_OUT)/fastboot_auto; zip -r ../$(TARGET_PRODUCT)-fastboot-flashall-$(BUILD_NUMBER).zip *
	zipnote $@ | sed 's/@ \([a-z]*.img\).encrypt/&\n@=\1\n/' | zipnote -w $@


name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  name := $(name)_debug
endif
name := $(name)-ota-amlogic-$(BUILD_NUMBER)

AMLOGIC_OTA_PACKAGE_TARGET := $(PRODUCT_OUT)/$(name).zip

$(AMLOGIC_OTA_PACKAGE_TARGET): KEY_CERT_PAIR := $(DEFAULT_KEY_CERT_PAIR)

ifeq ($(AB_OTA_UPDATER),true)
$(AMLOGIC_OTA_PACKAGE_TARGET): $(BRILLO_UPDATE_PAYLOAD)
else
$(AMLOGIC_OTA_PACKAGE_TARGET): $(BRO)
endif

EXTRA_SCRIPT := $(TARGET_DEVICE_DIR)/../../../device/amlogic/common/recovery/updater-script

$(AMLOGIC_OTA_PACKAGE_TARGET): $(AML_TARGET).zip $(BUILT_ODMIMAGE_TARGET)
	@echo "Package OTA2: $@"
ifeq ($(BOARD_USES_ODMIMAGE),true)
	@echo "copy $(INSTALLED_ODMIMAGE_TARGET)"
	mkdir -p $(AML_TARGET)/IMAGES
	cp $(INSTALLED_ODMIMAGE_TARGET) $(AML_TARGET)/IMAGES/
	cp $(PRODUCT_OUT)/odm.map $(AML_TARGET)/IMAGES/

	mkdir -p $(AML_TARGET)/META
	echo "odm_fs_type=$(BOARD_ODMIMAGE_FILE_SYSTEM_TYPE)" >> $(AML_TARGET)/META/misc_info.txt
	echo "odm_size=$(BOARD_ODMIMAGE_PARTITION_SIZE)" >> $(AML_TARGET)/META/misc_info.txt
	echo "odm_journal_size=$(BOARD_ODMIMAGE_JOURNAL_SIZE)" >> $(AML_TARGET)/META/misc_info.txt
	echo "odm_extfs_inode_count=$(BOARD_ODMIMAGE_EXTFS_INODE_COUNT)" >> $(AML_TARGET)/META/misc_info.txt
	mkdir -p $(AML_TARGET)/ODM
	cp -a $(PRODUCT_OUT)/odm/* $(AML_TARGET)/ODM/
endif
ifneq ($(INSTALLED_AMLOGIC_BOOTLOADER_TARGET),)
	@echo "copy $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET)"
	mkdir -p $(AML_TARGET)/IMAGES
	cp $(INSTALLED_AMLOGIC_BOOTLOADER_TARGET) $(AML_TARGET)/IMAGES/bootloader.img
endif
ifneq ($(INSTALLED_AML_LOGO),)
	@echo "copy $(INSTALLED_AML_LOGO)"
	mkdir -p $(AML_TARGET)/IMAGES
	cp $(INSTALLED_AML_LOGO) $(AML_TARGET)/IMAGES/
endif
ifeq ($(strip $(TARGET_OTA_UPDATE_DTB)),true)
	@echo "copy $(INSTALLED_BOARDDTB_TARGET)"
	mkdir -p $(AML_TARGET)/IMAGES
	cp $(INSTALLED_BOARDDTB_TARGET) $(AML_TARGET)/IMAGES/
endif
ifeq ($(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY), true)
	@echo "PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY is $(PRODUCT_BUILD_SECURE_BOOT_IMAGE_DIRECTLY)"
	mkdir -p $(AML_TARGET)/IMAGES
	cp $(INSTALLED_BOOTIMAGE_TARGET)            $(AML_TARGET)/IMAGES/boot.img
	-cp $(INSTALLED_RECOVERYIMAGE_TARGET)        $(AML_TARGET)/IMAGES/recovery.img
else
	-cp $(PRODUCT_OUT)/recovery.img $(AML_TARGET)/IMAGES/recovery.img
endif
	$(hide) PATH=$(foreach p,$(INTERNAL_USERIMAGES_BINARY_PATHS),$(p):)$$PATH MKBOOTIMG=$(MKBOOTIMG) \
	   ./device/amlogic/common/ota_amlogic.py -v \
	   --block \
	   --extracted_input_target_files $(patsubst %.zip,%,$(BUILT_TARGET_FILES_PACKAGE)) \
	   -p $(HOST_OUT) \
	   -k $(DEFAULT_KEY_CERT_PAIR) \
	   $(if $(OEM_OTA_CONFIG), -o $(OEM_OTA_CONFIG)) \
	   $(BUILT_TARGET_FILES_PACKAGE) $@

.PHONY: ota_amlogic
ota_amlogic: $(AMLOGIC_OTA_PACKAGE_TARGET)

ifeq ($(TARGET_SUPPORT_USB_BURNING_V2),true)
INSTALLED_AML_EMMC_BIN := $(PRODUCT_OUT)/aml_emmc_mirror.bin.gz
PRODUCT_CFG_EMMC_LGC_TABLE	:= $(TARGET_DEVICE_DIR)/upgrade/aml_emmc_logic_table.xml
ifeq ($(wildcard $(PRODUCT_CFG_EMMC_LGC_TABLE)),)
	PRODUCT_CFG_EMMC_LGC_TABLE	:= \
		$(PRODUCT_COMMON_DIR)/upgrade_4.9/$(notdir $(PRODUCT_CFG_EMMC_LGC_TABLE))
endif#ifeq ($(wildcard $(PRODUCT_CFG_EMMC_LGC_TABLE)),)
PRODUCT_CFG_EMMC_CAP := bootloader/uboot-repo/bl33/include/emmc_partitions.h

$(INSTALLED_AML_EMMC_BIN): $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET) $(PRODUCT_CFG_EMMC_CAP) \
	$(PRODUCT_CFG_EMMC_LGC_TABLE) | $(SIMG2IMG) $(MINIGZIP)
	@echo "Packaging $(INSTALLED_AML_EMMC_BIN)"
	@echo $(AML_EMMC_BIN_GENERATOR) $(PRODUCT_CFG_EMMC_CAP) $(PRODUCT_CFG_EMMC_LGC_TABLE) $< $(basename $@) $(SIMG2IMG)
	$(AML_EMMC_BIN_GENERATOR) $(PRODUCT_CFG_EMMC_CAP) $(PRODUCT_CFG_EMMC_LGC_TABLE) $< $(basename $@) $(SIMG2IMG)
	$(MINIGZIP) $(basename $@)
	@echo "installed $@"

.PHONY: aml_emmc_bin
aml_emmc_bin :$(INSTALLED_AML_EMMC_BIN)
endif # ifeq ($(TARGET_SUPPORT_USB_BURNING_V2),true)

#define the OTA package for cipackage build.
ota_name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  ota_name := $(ota_name)_debug
endif
ota_name := $(ota_name)-ota-$(FILE_NAME_TAG)
INTERNAL_OTA_PACKAGE_TARGET := $(PRODUCT_OUT)/$(ota_name).zip

droidcore: $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET) $(INSTALLED_MANIFEST_XML) $(INSTALLED_AML_FASTBOOT_ZIP)
otapackage: $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET) $(INSTALLED_MANIFEST_XML) $(INSTALLED_AML_FASTBOOT_ZIP)
ota_amlogic: $(INSTALLED_AML_UPGRADE_PACKAGE_TARGET) $(INSTALLED_MANIFEST_XML) $(INSTALLED_AML_FASTBOOT_ZIP) otapackage
cipackage: $(INTERNAL_OTA_PACKAGE_TARGET) $(INSTALLED_MANIFEST_XML) $(INSTALLED_AML_FASTBOOT_ZIP)
