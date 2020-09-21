### GENERATED, do not edit
### for changes, see genmakefiles.py
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := libv8gen
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
V8_LOCAL_JS_LIBRARY_FILES := \
	src/js/macros.py \
	src/messages.h \
	src/js/prologue.js \
	src/js/runtime.js \
	src/js/v8natives.js \
	src/js/array.js \
	src/js/string.js \
	src/js/arraybuffer.js \
	src/js/typedarray.js \
	src/js/collection.js \
	src/js/weak-collection.js \
	src/js/collection-iterator.js \
	src/js/promise.js \
	src/js/messages.js \
	src/js/templates.js \
	src/js/spread.js \
	src/js/proxy.js \
	src/js/harmony-string-padding.js \
	src/debug/mirrors.js \
	src/debug/debug.js \
	src/debug/liveedit.js
V8_LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES := \
	src/js/macros.py \
	src/messages.h \
	src/js/harmony-atomics.js
LOCAL_SRC_FILES += src/snapshot/snapshot-empty.cc
LOCAL_JS_LIBRARY_FILES := $(addprefix $(LOCAL_PATH)/, $(V8_LOCAL_JS_LIBRARY_FILES))
LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES := $(addprefix $(LOCAL_PATH)/, $(V8_LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES))
generated_sources := $(call local-generated-sources-dir)
JS2C_PY := $(generated_sources)/js2c.py $(generated_sources)/jsmin.py
$(JS2C_PY): $(generated_sources)/%.py : $(LOCAL_PATH)/tools/%.py | $(ACP)
	@echo "Copying $@"
	$(copy-file-to-target)
GEN1 := $(generated_sources)/libraries.cc
$(GEN1): SCRIPT := $(generated_sources)/js2c.py
$(GEN1): $(LOCAL_JS_LIBRARY_FILES) $(JS2C_PY)
	@echo "Generating libraries.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $@ CORE $(LOCAL_JS_LIBRARY_FILES)
V8_GENERATED_LIBRARIES := $(generated_sources)/libraries.cc
GEN2 := $(generated_sources)/experimental-libraries.cc
$(GEN2): SCRIPT := $(generated_sources)/js2c.py
$(GEN2): $(LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES) $(JS2C_PY)
	@echo "Generating experimental-libraries.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $@ EXPERIMENTAL $(LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES)
V8_GENERATED_LIBRARIES += $(generated_sources)/experimental-libraries.cc
GEN3 := $(generated_sources)/extra-libraries.cc
$(GEN3): SCRIPT := $(generated_sources)/js2c.py
$(GEN3): $(JS2C_PY)
	@echo "Generating extra-libraries.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $@ EXTRAS
V8_GENERATED_LIBRARIES += $(generated_sources)/extra-libraries.cc
GEN4 := $(generated_sources)/experimental-extra-libraries.cc
$(GEN4): SCRIPT := $(generated_sources)/js2c.py
$(GEN4): $(JS2C_PY)
	@echo "Generating experimental-extra-libraries.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $@ EXPERIMENTAL_EXTRAS
V8_GENERATED_LIBRARIES += $(generated_sources)/experimental-extra-libraries.cc
LOCAL_GENERATED_SOURCES += $(V8_GENERATED_LIBRARIES)
include $(BUILD_STATIC_LIBRARY)
