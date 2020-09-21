LOCAL_PATH := $(call my-dir)

app_module_tags := optional
app_src_files := am_tv/am_tv.c


app_cflags := -DANDROID -DAMLINUX -DFONT_FREETYPE -DCHIP_8226M
app_arm_mode := arm
app_c_includes := $(LOCAL_PATH)/../include/am_adp\
            $(LOCAL_PATH)/../include/am_mw\
            $(LOCAL_PATH)/../include/am_app\
            $(LOCAL_PATH)/../include/am_mw/libdvbsi\
            $(LOCAL_PATH)/../include/am_mw/libdvbsi/descriptors\
            $(LOCAL_PATH)/../include/am_mw/libdvbsi/tables\
            $(LOCAL_PATH)/../include/am_mw/atsc\
            $(LOCAL_PATH)/../android/ndk/include\
            packages/amlogic/LibPlayer/amadec/include\
            packages/amlogic/LibPlayer/amcodec/include\
            external/icu4c/common\
            $(LOCAL_PATH)/../../../frameworks/av/LibPlayer/amcodec/include\
            $(LOCAL_PATH)/../../../frameworks/av/LibPlayer/dvbplayer/include\
            $(LOCAL_PATH)/../../../frameworks/av/LibPlayer/amadec/include\
            $(LOCAL_PATH)/../../icu/icu4c/source/common\
            common/include/linux/amlogic\
            $(LOCAL_PATH)/../../libzvbi/src\
            external/sqlite/dist
app_static_libraries := 
app_shared_libraries := libicuuc libzvbi libam_adp libam_mw libsqlite libamplayer liblog libc 
app_prelink_module := false


include $(CLEAR_VARS)
LOCAL_MODULE    := libam_app
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := $(app_module_tags)
LOCAL_SRC_FILES := $(app_src_files)
LOCAL_CFLAGS+=$(app_cflags)
LOCAL_ARM_MODE := $(app_arm_mode)
LOCAL_C_INCLUDES := $(app_c_includes)
LOCAL_STATIC_LIBRARIES += $(app_static_libraries)
LOCAL_SHARED_LIBRARIES += $(app_shared_libraries)
LOCAL_PRELINK_MODULE := $(app_prelink_module)
#LOCAL_32_BIT_ONLY := true
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libam_app
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := $(app_module_tags)
LOCAL_SRC_FILES := $(app_src_files)
LOCAL_CFLAGS+=$(app_cflags)
LOCAL_ARM_MODE := $(app_arm_mode)
LOCAL_C_INCLUDES := $(app_c_includes)
LOCAL_STATIC_LIBRARIES += $(app_static_libraries)
LOCAL_SHARED_LIBRARIES += $(app_shared_libraries)
LOCAL_PRELINK_MODULE := $(app_prelink_module)
#LOCAL_32_BIT_ONLY := true
include $(BUILD_STATIC_LIBRARY)



