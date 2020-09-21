/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.setup;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.verify;
import static org.robolectric.shadow.api.Shadow.extract;

import android.arch.lifecycle.ViewModelProviders;
import android.content.Intent;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.os.Bundle;

import com.android.tv.settings.R;
import com.android.tv.settings.TvSettingsRobolectricTestRunner;
import com.android.tv.settings.connectivity.WifiConfigHelper;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.testutils.ShadowStateMachine;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(TvSettingsRobolectricTestRunner.class)
@Config(shadows = ShadowStateMachine.class)
public class AddPageBasedOnNetworkStateTest {
    private WifiSetupActivity mActivity;
    private AddPageBasedOnNetworkState mAddPageBasedOnNetworkState;
    private UserChoiceInfo mUserChoiceInfo;
    @Mock
    private State.StateCompleteListener mStateCompleteListener;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Mockito.spy(Robolectric.buildActivity(
                        WifiSetupActivity.class, new Intent().putExtras(new Bundle())).get());
        StateMachine stateMachine = ViewModelProviders.of(mActivity).get(StateMachine.class);
        mUserChoiceInfo = ViewModelProviders.of(mActivity).get(UserChoiceInfo.class);
        ShadowStateMachine shadowStateMachine = extract(stateMachine);
        shadowStateMachine.setListener(mStateCompleteListener);
        mAddPageBasedOnNetworkState = new AddPageBasedOnNetworkState(mActivity);
    }

    @Test
    public void testForward_otherNetwork() {
        mUserChoiceInfo.init();
        mUserChoiceInfo.put(UserChoiceInfo.SELECT_WIFI, getString(R.string.other_network));
        mAddPageBasedOnNetworkState.processForward();
        assertTrue(mUserChoiceInfo.getWifiConfiguration().hiddenSSID);
        verify(mStateCompleteListener).onComplete(StateMachine.OTHER_NETWORK);
    }

    @Test
    public void testForward_clearPassword_ssidNotMatch() {
        mUserChoiceInfo.init();
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = "atv01";
        scanResult.capabilities = "";
        mUserChoiceInfo.setChosenNetwork(scanResult);
        mUserChoiceInfo.put(UserChoiceInfo.PASSWORD, "123");
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.SSID = "atv02";
        mUserChoiceInfo.setWifiConfiguration(wifiConfiguration);
        WifiConfigHelper.saveConfiguration(mActivity, wifiConfiguration);
        assertNotNull(mUserChoiceInfo.getChosenNetwork());
        mAddPageBasedOnNetworkState.processForward();
        assertNull(mUserChoiceInfo.getPageSummary(UserChoiceInfo.PASSWORD));
    }

    @Test
    public void testForward_clearPassword_ssidIsNull() {
        mUserChoiceInfo.init();
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = "atv";
        scanResult.capabilities = "";
        mUserChoiceInfo.setChosenNetwork(scanResult);
        mUserChoiceInfo.put(UserChoiceInfo.PASSWORD, "123");
        mUserChoiceInfo.setWifiConfiguration(null);
        mAddPageBasedOnNetworkState.processForward();
        assertNull(mUserChoiceInfo.getPageSummary(UserChoiceInfo.PASSWORD));
    }

    @Test
    public void testForward_networkSaved() {
        mUserChoiceInfo.init();
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = "atv";
        scanResult.capabilities = "";
        mUserChoiceInfo.setChosenNetwork(scanResult);
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = "\"atv\"";
        WifiConfigHelper.saveConfiguration(mActivity, config);
        mAddPageBasedOnNetworkState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.KNOWN_NETWORK);
    }

    @Test
    public void testForward_networkNotSaved() {
        mUserChoiceInfo.init();
        ScanResult scanResult = new ScanResult();
        scanResult.SSID = "atv";
        scanResult.capabilities = "";
        mUserChoiceInfo.setChosenNetwork(scanResult);
        mUserChoiceInfo.put(UserChoiceInfo.PASSWORD, "123");
        WifiConfiguration config = new WifiConfiguration();
        config.networkId = -1;
        WifiConfigHelper.saveConfiguration(mActivity, config);
        mAddPageBasedOnNetworkState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.ADD_START);
    }

    private String getString(int id) {
        return RuntimeEnvironment.application.getString(id);
    }
}
