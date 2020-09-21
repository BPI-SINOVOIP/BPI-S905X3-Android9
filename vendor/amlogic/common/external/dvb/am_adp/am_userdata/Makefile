BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_userdata
am_userdata_SRCS=am_userdata.c

SUBDIRS=
am_userdata_OBJS=

ifeq ($(EMU_DVR), y)
	SUBDIRS+=emu
	am_userdata_OBJS+=emu/emu
else
	SUBDIRS+=aml
	am_userdata_OBJS+=aml/aml
endif

include $(BASE)/rule/rule.mk
