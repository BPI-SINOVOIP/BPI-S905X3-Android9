LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# platform-robolectric-android-all-stubs is a stubbed out android-all JAR. This is used in place of
# the SDK stubs JAR for apps that can use hidden APIs like Settings.
# To use this, add this to LOCAL_STATIC_JAVA_LIBRARIES of your test library.

# This jar is generated from the command
#   java -jar \
#     $ANDROID_HOST_OUT/framework/mkstubs.jar \
#     $OUT/../../common/obj/JAVA_LIBRARIES/robolectric_android-all_intermediates/classes.jar \
#     android-all-stubs.jar '+*'

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := \
    platform-robolectric-android-all-stubs:android-all/android-all-stubs.jar

include $(BUILD_MULTI_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
