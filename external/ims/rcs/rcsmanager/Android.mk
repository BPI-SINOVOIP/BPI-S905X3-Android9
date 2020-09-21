 # Copyright (c) 2015, Motorola Mobility LLC
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:
 #     - Redistributions of source code must retain the above copyright
 #       notice, this list of conditions and the following disclaimer.
 #     - Redistributions in binary form must reproduce the above copyright
 #       notice, this list of conditions and the following disclaimer in the
 #       documentation and/or other materials provided with the distribution.
 #     - Neither the name of Motorola Mobility nor the
 #       names of its contributors may be used to endorse or promote products
 #       derived from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 # THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 # FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 # DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 # OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 # HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 # LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 # OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 # DAMAGE.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/src/java
LOCAL_SRC_FILES := \
    $(call all-java-files-under, src/java)
LOCAL_SRC_FILES += src/java/com/android/ims/internal/IRcsService.aidl \
        src/java/com/android/ims/internal/IRcsPresence.aidl \
        src/java/com/android/ims/IRcsPresenceListener.aidl

LOCAL_JAVA_LIBRARIES += ims-common

#LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := com.android.ims.rcsmanager
LOCAL_REQUIRED_MODULES := com.android.ims.rcsmanager.xml
include $(BUILD_JAVA_LIBRARY)

# We need to put the permissions XML file into /system/etc/permissions/ so the
# JAR can be dynamically loaded.
include $(CLEAR_VARS)
LOCAL_MODULE := com.android.ims.rcsmanager.xml
#LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


