LOCAL_PATH := $(call my-dir)

### shared library

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	android/com_droidlogic_inputmethod_remote_PinyinDecoderService.cpp \
	share/dictbuilder.cpp \
	share/dictlist.cpp \
	share/dicttrie.cpp \
	share/lpicache.cpp \
	share/matrixsearch.cpp \
	share/mystdlib.cpp \
	share/ngram.cpp \
	share/pinyinime.cpp \
	share/searchutility.cpp \
	share/spellingtable.cpp \
	share/spellingtrie.cpp \
	share/splparser.cpp \
	share/userdict.cpp \
	share/utf16char.cpp \
	share/utf16reader.cpp \
	share/sync.cpp

LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)
#LOCAL_LDFLAGS += -lpthread
LOCAL_MODULE := libjni_remoteime
LOCAL_SHARED_LIBRARIES := libcutils libutils
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
