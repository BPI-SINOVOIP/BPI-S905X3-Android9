/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.task;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;

import static com.android.managedprovisioning.task.AddWifiNetworkTask.ADD_NETWORK_FAIL;

import static junit.framework.Assert.assertTrue;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.ComponentName;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Looper;
import android.support.test.filters.SmallTest;

import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.model.WifiInfo;
import com.android.managedprovisioning.task.wifi.NetworkMonitor;
import com.android.managedprovisioning.task.wifi.WifiConfigurationProvider;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * Unit-tests for {@link AddWifiNetworkTask}.
 */
@SmallTest
public class AddWifiNetworkTaskTest {
    private static final int TEST_USER_ID = 123;
    private static final ComponentName ADMIN = new ComponentName("com.test.admin", ".Receiver");
    private static final String TEST_SSID = "TEST_SSID";
    private static final String TEST_SSID_2 = "TEST_SSID_2";
    private static final ProvisioningParams NO_WIFI_INFO_PARAMS = new ProvisioningParams.Builder()
            .setProvisioningAction(ACTION_PROVISION_MANAGED_PROFILE)
            .setDeviceAdminComponentName(ADMIN)
            .setWifiInfo(null)
            .build();
    private static final ProvisioningParams WIFI_INFO_PARAMS = new ProvisioningParams.Builder()
            .setProvisioningAction(ACTION_PROVISION_MANAGED_PROFILE)
            .setDeviceAdminComponentName(ADMIN)
            .setWifiInfo(new WifiInfo.Builder().setSsid(TEST_SSID).build())
            .build();
    private static final int ADD_NETWORK_OK = 0;

    @Mock private Context mContext;
    @Mock private ConnectivityManager mConnectivityManager;
    @Mock private WifiManager mWifiManager;
    @Mock private NetworkMonitor mNetworkMonitor;
    @Mock private WifiConfigurationProvider mWifiConfigurationProvider;
    @Mock private AbstractProvisioningTask.Callback mCallback;
    @Mock private Utils mUtils;
    @Mock private android.net.wifi.WifiInfo mWifiInfo;
    @Mock private AddWifiNetworkTask.Injector mTestInjector;

    private AddWifiNetworkTask mTask;

    @Before
    public void setUp() {
        System.setProperty("dexmaker.share_classloader", "true");
        MockitoAnnotations.initMocks(this);

        when(mContext.getSystemService(Context.WIFI_SERVICE)).thenReturn(mWifiManager);
        when(mContext.getSystemService(Context.CONNECTIVITY_SERVICE))
                .thenReturn(mConnectivityManager);
    }

    @Test
    public void testNoWifiInfo() {
        // GIVEN that no wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                NO_WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // WHEN running the task
        runTask();

        // THEN success should be called
        verify(mCallback).onSuccess(mTask);
    }

    @Test
    public void testWifiManagerNull() {
        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that mWifiManager is null
        when(mContext.getSystemService(Context.WIFI_SERVICE)).thenReturn(null);

        // WHEN running the task
        runTask();

        // THEN error should be called
        verify(mCallback).onError(mTask, 0);
    }

    @Test
    public void testFailToEnableWifi() {
        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that wifi is not enabled
        when(mWifiManager.isWifiEnabled()).thenReturn(false);

        // GIVEN that enabling wifi failed
        when(mWifiManager.setWifiEnabled(true)).thenReturn(false);

        // WHEN running the task
        runTask();

        // THEN error should be called
        verify(mCallback).onError(mTask, 0);
    }

    @Test
    public void testIsConnectedToSpecifiedWifiTrue() {
        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that wifi is enabled
        when(mWifiManager.isWifiEnabled()).thenReturn(true);

        // GIVEN connected to wifi
        when(mUtils.isConnectedToWifi(mContext)).thenReturn(true);

        // GIVEN the connected SSID is the same as the wifi param
        when(mWifiManager.getConnectionInfo()).thenReturn(mWifiInfo);
        when(mWifiInfo.getSSID()).thenReturn(TEST_SSID);

        // WHEN running the task
        runTask();

        // THEN success should be called
        verify(mCallback).onSuccess(mTask);
    }

    @Test
    public void testNoWifiInfoInProvider() {
        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that wifi is enabled
        when(mWifiManager.isWifiEnabled()).thenReturn(true);

        // GIVEN connected to wifi
        when(mUtils.isConnectedToWifi(mContext)).thenReturn(true);

        // GIVEN the connected SSID is different from the one in wifi param
        when(mWifiManager.getConnectionInfo()).thenReturn(mWifiInfo);
        when(mWifiInfo.getSSID()).thenReturn(TEST_SSID_2);

        // WHEN running the task
        runTask();

        // GIVEN generateWifiConfiguration is null
        when(mWifiConfigurationProvider.generateWifiConfiguration(any()))
                .thenReturn(null);

        // THEN error should be called
        verify(mCallback).onError(mTask, 0);
    }

    @Test
    public void testFailingAddingNetwork() {

        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that wifi is enabled
        when(mWifiManager.isWifiEnabled()).thenReturn(true);

        // GIVEN connected to wifi
        when(mUtils.isConnectedToWifi(mContext)).thenReturn(true);

        // GIVEN the connected SSID is different from the one in wifi param
        when(mWifiManager.getConnectionInfo()).thenReturn(mWifiInfo);
        when(mWifiInfo.getSSID()).thenReturn(TEST_SSID_2);

        // GIVEN WifiConfiguration is not empty
        when(mWifiConfigurationProvider.generateWifiConfiguration(any()))
                .thenReturn(new WifiConfiguration());

        // GIVEN addNetwork always fail
        when(mWifiManager.addNetwork(any())).thenReturn(ADD_NETWORK_FAIL);

        // WHEN running the task
        runTask();

        // THEN error should be called
        verify(mCallback).onError(mTask, 0);
    }

    @Test
    public void testFailingToReconnectAfterAddingNetwork() {
        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that wifi is enabled
        when(mWifiManager.isWifiEnabled()).thenReturn(true);

        // GIVEN connected to wifi
        when(mUtils.isConnectedToWifi(mContext)).thenReturn(true);

        // GIVEN the connected SSID is different from the one in wifi param
        when(mWifiManager.getConnectionInfo()).thenReturn(mWifiInfo);
        when(mWifiInfo.getSSID()).thenReturn(TEST_SSID_2);

        // GIVEN WifiConfiguration is not empty
        when(mWifiConfigurationProvider.generateWifiConfiguration(any()))
                .thenReturn(new WifiConfiguration());

        // GIVEN addNetwork OK
        when(mWifiManager.addNetwork(any())).thenReturn(ADD_NETWORK_OK);

        // GIVEN reconnect fail after adding network
        when(mWifiManager.reconnect()).thenReturn(false);

        // WHEN running the task
        runTask();

        // THEN wifi should be enabled
        assertTrue(mWifiManager.isWifiEnabled());

        // THEN error should be called
        verify(mCallback).onError(mTask, 0);
    }

    @Test
    public void testReconnectAfterAddingNetworkSuccess() {
        // GIVEN that wifi info was passed in the parameter
        mTask = new AddWifiNetworkTask(mNetworkMonitor, mWifiConfigurationProvider, mContext,
                WIFI_INFO_PARAMS, mCallback, mUtils, mTestInjector);

        // GIVEN that wifi is enabled
        when(mWifiManager.isWifiEnabled()).thenReturn(true);

        // GIVEN connected to wifi
        when(mUtils.isConnectedToWifi(mContext)).thenReturn(true);

        // GIVEN the connected SSID is different from the one in wifi param
        when(mWifiManager.getConnectionInfo()).thenReturn(mWifiInfo);
        when(mWifiInfo.getSSID()).thenReturn(TEST_SSID_2);

        // GIVEN WifiConfiguration is not empty
        when(mWifiConfigurationProvider.generateWifiConfiguration(any()))
                .thenReturn(new WifiConfiguration());

        // GIVEN addNetwork OK
        when(mWifiManager.addNetwork(any())).thenReturn(ADD_NETWORK_OK);

        // GIVEN reconnect ok after adding network
        when(mWifiManager.reconnect()).thenReturn(true);

        // WHEN running the task
        runTask();

        // GIVEN it connect back to the specified network
        when(mWifiInfo.getSSID()).thenReturn(TEST_SSID);

        // WHEN network is re-connected
        mTask.onNetworkConnected();

        // THEN success should be called
        verify(mCallback).onSuccess(mTask);
    }

    private void runTask() {
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        mTask.run(TEST_USER_ID);
    }
}
