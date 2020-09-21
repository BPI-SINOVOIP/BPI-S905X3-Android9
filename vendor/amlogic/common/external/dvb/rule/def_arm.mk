

ifeq ($(TARGET),android)
	CROSS_COMPILE=arm-eabi-
endif

ifeq ($(TARGET), 8226m)
	CROSS_COMPILE=arm-none-linux-gnueabi-
endif
