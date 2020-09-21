#CFLAGS+=-O0
ifneq ($(TARGET),android)
	LDFLAGS+=-lpthread -lrt -lsqlite3 -lm -ldl
else
	ifeq ($(ANDROID_VERSION),2.2)
		LICUDATA:=-licudata
		LGPSSTUB:=-lgpsstub
	else
		LICUDATA:=
		LGPSSTUB:=
	endif
	ifeq ($(ANDROID_VERSION),4.0)
		LGL:=-lGLESv2_dbg -lGLESv2 -lstlport -lgabi++
		LUI:=-lstagefright_foundation -lgui -lskia -ljpeg -lemoji
	else
		LGL:=
		LUI:=
	endif
	LDFLAGS+= -lsqlite -licui18n -lamplayer
#-liconv
endif


ifneq ($(LIBICONV),n)
ifneq ($(TARGET),android)
	#LDFLAGS+=-liconv
endif
endif

ifneq ($(KERNEL_INCDIR), )
	CFLAGS+=$(patsubst %,-I%,$(KERNEL_INCDIR))
endif

ifneq ($(ROOTFS_INCDIR), )
	CFLAGS+=$(patsubst %,-I%,$(ROOTFS_INCDIR))
endif

ifneq ($(NDK_INCDIR), )
	CFLAGS+=$(patsubst %,-I%,$(NDK_INCDIR))
endif

ifneq ($(EX_INCDIR), )
	CFLAGS+=-I$(EX_INCDIR)
endif


ifneq ($(ROOTFS_LIBDIR), )
	LDFLAGS+=-L$(ROOTFS_LIBDIR)
endif

ifneq ($(EX_LIBDIR), )
	LDFLAGS+=-L$(EX_LIBDIR)
endif

LDFLAGS+=-L$(ROOTDIR)/lib/$(ARCH)

ifeq ($(DEBUG),y)
	CFLAGS+=-g
endif

ifeq ($(CHIP), 8226h)
	CFLAGS+=-DCHIP_8226H -DAMLINUX
endif

ifeq ($(CHIP), 8226m)
	CFLAGS+=-DCHIP_8226M -DAMLINUX
endif

ifeq ($(CHIP), 8626x)
	CFLAGS+=-DCHIP_8626X -DAMLINUX
endif

ifeq ($(TARGET), android)
	CFLAGS+=-DANDROID -DAMLINUX
endif


ifeq ($(TARGET),android)
	LDSHARED_FLAGS+=-shared -Wl,--fix-cortex-a8
endif


ifeq ($(TARGET),android)
LDFLAGS+=-llog -lutils -lmedia -lz -lui $(LUI) -lcutils -lbinder -lEGL $(LGL) -lsonivox -licuuc -lexpat -lsurfaceflinger_client -lhardware -lhardware_legacy -lpixelflinger -lnetutils -lcamera_client $(LICUDATA) -lwpa_client $(LGPSSTUB) -W1,-T $(ARMELF_ANDROID) -Wl,-dynamic-linker,/system/bin/linker -include $(ANDROID_SYSTEM_CORE)/AndroidConfig.h
CFLAGS+=-O2 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300 -fno-exceptions -Wno-multichar -msoft-float -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -march=armv7-a -mfloat-abi=softfp -mfpu=neon  -I  $(ANDROID_SYSTEM_CORE) -mthumb-interwork -DANDROID -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point -DNDEBUG -g -Wstrict-aliasing=2 -finline-functions -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers -DNDEBUG -UDEBUG -nostdlib -Bdynamic -Wl,-T,$(ARMELF_X_ANDROID) -Wl,-dynamic-linker,/system/bin/linker 

SHARELIB_LDFLAGS=-nostdlib -Wl,-T,$(ARMELF_XSC_ANDROID)   -Wl,-shared,-Bsymbolic   -Wl,--whole-archive   -Wl,--no-whole-archive  -Wl,--no-undefined  -Wl,--fix-cortex-a8	


endif

ifneq ($(NDK_LIBDIR), )
LDFLAGS+=-L$(NDK_LIBDIR)
endif

ifeq ($(TARGET),android)
LDFLAGS+= -lc -lgcc -lm -ldl -lstdc++ 
endif



ifeq ($(MEMWATCH), y)
	CFLAGS+=-DMEMWATCH -DMEMWATCH_STDIO
endif

ifeq ($(LINUX_INPUT), y)
	CFLAGS+=-DLINUX_INPUT
endif

ifeq ($(TTY_INPUT), y)
	CFLAGS+=-DTTY_INPUT
endif

ifeq ($(SDL_INPUT), y)
	CFLAGS+=-DSDL_INPUT
	LDFLAGS+=-lSDL
endif


ifeq ($(EMU_DEMUX), y)
	CFLAGS+=-DEMU_DEMUX
endif

ifeq ($(LINUX_DVB_FEND), y)
	CFLAGS+=-DLINUX_DVB_FEND
endif

ifeq ($(EMU_FEND), y)
	CFLAGS+=-DEMU_FEND
endif

ifeq ($(EMU_SMC), y)
	CFLAGS+=-DEMU_SMC
endif

ifeq ($(SDL_OSD), y)
	CFLAGS+=-DSDL_OSD
	LDFLAGS+=-lSDL
endif

ifeq ($(EMU_DSC), y)
	CFLAGS+=-DEMU_DSC
endif

ifeq ($(EMU_AV), y)
	CFLAGS+=-DEMU_AV
else
ifneq ($(TARGET),android)
	LDFLAGS+=-Wl,-rpath-link $(ROOTFS_LIBDIR) -lamplayer -lamcodec -lamadec
endif
endif

ifeq ($(IMG_BMP), y)
	CFLAGS+=-DIMG_BMP
endif

ifeq ($(IMG_PNG), y)
	CFLAGS+=-DIMG_PNG
	LDFLAGS+=-lpng -lz
endif

ifeq ($(IMG_JPEG), y)
	CFLAGS+=-DIMG_JPEG
	LDFLAGS+=-ljpeg
endif

ifeq ($(IMG_GIF), y)
	CFLAGS+=-DIMG_GIF
endif

ifeq ($(IMG_TIFF), y)
	CFLAGS+=-DIMG_TIFF
	LDFLAGS+=-ltiff
endif

ifeq ($(FONT_FREETYPE), y)
	CFLAGS+=-DFONT_FREETYPE
ifeq ($(CHIP), )
	CFLAGS+=-I/usr/include/freetype2
else
	CFLAGS+=-I$(ROOTFS_INCDIR)/freetype2
endif
	LDFLAGS+=-lfreetype -lz
endif

ifeq ($(AMLOGIC_LIBPLAYER)), y)
        CFLAGS+=-DAMLOGIC_LIBPLAYER
endif
