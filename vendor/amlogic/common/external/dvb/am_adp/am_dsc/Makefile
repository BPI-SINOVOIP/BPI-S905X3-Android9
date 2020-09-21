BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_dsc
am_dsc_SRCS=am_dsc.c

SUBDIRS=
am_dsc_OBJS=

ifeq ($(EMU_DSC), y)
	SUBDIRS+=emu
	am_dsc_OBJS+=emu/emu
else
	SUBDIRS+=aml
	am_dsc_OBJS+=aml/aml
endif

include $(BASE)/rule/rule.mk
