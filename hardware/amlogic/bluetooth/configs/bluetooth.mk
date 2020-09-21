#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#Support modules:
#   bcm40183, AP6210, AP6476, AP6330, AP62x2,AP6335,mt5931 & mt6622

$(warning BLUETOOTH_MODULE is $(BLUETOOTH_MODULE))
ifneq ($(BLUETOOTH_INF),)
$(warning BLUETOOTH_INF is $(BLUETOOTH_INF))
else
$(warning BLUETOOTH_INF is not set)
endif

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
    PRODUCT_PROPERTY_OVERRIDES += config.disable_bluetooth=false \
    ro.vendor.autoconnectbt.isneed=false \
    ro.vendor.autoconnectbt.macprefix=00:CD:FF \
    ro.vendor.autoconnectbt.btclass=50c \
    ro.vendor.autoconnectbt.nameprefix=Amlogic_RC \
    ro.vendor.autoconnectbt.rssilimit=70

else
    PRODUCT_PROPERTY_OVERRIDES += config.disable_bluetooth=true

endif

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
PRODUCT_PACKAGES += Bluetooth \
    bt_vendor.conf \
    bt_stack.conf \
    bt_did.conf \
    auto_pair_devlist.conf \
    libbt-hci \
    bluetooth.default \
    audio.a2dp.default \
    libbt-client-api \
    com.broadcom.bt \
    com.broadcom.bt.xml \
    android.hardware.bluetooth@1.0-impl \
    android.hardware.bluetooth@1.0-service

PRODUCT_COPY_FILES += \
    hardware/amlogic/bluetooth/broadcom/vendor/data/auto_pairing.conf:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/auto_pairing.conf \
    hardware/amlogic/bluetooth/broadcom/vendor/data/blacklist.conf:$(TARGET_COPY_OUT_VENDOR)/etc/bluetooth/blacklist.conf


endif

#################################################################################bt rc
$(shell cp hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc.template hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc)
PRODUCT_COPY_FILES += hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.amlogic.bluetooth.rc

#########################################################################
#
#                            SEI BT Remote Control
#
#########################################################################
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,hardware/amlogic/bluetooth/configs/hbg_rcu,vendor/etc) \
    hardware/amlogic/bluetooth/configs/init_rc/init.hbg.remote.rc:/vendor/etc/init/init.hbg.remote.rc

#########################################################################

################################################################################## RTKBT
ifeq ($(BLUETOOTH_MODULE),RTKBT)
#ifneq ($(filter rtl8761 rtl8723bs rtl8723bu rtl8821 rtl8822bu rtl8822bs, $(BLUETOOTH_MODULE)),)

ifeq ($(BLUETOOTH_INF), USB)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/rtk_btusb.ko" hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc)
endif

BLUETOOTH_USR_RTK_BLUEDROID := true
#Realtek add start
$(call inherit-product, hardware/amlogic/bluetooth/realtek/rtkbt/rtkbt.mk )
#realtek add end
PRODUCT_PACKAGES += libbt-vendor

#Realtek add start
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.bluetooth.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml
#realtek add end
endif

################################################################################## qcabt
ifeq ($(BLUETOOTH_MODULE),QCABT)
BOARD_HAVE_BLUETOOTH_QCOM := true

ifeq ($(BLUETOOTH_INF), USB)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/bt_usb_qcom.ko" hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc)
endif

#qca add start
$(call inherit-product, hardware/amlogic/bluetooth/qualcomm/qcabt.mk )
#qca add end


endif

##################################################################################bcmbt
ifeq ($(BLUETOOTH_MODULE), BCMBT)

#load bcm mk
$(call inherit-product, hardware/amlogic/bluetooth/broadcom/bcmbt.mk )
#load bcm mk end
ifeq ($(BLUETOOTH_INF), USB)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/btusb.ko" hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc)
endif

BOARD_HAVE_BLUETOOTH_BROADCOM := true
BCM_BLUETOOTH_LPM_ENABLE := true

PRODUCT_PACKAGES += libbt-vendor

endif

##################################################################################mtkbt
ifeq ($(BLUETOOTH_MODULE), MTKBT)

#load mtk mk
$(call inherit-product, hardware/amlogic/bluetooth/mtk/mtkbt/mtkbt.mk )
#load mtk mk end
ifeq ($(BLUETOOTH_INF), USB)
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/btmtk_usb.ko" hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc)
else
$(shell sed -i "1a\    insmod \/vendor\/lib/modules\/btmtksdio.ko" hardware/amlogic/bluetooth/configs/init_rc/init.amlogic.bluetooth.rc)
endif

BOARD_HAVE_BLUETOOTH_MTK := true

PRODUCT_PACKAGES += libbt-vendor \
                    libbluetooth_mtkbt

endif

