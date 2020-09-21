BASE=../../../..

include $(BASE)/rule/def.mk

CFLAGS+=-I$(ROOTDIR)/include/am_adp/libdvbsi -I$(ROOTDIR)/include/am_adp/libdvbsi/tables
O_TARGET=tables
tables_SRCS=cat.c   atsc_eit.c atsc_ett.c atsc_mgt.c atsc_stt.c atsc_vct.c eit.c     nit.c     pat.c     pmt.c     sdt.c     tot.c	bat.c huffman_decode.c atsc_cea.c

include $(BASE)/rule/rule.mk
