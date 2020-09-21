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

native_tests := \
    adbd_test \
    bionic-unit-tests \
    bionic-unit-tests-static \
    bluetoothtbd_test \
    bootstat_tests \
    boringssl_crypto_test \
    boringssl_ssl_test \
    buffer_hub-test \
    buffer_hub_queue-test \
    buffer_hub_queue_producer-test \
    bugreportz_test \
    bsdiff_unittest \
    camera_client_test \
    crashcollector \
    debuggerd_test \
    dumpstate_test \
    dumpstate_test_fixture \
    dumpsys_test \
    dvr_api-test \
    dvr_buffer_queue-test \
    dvr_display-test \
    gatekeeperd-unit-tests \
    hello_world_test \
    hwui_unit_tests \
    incident_helper_test \
    incidentd_test \
    init_tests \
    inputflinger_tests_InputDispatcher_test \
    inputflinger_tests_InputReader_test \
    installd_cache_test \
    installd_dexopt_test \
    installd_otapreopt_test \
    installd_service_test \
    installd_utils_test \
    JniInvocation_test \
    libandroidfw_tests \
    libappfuse_test \
    libbase_test \
    libcutils_test \
    libcutils_test_static \
    libgui_test \
    libhidl_test \
    libinput_tests_InputChannel_test \
    libinput_tests_InputEvent_test \
    libinput_tests_InputPublisherAndConsumer_test \
    libinput_tests_VelocityTracker_test \
    libjavacore-unit-tests \
    liblog-unit-tests \
    libminijail_unittest_gtest \
    libperfmgr_test \
    libtextclassifier_tests \
    libsurfaceflinger_unittest \
    libvintf_test \
    libwifi-system_tests \
    linker-unit-tests \
    logcat-unit-tests \
    logd-unit-tests \
    kernel-config-unit-tests \
    malloc_debug_unit_tests \
    memory_replay_tests \
    memunreachable_test \
    minadbd_test \
    minikin_tests \
    mj_system_unittest_gtest \
    mj_util_unittest_gtest \
    mtp_ffs_handle_test \
    net_test_avrcp \
    net_test_bluetooth \
    net_test_bta \
    net_test_btcore \
    net_test_btif \
    net_test_btif_profile_queue \
    net_test_btpackets \
    net_test_btu_message_loop \
    net_test_device \
    net_test_hci \
    net_test_osi \
    net_test_performance \
    net_test_stack \
    net_test_stack_ad_parser \
    net_test_stack_multi_adv \
    net_test_stack_smp \
    net_test_types \
    netd_integration_test \
    netd_unit_test \
    nfc_test_utils \
    pagemap_test \
    perfetto_integrationtests \
    perfprofd_test \
    posix_async_io_test \
    prioritydumper_test \
    recovery_component_test \
    recovery_unit_test \
    scrape_mmap_addr \
    simpleperf_cpu_hotplug_test \
    simpleperf_unit_test \
    statsd_test \
    syscall_filter_unittest_gtest \
    time-unit-tests \
    update_engine_unittests \
    vintf_object_test \
    wificond_unit_test \
    wifilogd_unit_test \
    ziparchive-tests \
    NeuralNetworksTest_static \
    SurfaceFlinger_test \
    lmkd_unit_test

# Android Things specific tests
ifeq ($(PRODUCT_IOT),true)

native_tests += \
    crash_reporter_tests \
    libandroidthings_pio_tests \
    libbrillo_test \
    libchrome_test \
    libusersensors_binder_test \
    peripheralman_unittests \
    sensors_userdriver_test \
    userinputdriver_test

endif  # PRODUCT_IOT == true
