# Copyright (C) 2013 The Android Open Source Project
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

NOTO_DIR := $(call my-dir)

include $(call all-makefiles-under,$(NOTO_DIR))

# We have to use BUILD_PREBUILT instead of PRODUCT_COPY_FILES,
# to copy over the NOTICE file.
#############################################################################
# $(1): The source file name in LOCAL_PATH.
#       It also serves as the module name and the dest file name.
#############################################################################
define build-one-font-module
$(eval include $(CLEAR_VARS))\
$(eval LOCAL_MODULE := $(1))\
$(eval LOCAL_SRC_FILES := $(1))\
$(eval LOCAL_MODULE_CLASS := ETC)\
$(eval LOCAL_MODULE_TAGS := optional)\
$(eval LOCAL_MODULE_PATH := $(TARGET_OUT)/fonts)\
$(eval include $(BUILD_PREBUILT))
endef


#############################################################################
# First "build" the Noto CJK fonts, which have a different directory and
# copyright holder. These are not included in MINIMAL_FONT_FOOTPRINT builds.
#############################################################################
ifneq ($(MINIMAL_FONT_FOOTPRINT),true)
LOCAL_PATH := $(NOTO_DIR)/cjk

font_src_files := \
    NotoSansCJK-Regular.ttc

$(foreach f, $(font_src_files), $(call build-one-font-module, $(f)))
font_src_files :=

endif # !MINIMAL_FONT_FOOTPRINT

#############################################################################
# Similary "build" the Noto CJK fonts for serif family.
# These are not included in SMALLER_FONT_FOOTPRINT builds.
#############################################################################
ifeq ($(filter true,$(EXCLUDE_SERIF_FONTS) $(SMALLER_FONT_FOOTPRINT)),)
LOCAL_PATH := $(NOTO_DIR)/cjk

font_src_files := \
    NotoSerifCJK-Regular.ttc

$(foreach f, $(font_src_files), $(call build-one-font-module, $(f)))
font_src_files :=

endif # !EXCLUDE_SERIF_FONTS && !SMALLER_FONT_FOOTPRINT

#############################################################################
# Now "build" the Noto Color Emoji font, which is in its own directory. It is
# not included in the MINIMAL_FONT_FOOTPRINT builds.
#############################################################################
ifneq ($(MINIMAL_FONT_FOOTPRINT),true)
LOCAL_PATH := $(NOTO_DIR)/emoji

font_src_files := \
    NotoColorEmoji.ttf

$(foreach f, $(font_src_files), $(call build-one-font-module, $(f)))
font_src_files :=

endif # !MINIMAL_FONT_FOOTPRINT

#############################################################################
# Now "build" the rest of the fonts, which live in a separate subdirectory.
#############################################################################
LOCAL_PATH := $(NOTO_DIR)/other

#############################################################################
# The following fonts are included in all builds.
#############################################################################
font_src_files := \
    NotoSerif-Regular.ttf \
    NotoSerif-Bold.ttf \
    NotoSerif-Italic.ttf \
    NotoSerif-BoldItalic.ttf

#############################################################################
# The following fonts are excluded from SMALLER_FONT_FOOTPRINT builds.
#############################################################################
ifneq ($(SMALLER_FONT_FOOTPRINT),true)
font_src_files += \
    NotoSansAdlam-Regular.ttf \
    NotoSansAhom-Regular.otf \
    NotoSansAnatolianHieroglyphs-Regular.otf \
    NotoSansAvestan-Regular.ttf \
    NotoSansBalinese-Regular.ttf \
    NotoSansBamum-Regular.ttf \
    NotoSansBassaVah-Regular.otf \
    NotoSansBatak-Regular.ttf \
    NotoSansBengali-Bold.ttf \
    NotoSansBengali-Regular.ttf \
    NotoSansBengaliUI-Bold.ttf \
    NotoSansBengaliUI-Regular.ttf \
    NotoSansBhaiksuki-Regular.otf \
    NotoSansBrahmi-Regular.ttf \
    NotoSansBuginese-Regular.ttf \
    NotoSansBuhid-Regular.ttf \
    NotoSansCanadianAboriginal-Regular.ttf \
    NotoSansCarian-Regular.ttf \
    NotoSansChakma-Regular.ttf \
    NotoSansCham-Bold.ttf \
    NotoSansCham-Regular.ttf \
    NotoSansCherokee-Regular.ttf \
    NotoSansCoptic-Regular.ttf \
    NotoSansCuneiform-Regular.ttf \
    NotoSansCypriot-Regular.ttf \
    NotoSansDeseret-Regular.ttf \
    NotoSansEgyptianHieroglyphs-Regular.ttf \
    NotoSansElbasan-Regular.otf \
    NotoSansEthiopic-Bold.ttf \
    NotoSansEthiopic-Regular.ttf \
    NotoSansGlagolitic-Regular.ttf \
    NotoSansGothic-Regular.ttf \
    NotoSansGujarati-Bold.ttf \
    NotoSansGujarati-Regular.ttf \
    NotoSansGujaratiUI-Bold.ttf \
    NotoSansGujaratiUI-Regular.ttf \
    NotoSansGurmukhi-Bold.ttf \
    NotoSansGurmukhi-Regular.ttf \
    NotoSansGurmukhiUI-Bold.ttf \
    NotoSansGurmukhiUI-Regular.ttf \
    NotoSansHanunoo-Regular.ttf \
    NotoSansHatran-Regular.otf \
    NotoSansImperialAramaic-Regular.ttf \
    NotoSansInscriptionalPahlavi-Regular.ttf \
    NotoSansInscriptionalParthian-Regular.ttf \
    NotoSansJavanese-Regular.ttf \
    NotoSansKaithi-Regular.ttf \
    NotoSansKannada-Bold.ttf \
    NotoSansKannada-Regular.ttf \
    NotoSansKannadaUI-Bold.ttf \
    NotoSansKannadaUI-Regular.ttf \
    NotoSansKayahLi-Regular.ttf \
    NotoSansKharoshthi-Regular.ttf \
    NotoSansKhmerUI-Bold.ttf \
    NotoSansKhmerUI-Regular.ttf \
    NotoSansLao-Bold.ttf \
    NotoSansLao-Regular.ttf \
    NotoSansLaoUI-Bold.ttf \
    NotoSansLaoUI-Regular.ttf \
    NotoSansLepcha-Regular.ttf \
    NotoSansLimbu-Regular.ttf \
    NotoSansLinearA-Regular.otf \
    NotoSansLinearB-Regular.ttf \
    NotoSansLisu-Regular.ttf \
    NotoSansLycian-Regular.ttf \
    NotoSansLydian-Regular.ttf \
    NotoSansMalayalam-Bold.ttf \
    NotoSansMalayalam-Regular.ttf \
    NotoSansMalayalamUI-Bold.ttf \
    NotoSansMalayalamUI-Regular.ttf \
    NotoSansMandaic-Regular.ttf \
    NotoSansManichaean-Regular.otf \
    NotoSansMarchen-Regular.otf \
    NotoSansMeeteiMayek-Regular.ttf \
    NotoSansMeroitic-Regular.otf \
    NotoSansMiao-Regular.otf \
    NotoSansMongolian-Regular.ttf \
    NotoSansMro-Regular.otf \
    NotoSansMultani-Regular.otf \
    NotoSansMyanmar-Bold.ttf \
    NotoSansMyanmar-Regular.ttf \
    NotoSansMyanmarUI-Bold.ttf \
    NotoSansMyanmarUI-Regular.ttf \
    NotoSansNabataean-Regular.otf \
    NotoSansNewa-Regular.otf \
    NotoSansNewTaiLue-Regular.ttf \
    NotoSansNKo-Regular.ttf \
    NotoSansOgham-Regular.ttf \
    NotoSansOlChiki-Regular.ttf \
    NotoSansOldItalic-Regular.ttf \
    NotoSansOldNorthArabian-Regular.otf \
    NotoSansOldPermic-Regular.otf \
    NotoSansOldPersian-Regular.ttf \
    NotoSansOldSouthArabian-Regular.ttf \
    NotoSansOldTurkic-Regular.ttf \
    NotoSansOriya-Bold.ttf \
    NotoSansOriya-Regular.ttf \
    NotoSansOriyaUI-Bold.ttf \
    NotoSansOriyaUI-Regular.ttf \
    NotoSansOsage-Regular.ttf \
    NotoSansOsmanya-Regular.ttf \
    NotoSansPahawhHmong-Regular.otf \
    NotoSansPalmyrene-Regular.otf \
    NotoSansPauCinHau-Regular.otf \
    NotoSansPhagsPa-Regular.ttf \
    NotoSansPhoenician-Regular.ttf \
    NotoSansRejang-Regular.ttf \
    NotoSansRunic-Regular.ttf \
    NotoSansSamaritan-Regular.ttf \
    NotoSansSaurashtra-Regular.ttf \
    NotoSansSharada-Regular.otf \
    NotoSansShavian-Regular.ttf \
    NotoSansSinhala-Bold.ttf \
    NotoSansSinhala-Regular.ttf \
    NotoSansSinhalaUI-Bold.otf \
    NotoSansSinhalaUI-Regular.otf \
    NotoSansSoraSompeng-Regular.otf \
    NotoSansSundanese-Regular.ttf \
    NotoSansSylotiNagri-Regular.ttf \
    NotoSansSyriacEastern-Regular.ttf \
    NotoSansSyriacEstrangela-Regular.ttf \
    NotoSansSyriacWestern-Regular.ttf \
    NotoSansTagalog-Regular.ttf \
    NotoSansTagbanwa-Regular.ttf \
    NotoSansTaiLe-Regular.ttf \
    NotoSansTaiTham-Regular.ttf \
    NotoSansTaiViet-Regular.ttf \
    NotoSansTamil-Bold.ttf \
    NotoSansTamil-Regular.ttf \
    NotoSansTamilUI-Bold.ttf \
    NotoSansTamilUI-Regular.ttf \
    NotoSansTelugu-Bold.ttf \
    NotoSansTelugu-Regular.ttf \
    NotoSansTeluguUI-Bold.ttf \
    NotoSansTeluguUI-Regular.ttf \
    NotoSansThaana-Bold.ttf \
    NotoSansThaana-Regular.ttf \
    NotoSansTibetan-Bold.ttf \
    NotoSansTibetan-Regular.ttf \
    NotoSansTifinagh-Regular.ttf \
    NotoSansUgaritic-Regular.ttf \
    NotoSansVai-Regular.ttf \
    NotoSansYi-Regular.ttf
endif # !SMALLER_FONT_FOOTPRINT

#############################################################################
# The following fonts are excluded from MINIMAL_FONT_FOOTPRINT builds.
#############################################################################
ifneq ($(MINIMAL_FONT_FOOTPRINT),true)
font_src_files += \
    NotoNaskhArabic-Regular.ttf \
    NotoNaskhArabic-Bold.ttf \
    NotoNaskhArabicUI-Regular.ttf \
    NotoNaskhArabicUI-Bold.ttf \
    NotoSansArmenian-Regular.ttf \
    NotoSansArmenian-Bold.ttf \
    NotoSansDevanagari-Regular.ttf \
    NotoSansDevanagari-Bold.ttf \
    NotoSansDevanagariUI-Regular.ttf \
    NotoSansDevanagariUI-Bold.ttf \
    NotoSansGeorgian-Regular.ttf \
    NotoSansGeorgian-Bold.ttf \
    NotoSansHebrew-Regular.ttf \
    NotoSansHebrew-Bold.ttf \
    NotoSansSymbols-Regular-Subsetted.ttf \
    NotoSansSymbols-Regular-Subsetted2.ttf \
    NotoSansThai-Regular.ttf \
    NotoSansThai-Bold.ttf \
    NotoSansThaiUI-Regular.ttf \
    NotoSansThaiUI-Bold.ttf
endif # !MINIMAL_FONT_FOOTPRINT

ifeq ($(filter true,$(EXCLUDE_SERIF_FONTS) $(SMALLER_FONT_FOOTPRINT)),)
font_src_files += \
    NotoSerifArmenian-Bold.ttf \
    NotoSerifArmenian-Regular.ttf \
    NotoSerifBengali-Bold.ttf \
    NotoSerifBengali-Regular.ttf \
    NotoSerifDevanagari-Bold.ttf \
    NotoSerifDevanagari-Regular.ttf \
    NotoSerifEthiopic-Bold.otf \
    NotoSerifEthiopic-Regular.otf \
    NotoSerifGeorgian-Bold.ttf \
    NotoSerifGeorgian-Regular.ttf \
    NotoSerifGujarati-Bold.ttf \
    NotoSerifGujarati-Regular.ttf \
    NotoSerifGurmukhi-Bold.otf \
    NotoSerifGurmukhi-Regular.otf \
    NotoSerifHebrew-Bold.ttf \
    NotoSerifHebrew-Regular.ttf \
    NotoSerifKannada-Bold.ttf \
    NotoSerifKannada-Regular.ttf \
    NotoSerifKhmer-Bold.otf \
    NotoSerifKhmer-Regular.otf \
    NotoSerifLao-Bold.ttf \
    NotoSerifLao-Regular.ttf \
    NotoSerifMalayalam-Bold.ttf \
    NotoSerifMalayalam-Regular.ttf \
    NotoSerifMyanmar-Bold.otf \
    NotoSerifMyanmar-Regular.otf \
    NotoSerifSinhala-Bold.otf \
    NotoSerifSinhala-Regular.otf \
    NotoSerifTamil-Bold.ttf \
    NotoSerifTamil-Regular.ttf \
    NotoSerifTelugu-Bold.ttf \
    NotoSerifTelugu-Regular.ttf \
    NotoSerifThai-Bold.ttf \
    NotoSerifThai-Regular.ttf
endif # !EXCLUDE_SERIF_FONTS && !SMALLER_FONT_FOOTPRINT

$(foreach f, $(font_src_files), $(call build-one-font-module, $(f)))

#############################################################################
# Now "build" the variable fonts, which live in a separate subdirectory.
# The only variable fonts are for Khmer Sans, which is excluded in
# SMALLER_FONT_FOOTPRINT build.
#############################################################################

ifneq ($(SMALLER_FONT_FOOTPRINT),true)

LOCAL_PATH := $(NOTO_DIR)/other-vf

font_src_files := \
    NotoSansKhmer-VF.ttf

$(foreach f, $(font_src_files), $(call build-one-font-module, $(f)))

endif # !SMALLER_FONT_FOOTPRINT

NOTO_DIR :=
build-one-font-module :=
font_src_files :=




