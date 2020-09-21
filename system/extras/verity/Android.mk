LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := generate_verity_key
LOCAL_SRC_FILES := generate_verity_key.c
LOCAL_CFLAGS := -Wall -Werror
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto_utils libcrypto
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := VeritySigner.java Utils.java
LOCAL_MODULE := VeritySigner
LOCAL_JAR_MANIFEST := VeritySigner.mf
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := bouncycastle-host
include $(BUILD_HOST_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := BootSignature.java VeritySigner.java Utils.java
LOCAL_MODULE := BootSignature
LOCAL_JAR_MANIFEST := BootSignature.mf
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_JAVA_LIBRARIES := bouncycastle-host
include $(BUILD_HOST_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := verity_verifier.cpp
LOCAL_MODULE := verity_verifier
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_HOST_OS := linux
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_SANITIZE := integer
LOCAL_STATIC_LIBRARIES := \
    libfec \
    libfec_rs \
    libcrypto_utils \
    libcrypto \
    libext4_utils \
    libsparse \
    libsquashfs_utils \
    libbase \
    libz
LOCAL_CFLAGS := -Wall -Werror
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := verity_signer
LOCAL_MODULE := verity_signer
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := VeritySigner
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := boot_signer
LOCAL_MODULE := boot_signer
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := BootSignature
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := build_verity_metadata.py
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := build_verity_metadata.py
LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := build_verity_tree
LOCAL_SRC_FILES := build_verity_tree.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libsparse libz
LOCAL_SHARED_LIBRARIES := libcrypto libbase
LOCAL_CFLAGS += -Wall -Werror
include $(BUILD_HOST_EXECUTABLE)

include $(call first-makefiles-under,$(LOCAL_PATH))
