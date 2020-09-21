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

import static org.mockito.Mockito.verify;
import static org.robolectric.shadow.api.Shadow.extract;

import android.arch.lifecycle.ViewModelProviders;
import android.net.wifi.WifiConfiguration;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.TvSettingsRobolectricTestRunner;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.testutils.ShadowStateMachine;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

@RunWith(TvSettingsRobolectricTestRunner.class)
@Config(shadows = ShadowStateMachine.class)
public class AddStartStateTest {
    private WifiSetupActivity mActivity;
    private AddStartState mAddStartState;
    private UserChoiceInfo mUserChoiceInfo;
    @Mock
    private State.StateCompleteListener mStateCompleteListener;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mActivity = Robolectric.buildActivity(WifiSetupActivity.class).get();
        StateMachine stateMachine = ViewModelProviders.of(mActivity).get(StateMachine.class);
        mUserChoiceInfo = ViewModelProviders.of(mActivity).get(UserChoiceInfo.class);
        ShadowStateMachine shadowStateMachine = (ShadowStateMachine) extract(stateMachine);
        shadowStateMachine.setListener(mStateCompleteListener);
        mAddStartState = new AddStartState(mActivity);
    }

    @Test
    public void testForward_WEP_NeedPassword() {
        mUserChoiceInfo.init();
        mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_WEP);
        mUserChoiceInfo.setWifiConfiguration(new WifiConfiguration());
        mAddStartState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.PASSWORD);
    }

    @Test
    public void testForward_WPA_NeedPassword() {
        mUserChoiceInfo.init();
        mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_PSK);
        mUserChoiceInfo.setWifiConfiguration(new WifiConfiguration());
        mAddStartState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.PASSWORD);
    }

    @Test
    public void testForward_EAP_NeedPassword() {
        mUserChoiceInfo.init();
        mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_EAP);
        mUserChoiceInfo.setWifiConfiguration(new WifiConfiguration());
        mAddStartState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.PASSWORD);
    }

    @Test
    public void testForward_AlreadyHasPassword() {
        mUserChoiceInfo.init();
        mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_PSK);
        WifiConfiguration config = new WifiConfiguration();
        config.preSharedKey = "PasswordTest";
        mUserChoiceInfo.setWifiConfiguration(config);
        mAddStartState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.CONNECT);
    }

    @Test
    public void testForward_DoNotNeedPassword() {
        mUserChoiceInfo.init();
        mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_NONE);
        mAddStartState.processForward();
        verify(mStateCompleteListener).onComplete(StateMachine.CONNECT);
    }
}
