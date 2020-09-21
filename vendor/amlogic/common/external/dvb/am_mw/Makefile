OUTPUT = libam_mw.so
DEFINES = -DLINUX_DVB_FEND -DLOG_LEVEL=1 -DLIBICONV_PLUG
USING_LIBS =
USING_LIBS_PATH = $(ROOT_DIR)/am_adp
OBJS = $(patsubst %.c,%.o,$(SRC_FILES))
LOCAL_PATH = $(ROOT_DIR)/am_mw
INSTALL_DIR = $(TARGET_DIR)/usr/lib/

LOCAL_C_INCLUDES := -I $(LOCAL_PATH)/../include/am_adp\
		    -I $(LOCAL_PATH)/../include/am_mw\
		    -I $(LOCAL_PATH)/../include/am_adp/libdvbsi\
		    -I $(LOCAL_PATH)/../include/am_adp/libdvbsi/descriptors\
		    -I $(LOCAL_PATH)/../include/am_adp/libdvbsi/tables\
		    -I $(LOCAL_PATH)/am_closecaption/am_vbi\
		    -I $(LOCAL_PATH)/../android/ndk/include\
		    -I $(LOCAL_PATH)/../am_adp/am_open_lib/am_ci\
		    -I $(LOCAL_PATH)/../../aml_zvbi-1.0/src

#$(LOCAL_PATH)/../../libzvbi/src\
#external/sqlite/dist\

SRC_FILES = $(wildcard am_db/*.c\
		   am_epg/am_epg.c\
		   am_rec/am_rec.c\
		   am_scan/am_scan.c\
		   am_sub2/*.c\
		   am_tt2/am_tt.c\
		   am_si/am_si.c\
		   am_si/atsc/*.c\
		   am_fend_ctrl/*.c\
		   am_caman/*.c\
		   am_cc/*.c\
		   am_upd/am_upd.c\
           am_closecaption/*.c\
           am_closecaption/am_vbi/*.c\
           am_closecaption/am_vbi/linux_vbi/*.c\
           am_check_scramb/*.c)

CFLAGS   := -c -Wall -shared -fPIC -Wno-unknown-pragmas -Wno-format -O3 -fexceptions -fnon-call-exceptions

CFLAGS += $(LOCAL_C_INCLUDES) $(DEFINES)
LDFLAGS  := -shared -fPIC -L$(USING_LIBS_PATH) -L$(INSTALL_DIR) -lzvbi -lam_adp -lsqlite3 -lamadec

all : $(OBJS) $(OUTPUT)

$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

install:
	-install -m 555 ${OUTPUT} $(INSTALL_DIR)

clean:
	@rm -f $(OBJS)
