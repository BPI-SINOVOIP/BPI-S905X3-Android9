OUTPUT = libam_adp.so
DEFINES = -DLINUX_DVB_FEND -DLOG_LEVEL=1
USING_LIBS =
USING_LIBS_PATH =
OBJS = $(patsubst %.c,%.o,$(SRC_FILES))
LOCAL_PATH = $(ROOT_DIR)/am_adp
INSTALL_DIR = $(STAGING_DIR)/usr/lib
AMADEC_C_INCLUDES =
LOCAL_C_INCLUDES := -I $(LOCAL_PATH)/../include/am_adp\
		    -I$(LOCAL_PATH)/../include/am_adp/libdvbsi\
		    -I$(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		    -I$(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		    -I$(LOCAL_PATH)/am_open_lib/am_ci\
		    -I$(LOCAL_PATH)/../include/am_mw\
		    -I$(LOCAL_PATH)/../android/ndk/include\
			-I$(LOCAL_PATH)/am_fend

SRC_FILES = $(wildcard am_dmx/*.c am_dmx/linux_dvb/*.c \
			am_fend/*.c am_fend/linux_dvb/*.c am_fend/linux_v4l2/*.c\
			am_av/*.c am_av/aml/*.c\
	        am_dvr/*.c am_dvr/linux_dvb/*.c\
	        am_dmx/dvr/*.c\
	        am_aout/*.c\
	        am_vout/*.c am_vout/aml/*.c\
	        am_misc/am_adplock.c am_misc/am_misc.c am_misc/am_sig_handler.c\
	        am_time/*.c\
	        am_evt/*.c\
			am_kl/*.c\
	        am_dsc/*.c am_dsc/aml/*.c\
	        am_smc/*.c\
	        am_smc/aml/*.c\
		    am_userdata/*.c am_userdata/aml/*.c am_userdata/emu/*.c\
		    am_pes/*.c \
		    am_ad/*.c\
		    am_open_lib/libdvbsi/tables/*.c\
		    am_open_lib/libdvbsi/*.c\
		    am_open_lib/libdvbsi/descriptors/*.c\
		    am_open_lib/am_ci/libdvben50221/*.c\
		    am_open_lib/am_ci/libucsi/dvb/types.c\
		    am_open_lib/am_ci/libdvbapi/dvbca.c\
		    am_open_lib/am_ci/libucsi/mpeg/pmt_section.c\
		    am_open_lib/am_ci/*.c\
		    am_open_lib/am_freesat/freesat.c \
		    am_tfile/am_tfile.c)

CFLAGS   := -c -Wall -shared -fPIC -Wno-unknown-pragmas -Wno-format -O3 -fexceptions -fnon-call-exceptions -DUSE_ADEC_IN_DVB

CFLAGS += $(LOCAL_C_INCLUDES) $(DEFINES)
LDFLAGS  := -shared -fPIC -L$(TARGET_DIR)/usr/lib -lamadec

all : $(OBJS) $(OUTPUT)

$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(USING_LIBS_PATH) $(USING_LIBS)

install:
	-install -m 555 ${OUTPUT} $(TARGET_DIR)/usr/lib/
	-mkdir -p $(STAGING_DIR)/usr/include/aml_dvb
	-cp -ar $(LOCAL_PATH)/../include/am_adp $(STAGING_DIR)/usr/include/aml_dvb

clean:
	@rm -f $(OBJS)
