LOCAL_PATH:= $(call my-dir)

# Build the Phone app which includes the emergency dialer. See Contacts
# for the 'other' dialer.
include $(CLEAR_VARS)

phone_common_dir := ../../apps/PhoneCommon

src_dirs := src $(phone_common_dir)/src sip/src
res_dirs := res $(phone_common_dir)/res sip/res

LOCAL_JAVA_LIBRARIES := \
        telephony-common \
        voip-common \
        ims-common \
        org.apache.http.legacy

LOCAL_STATIC_ANDROID_LIBRARIES := \
        android-support-v7-appcompat \
        android-support-v7-preference \
        android-support-v7-recyclerview \
        android-support-v14-preference

LOCAL_STATIC_JAVA_LIBRARIES := \
        guava \
        volley

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs))
LOCAL_SRC_FILES += \
        src/com/android/phone/EventLogTags.logtags \
        src/com/android/phone/INetworkQueryService.aidl \
        src/com/android/phone/INetworkQueryServiceCallback.aidl
LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))
LOCAL_USE_AAPT2 := true

LOCAL_AAPT_FLAGS := \
    --extra-packages com.android.phone.common \
    --extra-packages com.android.services.telephony.sip

LOCAL_PACKAGE_NAME := TeleService
LOCAL_PRIVATE_PLATFORM_APIS := true

LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

LOCAL_PROGUARD_FLAG_FILES := proguard.flags sip/proguard.flags

include frameworks/base/packages/SettingsLib/common.mk

include $(BUILD_PACKAGE)

# Build the test package
include $(call all-makefiles-under,$(LOCAL_PATH))
