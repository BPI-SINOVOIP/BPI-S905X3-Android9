# Makefile for linuxtv.org dvb-apps/lib/libucsi/dvb

.PHONY: sub-error-dvb

sub-error-dvb:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += dvb/bat_section.o           \
           dvb/dit_section.o           \
           dvb/eit_section.o           \
           dvb/int_section.o           \
           dvb/nit_section.o           \
           dvb/rst_section.o           \
           dvb/sdt_section.o           \
           dvb/sit_section.o           \
           dvb/st_section.o            \
           dvb/tdt_section.o           \
           dvb/tot_section.o           \
           dvb/tva_container_section.o \
           dvb/types.o

sub-install += dvb

else

includes = ac3_descriptor.h                                    \
           adaptation_field_data_descriptor.h                  \
           ait_application_descriptor.h                        \
           ait_application_icons_descriptor.h                  \
           ait_application_name_descriptor.h                   \
           ait_external_application_authorisation_descriptor.h \
           ancillary_data_descriptor.h                         \
           announcement_support_descriptor.h                   \
           application_signalling_descriptor.h                 \
           bat_section.h                                       \
           bouquet_name_descriptor.h                           \
           ca_identifier_descriptor.h                          \
           cable_delivery_descriptor.h                         \
           cell_frequency_link_descriptor.h                    \
           cell_list_descriptor.h                              \
           component_descriptor.h                              \
           content_descriptor.h                                \
           content_identifier_descriptor.h                     \
           country_availability_descriptor.h                   \
           data_broadcast_descriptor.h                         \
           data_broadcast_id_descriptor.h                      \
           default_authority_descriptor.h                      \
           descriptor.h                                        \
           dit_section.h                                       \
           dsng_descriptor.h                                   \
           eit_section.h                                       \
           extended_event_descriptor.h                         \
           frequency_list_descriptor.h                         \
           int_section.h                                       \
           ip_mac_platform_name_descriptor.h                   \
           ip_mac_platform_provider_name_descriptor.h          \
           ip_mac_stream_location_descriptor.h                 \
           linkage_descriptor.h                                \
           local_time_offset_descriptor.h                      \
           mhp_data_broadcast_id_descriptor.h                  \
           mosaic_descriptor.h                                 \
           mpe_fec_section.h                                   \
           multilingual_bouquet_name_descriptor.h              \
           multilingual_component_descriptor.h                 \
           multilingual_network_name_descriptor.h              \
           multilingual_service_name_descriptor.h              \
           network_name_descriptor.h                           \
           nit_section.h                                       \
           nvod_reference_descriptor.h                         \
           parental_rating_descriptor.h                        \
           partial_transport_stream_descriptor.h               \
           pdc_descriptor.h                                    \
           private_data_specifier_descriptor.h                 \
           related_content_descriptor.h                        \
           rnt_rar_over_dvb_stream_descriptor.h                \
           rnt_rar_over_ip_descriptor.h                        \
           rnt_rnt_scan_descriptor.h                           \
           rst_section.h                                       \
           s2_satellite_delivery_descriptor.h                  \
           satellite_delivery_descriptor.h                     \
           scrambling_descriptor.h                             \
           sdt_section.h                                       \
           section.h                                           \
           service_availability_descriptor.h                   \
           service_descriptor.h                                \
           service_identifier_descriptor.h                     \
           service_list_descriptor.h                           \
           service_move_descriptor.h                           \
           short_event_descriptor.h                            \
           short_smoothing_buffer_descriptor.h                 \
           sit_section.h                                       \
           st_section.h                                        \
           stream_identifier_descriptor.h                      \
           stuffing_descriptor.h                               \
           subtitling_descriptor.h                             \
           target_ip_address_descriptor.h                      \
           target_ipv6_address_descriptor.h                    \
           target_ip_slash_descriptor.h                        \
           target_ip_source_slash_descriptor.h                 \
           target_ipv6_slash_descriptor.h                      \
           target_ipv6_source_slash_descriptor.h               \
           tdt_section.h                                       \
           telephone_descriptor.h                              \
           teletext_descriptor.h                               \
           terrestrial_delivery_descriptor.h                   \
           time_shifted_event_descriptor.h                     \
           time_shifted_service_descriptor.h                   \
           time_slice_fec_identifier_descriptor.h              \
           tot_section.h                                       \
           transport_stream_descriptor.h                       \
           tva_container_section.h                             \
           tva_id_descriptor.h                                 \
           types.h                                             \
           vbi_data_descriptor.h                               \
           vbi_teletext_descriptor.h

include ../../../Make.rules

lib_name = libucsi/dvb

endif
