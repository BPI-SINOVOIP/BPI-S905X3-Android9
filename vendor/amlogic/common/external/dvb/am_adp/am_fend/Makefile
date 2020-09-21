BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_fend
am_fend_SRCS=am_fend.c am_fend_diseqc_cmd.c am_rotor_calc.c am_vlfend.c

ifeq ($(EMU_FEND), y)
	SUBDIRS+=emu
	am_fend_OBJS+=emu/emu
else
ifeq ($(LINUX_DVB_FEND), y)
	SUBDIRS+=linux_dvb linux_v4l2
	am_fend_OBJS+=linux_dvb/linux_dvb linux_v4l2/linux_v4l2
endif
endif

include $(BASE)/rule/rule.mk
