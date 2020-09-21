# Makefile for linuxtv.org dvb-apps/lib/libucsi/mpeg

.PHONY: sub-error-mpeg

sub-error-mpeg:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += mpeg/cat_section.o      \
           mpeg/metadata_section.o \
           mpeg/odsmt_section.o    \
           mpeg/pat_section.o      \
           mpeg/pmt_section.o      \
           mpeg/tsdt_section.o

sub-install += mpeg

else

includes = audio_stream_descriptor.h                 \
           ca_descriptor.h                           \
           cat_section.h                             \
           content_labelling_descriptor.h            \
           copyright_descriptor.h                    \
           data_stream_alignment_descriptor.h        \
           datagram_section.h                        \
           descriptor.h                              \
           external_es_id_descriptor.h               \
           fmc_descriptor.h                          \
           fmxbuffer_size_descriptor.h               \
           hierarchy_descriptor.h                    \
           ibp_descriptor.h                          \
           iod_descriptor.h                          \
           iso_639_language_descriptor.h             \
           maximum_bitrate_descriptor.h              \
           metadata_descriptor.h                     \
           metadata_pointer_descriptor.h             \
           metadata_section.h                        \
           metadata_std_descriptor.h                 \
           mpeg4_audio_descriptor.h                  \
           mpeg4_video_descriptor.h                  \
           multiplex_buffer_descriptor.h             \
           multiplex_buffer_utilization_descriptor.h \
           muxcode_descriptor.h                      \
           odsmt_section.h                           \
           pat_section.h                             \
           pmt_section.h                             \
           private_data_indicator_descriptor.h       \
           registration_descriptor.h                 \
           section.h                                 \
           sl_descriptor.h                           \
           smoothing_buffer_descriptor.h             \
           std_descriptor.h                          \
           system_clock_descriptor.h                 \
           target_background_grid_descriptor.h       \
           tsdt_section.h                            \
           types.h                                   \
           video_stream_descriptor.h                 \
           video_window_descriptor.h

include ../../../Make.rules

lib_name = libucsi/mpeg

endif
