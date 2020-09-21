/*
 * Copyright 2017 The Android Open Source Project
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

package com.android.bluetooth.btservice;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.AlarmManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.IBluetoothCallback;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.media.AudioManager;
import android.os.Binder;
import android.os.Looper;
import android.os.PowerManager;
import android.os.Process;
import android.os.SystemProperties;
import android.os.UserManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.test.mock.MockContentResolver;

import com.android.bluetooth.R;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class AdapterServiceTest {
    private AdapterService mAdapterService;

    private @Mock Context mMockContext;
    private @Mock ApplicationInfo mMockApplicationInfo;
    private @Mock AlarmManager mMockAlarmManager;
    private @Mock Resources mMockResources;
    private @Mock UserManager mMockUserManager;
    private @Mock ProfileService mMockGattService;
    private @Mock ProfileService mMockService;
    private @Mock ProfileService mMockService2;
    private @Mock IBluetoothCallback mIBluetoothCallback;
    private @Mock Binder mBinder;
    private @Mock AudioManager mAudioManager;

    private static final int CONTEXT_SWITCH_MS = 100;
    private static final int ONE_SECOND_MS = 1000;
    private static final int NATIVE_INIT_MS = 8000;

    private PowerManager mPowerManager;
    private PackageManager mMockPackageManager;
    private MockContentResolver mMockContentResolver;

    @Before
    public void setUp() throws PackageManager.NameNotFoundException {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        Assert.assertNotNull(Looper.myLooper());

        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mAdapterService = new AdapterService();
            }
        });
        mMockPackageManager = mock(PackageManager.class);
        mMockContentResolver = new MockContentResolver(mMockContext);
        MockitoAnnotations.initMocks(this);
        mPowerManager = (PowerManager) InstrumentationRegistry.getTargetContext()
                .getSystemService(Context.POWER_SERVICE);

        when(mMockContext.getApplicationInfo()).thenReturn(mMockApplicationInfo);
        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);
        when(mMockContext.getApplicationContext()).thenReturn(mMockContext);
        when(mMockContext.getResources()).thenReturn(mMockResources);
        when(mMockContext.getUserId()).thenReturn(Process.BLUETOOTH_UID);
        when(mMockContext.getPackageManager()).thenReturn(mMockPackageManager);
        when(mMockContext.getSystemService(Context.USER_SERVICE)).thenReturn(mMockUserManager);
        when(mMockContext.getSystemService(Context.POWER_SERVICE)).thenReturn(mPowerManager);
        when(mMockContext.getSystemService(Context.ALARM_SERVICE)).thenReturn(mMockAlarmManager);
        when(mMockContext.getSystemService(Context.AUDIO_SERVICE)).thenReturn(mAudioManager);

        when(mMockResources.getBoolean(R.bool.profile_supported_gatt)).thenReturn(true);
        when(mMockResources.getBoolean(R.bool.profile_supported_pbap)).thenReturn(true);
        when(mMockResources.getBoolean(R.bool.profile_supported_pan)).thenReturn(true);

        when(mIBluetoothCallback.asBinder()).thenReturn(mBinder);

        doReturn(Process.BLUETOOTH_UID).when(mMockPackageManager)
                .getPackageUidAsUser(any(), anyInt(), anyInt());

        when(mMockGattService.getName()).thenReturn("GattService");
        when(mMockService.getName()).thenReturn("Service1");
        when(mMockService2.getName()).thenReturn("Service2");

        // Attach a context to the service for permission checks.
        mAdapterService.attach(mMockContext, null, null, null, null, null);

        mAdapterService.onCreate();
        mAdapterService.registerCallback(mIBluetoothCallback);

        Config.init(mMockContext);
    }

    @After
    public void tearDown() {
        mAdapterService.unregisterCallback(mIBluetoothCallback);
        mAdapterService.cleanup();
        Config.init(InstrumentationRegistry.getTargetContext());
    }

    private void verifyStateChange(int prevState, int currState, int callNumber, int timeoutMs) {
        try {
            verify(mIBluetoothCallback, timeout(timeoutMs)
                    .times(callNumber)).onBluetoothStateChange(prevState, currState);
        } catch (Exception e) {
            // the mocked onBluetoothStateChange doesn't throw exceptions
        }
    }

    private void doEnable(int invocationNumber, boolean onlyGatt) {
        Assert.assertFalse(mAdapterService.isEnabled());

        final int startServiceCalls = 2 * (onlyGatt ? 1 : 3); // Start and stop GATT + 2

        mAdapterService.enable();

        verifyStateChange(BluetoothAdapter.STATE_OFF, BluetoothAdapter.STATE_BLE_TURNING_ON,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        // Start GATT
        verify(mMockContext, timeout(CONTEXT_SWITCH_MS).times(
                startServiceCalls * invocationNumber + 1)).startService(any());
        mAdapterService.addProfile(mMockGattService);
        mAdapterService.onProfileServiceStateChanged(mMockGattService, BluetoothAdapter.STATE_ON);

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_ON, BluetoothAdapter.STATE_BLE_ON,
                invocationNumber + 1, NATIVE_INIT_MS);

        mAdapterService.onLeServiceUp();

        verifyStateChange(BluetoothAdapter.STATE_BLE_ON, BluetoothAdapter.STATE_TURNING_ON,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        if (!onlyGatt) {
            // Start Mock PBAP and PAN services
            verify(mMockContext, timeout(ONE_SECOND_MS).times(
                    startServiceCalls * invocationNumber + 3)).startService(any());
            mAdapterService.addProfile(mMockService);
            mAdapterService.addProfile(mMockService2);
            mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_ON);
            mAdapterService.onProfileServiceStateChanged(mMockService2, BluetoothAdapter.STATE_ON);
        }

        verifyStateChange(BluetoothAdapter.STATE_TURNING_ON, BluetoothAdapter.STATE_ON,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        final int scanMode = mAdapterService.getScanMode();
        Assert.assertTrue(scanMode == BluetoothAdapter.SCAN_MODE_CONNECTABLE
                || scanMode == BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        Assert.assertTrue(mAdapterService.isEnabled());
    }

    private void doDisable(int invocationNumber, boolean onlyGatt) {
        Assert.assertTrue(mAdapterService.isEnabled());

        final int startServiceCalls = 2 * (onlyGatt ? 1 : 3); // Start and stop GATT + 2

        mAdapterService.disable();

        verifyStateChange(BluetoothAdapter.STATE_ON, BluetoothAdapter.STATE_TURNING_OFF,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        if (!onlyGatt) {
            // Stop PBAP and PAN
            verify(mMockContext, timeout(ONE_SECOND_MS).times(
                    startServiceCalls * invocationNumber + 5)).startService(any());
            mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_OFF);
            mAdapterService.onProfileServiceStateChanged(mMockService2, BluetoothAdapter.STATE_OFF);
        }

        verifyStateChange(BluetoothAdapter.STATE_TURNING_OFF, BluetoothAdapter.STATE_BLE_ON,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        mAdapterService.onBrEdrDown();

        verifyStateChange(BluetoothAdapter.STATE_BLE_ON, BluetoothAdapter.STATE_BLE_TURNING_OFF,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        // Stop GATT
        verify(mMockContext, timeout(ONE_SECOND_MS).times(
                startServiceCalls * invocationNumber + startServiceCalls)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockGattService, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_OFF, BluetoothAdapter.STATE_OFF,
                invocationNumber + 1, CONTEXT_SWITCH_MS);

        Assert.assertFalse(mAdapterService.isEnabled());
    }

    /**
     * Test: Turn Bluetooth on.
     * Check whether the AdapterService gets started.
     */
    @Test
    public void testEnable() {
        doEnable(0, false);
    }

    /**
     * Test: Turn Bluetooth on/off.
     * Check whether the AdapterService gets started and stopped.
     */
    @Test
    public void testEnableDisable() {
        doEnable(0, false);
        doDisable(0, false);
    }

    /**
     * Test: Turn Bluetooth on/off with only GATT supported.
     * Check whether the AdapterService gets started and stopped.
     */
    @Test
    public void testEnableDisableOnlyGatt() {
        Context mockContext = mock(Context.class);
        Resources mockResources = mock(Resources.class);

        when(mockContext.getApplicationInfo()).thenReturn(mMockApplicationInfo);
        when(mockContext.getContentResolver()).thenReturn(mMockContentResolver);
        when(mockContext.getApplicationContext()).thenReturn(mockContext);
        when(mockContext.getResources()).thenReturn(mockResources);
        when(mockContext.getUserId()).thenReturn(Process.BLUETOOTH_UID);
        when(mockContext.getPackageManager()).thenReturn(mMockPackageManager);
        when(mockContext.getSystemService(Context.USER_SERVICE)).thenReturn(mMockUserManager);
        when(mockContext.getSystemService(Context.POWER_SERVICE)).thenReturn(mPowerManager);
        when(mockContext.getSystemService(Context.ALARM_SERVICE)).thenReturn(mMockAlarmManager);

        when(mockResources.getBoolean(R.bool.profile_supported_gatt)).thenReturn(true);

        Config.init(mockContext);
        doEnable(0, true);
        doDisable(0, true);
    }

    /**
     * Test: Don't start GATT
     * Check whether the AdapterService quits gracefully
     */
    @Test
    public void testGattStartTimeout() {
        Assert.assertFalse(mAdapterService.isEnabled());

        mAdapterService.enable();

        verifyStateChange(BluetoothAdapter.STATE_OFF, BluetoothAdapter.STATE_BLE_TURNING_ON, 1,
                CONTEXT_SWITCH_MS);

        // Start GATT
        verify(mMockContext, timeout(CONTEXT_SWITCH_MS).times(1)).startService(any());
        mAdapterService.addProfile(mMockGattService);

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_ON,
                BluetoothAdapter.STATE_BLE_TURNING_OFF, 1,
                AdapterState.BLE_START_TIMEOUT_DELAY + CONTEXT_SWITCH_MS);

        // Stop GATT
        verify(mMockContext, timeout(AdapterState.BLE_STOP_TIMEOUT_DELAY + CONTEXT_SWITCH_MS)
                .times(2)).startService(any());

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_OFF, BluetoothAdapter.STATE_OFF, 1,
                CONTEXT_SWITCH_MS);

        Assert.assertFalse(mAdapterService.isEnabled());
    }

    /**
     * Test: Don't stop GATT
     * Check whether the AdapterService quits gracefully
     */
    @Test
    public void testGattStopTimeout() {
        doEnable(0, false);
        Assert.assertTrue(mAdapterService.isEnabled());

        mAdapterService.disable();

        verifyStateChange(BluetoothAdapter.STATE_ON, BluetoothAdapter.STATE_TURNING_OFF, 1,
                CONTEXT_SWITCH_MS);

        // Stop PBAP and PAN
        verify(mMockContext, timeout(ONE_SECOND_MS).times(5)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_OFF);
        mAdapterService.onProfileServiceStateChanged(mMockService2, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_TURNING_OFF, BluetoothAdapter.STATE_BLE_ON, 1,
                CONTEXT_SWITCH_MS);

        mAdapterService.onBrEdrDown();

        verifyStateChange(BluetoothAdapter.STATE_BLE_ON, BluetoothAdapter.STATE_BLE_TURNING_OFF, 1,
                CONTEXT_SWITCH_MS);

        // Stop GATT
        verify(mMockContext, timeout(ONE_SECOND_MS).times(6)).startService(any());

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_OFF, BluetoothAdapter.STATE_OFF, 1,
                AdapterState.BLE_STOP_TIMEOUT_DELAY + CONTEXT_SWITCH_MS);

        Assert.assertFalse(mAdapterService.isEnabled());
    }

    /**
     * Test: Don't start a classic profile
     * Check whether the AdapterService quits gracefully
     */
    @Test
    public void testProfileStartTimeout() {
        Assert.assertFalse(mAdapterService.isEnabled());

        mAdapterService.enable();

        verifyStateChange(BluetoothAdapter.STATE_OFF, BluetoothAdapter.STATE_BLE_TURNING_ON, 1,
                CONTEXT_SWITCH_MS);

        // Start GATT
        verify(mMockContext, timeout(CONTEXT_SWITCH_MS).times(1)).startService(any());
        mAdapterService.addProfile(mMockGattService);
        mAdapterService.onProfileServiceStateChanged(mMockGattService, BluetoothAdapter.STATE_ON);

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_ON, BluetoothAdapter.STATE_BLE_ON, 1,
                NATIVE_INIT_MS);

        mAdapterService.onLeServiceUp();

        verifyStateChange(BluetoothAdapter.STATE_BLE_ON, BluetoothAdapter.STATE_TURNING_ON, 1,
                CONTEXT_SWITCH_MS);

        // Register Mock PBAP and PAN services, only start one
        verify(mMockContext, timeout(ONE_SECOND_MS).times(3)).startService(any());
        mAdapterService.addProfile(mMockService);
        mAdapterService.addProfile(mMockService2);
        mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_ON);

        verifyStateChange(BluetoothAdapter.STATE_TURNING_ON, BluetoothAdapter.STATE_TURNING_OFF, 1,
                AdapterState.BREDR_START_TIMEOUT_DELAY + CONTEXT_SWITCH_MS);

        // Stop PBAP and PAN
        verify(mMockContext, timeout(ONE_SECOND_MS).times(5)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_TURNING_OFF, BluetoothAdapter.STATE_BLE_ON, 1,
                CONTEXT_SWITCH_MS);
    }

    /**
     * Test: Don't stop a classic profile
     * Check whether the AdapterService quits gracefully
     */
    @Test
    public void testProfileStopTimeout() {
        doEnable(0, false);

        Assert.assertTrue(mAdapterService.isEnabled());

        mAdapterService.disable();

        verifyStateChange(BluetoothAdapter.STATE_ON, BluetoothAdapter.STATE_TURNING_OFF, 1,
                CONTEXT_SWITCH_MS);

        // Stop PBAP and PAN
        verify(mMockContext, timeout(ONE_SECOND_MS).times(5)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_TURNING_OFF,
                BluetoothAdapter.STATE_BLE_TURNING_OFF, 1,
                AdapterState.BREDR_STOP_TIMEOUT_DELAY + CONTEXT_SWITCH_MS);

        // Stop GATT
        verify(mMockContext, timeout(ONE_SECOND_MS).times(6)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockGattService, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_OFF, BluetoothAdapter.STATE_OFF, 1,
                AdapterState.BLE_STOP_TIMEOUT_DELAY + CONTEXT_SWITCH_MS);

        Assert.assertFalse(mAdapterService.isEnabled());
    }

    /**
     * Test: Toggle snoop logging setting
     * Check whether the AdapterService restarts fully
     */
    @Test
    public void testSnoopLoggingChange() {
        String snoopSetting =
                SystemProperties.get(AdapterService.BLUETOOTH_BTSNOOP_ENABLE_PROPERTY, "");
        SystemProperties.set(AdapterService.BLUETOOTH_BTSNOOP_ENABLE_PROPERTY, "false");
        doEnable(0, false);

        Assert.assertTrue(mAdapterService.isEnabled());

        Assert.assertFalse(
                SystemProperties.getBoolean(AdapterService.BLUETOOTH_BTSNOOP_ENABLE_PROPERTY,
                        true));

        SystemProperties.set(AdapterService.BLUETOOTH_BTSNOOP_ENABLE_PROPERTY, "true");

        mAdapterService.disable();

        verifyStateChange(BluetoothAdapter.STATE_ON, BluetoothAdapter.STATE_TURNING_OFF, 1,
                CONTEXT_SWITCH_MS);

        // Stop PBAP and PAN
        verify(mMockContext, timeout(ONE_SECOND_MS).times(5)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockService, BluetoothAdapter.STATE_OFF);
        mAdapterService.onProfileServiceStateChanged(mMockService2, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_TURNING_OFF, BluetoothAdapter.STATE_BLE_ON, 1,
                CONTEXT_SWITCH_MS);

        // Don't call onBrEdrDown().  The Adapter should turn itself off.

        verifyStateChange(BluetoothAdapter.STATE_BLE_ON, BluetoothAdapter.STATE_BLE_TURNING_OFF, 1,
                CONTEXT_SWITCH_MS);

        // Stop GATT
        verify(mMockContext, timeout(ONE_SECOND_MS).times(6)).startService(any());
        mAdapterService.onProfileServiceStateChanged(mMockGattService, BluetoothAdapter.STATE_OFF);

        verifyStateChange(BluetoothAdapter.STATE_BLE_TURNING_OFF, BluetoothAdapter.STATE_OFF, 1,
                CONTEXT_SWITCH_MS);

        Assert.assertFalse(mAdapterService.isEnabled());

        // Restore earlier setting
        SystemProperties.set(AdapterService.BLUETOOTH_BTSNOOP_ENABLE_PROPERTY, snoopSetting);
    }
}
