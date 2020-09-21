# determine which oem crypto level API is being used on this device
# so we can link with the appropriate libs.
# ifneq (,$(filter libwvdrm_L1, $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
#  LOCAL_OEMCRYPTO_LEVEL := 1
# else
#  LOCAL_OEMCRYPTO_LEVEL := 3
# endif

ifeq ($(BOARD_WIDEVINE_OEMCRYPTO_LEVEL),1)
LOCAL_OEMCRYPTO_LEVEL := 1
else
LOCAL_OEMCRYPTO_LEVEL := 3
endif

