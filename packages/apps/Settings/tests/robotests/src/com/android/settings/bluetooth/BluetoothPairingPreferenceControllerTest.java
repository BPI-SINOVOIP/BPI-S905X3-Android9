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

package com.android.settings.bluetooth;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.Intent;
import android.support.v7.preference.Preference;

import com.android.settings.R;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Answers;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class BluetoothPairingPreferenceControllerTest {

    private static final int ORDER = 1;

    @Mock(answer = Answers.RETURNS_DEEP_STUBS)
    private DashboardFragment mFragment;

    private Context mContext;
    private Preference mPreference;

    private BluetoothPairingPreferenceController mController;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mContext = spy(RuntimeEnvironment.application);
        when(mFragment.getPreferenceScreen().getContext()).thenReturn(mContext);

        mPreference = new Preference(mContext);
        mPreference.setKey(BluetoothPairingPreferenceController.KEY_PAIRING);

        mController = new BluetoothPairingPreferenceController(mContext, mFragment);
    }

    @Test
    public void testCreateBluetoothPairingPreference() {
        Preference pref = mController.createBluetoothPairingPreference(ORDER);

        assertThat(pref.getKey()).isEqualTo(BluetoothPairingPreferenceController.KEY_PAIRING);
        assertThat(pref.getIcon()).isEqualTo(mContext.getDrawable(R.drawable.ic_menu_add));
        assertThat(pref.getOrder()).isEqualTo(ORDER);
        assertThat(pref.getTitle())
            .isEqualTo(mContext.getString(R.string.bluetooth_pairing_pref_title));
    }

    @Test
    public void testHandlePreferenceTreeClick_startFragment() {
        doNothing().when(mContext).startActivity(any(Intent.class));

        mController.handlePreferenceTreeClick(mPreference);

        verify(mContext).startActivity(any(Intent.class));
    }
}
