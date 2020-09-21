BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_dmx
am_dmx_SRCS=am_dmx.c

SUBDIRS=
am_dmx_OBJS=

ifeq ($(EMU_DEMUX), y)
	SUBDIRS+=emu
	am_dmx_OBJS+=emu/emu
else
	SUBDIRS+=linux_dvb dvr
	am_dmx_OBJS+=linux_dvb/linux_dvb dvr/dvr
endif

include $(BASE)/rule/rule.mk
