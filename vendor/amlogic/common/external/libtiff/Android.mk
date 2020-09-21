LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

src_files:= \
    tif_aux.c \
    tif_close.c \
    tif_codec.c \
    tif_color.c \
    tif_compress.c \
    tif_dir.c \
    tif_dirinfo.c \
    tif_dirread.c \
    tif_dirwrite.c \
    tif_dumpmode.c \
    tif_error.c \
    tif_extension.c \
    tif_fax3.c \
    tif_fax3sm.c \
    tif_flush.c \
    tif_getimage.c \
    tif_jbig.c \
    tif_jpeg.c \
    tif_luv.c \
    tif_lzw.c \
    tif_next.c \
    tif_ojpeg.c \
    tif_open.c \
    tif_packbits.c \
    tif_pixarlog.c \
    tif_predict.c \
    tif_print.c \
    tif_read.c \
    tif_strip.c \
    tif_swab.c \
    tif_thunder.c \
    tif_tile.c \
    tif_unix.c \
    tif_version.c \
    tif_warning.c \
    tif_write.c \
    tif_zip.c


include $(CLEAR_VARS)

LOCAL_SRC_FILES:= $(src_files)

LOCAL_C_INCLUDES := \
  external/jpeg/ \
  external/zlib

LOCAL_STATIC_LIBRARIES := \
  libz

LOCAL_MODULE:= libtiff
LOCAL_32_BIT_ONLY := true

LOCAL_SHARED_LIBRARIES:= \
  libjpeg

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

#####static lib####
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(src_files)


LOCAL_C_INCLUDES := \
  external/jpeg/ \
  external/zlib \
  external/libjpeg-turbo
LOCAL_SHARED_LIBRARIES:= \
  libjpeg
LOCAL_STATIC_LIBRARIES := \
  libz
LOCAL_MODULE:= libtiff_static
include $(BUILD_STATIC_LIBRARY)
