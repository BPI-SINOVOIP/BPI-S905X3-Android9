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


import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.verify;
import static org.robolectric.shadow.api.Shadow.extract;

import android.arch.lifecycle.ViewModelProviders;
import android.net.IpConfiguration;
import android.net.wifi.WifiConfiguration;

import com.android.tv.settings.TvSettingsRobolectricTestRunner;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.testutils.ShadowStateMachine;
import com.android.tv.settings.testutils.TvShadowWifiManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

@RunWith(TvSettingsRobolectricTestRunner.class)
@Config(shadows = {
                ShadowStateMachine.class,
                TvShadowWifiManager.class
        })
public class ConnectStateTest {
    private WifiSetupActivity mActivity;
    private ConnectState mState;
    private UserChoiceInfo mUserChoiceInfo;
    @Mock
    private WifiConfiguration mWifiConfiguration;
    @Mock
    private State.StateCompleteListener mStateCompleteListener;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Mockito.spy(Robolectric.buildActivity(WifiSetupActivity.class).create().get());
        doNothing().when(mActivity).onFragmentChange(any(), anyBoolean());
        mUserChoiceInfo = ViewModelProviders.of(mActivity).get(UserChoiceInfo.class);
        StateMachine stateMachine = ViewModelProviders.of(mActivity).get(StateMachine.class);
        ShadowStateMachine sm = (ShadowStateMachine) extract(stateMachine);
        sm.setListener(mStateCompleteListener);
        mUserChoiceInfo.init();
        mState = new ConnectState(mActivity);
    }

    @Test
    public void testProcessForward_networkNotSaved() {
        AdvancedOptionsFlowInfo advFlowInfo = ViewModelProviders.of(mActivity).get(
                AdvancedOptionsFlowInfo.class);
        advFlowInfo.setIpConfiguration(new IpConfiguration());
        mWifiConfiguration.networkId = -1;
        mUserChoiceInfo.setWifiConfiguration(mWifiConfiguration);
        mState.processForward();
        verify(mWifiConfiguration).setIpConfiguration(any());
    }
}
