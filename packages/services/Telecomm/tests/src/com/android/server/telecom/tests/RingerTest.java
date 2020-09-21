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
 * limitations under the License
 */

package com.android.server.telecom.tests;

import android.app.NotificationManager;
import android.content.Context;
import android.content.res.Resources;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.Ringtone;
import android.net.Uri;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.telecom.TelecomManager;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.telecom.AsyncRingtonePlayer;
import com.android.server.telecom.Call;
import com.android.server.telecom.InCallController;
import com.android.server.telecom.InCallTonePlayer;
import com.android.server.telecom.Ringer;
import com.android.server.telecom.RingtoneFactory;
import com.android.server.telecom.SystemSettingsUtil;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(JUnit4.class)
public class RingerTest extends TelecomTestCase {
    private static final Uri FAKE_RINGTONE_URI = Uri.parse("content://media/fake/audio/1729");

    @Mock InCallTonePlayer.Factory mockPlayerFactory;
    @Mock SystemSettingsUtil mockSystemSettingsUtil;
    @Mock AsyncRingtonePlayer mockRingtonePlayer;
    @Mock RingtoneFactory mockRingtoneFactory;
    @Mock Vibrator mockVibrator;
    @Mock InCallController mockInCallController;

    @Mock InCallTonePlayer mockTonePlayer;
    @Mock Call mockCall1;
    @Mock Call mockCall2;

    Ringer mRingerUnderTest;
    AudioManager mockAudioManager;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        mContext = mComponentContextFixture.getTestDouble().getApplicationContext();
        mRingerUnderTest = new Ringer(mockPlayerFactory, mContext, mockSystemSettingsUtil,
                mockRingtonePlayer, mockRingtoneFactory, mockVibrator, mockInCallController);
        when(mockPlayerFactory.createPlayer(anyInt())).thenReturn(mockTonePlayer);
        mockAudioManager =
                (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        when(mockAudioManager.getRingerModeInternal()).thenReturn(AudioManager.RINGER_MODE_NORMAL);
        NotificationManager notificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        when(notificationManager.matchesCallFilter(any(Bundle.class))).thenReturn(true);
    }

    @SmallTest
    @Test
    public void testNoActionInTheaterMode() {
        // Start call waiting to make sure that it doesn't stop when we start ringing
        mRingerUnderTest.startCallWaiting(mockCall1);
        when(mockSystemSettingsUtil.isTheaterModeOn(any(Context.class))).thenReturn(true);
        assertFalse(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer, never()).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testNoActionWithExternalRinger() {
        Bundle externalRingerExtra = new Bundle();
        externalRingerExtra.putBoolean(TelecomManager.EXTRA_CALL_EXTERNAL_RINGER, true);
        when(mockCall1.getIntentExtras()).thenReturn(externalRingerExtra);
        when(mockCall2.getIntentExtras()).thenReturn(externalRingerExtra);
        // Start call waiting to make sure that it doesn't stop when we start ringing
        mRingerUnderTest.startCallWaiting(mockCall1);
        assertFalse(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer, never()).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testNoActionWhenDialerRings() {
        // Start call waiting to make sure that it doesn't stop when we start ringing
        mRingerUnderTest.startCallWaiting(mockCall1);
        when(mockInCallController.doesConnectedDialerSupportRinging()).thenReturn(true);
        assertFalse(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer, never()).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testAudioFocusStillAcquiredWhenDialerRings() {
        // Start call waiting to make sure that it doesn't stop when we start ringing
        mRingerUnderTest.startCallWaiting(mockCall1);
        when(mockInCallController.doesConnectedDialerSupportRinging()).thenReturn(true);
        ensureRingerIsAudible();
        assertTrue(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer, never()).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testNoActionWhenCallIsSelfManaged() {
        // Start call waiting to make sure that it doesn't stop when we start ringing
        mRingerUnderTest.startCallWaiting(mockCall1);
        when(mockCall2.isSelfManaged()).thenReturn(true);
        // We do want to acquire audio focus when self-managed
        assertTrue(mRingerUnderTest.startRinging(mockCall2, true));
        verify(mockTonePlayer, never()).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testCallWaitingButNoRingForSpecificContacts() {
        NotificationManager notificationManager =
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        when(notificationManager.matchesCallFilter(any(Bundle.class))).thenReturn(false);
        // Start call waiting to make sure that it does stop when we start ringing
        mRingerUnderTest.startCallWaiting(mockCall1);
        verify(mockTonePlayer).startTone();

        assertFalse(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testVibrateButNoRingForNullRingtone() {
        mRingerUnderTest.startCallWaiting(mockCall1);
        when(mockRingtoneFactory.getRingtone(any(Call.class))).thenReturn(null);
        when(mockAudioManager.getRingerModeInternal()).thenReturn(AudioManager.RINGER_MODE_NORMAL);
        enableVibrationWhenRinging();
        assertFalse(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator).vibrate(eq(mRingerUnderTest.mDefaultVibrationEffect),
                any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testVibrateButNoRingForSilentRingtone() {
        mRingerUnderTest.startCallWaiting(mockCall1);
        Ringtone mockRingtone = mock(Ringtone.class);
        when(mockRingtoneFactory.getRingtone(any(Call.class))).thenReturn(mockRingtone);
        when(mockAudioManager.getRingerModeInternal()).thenReturn(AudioManager.RINGER_MODE_NORMAL);
        when(mockAudioManager.getStreamVolume(AudioManager.STREAM_RING)).thenReturn(0);
        enableVibrationWhenRinging();
        assertFalse(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator).vibrate(eq(mRingerUnderTest.mDefaultVibrationEffect),
                any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testCustomVibrationForRingtone() {
        Resources resources = mContext.getResources();
        when(resources.getStringArray(com.android.internal.R.array.config_ringtoneEffectUris))
                .thenReturn(new String[] { FAKE_RINGTONE_URI.toString() });
        mRingerUnderTest.startCallWaiting(mockCall1);
        Ringtone mockRingtone = mock(Ringtone.class);
        when(mockRingtoneFactory.getRingtone(any(Call.class))).thenReturn(mockRingtone);
        when(mockRingtone.getUri()).thenReturn(FAKE_RINGTONE_URI);
        enableVibrationWhenRinging();
        assertTrue(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator).vibrate(eq(VibrationEffect.get(FAKE_RINGTONE_URI, mContext)),
                any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testRingAndNoVibrate() {
        mRingerUnderTest.startCallWaiting(mockCall1);
        ensureRingerIsAudible();
        enableVibrationOnlyWhenNotRinging();
        assertTrue(mRingerUnderTest.startRinging(mockCall2, false));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testSilentRingWithHfpStillAcquiresFocus1() {
        mRingerUnderTest.startCallWaiting(mockCall1);
        Ringtone mockRingtone = mock(Ringtone.class);
        when(mockRingtoneFactory.getRingtone(any(Call.class))).thenReturn(mockRingtone);
        when(mockAudioManager.getRingerModeInternal()).thenReturn(AudioManager.RINGER_MODE_NORMAL);
        when(mockAudioManager.getStreamVolume(AudioManager.STREAM_RING)).thenReturn(0);
        enableVibrationOnlyWhenNotRinging();
        assertTrue(mRingerUnderTest.startRinging(mockCall2, true));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    @SmallTest
    @Test
    public void testSilentRingWithHfpStillAcquiresFocus2() {
        mRingerUnderTest.startCallWaiting(mockCall1);
        when(mockRingtoneFactory.getRingtone(any(Call.class))).thenReturn(null);
        when(mockAudioManager.getRingerModeInternal()).thenReturn(AudioManager.RINGER_MODE_NORMAL);
        when(mockAudioManager.getStreamVolume(AudioManager.STREAM_RING)).thenReturn(0);
        enableVibrationOnlyWhenNotRinging();
        assertTrue(mRingerUnderTest.startRinging(mockCall2, true));
        verify(mockTonePlayer).stopTone();
        verify(mockRingtonePlayer, never()).play(any(RingtoneFactory.class), any(Call.class));
        verify(mockVibrator, never())
                .vibrate(any(VibrationEffect.class), any(AudioAttributes.class));
    }

    private void ensureRingerIsAudible() {
        Ringtone mockRingtone = mock(Ringtone.class);
        when(mockRingtoneFactory.getRingtone(any(Call.class))).thenReturn(mockRingtone);
        when(mockAudioManager.getRingerModeInternal()).thenReturn(AudioManager.RINGER_MODE_NORMAL);
        when(mockAudioManager.getStreamVolume(AudioManager.STREAM_RING)).thenReturn(100);
    }

    private void enableVibrationWhenRinging() {
        when(mockVibrator.hasVibrator()).thenReturn(true);
        when(mockSystemSettingsUtil.canVibrateWhenRinging(any(Context.class))).thenReturn(true);
    }

    private void enableVibrationOnlyWhenNotRinging() {
        when(mockVibrator.hasVibrator()).thenReturn(true);
        when(mockSystemSettingsUtil.canVibrateWhenRinging(any(Context.class))).thenReturn(false);
    }
}
