# Rules for running robolectric tests.
#
# Uses the following variables:
#
#   LOCAL_JAVA_LIBRARIES
#   LOCAL_STATIC_JAVA_LIBRARIES
#   LOCAL_ROBOTEST_FAILURE_FATAL
#   LOCAL_ROBOTEST_TIMEOUT
#   LOCAL_TEST_PACKAGE
#   ROBOTEST_FAILURE_FATAL
#   ROBOTEST_FILTER
#   ROBOTEST_RUN_INDIVIDUALLY
#
#
# If ROBOTEST_FAILURE_FATAL is set to true then failing tests will cause a
# build failure. Otherwise failures will be logged but ignored by make.
#
# If ROBOTEST_FILTER is set to a regex then only tests matching that pattern
# will be run. This currently only works at the class level.
#
# TODO: Switch to a JUnit runner which can support method-level test
# filtering and use that rather than grep to implement ROBOTEST_FILTER.
#
# If ROBOTEST_RUN_INDIVIDUALLY is set to true, each test class will be run by a
# different JVM, preventing any interaction between different tests. This is
# significantly slower than running all tests within the same JVM, but prevents
# unwanted interactions.
#
# Tests classes are found by looking for *Test.java files in
# LOCAL_PATH recursively.

################################################
# General settings, independent of the module. #
################################################

### Used for running tests.

# Where to find Robolectric.
my_robolectric_script_path := $(call my-dir)
my_robolectric_path := $(my_robolectric_script_path)/lib
# Explicitly define the jars and their classpath ordering.
my_robolectric_jars := \
    $(my_robolectric_script_path)/../android-all/android-all-4.1.2_r1-robolectric-0.jar \
    $(my_robolectric_script_path)/../android-all/android-all-4.2.2_r1.2-robolectric-0.jar \
    $(my_robolectric_script_path)/../android-all/android-all-4.3_r2-robolectric-0.jar \
    $(my_robolectric_script_path)/../android-all/android-all-4.4_r1-robolectric-1.jar \
    $(my_robolectric_script_path)/../android-all/android-all-5.0.0_r2-robolectric-1.jar \
    $(my_robolectric_script_path)/../android-all/android-all-6.0.0_r1-robolectric-0.jar \
    $(my_robolectric_path)/accessibility-test-framework-2.1.jar \
    $(my_robolectric_path)/ant-1.8.0.jar \
    $(my_robolectric_path)/ant-launcher-1.8.0.jar \
    $(my_robolectric_path)/asm-5.0.1.jar \
    $(my_robolectric_path)/asm-analysis-5.0.1.jar \
    $(my_robolectric_path)/asm-commons-5.0.1.jar \
    $(my_robolectric_path)/asm-tree-5.0.1.jar \
    $(my_robolectric_path)/asm-util-5.0.1.jar \
    $(my_robolectric_path)/backport-util-concurrent-3.1.jar \
    $(my_robolectric_path)/bcprov-jdk16-1.46.jar \
    $(my_robolectric_path)/classworlds-1.1-alpha-2.jar \
    $(my_robolectric_path)/commons-codec-1.3.jar \
    $(my_robolectric_path)/commons-logging-1.1.1.jar \
    $(my_robolectric_path)/guava-19.0.jar \
    $(my_robolectric_path)/hamcrest-core-1.1.jar \
    $(my_robolectric_path)/hamcrest-core-1.3.jar \
    $(my_robolectric_path)/hamcrest-library-1.3.jar \
    $(my_robolectric_path)/httpclient-4.0.3.jar \
    $(my_robolectric_path)/httpcore-4.0.1.jar \
    $(my_robolectric_path)/icu4j-53.1.jar \
    $(my_robolectric_path)/junit-4.12.jar \
    $(my_robolectric_path)/maven-ant-tasks-2.1.3.jar \
    $(my_robolectric_path)/maven-artifact-2.2.1.jar \
    $(my_robolectric_path)/maven-artifact-manager-2.2.1.jar \
    $(my_robolectric_path)/maven-error-diagnostics-2.2.1.jar \
    $(my_robolectric_path)/maven-model-2.2.1.jar \
    $(my_robolectric_path)/maven-plugin-registry-2.2.1.jar \
    $(my_robolectric_path)/maven-profile-2.2.1.jar \
    $(my_robolectric_path)/maven-project-2.2.1.jar \
    $(my_robolectric_path)/maven-repository-metadata-2.2.1.jar \
    $(my_robolectric_path)/maven-settings-2.2.1.jar \
    $(my_robolectric_path)/nekohtml-1.9.6.2.jar \
    $(my_robolectric_path)/plexus-container-default-1.0-alpha-9-stable-1.jar \
    $(my_robolectric_path)/plexus-interpolation-1.11.jar \
    $(my_robolectric_path)/plexus-utils-1.5.15.jar \
    $(my_robolectric_path)/protobuf-java-2.6.1.jar \
    $(my_robolectric_path)/robolectric-3.1.1.jar \
    $(my_robolectric_path)/robolectric-annotations-3.1.1.jar \
    $(my_robolectric_path)/robolectric-resources-3.1.1.jar \
    $(my_robolectric_path)/robolectric-utils-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v23-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v22-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v21-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v19-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v18-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v17-3.1.1.jar \
    $(my_robolectric_path)/shadows-core-v16-3.1.1.jar \
    $(my_robolectric_path)/shadows-httpclient-3.1.1.jar \
    $(my_robolectric_path)/shadows-multidex-3.1.1.jar \
    $(my_robolectric_path)/sqlite4java-0.282.jar \
    $(my_robolectric_path)/vtd-xml-2.11.jar \
    $(my_robolectric_path)/wagon-file-1.0-beta-6.jar \
    $(my_robolectric_path)/wagon-http-lightweight-1.0-beta-6.jar \
    $(my_robolectric_path)/wagon-http-shared-1.0-beta-6.jar \
    $(my_robolectric_path)/wagon-provider-api-1.0-beta-6.jar \
    $(my_robolectric_path)/xercesMinimal-1.9.6.2.jar \
    $(my_robolectric_path)/xmlpull-1.1.3.1.jar \
    $(my_robolectric_path)/xpp3_min-1.1.4c.jar \
    $(my_robolectric_path)/xstream-1.4.8.jar

my_collect_target := $(LOCAL_MODULE)-coverage
my_report_target := $(LOCAL_MODULE)-jacoco
# Whether or not to ignore the result of running the robotests.
# LOCAL_ROBOTEST_FAILURE_FATAL will take precedence over ROBOTEST_FAILURE_FATAL,
# if present.
ifneq ($(strip $(LOCAL_ROBOTEST_FAILURE_FATAL)),)
    ifeq ($(strip $(LOCAL_ROBOTEST_FAILURE_FATAL)),true)
        my_failure_fatal := true
    else
        my_failure_fatal := false
    endif
else
    ifeq ($(strip $(ROBOTEST_FAILURE_FATAL)),true)
        my_failure_fatal := true
    else
        my_failure_fatal := false
    endif
endif
# The timeout for the command. A value of '0' means no timeout. The default is
# 10 minutes.
ifneq ($(strip $(LOCAL_ROBOTEST_TIMEOUT)),)
    my_timeout := $(LOCAL_ROBOTEST_TIMEOUT)
else
    my_timeout := 600
endif
# Command to filter the list of test classes.
ifeq ($(strip $(ROBOTEST_FILTER)),)
    # If not specified, defaults to including all the tests.
    my_test_filter_command := cat
else
    my_test_filter_command := grep -E "$(ROBOTEST_FILTER)"
endif

# The directory containing the sources.
ifeq ($(strip $(LOCAL_INSTRUMENT_SOURCE_DIRS)),)
    # If not specified, defaults to the src and java directories in the parent
    # directory.
    my_instrument_source_dirs := $(dir $(LOCAL_PATH))/src $(dir $(LOCAL_PATH))/java
else
    my_instrument_source_dirs := $(LOCAL_INSTRUMENT_SOURCE_DIRS)
endif

##########################
# Used by base_rules.mk. #
##########################

LOCAL_MODULE_CLASS := FAKE
# This is actually a phony target that is never built.
LOCAL_BUILT_MODULE_STEM := test.fake
# Since it is not built, it cannot be installed. But we will define our own
# dist files, depending on which of the specific targets is invoked.
LOCAL_UNINSTALLABLE_MODULE := true
# Do not build it for checkbuild or mma
LOCAL_DONT_CHECK_MODULE := true

include $(BUILD_SYSTEM)/base_rules.mk


#############################
# Module specific settings. #
#############################

### Used for running tests.

# The list of test classes. Robolectric requires an explicit list of tests to
# run, which is compiled from the Java files ending in "Test" within the
# directory from which this module is invoked.
my_tests := \
    $(sort $(shell find $(LOCAL_PATH) -type f -name '*Test.java' \
        | sed -e 's!.*\(com/google.*Test\)\.java!\1!' \
              -e 's!.*\(com/android.*Test\)\.java!\1!' \
        | sed 's!/!\.!g' \
        | $(my_test_filter_command)))
# The source jars containing the tests.
my_srcs_jars := \
    $(foreach lib, \
        $(LOCAL_JAVA_LIBRARIES) $(LOCAL_STATIC_JAVA_LIBRARIES), \
        $(call intermediates-dir-for,JAVA_LIBRARIES,$(lib),,COMMON)/classes-pre-proguard.jar) \
    $(foreach lib, \
        $(LOCAL_TEST_PACKAGE), \
        $(call intermediates-dir-for,APPS,$(lib),,COMMON)/classes-pre-proguard.jar)
# The jars needed to run the tests.
my_jars := $(my_robolectric_jars) \
    prebuilts/sdk/$(LOCAL_SDK_VERSION)/android.jar \
    $(my_srcs_jars)



# Run tests.
my_target := $(LOCAL_BUILT_MODULE)

android_all_lib_path := $(my_robolectric_script_path)/../android-all
my_robolectric_path := $(intermediates.COMMON)/android-all

android_all_jars := $(call find-files-in-subdirs,$(android_all_lib_path),*.jar,.)
copy_android_all_jars := $(foreach j,$(android_all_jars),\
    $(android_all_lib_path)/$(j):$(my_robolectric_path)/$(j))
$(my_robolectric_path): $(call copy-many-files,$(copy_android_all_jars))

shadow_jars := \
    shadows-core-v16-3.1.1.jar \
    shadows-core-v17-3.1.1.jar \
    shadows-core-v18-3.1.1.jar \
    shadows-core-v19-3.1.1.jar \
    shadows-core-v21-3.1.1.jar \
    shadows-core-v22-3.1.1.jar \
    shadows-core-v23-3.1.1.jar
copy_shadow_jars := $(foreach j,$(shadow_jars),\
    $(my_robolectric_script_path)/lib/$(j):$(my_robolectric_path)/$(j))
$(my_robolectric_path): $(call copy-many-files,$(copy_shadow_jars))

$(my_target): $(my_robolectric_path)

# Setting the DEBUG_ROBOLECTRIC environment variable will print additional logging from
# Robolectric and also make it wait for a debugger to be connected.
# For Android Studio / IntelliJ the debugger can be connected via the "remote" configuration:
#     https://www.jetbrains.com/help/idea/2016.2/run-debug-configuration-remote.html
# From command line the debugger can be connected via
#     jdb -attach localhost:5005
ifdef DEBUG_ROBOLECTRIC
    # The arguments to the JVM needed to debug the tests.
    # - server: wait for connection rather than connecting to a debugger
    # - transport: how to accept debugger connections (sockets)
    # - address: the port on which to accept debugger connections
    # - timeout: how long (in ms) to wait for a debugger to connect
    # - suspend: do not start running any code until the debugger connects
    my_java_args := \
        -Drobolectric.logging.enabled=true \
        -Xdebug -agentlib:jdwp=server=y,transport=dt_socket,address=5005,suspend=y

    # Remove the timeout so Robolectric doesn't get killed while debugging
    my_timeout := 0
endif

include $(my_robolectric_script_path)/robotest-internal.mk
# clean local variables
my_java_args :=
my_target :=

# Target for running robolectric tests using jacoco
my_target := $(my_collect_target)
my_jacoco_dir := \
    prebuilts/misc/common/jacoco

my_coverage_dir := $(dir $(LOCAL_BUILT_MODULE))/coverage/
my_coverage_file := $(my_coverage_dir)/jacoco.exec

# List of packages to exclude jacoco from running
my_jacoco_excludes := \
    org.robolectric.*:org.mockito.*:org.junit.*:org.objectweb.*:com.thoughtworks.xstream.*
# The Jacoco agent JAR.
my_jacoco_agent_jar := \
    $(my_jacoco_dir)/lib/jacocoagent.jar
my_coverage_java_args := \
    -javaagent:$(my_jacoco_agent_jar)=destfile=$(my_coverage_file),excludes=$(my_jacoco_excludes)
my_java_args := $(my_coverage_java_args)
include $(my_robolectric_script_path)/robotest-internal.mk
# Clear temporary variables
my_coverage_java_args :=
my_failure_fatal :=
my_jacoco_agent_jar :=
my_jacoco_dir :=
my_jacoco_excludes :=
my_java_args :=
my_robolectric_jars :=
my_robolectric_path :=
my_target :=
my_test_filter_command :=
my_tests :=

# Target for generating code coverage reports using jacoco.exec
my_target := $(my_report_target)
# The JAR file containing the report generation tool.
my_coverage_report_class := \
    com.google.android.jacoco.reporter.ReportGenerator
my_coverage_report_jar := \
    $(call intermediates-dir-for, \
        JAVA_LIBRARIES, jvm-jacoco-reporter,host)/javalib.jar
my_coverage_srcs_jars := $(my_srcs_jars)
my_coverage_report_dist_file := $(my_report_target)-html.zip

## jacoco code coverage reports
include $(my_robolectric_script_path)/report-internal.mk
# Clear temporary variables
my_coverage_dir :=
my_coverage_file :=
my_coverage_report_class :=
my_coverage_report_dist_file :=
my_coverage_report_jar :=
my_coverage_srcs_jars :=
my_robolectric_script_path :=
my_srcs_jars :=
my_target :=

# Clear local variables specific to this build.
LOCAL_ROBOTEST_FAILURE_FATAL :=
LOCAL_ROBOTEST_TIMEOUT :=
