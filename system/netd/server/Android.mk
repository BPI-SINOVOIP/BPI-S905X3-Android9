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

###
### netd service AIDL interface.
###
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -Werror -Wthread-safety
LOCAL_MODULE := libnetdaidl_static
LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libutils
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/binder
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder frameworks/native/aidl/binder
LOCAL_C_INCLUDES := $(LOCAL_PATH)/binder
LOCAL_SRC_FILES := \
        binder/android/net/INetd.aidl \
        binder/android/net/UidRange.cpp

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -Werror -Wthread-safety
LOCAL_MODULE := libnetdaidl
LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libutils
LOCAL_WHOLE_STATIC_LIBRARIES := libnetdaidl_static
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/binder

include $(BUILD_SHARED_LIBRARY)

###
### netd daemon.
###
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
        $(call include-path-for, libhardware_legacy)/hardware_legacy \
        bionic/libc/dns/include \
        external/mdnsresponder/mDNSShared \
        system/netd/include \

LOCAL_CPPFLAGS := -Wall -Werror -Wthread-safety -Wnullable-to-nonnull-conversion
LOCAL_MODULE := netd

# Bug: http://b/29823425 Disable -Wvarargs for Clang update to r271374
LOCAL_CPPFLAGS +=  -Wno-varargs \

ifeq ($(TARGET_ARCH), x86)
ifneq ($(TARGET_PRODUCT), gce_x86_phone)
        LOCAL_CPPFLAGS += -D NETLINK_COMPAT32
endif
endif

LOCAL_INIT_RC := netd.rc

LOCAL_SHARED_LIBRARIES := \
        android.system.net.netd@1.0 \
        android.system.net.netd@1.1 \
        libbinder \
        libbpf    \
        libcrypto \
        libcutils \
        libdl \
        libhidlbase \
        libhidltransport \
        liblog \
        liblogwrap \
        libmdnssd \
        libnetdaidl \
        libnetutils \
        libnetdutils \
        libselinux \
        libssl \
        libsysutils \
        libbase \
        libutils \
        libpcap \
        libqtaguid \

LOCAL_SRC_FILES := \
        BandwidthController.cpp \
        ClatdController.cpp \
        CommandListener.cpp \
        Controllers.cpp \
        DnsProxyListener.cpp \
        DummyNetwork.cpp \
        DumpWriter.cpp \
        EventReporter.cpp \
        FirewallController.cpp \
        FwmarkServer.cpp \
        IdletimerController.cpp \
        InterfaceController.cpp \
        IptablesRestoreController.cpp \
        LocalNetwork.cpp \
        MDnsSdListener.cpp \
        NetdCommand.cpp \
        NetdConstants.cpp \
        NetdHwService.cpp \
        NetdNativeService.cpp \
        NetlinkHandler.cpp \
        NetlinkManager.cpp \
        NetlinkCommands.cpp \
        NetlinkListener.cpp \
        Network.cpp \
        NetworkController.cpp \
        NFLogListener.cpp \
        PhysicalNetwork.cpp \
        PppController.cpp \
        ResolverController.cpp \
        RouteController.cpp \
        SockDiag.cpp \
        StrictController.cpp \
        TetherController.cpp \
        TrafficController.cpp \
        UidRanges.cpp \
        VirtualNetwork.cpp \
        WakeupController.cpp \
        XfrmController.cpp \
        TcpSocketMonitor.cpp \
        main.cpp \
        oem_iptables_hook.cpp \
        binder/android/net/UidRange.cpp \
        binder/android/net/metrics/INetdEventListener.aidl \
        dns/DnsTlsDispatcher.cpp \
        dns/DnsTlsQueryMap.cpp \
        dns/DnsTlsTransport.cpp \
        dns/DnsTlsServer.cpp \
        dns/DnsTlsSessionCache.cpp \
        dns/DnsTlsSocket.cpp \

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/binder

include $(BUILD_EXECUTABLE)


###
### ndc binary.
###
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -Werror -Wthread-safety
LOCAL_SANITIZE := unsigned-integer-overflow
LOCAL_CLANG := true
LOCAL_MODULE := ndc
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SRC_FILES := ndc.cpp

include $(BUILD_EXECUTABLE)

###
### netd unit tests.
###
include $(CLEAR_VARS)
LOCAL_MODULE := netd_unit_test
LOCAL_COMPATIBILITY_SUITE := device-tests
LOCAL_SANITIZE := unsigned-integer-overflow
LOCAL_CFLAGS := -Wall -Werror -Wunused-parameter -Wthread-safety
# Bug: http://b/29823425 Disable -Wvarargs for Clang update to r271374
LOCAL_CFLAGS += -Wno-varargs

LOCAL_C_INCLUDES := \
        bionic/libc/dns/include \
        system/netd/include \
        system/netd/server \
        system/netd/server/binder \
        system/netd/tests \
        system/core/logwrapper/include \

LOCAL_SRC_FILES := \
        InterfaceController.cpp InterfaceControllerTest.cpp \
        Controllers.cpp ControllersTest.cpp \
        NetdConstants.cpp IptablesBaseTest.cpp \
        IptablesRestoreController.cpp IptablesRestoreControllerTest.cpp \
        BandwidthController.cpp BandwidthControllerTest.cpp \
        FirewallControllerTest.cpp FirewallController.cpp \
        IdletimerController.cpp IdletimerControllerTest.cpp \
        NetlinkCommands.cpp NetlinkManager.cpp \
        RouteController.cpp RouteControllerTest.cpp \
        SockDiagTest.cpp SockDiag.cpp \
        StrictController.cpp StrictControllerTest.cpp \
        TetherController.cpp TetherControllerTest.cpp \
        TrafficController.cpp TrafficControllerTest.cpp \
        XfrmController.cpp XfrmControllerTest.cpp \
        TcpSocketMonitor.cpp \
        UidRanges.cpp \
        NetlinkListener.cpp \
        WakeupController.cpp WakeupControllerTest.cpp \
        NFLogListener.cpp NFLogListenerTest.cpp \
        binder/android/net/UidRange.cpp \
        binder/android/net/metrics/INetdEventListener.aidl \
        ../tests/tun_interface.cpp \
        dns/DnsTlsDispatcher.cpp \
        dns/DnsTlsTransport.cpp \
        dns/DnsTlsServer.cpp \
        dns/DnsTlsSessionCache.cpp \
        dns/DnsTlsSocket.cpp \

LOCAL_MODULE_TAGS := tests
LOCAL_STATIC_LIBRARIES := libgmock libpcap
LOCAL_SHARED_LIBRARIES := \
        libbpf    \
        libnetdaidl \
        libbase \
        libbinder \
        libcrypto \
        libcutils \
        liblog \
        liblogwrap \
        libnetutils \
        libnetdutils \
        libqtaguid \
        libsysutils \
        libutils \
        libssl \

include $(BUILD_NATIVE_TEST)

