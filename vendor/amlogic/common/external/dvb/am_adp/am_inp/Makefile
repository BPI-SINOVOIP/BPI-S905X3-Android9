BASE=../..

include $(BASE)/rule/def.mk

SUBDIRS=
am_inp_OBJS=

O_TARGET=am_inp
am_inp_SRCS=am_inp.c

ifeq ($(LINUX_INPUT), y)
	SUBDIRS+=linux_input
	am_inp_OBJS+=linux_input/linux_input
endif

ifeq ($(TTY_INPUT), y)
	SUBDIRS+=tty
	am_inp_OBJS+=tty/tty
endif

ifeq ($(SDL_INPUT), y)
	SUBDIRS+=sdl
	am_inp_OBJS+=sdl/sdl
endif

include $(BASE)/rule/rule.mk
