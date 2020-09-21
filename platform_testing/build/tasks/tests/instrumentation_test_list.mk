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

instrumentation_tests := \
    HelloWorldTests \
    BluetoothInstrumentationTests \
    crashcollector \
    LongevityPlatformLibTests \
    ManagedProvisioningTests \
    FrameworksCoreTests \
    BinderProxyCountingTestApp \
    BinderProxyCountingTestService \
    FrameworksNetTests \
    FrameworksUiServicesTests \
    BstatsTestApp \
    ConnTestApp \
    FrameworksServicesTests \
    JobTestApp \
    SuspendTestApp \
    FrameworksUtilTests \
    MtpDocumentsProviderTests \
    MtpTests \
    DocumentsUITests \
    ShellTests \
    SystemUITests \
    TestablesTests \
    FrameworksWifiApiTests \
    FrameworksWifiTests \
    FrameworksTelephonyTests \
    ContactsProviderTests \
    ContactsProviderTests2 \
    SettingsUnitTests \
    TelecomUnitTests \
    TraceurUiTests \
    AndroidVCardTests \
    PermissionFunctionalTests \
    BlockedNumberProviderTest \
    SettingsFunctionalTests \
    LauncherFunctionalTests \
    DownloadAppFunctionalTests \
    NotificationFunctionalTests \
    DexLoggerIntegrationTests \
    UsbTests \
    DownloadProviderTests \
    EmergencyInfoUnitTests \
    CalendarProviderTests \
    SettingsLibTests \
    RSTest \
    PrintSpoolerOutOfProcessTests \
    CellBroadcastReceiverUnitTests \
    TelephonyProviderTests \
    CarrierConfigTests \
    TeleServiceTests \
    PresencePollingTests \
    SettingsProviderTest \
    FrameworksLocationTests \
    FrameworksPrivacyLibraryTests \
    SettingsUITests

# Android Things specific tests
ifeq ($(PRODUCT_IOT),true)

instrumentation_tests += \
    AndroidThingsTests \
    ThingsIntegrationTests \
    WifiSetupUnitTests

endif  # PRODUCT_IOT == true

# Storage Manager may not exist on device
ifneq ($(filter StorageManager, $(PRODUCT_PACKAGES)),)

instrumentation_tests += StorageManagerUnitTests

endif
