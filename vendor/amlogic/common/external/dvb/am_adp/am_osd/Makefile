BASE=../..

include $(BASE)/rule/def.mk

O_TARGET=am_osd
am_osd_SRCS=am_osd.c sw_op.c clip.c

SUBDIRS=
am_osd_OBJS=

ifeq ($(SDL_OSD), y)
	SUBDIRS+=sdl
	am_osd_OBJS+=sdl/sdl
else
	SUBDIRS+=fb
	am_osd_OBJS+=fb/fb
	am_osd_SRCS+=ge2d_op.c cmemlib.c
endif

include $(BASE)/rule/rule.mk
