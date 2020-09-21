/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tradefed.targetprep;

import com.android.ddmlib.IDevice;
import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TcpDevice;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.util.BinaryState;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import org.easymock.Capture;
import org.easymock.EasyMock;

import java.io.File;

/**
 * Unit tests for {@link DeviceSetup}.
 */
public class DeviceSetupTest extends TestCase {

    private DeviceSetup mDeviceSetup;
    private ITestDevice mMockDevice;
    private IDevice mMockIDevice;
    private IDeviceBuildInfo mMockBuildInfo;
    private File mTmpDir;

    private static final int DEFAULT_API_LEVEL = 23;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockIDevice = EasyMock.createMock(IDevice.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andReturn("foo").anyTimes();
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
        EasyMock.expect(mMockDevice.getIDevice()).andStubReturn(mMockIDevice);
        mMockBuildInfo = new DeviceBuildInfo("0", "");
        mDeviceSetup = new DeviceSetup();
        mTmpDir = FileUtil.createTempDir("tmp");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        FileUtil.recursiveDelete(mTmpDir);
        super.tearDown();
    }

    /**
     * Simple normal case test for {@link DeviceSetup#setUp(ITestDevice, IBuildInfo)}.
     */
    public void testSetup() throws Exception {
        Capture<String> setPropCapture = new Capture<>();
        doSetupExpectations(true, setPropCapture);
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);

        String setProp = setPropCapture.getValue();
        assertTrue("Set prop doesn't contain ro.telephony.disable-call=true",
                setProp.contains("ro.telephony.disable-call=true\n"));
        assertTrue("Set prop doesn't contain ro.audio.silent=1",
                setProp.contains("ro.audio.silent=1\n"));
        assertTrue("Set prop doesn't contain ro.test_harness=1",
                setProp.contains("ro.test_harness=1\n"));
        assertTrue("Set prop doesn't contain ro.monkey=1",
                setProp.contains("ro.monkey=1\n"));
    }

    public void testSetup_airplane_mode_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "airplane_mode_on", "1");
        doCommandsExpectations(
                "am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAirplaneMode(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_airplane_mode_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "airplane_mode_on", "0");
        doCommandsExpectations(
                "am broadcast -a android.intent.action.AIRPLANE_MODE --ez state false");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAirplaneMode(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_data_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "mobile_data", "1");
        doCommandsExpectations("svc data enable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setData(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_data_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "mobile_data", "0");
        doCommandsExpectations("svc data disable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setData(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_cell_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "cell_on", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setCell(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_cell_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "cell_on", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setCell(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_cell_auto_setting_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "clockwork_cell_auto_setting", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setCellAutoSetting(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_cell_auto_setting_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "clockwork_cell_auto_setting", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setCellAutoSetting(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wifi_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "wifi_on", "1");
        doCommandsExpectations("svc wifi enable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWifi(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wifi_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "wifi_on", "0");
        doCommandsExpectations("svc wifi disable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWifi(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wifi_watchdog_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "wifi_watchdog", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWifiWatchdog(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wifi_watchdog_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "wifi_watchdog", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWifiWatchdog(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_disable_cw_wifi_mediator_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "cw_disable_wifimediator", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableCwWifiMediator(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_disable_cw_wifi_mediator_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "cw_disable_wifimediator", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableCwWifiMediator(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wifi_scan_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "wifi_scan_always_enabled", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWifiScanAlwaysEnabled(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wifi_scan_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "wifi_scan_always_enabled", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWifiScanAlwaysEnabled(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_ethernet_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("ifconfig eth0 up");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setEthernet(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_ethernet_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("ifconfig eth0 down");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setEthernet(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_bluetooth_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("service call bluetooth_manager 6");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setBluetooth(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_bluetooth_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("service call bluetooth_manager 8");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setBluetooth(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_nfc_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("svc nfc enable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setNfc(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_nfc_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("svc nfc disable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setNfc(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_adaptive_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "screen_brightness_mode", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAdaptiveBrightness(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_adaptive_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "screen_brightness_mode", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAdaptiveBrightness(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_brightness() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "screen_brightness", "50");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenBrightness(50);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_stayon_default() throws Exception {
        doSetupExpectations(false /* Expect no screen on command */, new Capture<String>());
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAlwaysOn(BinaryState.IGNORE);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_stayon_off() throws Exception {
        doSetupExpectations(false /* Expect no screen on command */, new Capture<String>());
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("svc power stayon false");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAlwaysOn(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_timeout() throws Exception {
        doSetupExpectations(false /* Expect no screen on command */, new Capture<String>());
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "screen_off_timeout", "5000");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAlwaysOn(BinaryState.IGNORE);
        mDeviceSetup.setScreenTimeoutSecs(5l);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_ambient_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "doze_enabled", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAmbientMode(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_ambient_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "doze_enabled", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenAmbientMode(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wake_gesture_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "wake_gesture_enabled", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWakeGesture(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_wake_gesture_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "wake_gesture_enabled", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setWakeGesture(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_saver_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "screensaver_enabled", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenSaver(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_screen_saver_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "screensaver_enabled", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setScreenSaver(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_notification_led_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "notification_light_pulse", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setNotificationLed(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_notification_led_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "notification_light_pulse", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setNotificationLed(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testInstallNonMarketApps_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "install_non_market_apps", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setInstallNonMarketApps(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testInstallNonMarketApps_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "install_non_market_apps", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setInstallNonMarketApps(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_trigger_media_mounted() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations(
                "am broadcast -a android.intent.action.MEDIA_MOUNTED "
                        + "-d file://${EXTERNAL_STORAGE} --receiver-include-background");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setTriggerMediaMounted(true);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_location_gps_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "location_providers_allowed", "+gps");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setLocationGps(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_location_gps_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "location_providers_allowed", "-gps");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setLocationGps(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_location_network_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "location_providers_allowed", "+network");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setLocationNetwork(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_location_network_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("secure", "location_providers_allowed", "-network");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setLocationNetwork(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_rotate_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "accelerometer_rotation", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAutoRotate(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_rotate_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("system", "accelerometer_rotation", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAutoRotate(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_battery_saver_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "low_power", "1");
        doCommandsExpectations("dumpsys battery unplug");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setBatterySaver(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_legacy_battery_saver_on() throws Exception {
        doSetupExpectations(21); // API level Lollipop
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "low_power", "1");
        doCommandsExpectations("dumpsys battery set usb 0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setBatterySaver(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_battery_saver_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "low_power", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setBatterySaver(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_battery_saver_trigger() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "low_power_trigger_level", "50");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setBatterySaverTrigger(50);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_enable_full_battery_stats_history() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("dumpsys batterystats --enable full-history");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setEnableFullBatteryStatsHistory(true);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_disable_doze() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("dumpsys deviceidle disable");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableDoze(true);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_update_time_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "auto_time", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAutoUpdateTime(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_update_time_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "auto_time", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAutoUpdateTime(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_update_timezone_on() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "auto_timezone", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAutoUpdateTimezone(BinaryState.ON);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_update_timezone_off() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "auto_timezone", "0");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setAutoUpdateTimezone(BinaryState.OFF);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_set_timezone_LA() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doCommandsExpectations("setprop \"persist.sys.timezone\" \"America/Los_Angeles\"");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setTimezone("America/Los_Angeles");
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_no_disable_dialing() throws Exception {
        Capture<String> setPropCapture = new Capture<>();
        doSetupExpectations(true, setPropCapture);
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableDialing(false);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);

        assertFalse("Set prop contains ro.telephony.disable-call=true",
                setPropCapture.getValue().contains("ro.telephony.disable-call=true\n"));
    }

    public void testSetup_sim_data() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "multi_sim_data_call", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDefaultSimData(1);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_sim_voice() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "multi_sim_voice_call", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDefaultSimVoice(1);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_sim_sms() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSettingExpectations("global", "multi_sim_sms", "1");
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDefaultSimSms(1);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);
    }

    public void testSetup_no_disable_audio() throws Exception {
        Capture<String> setPropCapture = new Capture<>();
        doSetupExpectations(true, setPropCapture);
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableAudio(false);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);

        assertFalse("Set prop contains ro.audio.silent=1",
                setPropCapture.getValue().contains("ro.audio.silent=1\n"));
    }

    public void testSetup_no_test_harness() throws Exception {
        Capture<String> setPropCapture = new Capture<>();
        doSetupExpectations(true, setPropCapture);
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setTestHarness(false);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);

        String setProp = setPropCapture.getValue();
        assertFalse("Set prop contains ro.test_harness=1",
                setProp.contains("ro.test_harness=1\n"));
        assertFalse("Set prop contains ro.monkey=1",
                setProp.contains("ro.monkey=1\n"));
    }

    public void testSetup_disalbe_dalvik_verifier() throws Exception {
        Capture<String> setPropCapture = new Capture<>();
        doSetupExpectations(true, setPropCapture);
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableDalvikVerifier(true);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);

        String setProp = setPropCapture.getValue();
        assertTrue("Set prop doesn't contain dalvik.vm.dexopt-flags=v=n",
                setProp.contains("dalvik.vm.dexopt-flags=v=n\n"));
    }

    /**
     * Test {@link DeviceSetup#setUp(ITestDevice, IBuildInfo)} when free space check fails.
     */
    public void testSetup_freespace() throws Exception {
        doSetupExpectations();
        mDeviceSetup.setMinExternalStorageKb(500);
        EasyMock.expect(mMockDevice.getExternalStoreFreeSpace()).andReturn(1L);
        EasyMock.replay(mMockDevice);
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("DeviceNotAvailableException not thrown");
        } catch (DeviceNotAvailableException e) {
            // expected
        }
    }

    /**
     * Test {@link DeviceSetup#setUp(ITestDevice, IBuildInfo)} when local data path does not
     * exist.
     */
    public void testSetup_badLocalData() throws Exception {
        doSetupExpectations();
        mDeviceSetup.setLocalDataPath(new File("idontexist"));
        EasyMock.replay(mMockDevice);
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
    }

    /**
     * Test normal case {@link DeviceSetup#setUp(ITestDevice, IBuildInfo)} when local data
     * is synced
     */
    public void testSetup_syncData() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        doSyncDataExpectations(true);

        EasyMock.replay(mMockDevice, mMockIDevice);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice, mMockIDevice);
    }

    /**
     * Test case {@link DeviceSetup#setUp(ITestDevice, IBuildInfo)} when local data fails to be
     * synced.
     */
    public void testSetup_syncDataFails() throws Exception {
        doSetupExpectations();
        doSyncDataExpectations(false);
        EasyMock.replay(mMockDevice, mMockIDevice);
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetup_legacy() throws Exception {
        Capture<String> setPropCapture = new Capture<>();
        doSetupExpectations(true, setPropCapture);
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDeprecatedAudioSilent(false);
        mDeviceSetup.setDeprecatedMinExternalStoreSpace(1000);
        mDeviceSetup.setDeprecatedSetProp("key=value");
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);

        EasyMock.verify(mMockDevice);

        String setProp = setPropCapture.getValue();
        assertTrue("Set prop doesn't contain ro.telephony.disable-call=true",
                setProp.contains("ro.telephony.disable-call=true\n"));
        assertTrue("Set prop doesn't contain ro.test_harness=1",
                setProp.contains("ro.test_harness=1\n"));
        assertTrue("Set prop doesn't contain ro.monkey=1",
                setProp.contains("ro.monkey=1\n"));
        assertTrue("Set prop doesn't contain key=value",
                setProp.contains("key=value\n"));
    }

    @SuppressWarnings("deprecation")
    public void testSetup_legacy_storage_conflict() throws Exception {
        doSetupExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setMinExternalStorageKb(1000);
        mDeviceSetup.setDeprecatedMinExternalStoreSpace(1000);
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError expected");
        } catch (TargetSetupError e) {
            // Expected
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetup_legacy_silent_conflict() throws Exception {
        doSetupExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setDisableAudio(false);
        mDeviceSetup.setDeprecatedAudioSilent(false);
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError expected");
        } catch (TargetSetupError e) {
            // Expected
        }
    }

    @SuppressWarnings("deprecation")
    public void testSetup_legacy_setprop_conflict() throws Exception {
        doSetupExpectations();
        EasyMock.replay(mMockDevice);

        mDeviceSetup.setProperty("key", "value");
        mDeviceSetup.setDeprecatedSetProp("key=value");
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError expected");
        } catch (TargetSetupError e) {
            // Expected
        }
    }

    public void test_restore_properties_previous_exists() throws Exception {
        File f = new File("");
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        EasyMock.expect(mMockDevice.pullFile("/data/local.prop")).andReturn(f).once();
        EasyMock.expect(mMockDevice.pushFile(f, "/data/local.prop")).andReturn(true).once();
        mMockDevice.reboot();
        EasyMock.expectLastCall().once();

        EasyMock.replay(mMockDevice);

        mDeviceSetup.setRestoreProperties(true);
        mDeviceSetup.setProperty("key", "value");
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
        mDeviceSetup.tearDown(mMockDevice, mMockBuildInfo, null);

        EasyMock.verify(mMockDevice);
    }

    public void test_restore_properties_previous_doesnt_exists() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        EasyMock.expect(mMockDevice.pullFile("/data/local.prop")).andReturn(null).once();
        EasyMock.expect(mMockDevice.executeShellCommand("rm -f /data/local.prop"))
                .andReturn(null)
                .once();
        mMockDevice.reboot();
        EasyMock.expectLastCall().once();

        EasyMock.replay(mMockDevice);

        mDeviceSetup.setRestoreProperties(true);
        mDeviceSetup.setProperty("key", "value");
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
        mDeviceSetup.tearDown(mMockDevice, mMockBuildInfo, null);

        EasyMock.verify(mMockDevice);
    }

    public void test_restore_settings() throws Exception {
        doSetupExpectations();
        doCheckExternalStoreSpaceExpectations();
        EasyMock.expect(mMockDevice.getApiLevel()).andStubReturn(23);
        EasyMock.expect(mMockDevice.getSetting("system", "key")).andReturn("orig").once();
        mMockDevice.setSetting("system", "key", "value");
        EasyMock.expectLastCall().once();
        EasyMock.expect(mMockDevice.getSetting("global", "key2")).andReturn("orig2").once();
        mMockDevice.setSetting("global", "key2", "value2");
        EasyMock.expectLastCall().once();
        EasyMock.expect(mMockDevice.getSetting("secure", "key3")).andReturn("orig3").once();
        mMockDevice.setSetting("secure", "key3", "value3");
        EasyMock.expectLastCall().once();
        mMockDevice.setSetting("system", "key", "orig");
        EasyMock.expectLastCall().once();
        mMockDevice.setSetting("global", "key2", "orig2");
        EasyMock.expectLastCall().once();
        mMockDevice.setSetting("secure", "key3", "orig3");
        EasyMock.expectLastCall().once();

        EasyMock.replay(mMockDevice);

        mDeviceSetup.setRestoreSettings(true);
        mDeviceSetup.setSystemSetting("key", "value");
        mDeviceSetup.setGlobalSetting("key2", "value2");
        mDeviceSetup.setSecureSetting("key3", "value3");
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
        mDeviceSetup.tearDown(mMockDevice, mMockBuildInfo, null);

        EasyMock.verify(mMockDevice);
    }

    public void testTearDown() throws Exception {
        EasyMock.replay(mMockDevice);
        mDeviceSetup.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice);
    }

    public void testTearDown_disconnectFromWifi() throws Exception {
        EasyMock.expect(mMockDevice.isWifiEnabled()).andReturn(Boolean.TRUE);
        EasyMock.expect(mMockDevice.disconnectFromWifi()).andReturn(Boolean.TRUE);
        mDeviceSetup.setWifiNetwork("wifi_network");
        EasyMock.replay(mMockDevice);
        mDeviceSetup.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice);
    }

    /** Test that tearDown is inop when using a stub device instance. */
    public void testTearDown_tcpDevice() throws Exception {
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new TcpDevice("tcp-device-0"));
        EasyMock.replay(mMockDevice);
        mDeviceSetup.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockDevice);
    }

    public void testSetup_rootDisabled() throws Exception {
        doSetupExpectations(true /* screenOn */, false /* root enabled */,
                false /* root response */, DEFAULT_API_LEVEL, new Capture<>());
        doCheckExternalStoreSpaceExpectations();
        EasyMock.replay(mMockDevice);
        mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockDevice);
    }

    public void testSetup_rootFailed() throws Exception {
        doSetupExpectations(true /* screenOn */, true /* root enabled */,
                false /* root response */, DEFAULT_API_LEVEL, new Capture<>());
        EasyMock.replay(mMockDevice);
        try {
            mDeviceSetup.setUp(mMockDevice, mMockBuildInfo);
            fail("TargetSetupError expected");
        } catch (TargetSetupError e) {
            // Expected
        }
    }

    /** Set EasyMock expectations for a normal setup call */
    private void doSetupExpectations() throws DeviceNotAvailableException, ConfigurationException {
        doSetupExpectations(true /* screen on */, true /* root enabled */, true /* root response */,
                DEFAULT_API_LEVEL, new Capture<String>());
    }

    /** Set EasyMock expectations for a normal setup call */
    private void doSetupExpectations(int apiLevel)
            throws DeviceNotAvailableException, ConfigurationException {
        doSetupExpectations(true /* screen on */, true /* root enabled */, true /* root response */,
                apiLevel, new Capture<String>());
    }

    /** Set EasyMock expectations for a normal setup call */
    private void doSetupExpectations(boolean screenOn, Capture<String> setPropCapture)
            throws DeviceNotAvailableException, ConfigurationException {
        doSetupExpectations(screenOn, true /* root enabled */, true /* root response */,
                DEFAULT_API_LEVEL, setPropCapture);
    }

    /** Set EasyMock expectations for a normal setup call */
    private void doSetupExpectations(
            boolean screenOn,
            boolean adbRootEnabled,
            boolean adbRootResponse,
            int apiLevel,
            Capture<String> setPropCapture)
            throws DeviceNotAvailableException, ConfigurationException {
        TestDeviceOptions options = new TestDeviceOptions();
        OptionSetter setter = new OptionSetter(options);
        setter.setOptionValue("enable-root", Boolean.toString(adbRootEnabled));
        EasyMock.expect(mMockDevice.getOptions()).andReturn(options).once();
        if (adbRootEnabled) {
            EasyMock.expect(mMockDevice.enableAdbRoot()).andReturn(adbRootResponse);
        }
        EasyMock.expect(mMockDevice.clearErrorDialogs()).andReturn(Boolean.TRUE);
        EasyMock.expect(mMockDevice.getApiLevel()).andReturn(apiLevel);
        // expect push of local.prop file to change system properties
        EasyMock.expect(mMockDevice.pushString(EasyMock.capture(setPropCapture),
                EasyMock.contains("local.prop"))).andReturn(Boolean.TRUE);
        EasyMock.expect(mMockDevice.executeShellCommand(
                EasyMock.matches("chmod 644 .*local.prop"))).andReturn("");
        mMockDevice.reboot();
        if (screenOn) {
            EasyMock.expect(mMockDevice.executeShellCommand("svc power stayon true")).andReturn("");
            EasyMock.expect(mMockDevice.executeShellCommand("input keyevent 82")).andReturn("");
            EasyMock.expect(mMockDevice.executeShellCommand("input keyevent 3")).andReturn("");
        }
    }

    /**
     * Perform common EasyMock expect operations for a setUp call which syncs local data
     */
    private void doSyncDataExpectations(boolean result) throws DeviceNotAvailableException {
        mDeviceSetup.setLocalDataPath(mTmpDir);
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(mMockIDevice);
        String mntPoint = "/sdcard";
        EasyMock.expect(mMockIDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE)).andReturn(
                mntPoint);
        EasyMock.expect(mMockDevice.syncFiles(mTmpDir, mntPoint)).andReturn(result);
    }

    private void doCheckExternalStoreSpaceExpectations() throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getExternalStoreFreeSpace()).andReturn(1000l);
    }

    private void doCommandsExpectations(String... commands)
            throws DeviceNotAvailableException {
        for (String command : commands) {
            EasyMock.expect(mMockDevice.executeShellCommand(command)).andReturn("");
        }
    }

    private void doSettingExpectations(String namespace, String key, String value)
            throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getApiLevel()).andReturn(22);
        mMockDevice.setSetting(namespace, key, value);
        EasyMock.expectLastCall();
    }
}
