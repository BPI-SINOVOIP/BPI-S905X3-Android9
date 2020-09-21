BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_dvr
am_dvr_SRCS=am_dvr.c

SUBDIRS=
am_dvr_OBJS=

ifeq ($(EMU_DVR), y)
	SUBDIRS+=emu
	am_dvr_OBJS+=emu/emu
else
	SUBDIRS+=linux_dvb
	am_dvr_OBJS+=linux_dvb/linux_dvb
endif

include $(BASE)/rule/rule.mk
