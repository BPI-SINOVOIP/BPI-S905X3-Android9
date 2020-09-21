LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  iw.c genl.c event.c info.c phy.c \
  interface.c ibss.c station.c survey.c util.c ocb.c \
  mesh.c mpath.c mpp.c scan.c reg.c \
  reason.c status.c connect.c link.c offch.c ps.c cqm.c \
  bitrate.c wowlan.c coalesce.c roc.c p2p.c vendor.c \
  sections.c

LOCAL_CFLAGS += -D_GNU_SOURCE -DCONFIG_LIBNL20

# Silence some warnings for now. Needs to be fixed upstream. b/26105799
LOCAL_CFLAGS += -Wno-unused-parameter \
                -Wno-sign-compare \
                -Wno-format \
                -Wno-absolute-value \
                -Werror
LOCAL_CLANG_CFLAGS += -Wno-enum-conversion

LOCAL_LDFLAGS := -Wl,--no-gc-sections
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libnl
LOCAL_MODULE := iw

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_GENERATED_SOURCES := $(local-generated-sources-dir)/version.c
$(LOCAL_GENERATED_SOURCES) : $(LOCAL_PATH)/version.sh
	@echo "Generated: $@"
	@mkdir -p $(dir $@)
	$(hide) echo '#include "iw.h"' >$@
	$(hide) echo "const char iw_version[] = $$(grep ^VERSION $< | sed "s/VERSION=//");" >>$@

include $(BUILD_EXECUTABLE)
