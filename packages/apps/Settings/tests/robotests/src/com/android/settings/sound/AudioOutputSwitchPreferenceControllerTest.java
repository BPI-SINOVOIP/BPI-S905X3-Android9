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

package com.android.settings.sound;


import static android.media.AudioManager.DEVICE_OUT_BLUETOOTH_SCO;
import static android.media.AudioManager.STREAM_RING;
import static android.media.AudioManager.STREAM_VOICE_CALL;
import static android.media.AudioSystem.DEVICE_OUT_ALL_SCO;
import static android.media.AudioSystem.DEVICE_OUT_BLUETOOTH_A2DP;
import static android.media.AudioSystem.DEVICE_OUT_BLUETOOTH_SCO_HEADSET;
import static android.media.AudioSystem.DEVICE_OUT_HEARING_AID;
import static android.media.AudioSystem.STREAM_MUSIC;

import static com.android.settings.core.BasePreferenceController.AVAILABLE;
import static com.android.settings.core.BasePreferenceController.CONDITIONALLY_UNAVAILABLE;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import static org.robolectric.Shadows.shadowOf;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.pm.PackageManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;
import android.util.FeatureFlagUtils;

import com.android.settings.R;
import com.android.settings.core.FeatureFlags;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.testutils.shadow.ShadowAudioManager;
import com.android.settings.testutils.shadow.ShadowBluetoothUtils;
import com.android.settings.testutils.shadow.ShadowMediaRouter;
import com.android.settingslib.bluetooth.A2dpProfile;
import com.android.settingslib.bluetooth.BluetoothCallback;
import com.android.settingslib.bluetooth.BluetoothEventManager;
import com.android.settingslib.bluetooth.HeadsetProfile;
import com.android.settingslib.bluetooth.HearingAidProfile;
import com.android.settingslib.bluetooth.LocalBluetoothManager;
import com.android.settingslib.bluetooth.LocalBluetoothProfileManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.shadows.ShadowBluetoothDevice;
import org.robolectric.shadows.ShadowPackageManager;

import java.util.ArrayList;
import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
@Config(shadows = {
        ShadowAudioManager.class,
        ShadowMediaRouter.class,
        ShadowBluetoothUtils.class,
        ShadowBluetoothDevice.class}
)
public class AudioOutputSwitchPreferenceControllerTest {
    private static final String TEST_KEY = "Test_Key";
    private static final String TEST_DEVICE_NAME_1 = "Test_A2DP_BT_Device_NAME_1";
    private static final String TEST_DEVICE_NAME_2 = "Test_A2DP_BT_Device_NAME_2";
    private static final String TEST_DEVICE_ADDRESS_1 = "00:A1:A1:A1:A1:A1";
    private static final String TEST_DEVICE_ADDRESS_2 = "00:B2:B2:B2:B2:B2";
    private static final String TEST_DEVICE_ADDRESS_3 = "00:C3:C3:C3:C3:C3";
    private final static long HISYNCID1 = 10;
    private final static long HISYNCID2 = 11;

    @Mock
    private LocalBluetoothManager mLocalManager;
    @Mock
    private BluetoothEventManager mBluetoothEventManager;
    @Mock
    private LocalBluetoothProfileManager mLocalBluetoothProfileManager;
    @Mock
    private A2dpProfile mA2dpProfile;
    @Mock
    private HeadsetProfile mHeadsetProfile;
    @Mock
    private HearingAidProfile mHearingAidProfile;

    private Context mContext;
    private PreferenceScreen mScreen;
    private ListPreference mPreference;
    private ShadowAudioManager mShadowAudioManager;
    private ShadowMediaRouter mShadowMediaRouter;
    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothDevice mBluetoothDevice;
    private BluetoothDevice mLeftBluetoothHapDevice;
    private BluetoothDevice mRightBluetoothHapDevice;
    private LocalBluetoothManager mLocalBluetoothManager;
    private AudioSwitchPreferenceController mController;
    private List<BluetoothDevice> mProfileConnectedDevices;
    private List<BluetoothDevice> mHearingAidActiveDevices;
    private List<BluetoothDevice> mEmptyDevices;
    private ShadowPackageManager mPackageManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mContext = spy(RuntimeEnvironment.application);

        mShadowAudioManager = ShadowAudioManager.getShadow();
        mShadowMediaRouter = ShadowMediaRouter.getShadow();

        ShadowBluetoothUtils.sLocalBluetoothManager = mLocalManager;
        mLocalBluetoothManager = ShadowBluetoothUtils.getLocalBtManager(mContext);

        when(mLocalBluetoothManager.getEventManager()).thenReturn(mBluetoothEventManager);
        when(mLocalBluetoothManager.getProfileManager()).thenReturn(mLocalBluetoothProfileManager);
        when(mLocalBluetoothProfileManager.getA2dpProfile()).thenReturn(mA2dpProfile);
        when(mLocalBluetoothProfileManager.getHearingAidProfile()).thenReturn(mHearingAidProfile);
        when(mLocalBluetoothProfileManager.getHeadsetProfile()).thenReturn(mHeadsetProfile);
        mPackageManager = Shadow.extract(mContext.getPackageManager());
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, true);

        mBluetoothManager = new BluetoothManager(mContext);
        mBluetoothAdapter = mBluetoothManager.getAdapter();

        mBluetoothDevice = spy(mBluetoothAdapter.getRemoteDevice(TEST_DEVICE_ADDRESS_1));
        when(mBluetoothDevice.getName()).thenReturn(TEST_DEVICE_NAME_1);
        when(mBluetoothDevice.isConnected()).thenReturn(true);

        mLeftBluetoothHapDevice = spy(mBluetoothAdapter.getRemoteDevice(TEST_DEVICE_ADDRESS_2));
        when(mLeftBluetoothHapDevice.isConnected()).thenReturn(true);
        mRightBluetoothHapDevice = spy(mBluetoothAdapter.getRemoteDevice(TEST_DEVICE_ADDRESS_3));
        when(mRightBluetoothHapDevice.isConnected()).thenReturn(true);

        mController = new AudioSwitchPreferenceControllerTestable(mContext, TEST_KEY);
        mScreen = spy(new PreferenceScreen(mContext, null));
        mPreference = new ListPreference(mContext);
        mProfileConnectedDevices = new ArrayList<>();
        mHearingAidActiveDevices = new ArrayList<>(2);
        mEmptyDevices = new ArrayList<>(2);

        when(mScreen.getPreferenceManager()).thenReturn(mock(PreferenceManager.class));
        when(mScreen.getContext()).thenReturn(mContext);
        when(mScreen.findPreference(mController.getPreferenceKey())).thenReturn(mPreference);
        mScreen.addPreference(mPreference);
        mController.displayPreference(mScreen);
    }

    @After
    public void tearDown() {
        mShadowAudioManager.reset();
        mShadowMediaRouter.reset();
        ShadowBluetoothUtils.reset();
    }

    @Test
    public void constructor_notSupportBluetooth_shouldReturnBeforeUsingLocalBluetoothManager() {
        ShadowBluetoothUtils.reset();
        mLocalBluetoothManager = ShadowBluetoothUtils.getLocalBtManager(mContext);

        AudioSwitchPreferenceController controller = new AudioSwitchPreferenceControllerTestable(
                mContext, TEST_KEY);
        controller.onStart();
        controller.onStop();

        assertThat(mLocalBluetoothManager).isNull();
    }

    @Test
    public void getAvailabilityStatus_disableFlagNoBluetoothFeature_returnUnavailable() {
        FeatureFlagUtils.setEnabled(mContext, FeatureFlags.AUDIO_SWITCHER_SETTINGS, false);
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, false);

        assertThat(mController.getAvailabilityStatus()).isEqualTo(CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void getAvailabilityStatus_disableFlagWithBluetoothFeature_returnUnavailable() {
        FeatureFlagUtils.setEnabled(mContext, FeatureFlags.AUDIO_SWITCHER_SETTINGS, false);
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, true);


        assertThat(mController.getAvailabilityStatus()).isEqualTo(CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void getAvailabilityStatus_enableFlagWithBluetoothFeature_returnAvailable() {
        FeatureFlagUtils.setEnabled(mContext, FeatureFlags.AUDIO_SWITCHER_SETTINGS, true);
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, true);

        assertThat(mController.getAvailabilityStatus()).isEqualTo(AVAILABLE);
    }

    @Test
    public void getAvailabilityStatus_enableFlagNoBluetoothFeature_returnUnavailable() {
        FeatureFlagUtils.setEnabled(mContext, FeatureFlags.AUDIO_SWITCHER_SETTINGS, true);
        mPackageManager.setSystemFeature(PackageManager.FEATURE_BLUETOOTH, false);

        assertThat(mController.getAvailabilityStatus()).isEqualTo(CONDITIONALLY_UNAVAILABLE);
    }

    @Test
    public void onStart_shouldRegisterCallbackAndRegisterReceiver() {
        mController.onStart();

        verify(mLocalBluetoothManager.getEventManager()).registerCallback(
                any(BluetoothCallback.class));
        verify(mContext).registerReceiver(any(BroadcastReceiver.class), any(IntentFilter.class));
        verify(mLocalBluetoothManager).setForegroundActivity(mContext);
    }

    @Test
    public void onStop_shouldUnregisterCallbackAndUnregisterReceiver() {
        mController.onStart();
        mController.onStop();

        verify(mLocalBluetoothManager.getEventManager()).unregisterCallback(
                any(BluetoothCallback.class));
        verify(mContext).unregisterReceiver(any(BroadcastReceiver.class));
        verify(mLocalBluetoothManager).setForegroundActivity(null);
    }

    @Test
    public void onPreferenceChange_toThisDevice_shouldSetDefaultSummary() {
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);

        mController.onPreferenceChange(mPreference,
                mContext.getText(R.string.media_output_default_summary));

        assertThat(mPreference.getSummary()).isEqualTo(
                mContext.getText(R.string.media_output_default_summary));
    }

    /**
     * One Bluetooth devices are available, and select the device.
     * Preference summary should be device name.
     */
    @Test
    public void onPreferenceChange_toBtDevice_shouldSetBtDeviceName() {
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);

        mController.onPreferenceChange(mPreference, TEST_DEVICE_ADDRESS_1);

        assertThat(mPreference.getSummary()).isEqualTo(TEST_DEVICE_NAME_1);
    }

    /**
     * More than one Bluetooth devices are available, and select second device.
     * Preference summary should be second device name.
     */
    @Test
    public void onPreferenceChange_toBtDevices_shouldSetSecondBtDeviceName() {
        ShadowBluetoothDevice shadowBluetoothDevice;
        BluetoothDevice secondBluetoothDevice;
        secondBluetoothDevice = mBluetoothAdapter.getRemoteDevice(TEST_DEVICE_ADDRESS_2);
        shadowBluetoothDevice = shadowOf(secondBluetoothDevice);
        shadowBluetoothDevice.setName(TEST_DEVICE_NAME_2);
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);
        mController.mConnectedDevices.add(secondBluetoothDevice);

        mController.onPreferenceChange(mPreference, TEST_DEVICE_ADDRESS_2);

        assertThat(mPreference.getSummary()).isEqualTo(TEST_DEVICE_NAME_2);
    }

    /**
     * mConnectedDevices is empty.
     * onPreferenceChange should return false.
     */
    @Test
    public void onPreferenceChange_connectedDeviceIsNull_shouldReturnFalse() {
        mController.mConnectedDevices.clear();

        assertThat(mController.onPreferenceChange(mPreference, TEST_DEVICE_ADDRESS_1)).isFalse();
    }

    /**
     * Audio stream output to bluetooth sco headset which is the subset of all sco device.
     * isStreamFromOutputDevice should return true.
     */
    @Test
    public void isStreamFromOutputDevice_outputDeviceIsBtScoHeadset_shouldReturnTrue() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_BLUETOOTH_SCO_HEADSET);

        assertThat(mController.isStreamFromOutputDevice(STREAM_MUSIC, DEVICE_OUT_ALL_SCO)).isTrue();
    }

    /**
     * Audio stream is not STREAM_MUSIC or STREAM_VOICE_CALL.
     * findActiveDevice should return null.
     */
    @Test
    public void findActiveDevice_streamIsRing_shouldReturnNull() {
        assertThat(mController.findActiveDevice(STREAM_RING)).isNull();
    }

    /**
     * Audio stream is STREAM_MUSIC and output device is A2dp bluetooth device.
     * findActiveDevice should return A2dp active device.
     */
    @Test
    public void findActiveDevice_streamMusicToA2dpDevice_shouldReturnActiveA2dpDevice() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_BLUETOOTH_A2DP);
        mHearingAidActiveDevices.clear();
        mHearingAidActiveDevices.add(mLeftBluetoothHapDevice);
        when(mHeadsetProfile.getActiveDevice()).thenReturn(mLeftBluetoothHapDevice);
        when(mA2dpProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mHearingAidProfile.getActiveDevices()).thenReturn(mHearingAidActiveDevices);

        assertThat(mController.findActiveDevice(STREAM_MUSIC)).isEqualTo(mBluetoothDevice);
    }

    /**
     * Audio stream is STREAM_VOICE_CALL and output device is Hands free profile bluetooth device.
     * findActiveDevice should return Hands free profile active device.
     */
    @Test
    public void findActiveDevice_streamVoiceCallToHfpDevice_shouldReturnActiveHfpDevice() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_BLUETOOTH_SCO);
        mHearingAidActiveDevices.clear();
        mHearingAidActiveDevices.add(mLeftBluetoothHapDevice);
        when(mHeadsetProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mA2dpProfile.getActiveDevice()).thenReturn(mLeftBluetoothHapDevice);
        when(mHearingAidProfile.getActiveDevices()).thenReturn(mHearingAidActiveDevices);

        assertThat(mController.findActiveDevice(STREAM_VOICE_CALL)).isEqualTo(mBluetoothDevice);
    }

    /**
     * Audio stream is STREAM_MUSIC or STREAM_VOICE_CALL and output device is hearing aid profile
     * bluetooth device. And left side of HAP device is active.
     * findActiveDevice should return hearing aid device active device.
     */
    @Test
    public void findActiveDevice_streamToHapDeviceLeftActiveDevice_shouldReturnActiveHapDevice() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_HEARING_AID);
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);
        mController.mConnectedDevices.add(mLeftBluetoothHapDevice);
        mHearingAidActiveDevices.clear();
        mHearingAidActiveDevices.add(mLeftBluetoothHapDevice);
        mHearingAidActiveDevices.add(null);
        when(mHeadsetProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mA2dpProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mHearingAidProfile.getActiveDevices()).thenReturn(mHearingAidActiveDevices);

        assertThat(mController.findActiveDevice(STREAM_MUSIC)).isEqualTo(mLeftBluetoothHapDevice);
        assertThat(mController.findActiveDevice(STREAM_VOICE_CALL)).isEqualTo(
                mLeftBluetoothHapDevice);
    }

    /**
     * Audio stream is STREAM_MUSIC or STREAM_VOICE_CALL and output device is hearing aid profile
     * bluetooth device. And right side of HAP device is active.
     * findActiveDevice should return hearing aid device active device.
     */
    @Test
    public void findActiveDevice_streamToHapDeviceRightActiveDevice_shouldReturnActiveHapDevice() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_HEARING_AID);
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);
        mController.mConnectedDevices.add(mRightBluetoothHapDevice);
        mHearingAidActiveDevices.clear();
        mHearingAidActiveDevices.add(null);
        mHearingAidActiveDevices.add(mRightBluetoothHapDevice);
        mHearingAidActiveDevices.add(mRightBluetoothHapDevice);
        when(mHeadsetProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mA2dpProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mHearingAidProfile.getActiveDevices()).thenReturn(mHearingAidActiveDevices);

        assertThat(mController.findActiveDevice(STREAM_MUSIC)).isEqualTo(mRightBluetoothHapDevice);
        assertThat(mController.findActiveDevice(STREAM_VOICE_CALL)).isEqualTo(
                mRightBluetoothHapDevice);
    }

    /**
     * Audio stream is STREAM_MUSIC or STREAM_VOICE_CALL and output device is hearing aid
     * profile bluetooth device. And both are active device.
     * findActiveDevice should return only return the active device in mConnectedDevices.
     */
    @Test
    public void findActiveDevice_streamToHapDeviceTwoActiveDevice_shouldReturnActiveHapDevice() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_HEARING_AID);
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);
        mController.mConnectedDevices.add(mRightBluetoothHapDevice);
        mHearingAidActiveDevices.clear();
        mHearingAidActiveDevices.add(mLeftBluetoothHapDevice);
        mHearingAidActiveDevices.add(mRightBluetoothHapDevice);
        when(mHeadsetProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mA2dpProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mHearingAidProfile.getActiveDevices()).thenReturn(mHearingAidActiveDevices);

        assertThat(mController.findActiveDevice(STREAM_MUSIC)).isEqualTo(mRightBluetoothHapDevice);
        assertThat(mController.findActiveDevice(STREAM_VOICE_CALL)).isEqualTo(
                mRightBluetoothHapDevice);
    }

    /**
     * Audio stream is STREAM_MUSIC or STREAM_VOICE_CALL and output device is hearing aid
     * profile bluetooth device. And none of them are active.
     * findActiveDevice should return null.
     */
    @Test
    public void findActiveDevice_streamToOtherDevice_shouldReturnActiveHapDevice() {
        mShadowAudioManager.setOutputDevice(DEVICE_OUT_HEARING_AID);
        mController.mConnectedDevices.clear();
        mController.mConnectedDevices.add(mBluetoothDevice);
        mController.mConnectedDevices.add(mLeftBluetoothHapDevice);
        mHearingAidActiveDevices.clear();
        when(mHeadsetProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mA2dpProfile.getActiveDevice()).thenReturn(mBluetoothDevice);
        when(mHearingAidProfile.getActiveDevices()).thenReturn(mHearingAidActiveDevices);

        assertThat(mController.findActiveDevice(STREAM_MUSIC)).isNull();
        assertThat(mController.findActiveDevice(STREAM_VOICE_CALL)).isNull();
    }

    /**
     * Two hearing aid devices with different HisyncId
     * getConnectedHearingAidDevices should add both device to list.
     */
    @Test
    public void getConnectedHearingAidDevices_deviceHisyncIdIsDifferent_shouldAddBothToList() {
        mEmptyDevices.clear();
        mProfileConnectedDevices.clear();
        mProfileConnectedDevices.add(mLeftBluetoothHapDevice);
        mProfileConnectedDevices.add(mRightBluetoothHapDevice);
        when(mHearingAidProfile.getConnectedDevices()).thenReturn(mProfileConnectedDevices);
        when(mHearingAidProfile.getHiSyncId(mLeftBluetoothHapDevice)).thenReturn(HISYNCID1);
        when(mHearingAidProfile.getHiSyncId(mRightBluetoothHapDevice)).thenReturn(HISYNCID2);

        mEmptyDevices.addAll(mController.getConnectedHearingAidDevices());

        assertThat(mEmptyDevices).containsExactly(mLeftBluetoothHapDevice,
                mRightBluetoothHapDevice);
    }

    /**
     * Two hearing aid devices with same HisyncId
     * getConnectedHearingAidDevices should only add first device to list.
     */
    @Test
    public void getConnectedHearingAidDevices_deviceHisyncIdIsSame_shouldAddOneToList() {
        mEmptyDevices.clear();
        mProfileConnectedDevices.clear();
        mProfileConnectedDevices.add(mLeftBluetoothHapDevice);
        mProfileConnectedDevices.add(mRightBluetoothHapDevice);
        when(mHearingAidProfile.getConnectedDevices()).thenReturn(mProfileConnectedDevices);
        when(mHearingAidProfile.getHiSyncId(mLeftBluetoothHapDevice)).thenReturn(HISYNCID1);
        when(mHearingAidProfile.getHiSyncId(mRightBluetoothHapDevice)).thenReturn(HISYNCID1);

        mEmptyDevices.addAll(mController.getConnectedHearingAidDevices());

        assertThat(mEmptyDevices).containsExactly(mLeftBluetoothHapDevice);
    }

    /**
     * One A2dp device is connected.
     * getConnectedA2dpDevices should add this device to list.
     */
    @Test
    public void getConnectedA2dpDevices_oneConnectedA2dpDevice_shouldAddDeviceToList() {
        mEmptyDevices.clear();
        mProfileConnectedDevices.clear();
        mProfileConnectedDevices.add(mBluetoothDevice);
        when(mA2dpProfile.getConnectedDevices()).thenReturn(mProfileConnectedDevices);

        mEmptyDevices.addAll(mController.getConnectedA2dpDevices());

        assertThat(mEmptyDevices).containsExactly(mBluetoothDevice);
    }

    /**
     * More than one A2dp devices are connected.
     * getConnectedA2dpDevices should add all devices to list.
     */
    @Test
    public void getConnectedA2dpDevices_moreThanOneConnectedA2dpDevice_shouldAddDeviceToList() {
        mEmptyDevices.clear();
        mProfileConnectedDevices.clear();
        mProfileConnectedDevices.add(mBluetoothDevice);
        mProfileConnectedDevices.add(mLeftBluetoothHapDevice);
        when(mA2dpProfile.getConnectedDevices()).thenReturn(mProfileConnectedDevices);

        mEmptyDevices.addAll(mController.getConnectedA2dpDevices());

        assertThat(mEmptyDevices).containsExactly(mBluetoothDevice, mLeftBluetoothHapDevice);
    }

    /**
     * One hands free profile device is connected.
     * getConnectedA2dpDevices should add this device to list.
     */
    @Test
    public void getConnectedHfpDevices_oneConnectedHfpDevice_shouldAddDeviceToList() {
        mEmptyDevices.clear();
        mProfileConnectedDevices.clear();
        mProfileConnectedDevices.add(mBluetoothDevice);
        when(mHeadsetProfile.getConnectedDevices()).thenReturn(mProfileConnectedDevices);

        mEmptyDevices.addAll(mController.getConnectedHfpDevices());

        assertThat(mEmptyDevices).containsExactly(mBluetoothDevice);
    }

    /**
     * More than one hands free profile devices are connected.
     * getConnectedA2dpDevices should add all devices to list.
     */
    @Test
    public void getConnectedHfpDevices_moreThanOneConnectedHfpDevice_shouldAddDeviceToList() {
        mEmptyDevices.clear();
        mProfileConnectedDevices.clear();
        mProfileConnectedDevices.add(mBluetoothDevice);
        mProfileConnectedDevices.add(mLeftBluetoothHapDevice);
        when(mHeadsetProfile.getConnectedDevices()).thenReturn(mProfileConnectedDevices);

        mEmptyDevices.addAll(mController.getConnectedHfpDevices());

        assertThat(mEmptyDevices).containsExactly(mBluetoothDevice, mLeftBluetoothHapDevice);
    }

    private class AudioSwitchPreferenceControllerTestable extends
            AudioSwitchPreferenceController {
        AudioSwitchPreferenceControllerTestable(Context context, String key) {
            super(context, key);
        }

        @Override
        public void setActiveBluetoothDevice(BluetoothDevice device) {
        }

        @Override
        public String getPreferenceKey() {
            return TEST_KEY;
        }
    }
}