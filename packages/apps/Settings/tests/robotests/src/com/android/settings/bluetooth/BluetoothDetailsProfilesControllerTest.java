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
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothClass;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.PreferenceCategory;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.SettingsShadowBluetoothDevice;
import com.android.settingslib.bluetooth.A2dpProfile;
import com.android.settingslib.bluetooth.CachedBluetoothDevice;
import com.android.settingslib.bluetooth.LocalBluetoothManager;
import com.android.settingslib.bluetooth.LocalBluetoothProfile;
import com.android.settingslib.bluetooth.LocalBluetoothProfileManager;
import com.android.settingslib.bluetooth.MapProfile;
import com.android.settingslib.bluetooth.PbapServerProfile;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.annotation.Config;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = SettingsShadowBluetoothDevice.class)
public class BluetoothDetailsProfilesControllerTest extends BluetoothDetailsControllerTestBase {

    private BluetoothDetailsProfilesController mController;
    private List<LocalBluetoothProfile> mConnectableProfiles;
    private PreferenceCategory mProfiles;

    @Mock
    private LocalBluetoothManager mLocalManager;
    @Mock
    private LocalBluetoothProfileManager mProfileManager;

    @Override
    public void setUp() {
        super.setUp();

        mProfiles = spy(new PreferenceCategory(mContext));
        when(mProfiles.getPreferenceManager()).thenReturn(mPreferenceManager);

        mConnectableProfiles = new ArrayList<>();
        when(mLocalManager.getProfileManager()).thenReturn(mProfileManager);
        when(mCachedDevice.getConnectableProfiles()).thenAnswer(invocation ->
            new ArrayList<>(mConnectableProfiles)
        );

        setupDevice(mDeviceConfig);
        mController = new BluetoothDetailsProfilesController(mContext, mFragment, mLocalManager,
                mCachedDevice, mLifecycle);
        mProfiles.setKey(mController.getPreferenceKey());
        mScreen.addPreference(mProfiles);
    }

    static class FakeBluetoothProfile implements LocalBluetoothProfile {

        private Set<BluetoothDevice> mConnectedDevices = new HashSet<>();
        private Map<BluetoothDevice, Boolean> mPreferred = new HashMap<>();
        private Context mContext;
        private int mNameResourceId;

        private FakeBluetoothProfile(Context context, int nameResourceId) {
            mContext = context;
            mNameResourceId = nameResourceId;
        }

        @Override
        public String toString() {
            return mContext.getString(mNameResourceId);
        }

        @Override
        public boolean isConnectable() {
            return true;
        }

        @Override
        public boolean isAutoConnectable() {
            return true;
        }

        @Override
        public boolean connect(BluetoothDevice device) {
            mConnectedDevices.add(device);
            return true;
        }

        @Override
        public boolean disconnect(BluetoothDevice device) {
            mConnectedDevices.remove(device);
            return false;
        }

        @Override
        public int getConnectionStatus(BluetoothDevice device) {
            if (mConnectedDevices.contains(device)) {
                return BluetoothProfile.STATE_CONNECTED;
            } else {
                return BluetoothProfile.STATE_DISCONNECTED;
            }
        }

        @Override
        public boolean isPreferred(BluetoothDevice device) {
            return mPreferred.getOrDefault(device, false);
        }

        @Override
        public int getPreferred(BluetoothDevice device) {
            return isPreferred(device) ?
                    BluetoothProfile.PRIORITY_ON : BluetoothProfile.PRIORITY_OFF;
        }

        @Override
        public void setPreferred(BluetoothDevice device, boolean preferred) {
            mPreferred.put(device, preferred);
        }

        @Override
        public boolean isProfileReady() {
            return true;
        }

        @Override
        public int getProfileId() {
            return 0;
        }

        @Override
        public int getOrdinal() {
            return 0;
        }

        @Override
        public int getNameResource(BluetoothDevice device) {
            return mNameResourceId;
        }

        @Override
        public int getSummaryResourceForDevice(BluetoothDevice device) {
            return Utils.getConnectionStateSummary(getConnectionStatus(device));
        }

        @Override
        public int getDrawableResource(BluetoothClass btClass) {
            return 0;
        }
    }

    /**
     * Creates and adds a mock LocalBluetoothProfile to the list of connectable profiles for the
     * device.
     * @param profileNameResId  the resource id for the name used by this profile
     * @param deviceIsPreferred  whether this profile should start out as enabled for the device
     */
    private LocalBluetoothProfile addFakeProfile(int profileNameResId,
            boolean deviceIsPreferred) {
        LocalBluetoothProfile profile = new FakeBluetoothProfile(mContext, profileNameResId);
        profile.setPreferred(mDevice, deviceIsPreferred);
        mConnectableProfiles.add(profile);
        when(mProfileManager.getProfileByName(eq(profile.toString()))).thenReturn(profile);
        return profile;
    }

    /** Returns the list of SwitchPreference objects added to the screen - there should be one per
     *  Bluetooth profile.
     */
    private List<SwitchPreference> getProfileSwitches(boolean expectOnlyMConnectable) {
        if (expectOnlyMConnectable) {
            assertThat(mConnectableProfiles).isNotEmpty();
            assertThat(mProfiles.getPreferenceCount()).isEqualTo(mConnectableProfiles.size());
        }
        List<SwitchPreference> result = new ArrayList<>();
        for (int i = 0; i < mProfiles.getPreferenceCount(); i++) {
            result.add((SwitchPreference)mProfiles.getPreference(i));
        }
        return result;
    }

     private void verifyProfileSwitchTitles(List<SwitchPreference> switches) {
        for (int i = 0; i < switches.size(); i++) {
            String expectedTitle =
                mContext.getString(mConnectableProfiles.get(i).getNameResource(mDevice));
            assertThat(switches.get(i).getTitle()).isEqualTo(expectedTitle);
        }
    }

    @Test
    public void oneProfile() {
        addFakeProfile(R.string.bluetooth_profile_a2dp, true);
        showScreen(mController);
        verifyProfileSwitchTitles(getProfileSwitches(true));
    }

    @Test
    public void multipleProfiles() {
        addFakeProfile(R.string.bluetooth_profile_a2dp, true);
        addFakeProfile(R.string.bluetooth_profile_headset, false);
        showScreen(mController);
        List<SwitchPreference> switches = getProfileSwitches(true);
        verifyProfileSwitchTitles(switches);
        assertThat(switches.get(0).isChecked()).isTrue();
        assertThat(switches.get(1).isChecked()).isFalse();

        // Both switches should be enabled.
        assertThat(switches.get(0).isEnabled()).isTrue();
        assertThat(switches.get(1).isEnabled()).isTrue();

        // Make device busy.
        when(mCachedDevice.isBusy()).thenReturn(true);
        mController.onDeviceAttributesChanged();

        // There should have been no new switches added.
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(2);

        // Make sure both switches got disabled.
        assertThat(switches.get(0).isEnabled()).isFalse();
        assertThat(switches.get(1).isEnabled()).isFalse();
    }

    @Test
    public void disableThenReenableOneProfile() {
        addFakeProfile(R.string.bluetooth_profile_a2dp, true);
        addFakeProfile(R.string.bluetooth_profile_headset, true);
        showScreen(mController);
        List<SwitchPreference> switches = getProfileSwitches(true);
        SwitchPreference pref = switches.get(0);

        // Clicking the pref should cause the profile to become not-preferred.
        assertThat(pref.isChecked()).isTrue();
        pref.performClick();
        assertThat(pref.isChecked()).isFalse();
        assertThat(mConnectableProfiles.get(0).isPreferred(mDevice)).isFalse();

        // Make sure no new preferences were added.
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(2);

        // Clicking the pref again should make the profile once again preferred.
        pref.performClick();
        assertThat(pref.isChecked()).isTrue();
        assertThat(mConnectableProfiles.get(0).isPreferred(mDevice)).isTrue();

        // Make sure we still haven't gotten any new preferences added.
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(2);
    }

    @Test
    public void disconnectedDeviceOneProfile() {
        setupDevice(makeDefaultDeviceConfig().setConnected(false).setConnectionSummary(null));
        addFakeProfile(R.string.bluetooth_profile_a2dp, true);
        showScreen(mController);
        verifyProfileSwitchTitles(getProfileSwitches(true));
    }

    @Test
    public void pbapProfileStartsEnabled() {
        setupDevice(makeDefaultDeviceConfig());
        when(mCachedDevice.getPhonebookPermissionChoice())
            .thenReturn(CachedBluetoothDevice.ACCESS_ALLOWED);
        PbapServerProfile psp = mock(PbapServerProfile.class);
        when(psp.getNameResource(mDevice)).thenReturn(R.string.bluetooth_profile_pbap);
        when(psp.toString()).thenReturn(PbapServerProfile.NAME);
        when(mProfileManager.getPbapProfile()).thenReturn(psp);

        showScreen(mController);
        List<SwitchPreference> switches = getProfileSwitches(false);
        assertThat(switches.size()).isEqualTo(1);
        SwitchPreference pref = switches.get(0);
        assertThat(pref.getTitle()).isEqualTo(mContext.getString(R.string.bluetooth_profile_pbap));
        assertThat(pref.isChecked()).isTrue();

        pref.performClick();
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(1);
        verify(mCachedDevice).setPhonebookPermissionChoice(CachedBluetoothDevice.ACCESS_REJECTED);
    }

    @Test
    public void pbapProfileStartsDisabled() {
        setupDevice(makeDefaultDeviceConfig());
        when(mCachedDevice.getPhonebookPermissionChoice())
            .thenReturn(CachedBluetoothDevice.ACCESS_REJECTED);
        PbapServerProfile psp = mock(PbapServerProfile.class);
        when(psp.getNameResource(mDevice)).thenReturn(R.string.bluetooth_profile_pbap);
        when(psp.toString()).thenReturn(PbapServerProfile.NAME);
        when(mProfileManager.getPbapProfile()).thenReturn(psp);

        showScreen(mController);
        List<SwitchPreference> switches = getProfileSwitches(false);
        assertThat(switches.size()).isEqualTo(1);
        SwitchPreference pref = switches.get(0);
        assertThat(pref.getTitle()).isEqualTo(mContext.getString(R.string.bluetooth_profile_pbap));
        assertThat(pref.isChecked()).isFalse();

        pref.performClick();
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(1);
        verify(mCachedDevice).setPhonebookPermissionChoice(CachedBluetoothDevice.ACCESS_ALLOWED);
    }

    @Test
    public void mapProfile() {
        setupDevice(makeDefaultDeviceConfig());
        MapProfile mapProfile = mock(MapProfile.class);
        when(mapProfile.getNameResource(mDevice)).thenReturn(R.string.bluetooth_profile_map);
        when(mProfileManager.getMapProfile()).thenReturn(mapProfile);
        when(mProfileManager.getProfileByName(eq(mapProfile.toString()))).thenReturn(mapProfile);
        when(mCachedDevice.getMessagePermissionChoice())
            .thenReturn(CachedBluetoothDevice.ACCESS_REJECTED);
        showScreen(mController);
        List<SwitchPreference> switches = getProfileSwitches(false);
        assertThat(switches.size()).isEqualTo(1);
        SwitchPreference pref = switches.get(0);
        assertThat(pref.getTitle()).isEqualTo(mContext.getString(R.string.bluetooth_profile_map));
        assertThat(pref.isChecked()).isFalse();

        pref.performClick();
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(1);
        verify(mCachedDevice).setMessagePermissionChoice(BluetoothDevice.ACCESS_ALLOWED);
    }

    private A2dpProfile addMockA2dpProfile(boolean preferred, boolean supportsHighQualityAudio,
            boolean highQualityAudioEnabled) {
        A2dpProfile profile = mock(A2dpProfile.class);
        when(mProfileManager.getProfileByName(eq(profile.toString()))).thenReturn(profile);
        when(profile.getNameResource(mDevice)).thenReturn(R.string.bluetooth_profile_a2dp);
        when(profile.getHighQualityAudioOptionLabel(mDevice)).thenReturn(
            mContext.getString(R.string.bluetooth_profile_a2dp_high_quality_unknown_codec));
        when(profile.supportsHighQualityAudio(mDevice)).thenReturn(supportsHighQualityAudio);
        when(profile.isHighQualityAudioEnabled(mDevice)).thenReturn(highQualityAudioEnabled);
        when(profile.isPreferred(mDevice)).thenReturn(preferred);
        mConnectableProfiles.add(profile);
        return profile;
    }

    private SwitchPreference getHighQualityAudioPref() {
        return (SwitchPreference) mScreen.findPreference(
                BluetoothDetailsProfilesController.HIGH_QUALITY_AUDIO_PREF_TAG);
    }

    @Test
    public void highQualityAudio_prefIsPresentWhenSupported() {
        setupDevice(makeDefaultDeviceConfig());
        addMockA2dpProfile(true, true, true);
        showScreen(mController);
        SwitchPreference pref = getHighQualityAudioPref();
        assertThat(pref.getKey()).isEqualTo(
                BluetoothDetailsProfilesController.HIGH_QUALITY_AUDIO_PREF_TAG);

        // Make sure the preference works when clicked on.
        pref.performClick();
        A2dpProfile profile = (A2dpProfile) mConnectableProfiles.get(0);
        verify(profile).setHighQualityAudioEnabled(mDevice, false);
        pref.performClick();
        verify(profile).setHighQualityAudioEnabled(mDevice, true);
    }

    @Test
    public void highQualityAudio_prefIsAbsentWhenNotSupported() {
        setupDevice(makeDefaultDeviceConfig());
        addMockA2dpProfile(true, false, false);
        showScreen(mController);
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(1);
        SwitchPreference pref = (SwitchPreference) mProfiles.getPreference(0);
        assertThat(pref.getKey())
            .isNotEqualTo(BluetoothDetailsProfilesController.HIGH_QUALITY_AUDIO_PREF_TAG);
        assertThat(pref.getTitle()).isEqualTo(mContext.getString(R.string.bluetooth_profile_a2dp));
    }

    @Test
    public void highQualityAudio_busyDeviceDisablesSwitch() {
        setupDevice(makeDefaultDeviceConfig());
        addMockA2dpProfile(true, true, true);
        when(mCachedDevice.isBusy()).thenReturn(true);
        showScreen(mController);
        SwitchPreference pref = getHighQualityAudioPref();
        assertThat(pref.isEnabled()).isFalse();
    }

    @Test
    public void highQualityAudio_mediaAudioDisabledAndReEnabled() {
        setupDevice(makeDefaultDeviceConfig());
        A2dpProfile audioProfile = addMockA2dpProfile(true, true, true);
        showScreen(mController);
        assertThat(mProfiles.getPreferenceCount()).isEqualTo(2);

        // Disabling media audio should cause the high quality audio switch to disappear, but not
        // the regular audio one.
        SwitchPreference audioPref =
            (SwitchPreference) mScreen.findPreference(audioProfile.toString());
        audioPref.performClick();
        verify(audioProfile).setPreferred(mDevice, false);
        when(audioProfile.isPreferred(mDevice)).thenReturn(false);
        mController.onDeviceAttributesChanged();
        assertThat(audioPref.isVisible()).isTrue();
        SwitchPreference highQualityAudioPref = getHighQualityAudioPref();
        assertThat(highQualityAudioPref.isVisible()).isFalse();

        // And re-enabling media audio should make high quality switch to reappear.
        audioPref.performClick();
        verify(audioProfile).setPreferred(mDevice, true);
        when(audioProfile.isPreferred(mDevice)).thenReturn(true);
        mController.onDeviceAttributesChanged();
        highQualityAudioPref = getHighQualityAudioPref();
        assertThat(highQualityAudioPref.isVisible()).isTrue();
    }

    @Test
    public void highQualityAudio_mediaAudioStartsDisabled() {
        setupDevice(makeDefaultDeviceConfig());
        A2dpProfile audioProfile = addMockA2dpProfile(false, true, true);
        showScreen(mController);
        SwitchPreference audioPref =
            (SwitchPreference) mScreen.findPreference(audioProfile.toString());
        SwitchPreference highQualityAudioPref = getHighQualityAudioPref();
        assertThat(audioPref).isNotNull();
        assertThat(audioPref.isChecked()).isFalse();
        assertThat(highQualityAudioPref).isNotNull();
        assertThat(highQualityAudioPref.isVisible()).isFalse();
    }
}
