#
# Copyright (C) 2016 The Android Open Source Project
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
#
LOCAL_PATH := $(call my-dir)

# APCT build target
include $(CLEAR_VARS)
LOCAL_MODULE := netd_integration_test
LOCAL_COMPATIBILITY_SUITE := device-tests
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter
# Bug: http://b/29823425 Disable -Wvarargs for Clang update to r271374
LOCAL_CFLAGS += -Wno-varargs

EXTRA_LDLIBS := -lpthread
LOCAL_SHARED_LIBRARIES += libbase libbinder libbpf libcrypto libcutils liblog \
                          libnetd_client libnetutils libssl libutils
LOCAL_STATIC_LIBRARIES += libnetd_test_dnsresponder liblogwrap libnetdaidl_static \
                          libnetdutils libnetd_test_tun_interface libbpf
LOCAL_AIDL_INCLUDES := system/netd/server/binder
LOCAL_C_INCLUDES += system/netd/include system/netd/binder/include \
                    system/netd/server system/core/logwrapper/include \
                    system/netd/tests/dns_responder \
                    system/core/libnetutils/include \
                    bionic/libc/dns/include
# netd_integration_test.cpp is currently empty and exists only so that we can do:
# runtest -x system/netd/tests/netd_integration_test.cpp
LOCAL_SRC_FILES := binder_test.cpp \
                   bpf_base_test.cpp \
                   dns_responder/dns_responder.cpp \
                   dns_tls_test.cpp \
                   netd_integration_test.cpp \
                   netd_test.cpp \
                   ../server/NetdConstants.cpp \
                   ../server/binder/android/net/metrics/INetdEventListener.aidl \
                   ../server/dns/DnsTlsDispatcher.cpp \
                   ../server/dns/DnsTlsQueryMap.cpp \
                   ../server/dns/DnsTlsTransport.cpp \
                   ../server/dns/DnsTlsServer.cpp \
                   ../server/dns/DnsTlsSessionCache.cpp \
                   ../server/dns/DnsTlsSocket.cpp \
                   ../server/InterfaceController.cpp \
                   ../server/NetlinkCommands.cpp \
                   ../server/XfrmController.cpp
LOCAL_MODULE_TAGS := eng tests
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE)32
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE)64
include $(BUILD_NATIVE_TEST)

include $(call all-makefiles-under, $(LOCAL_PATH))
