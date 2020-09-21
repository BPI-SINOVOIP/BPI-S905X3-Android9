LOCAL_PATH:= $(call my-dir)
# build for image server
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  IImagePlayerService.cpp \
  ImagePlayerHal.cpp \
  main_imageserver.cpp \
  ImagePlayerService.cpp  \
  SysWrite.cpp \
  RGBPicture.c  \
  TIFF2RGBA.cpp \
  ImagePlayerProcessData.cpp \
  GetInMemory.cpp \

LOCAL_STATIC_LIBRARIES := libtiff_static
LOCAL_SHARED_LIBRARIES := \
  vendor.amlogic.hardware.imageserver@1.0 \
  vendor.amlogic.hardware.systemcontrol@1.1 \
  libhidlbase \
  libhidltransport \
  libbinder                   \
  libcutils                   \
  libutils                    \
  liblog                      \
  libhwui                     \
  libstagefright              \
  libjpeg                     \
  libz			      \
  libmedia                    \
  libdl                       \
  libcurl


LOCAL_C_INCLUDES += \
  frameworks/av/media/libstagefright/include/media \
  system/libhidl/transport/include/hidl \
  system/core/libutils/include \
  system/core/liblog/include \
  system/core/libsystem/include \
  system/core/libcutils/include \
  external/skia/include/core \
  external/skia/include/effects \
  external/skia/include/images \
  external/skia/include/private \
  external/skia/src/ports \
  external/skia/src/core \
  external/skia/include/utils \
  external/skia/include/core \
  external/skia/include/config \
  external/skia/include/android \
  external/skia/include/codec \
  external/libcxx/include \
  frameworks/av/include \
  frameworks/av \
  frameworks/native/libs/binder/include \
  system/core/base/include \
  system/libhidl/transport/base/1.0 \
  $(LOCAL_PATH)/../../../external/libtiff

LOCAL_MODULE_TAGS := optional

LOCAL_CPPFLAGS += -std=c++14

LOCAL_MODULE:= imageserver
#LOCAL_32_BIT_ONLY := true

LOCAL_INIT_RC := imageserver.rc
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
