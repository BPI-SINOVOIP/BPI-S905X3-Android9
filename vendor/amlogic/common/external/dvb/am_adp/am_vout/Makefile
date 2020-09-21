BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_vout
am_vout_SRCS=am_vout.c

SUBDIRS=
am_vout_OBJS=

ifeq ($(EMU_VOUT), y)
	SUBDIRS+=emu
	am_vout_OBJS+=emu/emu
else
	SUBDIRS+=aml
	am_vout_OBJS+=aml/aml
endif

include $(BASE)/rule/rule.mk
