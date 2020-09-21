# Makefile for linuxtv.org dvb-apps/lib/libucsi/atsc

.PHONY: sub-error-atsc

sub-error-atsc:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += atsc/atsc_text.o      \
           atsc/cvct_section.o   \
           atsc/dccsct_section.o \
           atsc/dcct_section.o   \
           atsc/eit_section.o    \
           atsc/ett_section.o    \
           atsc/mgt_section.o    \
           atsc/rrt_section.o    \
           atsc/stt_section.o    \
           atsc/tvct_section.o   \
           atsc/types.o

sub-install += atsc

else

includes = ac3_descriptor.h                   \
           caption_service_descriptor.h       \
           component_name_descriptor.h        \
           content_advisory_descriptor.h      \
           cvct_section.h                     \
           dcc_arriving_request_descriptor.h  \
           dcc_departing_request_descriptor.h \
           dccsct_section.h                   \
           dcct_section.h                     \
           descriptor.h                       \
           eit_section.h                      \
           ett_section.h                      \
           extended_channel_name_descriptor.h \
           genre_descriptor.h                 \
           mgt_section.h                      \
           rc_descriptor.h                    \
           rrt_section.h                      \
           section.h                          \
           service_location_descriptor.h      \
           stt_section.h                      \
           stuffing_descriptor.h              \
           time_shifted_service_descriptor.h  \
           tvct_section.h                     \
           types.h

include ../../../Make.rules

lib_name = libucsi/atsc

endif
