OUTPUT = am_vbi_test
DEFINES = -DLINUX_DVB_FEND -DLOG_LEVEL=1
USING_LIBS =
USING_LIBS_PATH =
OBJS = $(patsubst %.c,%.o,$(SRC_FILES))
LOCAL_PATH = $(shell pwd)
INSTALL_DIR = $(TARGET_DIR)/usr/bin

LOCAL_C_INCLUDES := -I $(ROOT_DIR)/include/am_adp\
		    -I$(ROOT_DIR)/include/am_adp/libdvbsi\
		    -I$(ROOT_DIR)/include/am_adp/libdvbsi/descriptors\
		    -I$(ROOT_DIR)/include/am_adp/libdvbsi/tables\
		    -I$(ROOT_DIR)/am_open_lib/am_ci\
		    -I$(ROOT_DIR)/include/am_mw\
		    -I$(ROOT_DIR)/android/ndk/include\
			-I$(ROOT_DIR)/am_fend\
			-I$(ROOT_DIR)/am_mw/am_closecaption/am_vbi\
			-I$(ROOT_DIR)/am_mw/am_closecaption

SRC_FILES = am_vbi_test.c

CFLAGS   := -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fexceptions -fnon-call-exceptions

CFLAGS += $(LOCAL_C_INCLUDES) $(DEFINES)
LDFLAGS  := -L$(TARGET_DIR)/usr/lib -lam_adp -lamadec -lam_mw -lamavutils -lsqlite3 -lzvbi

all : $(OBJS) $(OUTPUT)

$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(USING_LIBS_PATH) $(USING_LIBS)

install:
	-install -m 755 ${OUTPUT} $(INSTALL_DIR)

clean:
	@rm -f $(OBJS)
