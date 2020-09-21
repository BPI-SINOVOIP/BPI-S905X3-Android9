# -*- mode: makefile -*-

LOCAL_PATH := $(call my-dir)

define all-harmony-test-java-files-under
  $(patsubst ./%,%,$(shell cd $(LOCAL_PATH) && find $(2) -name "*.java" 2> /dev/null))
endef

harmony_jdwp_test_src_files := \
    $(call all-harmony-test-java-files-under,,src/test/java/)

harmony_public_jdwp_test_src_files := \
	$(filter-out $(call all-harmony-test-java-files-under,,src/test/java/org/apache/harmony/jpda/tests/jdwp/DDM/),$(harmony_jdwp_test_src_files))

# Common JDWP settings
jdwp_test_timeout_ms := 10000 # 10s.
jdwp_test_target_runtime_common_args :=  \
	-Djpda.settings.verbose=true \
	-Djpda.settings.timeout=$(jdwp_test_timeout_ms) \
	-Djpda.settings.waitingTime=$(jdwp_test_timeout_ms)

# CTS configuration
#
# We run in non-debug mode and support running with a forced abi. We must pass
# -Xcompiler-option --debuggable to ART so the tests are compiled with full
# debugging capability.
cts_jdwp_test_runtime_target := dalvikvm|\#ABI\#| -XXlib:libart.so -Xcompiler-option --debuggable
cts_jdwp_test_target_runtime_args := -Xcompiler-option --debuggable
cts_jdwp_test_target_runtime_args += $(jdwp_test_target_runtime_common_args)
cts_jdwp_test_target_runtime_args += -Djpda.settings.debuggeeJavaPath='$(cts_jdwp_test_runtime_target)'

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_public_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := CtsJdwp
LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true
LOCAL_CTS_TEST_PACKAGE := android.jdwp
LOCAL_CTS_TARGET_RUNTIME_ARGS := $(cts_jdwp_test_target_runtime_args)
LOCAL_ERROR_PRONE_FLAGS := -Xep:ArrayToString:ERROR
include $(BUILD_CTS_TARGET_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE := apache-harmony-jdwp-tests
LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/jdwp
include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_public_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit-host
LOCAL_MODULE := apache-harmony-jdwp-tests-host
include $(BUILD_HOST_JAVA_LIBRARY)

ifeq ($(HOST_OS),linux)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(harmony_jdwp_test_src_files)
LOCAL_JAVA_LIBRARIES := junit-hostdex
LOCAL_MODULE := apache-harmony-jdwp-tests-hostdex
include $(BUILD_HOST_DALVIK_JAVA_LIBRARY)
endif  # HOST_OS == linux
