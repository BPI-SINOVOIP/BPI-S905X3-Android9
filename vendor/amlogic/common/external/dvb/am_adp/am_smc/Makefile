BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_smc
am_smc_SRCS=am_smc.c

SUBDIRS=
am_smc_OBJS=

ifeq ($(EMU_SMC), y)
	SUBDIRS+=emu
	am_smc_OBJS+=emu/emu
else
	SUBDIRS+=aml
	am_smc_OBJS+=aml/aml
endif

include $(BASE)/rule/rule.mk
