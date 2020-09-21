/*
 * Copyright 2018 The Android Open Source Project
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
 * limitations under the License
 */
package com.android.settings.connecteddevice;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.v7.preference.PreferenceScreen;
import android.text.BidiFormatter;
import android.text.TextUtils;

import com.android.settings.core.BasePreferenceController;
import com.android.settings.bluetooth.AlwaysDiscoverable;
import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowBluetoothAdapter;
import com.android.settings.testutils.shadow.ShadowBluetoothPan;
import com.android.settings.testutils.shadow.ShadowLocalBluetoothAdapter;
import com.android.settingslib.widget.FooterPreference;
import com.android.settingslib.widget.FooterPreferenceMixin;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

import java.util.ArrayList;
import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = {ShadowBluetoothPan.class, ShadowBluetoothAdapter.class,
        ShadowLocalBluetoothAdapter.class})
public class DiscoverableFooterPreferenceControllerTest {
    private static final String DEVICE_NAME = "device name";
    private static final String KEY = "discoverable_footer_preference";

    @Mock
    private PackageManager mPackageManager;
    @Mock
    private PreferenceScreen mScreen;
    @Mock
    private FooterPreferenceMixin mFooterPreferenceMixin;
    @Mock
    private AlwaysDiscoverable mAlwaysDiscoverable;

    private Context mContext;
    private FooterPreference mPreference;
    private DiscoverableFooterPreferenceController mDiscoverableFooterPreferenceController;
    private BroadcastReceiver mBluetoothChangedReceiver;
    private ShadowApplication mShadowApplication;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mShadowApplication = Shadows.shadowOf(RuntimeEnvironment.application);
        mContext = spy(RuntimeEnvironment.application);

        doReturn(mPackageManager).when(mContext).getPackageManager();
        mDiscoverableFooterPreferenceController =
                new DiscoverableFooterPreferenceController(mContext);
        mPreference = spy(new FooterPreference(mContext));
        mDiscoverableFooterPreferenceController.init(mFooterPreferenceMixin, mPreference,
                mAlwaysDiscoverable);
        mBluetoothChangedReceiver = mDiscoverableFooterPreferenceController
                .mBluetoothChangedReceiver;
    }

    @Test
    public void getAvailabilityStatus_noBluetoothFeature_returnUnSupported() {
        when(mPackageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)).thenReturn(false);

        assertThat(mDiscoverableFooterPreferenceController.getAvailabilityStatus()).isEqualTo(
                BasePreferenceController.UNSUPPORTED_ON_DEVICE);
    }

    @Test
    public void getAvailabilityStatus_BluetoothFeature_returnAvailable() {
        when(mPackageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)).thenReturn(true);

        assertThat(mDiscoverableFooterPreferenceController.getAvailabilityStatus()).isEqualTo(
                BasePreferenceController.AVAILABLE);
    }

    @Test
    public void displayPreference() {
        when(mFooterPreferenceMixin.createFooterPreference()).thenReturn(mPreference);
        mDiscoverableFooterPreferenceController.displayPreference(mScreen);

        verify(mPreference).setKey(KEY);
        verify(mScreen).addPreference(mPreference);
    }

    @Test
    public void onResume() {
        mDiscoverableFooterPreferenceController.onResume();
        assertThat(getRegisteredBroadcastReceivers()).contains(mBluetoothChangedReceiver);
        verify(mAlwaysDiscoverable).start();
    }

    @Test
    public void onPause() {
        mDiscoverableFooterPreferenceController.onResume();
        mDiscoverableFooterPreferenceController.onPause();

        assertThat(getRegisteredBroadcastReceivers()).doesNotContain(mBluetoothChangedReceiver);
        verify(mAlwaysDiscoverable).stop();
    }

    @Test
    public void onBluetoothStateChanged_bluetoothOn_updateTitle() {
        ShadowLocalBluetoothAdapter.setName(DEVICE_NAME);
        sendBluetoothStateChangedIntent(BluetoothAdapter.STATE_ON);

        assertThat(mPreference.getTitle()).isEqualTo(generateTitle(DEVICE_NAME));
    }

    @Test
    public void onBluetoothStateChanged_bluetoothOff_updateTitle(){
        ShadowLocalBluetoothAdapter.setName(DEVICE_NAME);
        sendBluetoothStateChangedIntent(BluetoothAdapter.STATE_OFF);

        assertThat(mPreference.getTitle()).isEqualTo(generateTitle(null));
    }

    private CharSequence generateTitle(String deviceName) {
        if (deviceName == null) {
            return mContext.getString(R.string.bluetooth_off_footer);

        } else {
            return TextUtils.expandTemplate(
                    mContext.getText(R.string.bluetooth_device_name_summary),
                    BidiFormatter.getInstance().unicodeWrap(deviceName));
        }
    }

    private void sendBluetoothStateChangedIntent(int state) {
        Intent intent = new Intent(BluetoothAdapter.ACTION_STATE_CHANGED);
        intent.putExtra(BluetoothAdapter.EXTRA_STATE, state);
        mBluetoothChangedReceiver.onReceive(mContext, intent);
    }

    /**
     * Return a list of all the registered broadcast receivers
     */
    private List<BroadcastReceiver> getRegisteredBroadcastReceivers() {
        List<BroadcastReceiver> registeredBroadcastReceivers = new ArrayList();
        List<ShadowApplication.Wrapper> registeredReceivers =
                mShadowApplication.getRegisteredReceivers();
        for (ShadowApplication.Wrapper wrapper : registeredReceivers) {
            registeredBroadcastReceivers.add(wrapper.getBroadcastReceiver());
        }
        return registeredBroadcastReceivers;
    }
}