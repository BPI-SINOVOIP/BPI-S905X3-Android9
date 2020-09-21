LOCAL_PATH:= $(call my-dir)

## bundletool script

include $(CLEAR_VARS)

LOCAL_MODULE := bundletool
LOCAL_MODULE_TAG := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(HOST_OUT_JAVA_LIBRARIES)/bundletool$(COMMON_JAVA_PACKAGE_SUFFIX)
$(LOCAL_BUILT_MODULE): $(LOCAL_PATH)/etc/bundletool | $(ACP)
	@echo "Copy: $(PRIVATE_MODULE) ($@)"
	$(copy-file-to-new-target)
	$(hide) chmod 755 $@

## tool jar

include $(CLEAR_VARS)

LOCAL_MODULE := bundletool

LOCAL_SRC_FILES := $(call all-java-files-under, java)
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := guavalib jsr305lib dagger2-auto-value-host error_prone_annotations-2.0.18

LOCAL_ANNOTATION_PROCESSORS := dagger2-auto-value-host
LOCAL_ANNOTATION_PROCESSOR_CLASSES := com.google.auto.value.processor.AutoValueProcessor

LOCAL_JAR_MANIFEST := manifest.txt

include $(BUILD_HOST_JAVA_LIBRARY)
