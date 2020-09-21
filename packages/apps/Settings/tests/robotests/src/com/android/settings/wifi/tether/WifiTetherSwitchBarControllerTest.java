/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.settings.wifi.tether;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkPolicyManager;
import android.net.wifi.WifiManager;
import android.provider.Settings;

import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.widget.SwitchBar;
import com.android.settings.widget.SwitchBarController;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class WifiTetherSwitchBarControllerTest {
    @Mock
    private WifiManager mWifiManager;
    @Mock
    private ConnectivityManager mConnectivityManager;
    @Mock
    private NetworkPolicyManager mNetworkPolicyManager;

    private Context mContext;
    private SwitchBar mSwitchBar;
    private WifiTetherSwitchBarController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = spy(RuntimeEnvironment.application);
        mSwitchBar = new SwitchBar(mContext);
        when(mContext.getSystemService(Context.WIFI_SERVICE)).thenReturn(mWifiManager);
        when(mContext.getSystemService(Context.CONNECTIVITY_SERVICE)).thenReturn(
                mConnectivityManager);
        when(mContext.getSystemService(Context.NETWORK_POLICY_SERVICE)).thenReturn(
                mNetworkPolicyManager);

        mController = new WifiTetherSwitchBarController(mContext,
                new SwitchBarController(mSwitchBar));
    }

    @Test
    public void constructor_airplaneModeOn_switchBarDisabled() {
        Settings.Global.putInt(RuntimeEnvironment.application.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 1);

        final WifiTetherSwitchBarController controller = new WifiTetherSwitchBarController(
                mContext, new SwitchBarController(mSwitchBar));

        assertThat(mSwitchBar.isEnabled()).isFalse();
    }

    @Test
    public void startTether_fail_resetSwitchBar() {
        when(mNetworkPolicyManager.getRestrictBackground()).thenReturn(false);

        mController.startTether();
        mController.mOnStartTetheringCallback.onTetheringFailed();

        assertThat(mSwitchBar.isChecked()).isFalse();
        assertThat(mSwitchBar.isEnabled()).isTrue();
    }

    @Test
    public void onDataSaverChanged_setsEnabledCorrectly() {
        assertThat(mSwitchBar.isEnabled()).isTrue();

        // try to turn data saver on
        when(mNetworkPolicyManager.getRestrictBackground()).thenReturn(true);
        mController.onDataSaverChanged(true);
        assertThat(mSwitchBar.isEnabled()).isFalse();

        // lets turn data saver off again
        when(mNetworkPolicyManager.getRestrictBackground()).thenReturn(false);
        mController.onDataSaverChanged(false);
        assertThat(mSwitchBar.isEnabled()).isTrue();
    }

    @Test
    public void onSwitchToggled_onlyStartsTetherWhenNeeded() {
        when(mWifiManager.isWifiApEnabled()).thenReturn(true);
        mController.onSwitchToggled(true);

        verify(mConnectivityManager, never()).startTethering(anyInt(), anyBoolean(), any(), any());

        doReturn(false).when(mWifiManager).isWifiApEnabled();
        mController.onSwitchToggled(true);

        verify(mConnectivityManager, times(1))
                .startTethering(anyInt(), anyBoolean(), any(), any());
    }
}
