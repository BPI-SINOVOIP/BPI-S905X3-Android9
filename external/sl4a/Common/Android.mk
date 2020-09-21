#
#  Copyright (C) 2017 Google, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at:
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)


LOCAL_MODULE := sl4a.Common
LOCAL_MODULE_OWNER := google

LOCAL_STATIC_JAVA_LIBRARIES := guava android-common sl4a.Utils junit
LOCAL_JAVA_LIBRARIES := telephony-common
LOCAL_JAVA_LIBRARIES += ims-common
LOCAL_JAVA_LIBRARIES += bouncycastle

LOCAL_SRC_FILES := $(call all-java-files-under, src/com/googlecode/android_scripting)
LOCAL_SRC_FILES += $(call all-java-files-under, src/org/apache/commons/codec)

include $(BUILD_STATIC_JAVA_LIBRARY)
