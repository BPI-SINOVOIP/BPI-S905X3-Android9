BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_av
am_av_SRCS=am_av.c

SUBDIRS=
am_av_OBJS=

ifeq ($(EMU_AV), y)
	SUBDIRS+=emu
	am_av_OBJS+=emu/emu
else
	SUBDIRS+=aml
	am_av_OBJS+=aml/aml
endif


include $(BASE)/rule/rule.mk
