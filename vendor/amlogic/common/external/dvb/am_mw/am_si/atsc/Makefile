BASE=../../..

include $(BASE)/rule/def.mk

CFLAGS+=-I$(ROOTDIR)/include/am_mw/atsc

O_TARGET=atsc
atsc_SRCS=atsc_descriptor.c atsc_eit.c atsc_ett.c atsc_mgt.c atsc_rrt.c atsc_stt.c atsc_vct.c huffman_decode.c

include $(BASE)/rule/rule.mk
