generated_sources := $(local-generated-sources-dir)

ifeq ($(BCM_BLUETOOTH_LPM_ENABLE),true)
SRC := $(call my-dir)/include/vnd_amlogic_lpm.txt
else
SRC := $(call my-dir)/include/vnd_amlogic.txt
endif
ifeq (,$(wildcard $(SRC)))
# configuration file does not exist. Use default one
SRC := $(call my-dir)/include/vnd_generic.txt
endif

GEN := $(generated_sources)/vnd_buildcfg.h
TOOL := $(LOCAL_PATH)/gen-buildcfg.sh

$(GEN): PRIVATE_PATH := $(call my-dir)
$(GEN): PRIVATE_CUSTOM_TOOL = $(TOOL) $< $@
$(GEN): $(SRC)  $(TOOL)
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)
