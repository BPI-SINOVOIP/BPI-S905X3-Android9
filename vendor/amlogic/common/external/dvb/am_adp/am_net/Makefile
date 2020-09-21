BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_net
am_net_SRCS=am_net.c

SUBDIRS=
am_net_OBJS=

ifeq ($(EMU_DEMUX), y)
	SUBDIRS+=emu
	am_net_OBJS+=emu/emu
else
	SUBDIRS+=aml
	am_net_OBJS+=aml/aml
endif

include $(BASE)/rule/rule.mk

