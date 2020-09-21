BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_img
am_img_SRCS=am_img.c

ifeq ($(IMG_BMP), y)
	am_img_SRCS+=image_bmp.c
endif

ifeq ($(IMG_GIF), y)
	am_img_SRCS+=image_gif.c
endif

ifeq ($(IMG_JPEG), y)
	am_img_SRCS+=image_jpeg.c
endif

ifeq ($(IMG_PNG), y)
	am_img_SRCS+=image_png.c
endif

ifeq ($(IMG_TIFF), y)
	am_img_SRCS+=image_tiff.c
endif


include $(BASE)/rule/rule.mk
