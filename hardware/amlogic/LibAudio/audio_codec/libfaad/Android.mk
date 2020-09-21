LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libfaad
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

#helix aac decoder enabled
#ENABLE_HELIX_AAC_DECODER := true
ifdef ENABLE_HELIX_AAC_DECODER
LOCAL_SRC_FILES +=   \
    helixaac/aacdec.c                   \
    helixaac/aactabs.c                  \
    helixaac/bitstream.c                \
    helixaac/buffers.c                  \
    helixaac/dct4.c                     \
    helixaac/decelmnt.c                 \
    helixaac/dequant.c                  \
    helixaac/fft.c                      \
    helixaac/filefmt.c                  \
    helixaac/huffman_helix.c            \
    helixaac/hufftabs.c                 \
    helixaac/imdct.c                    \
    helixaac/noiseless.c                \
    helixaac/pns_helix.c                \
    helixaac/sbr.c                      \
    helixaac/sbrfft.c                   \
    helixaac/sbrfreq.c                  \
    helixaac/sbrhfadj.c                 \
    helixaac/sbrhfgen.c                 \
    helixaac/sbrhuff.c                  \
    helixaac/sbrimdct.c                 \
    helixaac/sbrmath.c                  \
    helixaac/sbrqmf.c                   \
    helixaac/sbrside.c                  \
    helixaac/sbrtabs.c                  \
    helixaac/stproc.c                   \
    helixaac/tns_helix.c                \
    helixaac/trigtabs.c                 \
    helixaac/trigtabs_fltgen.c
LOCAL_CFLAGS  += -DUSE_DEFAULT_STDLIB  -DUSE_HELIX_AAC_DECODER
endif


LOCAL_CFLAGS  += -Werror -Wunused-function

LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	  $(LOCAL_PATH)/codebook
#aac helix include file
LOCAL_C_INCLUDES  += $(LOCAL_PATH)/helixaac

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES += libutils  libz libbinder libdl libcutils libc liblog

LOCAL_MODULE    := libfaad
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
ifdef ENABLE_HELIX_AAC_DECODER
#helix aac files
LOCAL_SRC_FILES +=   \
    helixaac/aacdec.c                   \
    helixaac/aactabs.c                  \
    helixaac/bitstream.c                \
    helixaac/buffers.c                  \
    helixaac/dct4.c                     \
    helixaac/decelmnt.c                 \
    helixaac/dequant.c                  \
    helixaac/fft.c                      \
    helixaac/filefmt.c                  \
    helixaac/huffman_helix.c            \
    helixaac/hufftabs.c                 \
    helixaac/imdct.c                    \
    helixaac/noiseless.c                \
    helixaac/pns_helix.c                \
    helixaac/sbr.c                      \
    helixaac/sbrfft.c                   \
    helixaac/sbrfreq.c                  \
    helixaac/sbrhfadj.c                 \
    helixaac/sbrhfgen.c                 \
    helixaac/sbrhuff.c                  \
    helixaac/sbrimdct.c                 \
    helixaac/sbrmath.c                  \
    helixaac/sbrqmf.c                   \
    helixaac/sbrside.c                  \
    helixaac/sbrtabs.c                  \
    helixaac/stproc.c                   \
    helixaac/tns_helix.c                \
    helixaac/trigtabs.c                 \
    helixaac/trigtabs_fltgen.c
LOCAL_CFLAGS  += -DUSE_DEFAULT_STDLIB  -DUSE_HELIX_AAC_DECODER
endif


LOCAL_CFLAGS  += -Werror -Wunused-function

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	 $(LOCAL_PATH)/codebook

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
 
#aac helix include file
LOCAL_C_INCLUDES  += $(LOCAL_PATH)/helixaac

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES += libutils  libz libbinder libdl libcutils libc liblog

LOCAL_MODULE    := libfaad_sys
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
ifdef ENABLE_HELIX_AAC_DECODER
#helix aac files
LOCAL_SRC_FILES +=   \
    helixaac/aacdec.c                   \
    helixaac/aactabs.c                  \
    helixaac/bitstream.c                \
    helixaac/buffers.c                  \
    helixaac/dct4.c                     \
    helixaac/decelmnt.c                 \
    helixaac/dequant.c                  \
    helixaac/fft.c                      \
    helixaac/filefmt.c                  \
    helixaac/huffman_helix.c            \
    helixaac/hufftabs.c                 \
    helixaac/imdct.c                    \
    helixaac/noiseless.c                \
    helixaac/pns_helix.c                \
    helixaac/sbr.c                      \
    helixaac/sbrfft.c                   \
    helixaac/sbrfreq.c                  \
    helixaac/sbrhfadj.c                 \
    helixaac/sbrhfgen.c                 \
    helixaac/sbrhuff.c                  \
    helixaac/sbrimdct.c                 \
    helixaac/sbrmath.c                  \
    helixaac/sbrqmf.c                   \
    helixaac/sbrside.c                  \
    helixaac/sbrtabs.c                  \
    helixaac/stproc.c                   \
    helixaac/tns_helix.c                \
    helixaac/trigtabs.c                 \
    helixaac/trigtabs_fltgen.c
LOCAL_CFLAGS  += -DUSE_DEFAULT_STDLIB  -DUSE_HELIX_AAC_DECODER
endif


LOCAL_CFLAGS  += -Werror -Wunused-function

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	 $(LOCAL_PATH)/codebook

#aac helix include file
LOCAL_C_INCLUDES  += $(LOCAL_PATH)/helixaac

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
