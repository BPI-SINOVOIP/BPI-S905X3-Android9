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

LOCAL_PATH := $(call my-dir)
wificond_cpp_flags := -Wall -Werror -Wno-unused-parameter
wificond_parent_dir := $(LOCAL_PATH)/../
wificond_includes := \
    $(wificond_parent_dir)


###
### wificond daemon.
###
include $(CLEAR_VARS)
LOCAL_MODULE := wificond
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_INIT_RC := wificond.rc
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    main.cpp
LOCAL_SHARED_LIBRARIES := \
    android.hardware.wifi.offload@1.0 \
    libbinder \
    libbase \
    libcutils \
    libhidlbase \
    libhidltransport \
    libminijail \
    libutils \
    libwifi-system \
    libwifi-system-iface
LOCAL_STATIC_LIBRARIES := \
    libwificond
include $(BUILD_EXECUTABLE)

###
### wificond static library
###
include $(CLEAR_VARS)
LOCAL_MODULE := libwificond
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    ap_interface_binder.cpp \
    ap_interface_impl.cpp \
    client_interface_binder.cpp \
    client_interface_impl.cpp \
    logging_utils.cpp \
    scanning/channel_settings.cpp \
    scanning/hidden_network.cpp \
    scanning/offload_scan_callback_interface_impl.cpp \
    scanning/pno_network.cpp \
    scanning/pno_settings.cpp \
    scanning/radio_chain_info.cpp \
    scanning/scan_result.cpp \
    scanning/offload/scan_stats.cpp \
    scanning/single_scan_settings.cpp \
    scanning/scan_utils.cpp \
    scanning/scanner_impl.cpp \
    scanning/offload/offload_scan_manager.cpp \
    scanning/offload/offload_callback.cpp \
    scanning/offload/offload_service_utils.cpp \
    scanning/offload/offload_scan_utils.cpp \
    server.cpp
LOCAL_SHARED_LIBRARIES := \
    android.hardware.wifi.offload@1.0 \
    libbase \
    libutils \
    libhidlbase \
    libhidltransport \
    libwifi-system \
    libwifi-system-iface
LOCAL_WHOLE_STATIC_LIBRARIES := \
    libwificond_ipc \
    libwificond_nl \
    libwificond_event_loop
include $(BUILD_STATIC_LIBRARY)

###
### wificond netlink library
###
include $(CLEAR_VARS)
LOCAL_MODULE := libwificond_nl
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    net/mlme_event.cpp \
    net/netlink_manager.cpp \
    net/netlink_utils.cpp \
    net/nl80211_attribute.cpp \
    net/nl80211_packet.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase
include $(BUILD_STATIC_LIBRARY)

###
### wificond event loop library
###
include $(CLEAR_VARS)
LOCAL_MODULE := libwificond_event_loop
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    looper_backed_event_loop.cpp
LOCAL_WHOLE_STATIC_LIBRARIES := \
    liblog \
    libbase \
    libutils
include $(BUILD_STATIC_LIBRARY)

###
### wificond IPC interface library
###
include $(CLEAR_VARS)
LOCAL_MODULE := libwificond_ipc
LOCAL_AIDL_INCLUDES += $(LOCAL_PATH)/aidl
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_SRC_FILES := \
    ipc_constants.cpp \
    aidl/android/net/wifi/IApInterface.aidl \
    aidl/android/net/wifi/IApInterfaceEventCallback.aidl \
    aidl/android/net/wifi/IClientInterface.aidl \
    aidl/android/net/wifi/IInterfaceEventCallback.aidl \
    aidl/android/net/wifi/IPnoScanEvent.aidl \
    aidl/android/net/wifi/IScanEvent.aidl \
    aidl/android/net/wifi/IWificond.aidl \
    aidl/android/net/wifi/IWifiScannerImpl.aidl \
    scanning/channel_settings.cpp \
    scanning/hidden_network.cpp \
    scanning/pno_network.cpp \
    scanning/pno_settings.cpp \
    scanning/radio_chain_info.cpp \
    scanning/scan_result.cpp \
    scanning/single_scan_settings.cpp
LOCAL_SHARED_LIBRARIES := \
    libbinder
include $(BUILD_STATIC_LIBRARY)

###
### test util library
###
include $(CLEAR_VARS)
LOCAL_MODULE := libwificond_test_utils
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    tests/integration/binder_dispatcher.cpp \
    tests/integration/process_utils.cpp \
    tests/shell_utils.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase
LOCAL_WHOLE_STATIC_LIBRARIES := \
    libwificond_ipc \
    libwificond_event_loop
include $(BUILD_STATIC_LIBRARY)

###
### wificond unit tests.
###
include $(CLEAR_VARS)
LOCAL_MODULE := wificond_unit_test
LOCAL_COMPATIBILITY_SUITE := device-tests
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    tests/ap_interface_impl_unittest.cpp \
    tests/client_interface_impl_unittest.cpp \
    tests/looper_backed_event_loop_unittest.cpp \
    tests/main.cpp \
    tests/mock_client_interface_impl.cpp \
    tests/mock_netlink_manager.cpp \
    tests/mock_netlink_utils.cpp \
    tests/mock_offload.cpp \
    tests/mock_offload_callback_handlers.cpp \
    tests/mock_offload_scan_callback_interface.cpp \
    tests/mock_offload_scan_callback_interface_impl.cpp \
    tests/mock_offload_scan_manager.cpp \
    tests/mock_offload_service_utils.cpp \
    tests/mock_scan_utils.cpp \
    tests/netlink_manager_unittest.cpp \
    tests/netlink_utils_unittest.cpp \
    tests/nl80211_attribute_unittest.cpp \
    tests/nl80211_packet_unittest.cpp \
    tests/offload_callback_test.cpp \
    tests/offload_hal_test_constants.cpp \
    tests/offload_scan_manager_test.cpp \
    tests/offload_scan_utils_test.cpp \
    tests/offload_test_utils.cpp \
    tests/scanner_unittest.cpp \
    tests/scan_result_unittest.cpp \
    tests/scan_settings_unittest.cpp \
    tests/scan_stats_unittest.cpp \
    tests/scan_utils_unittest.cpp \
    tests/server_unittest.cpp
LOCAL_STATIC_LIBRARIES := \
    libgmock \
    libgtest \
    libwifi-system-test \
    libwifi-system-iface-test \
    libwificond \
    libwificond_nl
LOCAL_SHARED_LIBRARIES := \
    android.hardware.wifi.offload@1.0 \
    libbase \
    libbinder \
    libcutils \
    libhidltransport \
    libhidlbase \
    liblog \
    libutils \
    libwifi-system \
    libwifi-system-iface
include $(BUILD_NATIVE_TEST)

###
### wificond device integration tests.
###
include $(CLEAR_VARS)
LOCAL_MODULE := wificond_integration_test
LOCAL_CPPFLAGS := $(wificond_cpp_flags)
LOCAL_C_INCLUDES := $(wificond_includes)
LOCAL_SRC_FILES := \
    tests/integration/ap_interface_test.cpp \
    tests/integration/client_interface_test.cpp \
    tests/integration/life_cycle_test.cpp \
    tests/integration/scanner_test.cpp \
    tests/integration/service_test.cpp \
    tests/main.cpp \
    tests/shell_unittest.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase \
    libbinder \
    libcutils \
    libutils \
    libwifi-system \
    libwifi-system-iface
LOCAL_STATIC_LIBRARIES := \
    libgmock \
    libwificond_ipc \
    libwificond_test_utils
include $(BUILD_NATIVE_TEST)
