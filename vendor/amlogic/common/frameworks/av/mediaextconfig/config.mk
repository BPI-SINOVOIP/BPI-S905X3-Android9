

LOCAL_C_INCLUDES += \
    $(BOARD_AML_VENDOR_PATH)/frameworks/av/mediaextconfig/include/ \
    $(BOARD_AML_VENDOR_PATH)/frameworks/av/mediaextconfig/include/media/stagefright/ \
    $(TOP)/hardware/amlogic/media/ammediaext/

LOCAL_CFLAGS += -DWITH_AMLOGIC_MEDIA_EX_SUPPORT


