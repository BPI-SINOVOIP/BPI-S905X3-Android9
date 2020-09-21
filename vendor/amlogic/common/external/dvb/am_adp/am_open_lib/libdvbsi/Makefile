BASE=../../..

include $(BASE)/rule/def.mk

CFLAGS+=-I$(ROOTDIR)/include/am_adp/libdvbsi

SUBDIRS=descriptors tables

O_TARGET=dvbsi
dvbsi_SRCS=demux.c descriptor.c dvbpsi.c psi.c
dvbsi_OBJS=descriptors/descriptors tables/tables

include $(BASE)/rule/rule.mk
