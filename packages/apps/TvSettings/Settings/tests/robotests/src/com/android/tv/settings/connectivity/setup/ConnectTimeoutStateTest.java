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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.arch.lifecycle.ViewModelProviders;
import android.net.wifi.WifiConfiguration;

import com.android.tv.settings.TvSettingsRobolectricTestRunner;
import com.android.tv.settings.testutils.TvShadowWifiManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

@RunWith(TvSettingsRobolectricTestRunner.class)
@Config(shadows = TvShadowWifiManager.class)
public class ConnectTimeoutStateTest {
    private WifiSetupActivity mActivity;
    private ConnectTimeOutState mState;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Robolectric.buildActivity(WifiSetupActivity.class).create().get();
        mState = new ConnectTimeOutState(mActivity);
    }

    @Test
    public void testProcessForward() {
        String ssid = "aewrf";
        AdvancedOptionsFlowInfo advInfo =
                ViewModelProviders.of(mActivity).get(AdvancedOptionsFlowInfo.class);
        UserChoiceInfo userInfo = ViewModelProviders.of(mActivity).get(UserChoiceInfo.class);
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.SSID = ssid;
        userInfo.setWifiConfiguration(wifiConfiguration);
        advInfo.setCanStart(false);
        advInfo.setPrintableSsid(null);
        mState.processForward();
        assertTrue(advInfo.canStart());
        assertEquals(ssid, advInfo.getPrintableSsid());
    }
}
