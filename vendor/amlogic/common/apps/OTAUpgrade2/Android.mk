LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := libota

ifeq (($(shell test $(PLATFORM_SDK_VERSION) -ge 26 ) && ($(shell test $(PLATFORM_SDK_VERSION) -lt 28)  && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(PLATFORM_SDK_VERSION),28)
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
LOCAL_JAVA_LIBRARIES += org.apache.http.legacy
else
LOCAL_SDK_VERSION := current
endif

ifeq ($(PLATFORM_SDK_VERSION),23)
    LOCAL_SRC_FILES := $(TOP)/src/com/droidlogic/otaupgrade/BackupActivity.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/BadMovedSDcard.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/FileSelector.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/InstallPackage.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/LoaderReceiver.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/MainActivity.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/PrefUtils.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/UpdateActivity.java \
                       $(TOP)/src/com/droidlogic/otaupgrade/UpdateService.java
else
    LOCAL_SRC_FILES := $(call all-java-files-under, src)\
    $(call all-Iaidl-files-under, src)
endif
LOCAL_PACKAGE_NAME := OTAUpgrade
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PROGUARD_FLAGS := -include $(LOCAL_PATH)/proguard.flags

include $(BUILD_PACKAGE)
##############################################

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := libota:libs/libotaupdate.jar
include $(BUILD_MULTI_PREBUILT)



# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
