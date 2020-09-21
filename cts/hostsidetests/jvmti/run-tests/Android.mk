# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# shim classes. We use one that exposes the common functionality.
LOCAL_SHIM_CLASSES := \
  src/902-hello-transformation/src/art/Redefinition.java \
  src/903-hello-tagging/src/art/Main.java \
  src/989-method-trace-throw/src/art/Trace.java \
  src/993-breakpoints/src/art/Breakpoint.java \
  src/1902-suspend/src/art/Suspension.java \
  src/1911-get-local-var-table/src/art/Locals.java \
  src/1912-get-set-local-primitive/src/art/StackTrace.java \
  src/1923-frame-pop/src/art/FramePop.java \
  src/1927-exception-event/src/art/Exceptions.java \
  src/1930-monitor-info/src/art/Monitors.java \
  src/1934-jvmti-signal-thread/src/art/Threads.java \

LOCAL_SRC_FILES := $(LOCAL_SHIM_CLASSES)

# Actual test classes.
LOCAL_SRC_FILES += \
  src/901-hello-ti-agent/src/art/Test901.java \
  src/902-hello-transformation/src/art/Test902.java \
  src/903-hello-tagging/src/art/Test903.java \
  src/904-object-allocation/src/art/Test904.java \
  src/905-object-free/src/art/Test905.java \
  src/906-iterate-heap/src/art/Test906.java \
  src/907-get-loaded-classes/src/art/Test907.java \
    src/907-get-loaded-classes/src/art/Cerr.java \
  src/908-gc-start-finish/src/art/Test908.java \
  src/910-methods/src/art/Test910.java \
  src/911-get-stack-trace/src/art/Test911.java \
    src/911-get-stack-trace/src/art/AllTraces.java \
    src/911-get-stack-trace/src/art/ControlData.java \
    src/911-get-stack-trace/src/art/Frames.java \
    src/911-get-stack-trace/src/art/OtherThread.java \
    src/911-get-stack-trace/src/art/PrintThread.java \
    src/911-get-stack-trace/src/art/Recurse.java \
    src/911-get-stack-trace/src/art/SameThread.java \
    src/911-get-stack-trace/src/art/ThreadListTraces.java \
  src/912-classes/src-art/art/Test912.java \
    src/912-classes/src-art/art/DexData.java \
  src/913-heaps/src/art/Test913.java \
  src/914-hello-obsolescence/src/art/Test914.java \
  src/915-obsolete-2/src/art/Test915.java \
  src/917-fields-transformation/src/art/Test917.java \
  src/918-fields/src/art/Test918.java \
  src/919-obsolete-fields/src/art/Test919.java \
  src/920-objects/src/art/Test920.java \
  src/922-properties/src/art/Test922.java \
  src/923-monitors/src/art/Test923.java \
  src/924-threads/src/art/Test924.java \
  src/925-threadgroups/src/art/Test925.java \
  src/926-multi-obsolescence/src/art/Test926.java \
  src/927-timers/src/art/Test927.java \
  src/928-jni-table/src/art/Test928.java \
  src/930-hello-retransform/src/art/Test930.java \
  src/931-agent-thread/src/art/Test931.java \
  src/932-transform-saves/src/art/Test932.java \
  src/933-misc-events/src/art/Test933.java \
  src/940-recursive-obsolete/src/art/Test940.java \
  src/942-private-recursive/src/art/Test942.java \
  src/944-transform-classloaders/src/art/Test944.java \
  src/945-obsolete-native/src/art/Test945.java \
  src/947-reflect-method/src/art/Test947.java \
  src/951-threaded-obsolete/src/art/Test951.java \
  src/982-ok-no-retransform/src/art/Test982.java \
  src/983-source-transform-verify/src/art/Test983.java \
  src/984-obsolete-invoke/src/art/Test984.java \
  src/985-re-obsolete/src/art/Test985.java \
  src/986-native-method-bind/src/art/Test986.java \
  src/988-method-trace/src/art/Test988.java \
    src/988-method-trace/src/art/Test988Intrinsics.java \
  src/989-method-trace-throw/src/art/Test989.java \
  src/990-field-trace/src/art/Test990.java \
  src/991-field-trace-2/src/art/Test991.java \
  src/992-source-data/src/art/Test992.java \
    src/992-source-data/src/art/Target2.java \
  src/993-breakpoints/src/art/Test993.java \
  src/994-breakpoint-line/src/art/Test994.java \
  src/995-breakpoints-throw/src/art/Test995.java \
  src/996-breakpoint-obsolete/src/art/Test996.java \
  src/997-single-step/src/art/Test997.java \
  src/1900-track-alloc/src/art/Test1900.java \
  src/1901-get-bytecodes/src/art/Test1901.java \
  src/1902-suspend/src/art/Test1902.java \
  src/1903-suspend-self/src/art/Test1903.java \
  src/1904-double-suspend/src/art/Test1904.java \
  src/1906-suspend-list-me-first/src/art/Test1906.java \
  src/1907-suspend-list-self-twice/src/art/Test1907.java \
  src/1908-suspend-native-resume-self/src/art/Test1908.java \
  src/1909-per-agent-tls/src/art/Test1909.java \
  src/1910-transform-with-default/src/art/Test1910.java \
  src/1911-get-local-var-table/src/art/Test1911.java \
  src/1912-get-set-local-primitive/src/art/Test1912.java \
  src/1913-get-set-local-objects/src/art/Test1913.java \
  src/1914-get-local-instance/src/art/Test1914.java \
  src/1915-get-set-local-current-thread/src/art/Test1915.java \
  src/1916-get-set-current-frame/src/art/Test1916.java \
  src/1917-get-stack-frame/src/art/Test1917.java \
  src/1920-suspend-native-monitor/src/art/Test1920.java \
  src/1921-suspend-native-recursive-monitor/src/art/Test1921.java \
  src/1922-owned-monitors-info/src/art/Test1922.java \
  src/1923-frame-pop/src/art/Test1923.java \
  src/1924-frame-pop-toggle/src/art/Test1924.java \
  src/1925-self-frame-pop/src/art/Test1925.java \
  src/1926-missed-frame-pop/src/art/Test1926.java \
  src/1927-exception-event/src/art/Test1927.java \
  src/1928-exception-event-exception/src/art/Test1928.java \
  src/1930-monitor-info/src/art/Test1930.java \
  src/1931-monitor-events/src/art/Test1931.java \
  src/1932-monitor-events-misc/src/art/Test1932.java \
  src/1933-monitor-current-contended/src/art/Test1933.java \
  src/1934-jvmti-signal-thread/src/art/Test1934.java \
  src/1936-thread-end-events/src/art/Test1936.java \
  src/1937-transform-soft-fail/src/art/Test1937.java \
  src/1939-proxy-frames/src/art/Test1939.java \
  src/1941-dispose-stress/src/art/Test1941.java \
  src/1942-suspend-raw-monitor-exit/src/art/Test1942.java \
  src/1943-suspend-raw-monitor-wait/src/art/Test1943.java \

JVMTI_RUN_TEST_GENERATED_NUMBERS := \
  901 \
  902 \
  903 \
  904 \
  905 \
  906 \
  907 \
  908 \
  910 \
  911 \
  912 \
  913 \
  914 \
  915 \
  917 \
  918 \
  919 \
  920 \
  922 \
  923 \
  924 \
  925 \
  926 \
  927 \
  928 \
  930 \
  931 \
  932 \
  933 \
  940 \
  942 \
  944 \
  945 \
  947 \
  951 \
  982 \
  983 \
  984 \
  985 \
  986 \
  988 \
  989 \
  990 \
  991 \
  992 \
  993 \
  994 \
  995 \
  996 \
  997 \
  1900 \
  1901 \
  1902 \
  1903 \
  1904 \
  1906 \
  1907 \
  1908 \
  1909 \
  1910 \
  1911 \
  1912 \
  1913 \
  1914 \
  1915 \
  1916 \
  1917 \
  1920 \
  1921 \
  1922 \
  1923 \
  1924 \
  1925 \
  1926 \
  1927 \
  1928 \
  1930 \
  1931 \
  1932 \
  1933 \
  1934 \
  1936 \
  1937 \
  1939 \
  1941 \
  1942 \
  1943 \

# Try to enforce that the directories correspond to the Java files we pull in.
JVMTI_RUN_TEST_DIR_CHECK := $(sort $(foreach DIR,$(addprefix src/,$(JVMTI_RUN_TEST_GENERATED_NUMBERS)), \
  $(filter $(DIR)%,$(LOCAL_SRC_FILES))))
ifneq ($(sort $(LOCAL_SRC_FILES)),$(JVMTI_RUN_TEST_DIR_CHECK))
  $(error Missing file, compare $(sort $(LOCAL_SRC_FILES)) with $(JVMTI_RUN_TEST_DIR_CHECK))
endif

LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_JAVA_LANGUAGE_VERSION := 1.8
LOCAL_MODULE := run-test-jvmti-java

GENERATED_SRC_DIR := $(call local-generated-sources-dir)
JVMTI_RUN_TEST_GENERATED_FILES := \
  $(foreach NR,$(JVMTI_RUN_TEST_GENERATED_NUMBERS),$(GENERATED_SRC_DIR)/results.$(NR).expected.txt)

define GEN_JVMTI_RUN_TEST_GENERATED_FILE

GEN_INPUT := $(wildcard $(LOCAL_PATH)/src/$(1)*/expected.txt)
ifeq (true,$(ANDROID_COMPILE_WITH_JACK))
GEN_EXTRA_DIFF := $(wildcard $(LOCAL_PATH)/src/$(1)*/expected_jack.diff)
else ifeq (true,$(USE_D8))
GEN_EXTRA_DIFF := $(wildcard $(LOCAL_PATH)/src/$(1)*/expected_d8.diff)
else
GEN_EXTRA_DIFF :=
endif

GEN_OUTPUT := $(GENERATED_SRC_DIR)/results.$(1).expected.txt
$$(GEN_OUTPUT): PRIVATE_GEN_EXTRA_DIFF := $$(GEN_EXTRA_DIFF)
$$(GEN_OUTPUT): $$(GEN_INPUT) $$(GEN_EXTRA_DIFF)
	cp $$< $$@
ifneq (,$$(GEN_EXTRA_DIFF))
	(cd $$(dir $$@) && patch $$(notdir $$@)) < $$(PRIVATE_GEN_EXTRA_DIFF)
endif

GEN_INPUT :=
GEN_OUTPUT :=

endef

$(foreach NR,$(JVMTI_RUN_TEST_GENERATED_NUMBERS),\
  $(eval $(call GEN_JVMTI_RUN_TEST_GENERATED_FILE,$(NR))))
LOCAL_JAVA_RESOURCE_FILES := $(JVMTI_RUN_TEST_GENERATED_FILES)

# Avoid linking against any @hide APIs.
LOCAL_SDK_VERSION := current

include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
