OUTPUT = libzvbi.so
DEFINES = -D_REENTRANT -D_GNU_SOURCE -DENABLE_DVB=1 -DENABLE_V4L=1 -DENABLE_V4L2=1 -DPACKAGE=\"zvbi\" -DVERSION=\"0.2.33\" -DLIBICONV_PLUG
USING_LIBS =
USING_LIBS_PATH =
OBJS = $(patsubst %.c,%.o,$(SRC_FILES))
LOCAL_PATH = $(shell pwd)
INSTALL_DIR = $(TARGET_DIR)/usr/lib
AMADEC_C_INCLUDES =

SRC_FILES = $(wildcard src/*.c)

CFLAGS   := -c -Wall -shared -fPIC -Wno-unknown-pragmas -Wno-format -O3 -fexceptions -fnon-call-exceptions

LOCAL_C_INCLUDES := -I $(LOCAL_PATH)
LOCAL_C_INCLUDES += -I $(LOCAL_PATH)/../aml_dvb-1.0/include/am_adp

CFLAGS += $(LOCAL_C_INCLUDES) $(DEFINES)
ifeq ($(ARCH_IS_64), y)
CFLAGS+=-DHAVE_S64_U64
endif
LDFLAGS  := -shared -fPIC -L$(INSTALL_DIR)

all : $(OBJS) $(OUTPUT)

$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(USING_LIBS_PATH) $(USING_LIBS)

install:
	-install -m 555 ${OUTPUT} $(INSTALL_DIR)

clean:
	@rm -f $(OBJS)
