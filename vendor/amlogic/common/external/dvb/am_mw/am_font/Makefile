BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_font
am_font_SRCS=am_font.c

ifeq ($(FONT_FREETYPE), y)
	am_font_SRCS+=freetype.c
endif

include $(BASE)/rule/rule.mk
