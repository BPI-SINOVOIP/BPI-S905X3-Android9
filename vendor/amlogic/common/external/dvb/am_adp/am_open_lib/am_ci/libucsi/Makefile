# Makefile for linuxtv.org dvb-apps/lib/libucsi

includes = crc32.h            \
           descriptor.h       \
           endianops.h        \
           section.h          \
           section_buf.h      \
           transport_packet.h \
           types.h

objects  = crc32.o            \
           section_buf.o      \
           transport_packet.o

lib_name = libucsi

CPPFLAGS += -I../../lib

.PHONY: all

all: library

include atsc/Makefile
include dvb/Makefile
include mpeg/Makefile

.PHONY: $(sub-install)

install:: $(sub-install)

$(sub-install):
	$(MAKE) -C $@ install

include ../../Make.rules
