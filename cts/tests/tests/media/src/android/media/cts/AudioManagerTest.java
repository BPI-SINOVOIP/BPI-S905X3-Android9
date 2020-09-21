/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.media.cts;

import static android.media.AudioManager.ADJUST_LOWER;
import static android.media.AudioManager.ADJUST_RAISE;
import static android.media.AudioManager.ADJUST_SAME;
import static android.media.AudioManager.MODE_IN_CALL;
import static android.media.AudioManager.MODE_IN_COMMUNICATION;
import static android.media.AudioManager.MODE_NORMAL;
import static android.media.AudioManager.MODE_RINGTONE;
import static android.media.AudioManager.RINGER_MODE_NORMAL;
import static android.media.AudioManager.RINGER_MODE_SILENT;
import static android.media.AudioManager.RINGER_MODE_VIBRATE;
import static android.media.AudioManager.STREAM_ACCESSIBILITY;
import static android.media.AudioManager.STREAM_MUSIC;
import static android.media.AudioManager.STREAM_RING;
import static android.media.AudioManager.USE_DEFAULT_STREAM_TYPE;
import static android.media.AudioManager.VIBRATE_SETTING_OFF;
import static android.media.AudioManager.VIBRATE_SETTING_ON;
import static android.media.AudioManager.VIBRATE_SETTING_ONLY_SILENT;
import static android.media.AudioManager.VIBRATE_TYPE_NOTIFICATION;
import static android.media.AudioManager.VIBRATE_TYPE_RINGER;
import static android.provider.Settings.System.SOUND_EFFECTS_ENABLED;

import android.app.ActivityManager;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MicrophoneInfo;
import android.os.Vibrator;
import android.platform.test.annotations.AppModeFull;
import android.provider.Settings;
import android.provider.Settings.System;
import android.test.InstrumentationTestCase;
import android.util.Log;
import android.view.SoundEffectConstants;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class AudioManagerTest extends InstrumentationTestCase {
    private final static String TAG = "AudioManagerTest";

    private final static long MUTE_DELAY = 50;
    private final static int MP3_TO_PLAY = R.raw.testmp3;
    private final static long TIME_TO_PLAY = 2000;
    private final static String APPOPS_OP_STR = "android:write_settings";
    private AudioManager mAudioManager;
    private NotificationManager mNm;
    private boolean mHasVibrator;
    private boolean mUseFixedVolume;
    private boolean mIsTelevision;
    private boolean mIsSingleVolume;
    private boolean mSkipRingerTests;
    // From N onwards, ringer mode adjustments that toggle DND are not allowed unless
    // package has DND access. Many tests in this package toggle DND access in order
    // to get device out of the DND state for the test to proceed correctly.
    // But DND access is disabled completely on low ram devices,
    // so completely skip those tests here.
    // These tests are migrated to CTS verifier tests to ensure test coverage.
    private boolean mSupportNotificationPolicyAccess;
    private Context mContext;
    private final static int ASYNC_TIMING_TOLERANCE_MS = 50;
    private int mOriginalRingerMode;
    private Map<Integer, Integer> mOriginalStreamVolumes = new HashMap<>();
    private NotificationManager.Policy mOriginalNotificationPolicy;
    private int mOriginalZen;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        Utils.enableAppOps(mContext.getPackageName(), APPOPS_OP_STR, getInstrumentation());
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        Vibrator vibrator = (Vibrator) mContext.getSystemService(Context.VIBRATOR_SERVICE);
        mNm = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mHasVibrator = (vibrator != null) && vibrator.hasVibrator();
        mUseFixedVolume = mContext.getResources().getBoolean(
                Resources.getSystem().getIdentifier("config_useFixedVolume", "bool", "android"));
        PackageManager packageManager = mContext.getPackageManager();
        mIsTelevision = packageManager != null
                && (packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)
                        || packageManager.hasSystemFeature(PackageManager.FEATURE_TELEVISION));
        mIsSingleVolume = mContext.getResources().getBoolean(
                Resources.getSystem().getIdentifier("config_single_volume", "bool", "android"));
        mSkipRingerTests = mUseFixedVolume || mIsTelevision || mIsSingleVolume;
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        mSupportNotificationPolicyAccess = !am.isLowRamDevice();

        // Store the original volumes that that they can be recovered in tearDown().
        final int[] streamTypes = {
            AudioManager.STREAM_VOICE_CALL,
            AudioManager.STREAM_SYSTEM,
            AudioManager.STREAM_RING,
            AudioManager.STREAM_MUSIC,
            AudioManager.STREAM_ALARM,
            AudioManager.STREAM_NOTIFICATION,
            AudioManager.STREAM_DTMF,
            AudioManager.STREAM_ACCESSIBILITY,
        };
        mOriginalRingerMode = mAudioManager.getRingerMode();
        for (int streamType : streamTypes) {
            mOriginalStreamVolumes.put(streamType, mAudioManager.getStreamVolume(streamType));
        }

        if (mSupportNotificationPolicyAccess) {
            try {
                Utils.toggleNotificationPolicyAccess(
                        mContext.getPackageName(), getInstrumentation(), true);
                mOriginalNotificationPolicy = mNm.getNotificationPolicy();
                mOriginalZen = mNm.getCurrentInterruptionFilter();
            } finally {
                setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
                Utils.toggleNotificationPolicyAccess(
                        mContext.getPackageName(), getInstrumentation(), false);
            }
        }
    }

    @Override
    protected void tearDown() throws Exception {
        if (!mSupportNotificationPolicyAccess) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mNm.setNotificationPolicy(mOriginalNotificationPolicy);
            setInterruptionFilter(mOriginalZen);

            // Recover the volume and the ringer mode that the test may have overwritten.
            for (Map.Entry<Integer, Integer> e : mOriginalStreamVolumes.entrySet()) {
                mAudioManager.setStreamVolume(e.getKey(), e.getValue(),
                                              AudioManager.FLAG_ALLOW_RINGER_MODES);
            }
            mAudioManager.setRingerMode(mOriginalRingerMode);
        } finally {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);
        }
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testMicrophoneMute() throws Exception {
        mAudioManager.setMicrophoneMute(true);
        assertTrue(mAudioManager.isMicrophoneMute());
        mAudioManager.setMicrophoneMute(false);
        assertFalse(mAudioManager.isMicrophoneMute());
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testMicrophoneMuteIntent() throws Exception {
        final MyBlockingIntentReceiver receiver = new MyBlockingIntentReceiver();
        final boolean initialMicMute = mAudioManager.isMicrophoneMute();
        try {
            mContext.registerReceiver(receiver,
                    new IntentFilter(AudioManager.ACTION_MICROPHONE_MUTE_CHANGED));
            // change the mic mute state
            mAudioManager.setMicrophoneMute(!initialMicMute);
            // verify a change was reported
            final boolean intentFired = receiver.waitForMicMuteChanged(500/*ms*/);
            assertTrue("ACTION_MICROPHONE_MUTE_CHANGED wasn't fired", intentFired);
            // verify the mic mute state is expected
            final boolean newMicMute = mAudioManager.isMicrophoneMute();
            assertTrue("new mic mute state not as expected (" + !initialMicMute + ")",
                    newMicMute == !initialMicMute);
        } finally {
            mContext.unregisterReceiver(receiver);
            mAudioManager.setMicrophoneMute(initialMicMute);
        }
    }

    // helper class to simplify that abstracts out the handling of spurious wakeups in Object.wait()
    private static final class SafeWaitObject {
        private boolean mQuit = false;

        public void safeNotify() {
            synchronized (this) {
                mQuit = true;
                this.notify();
            }
        }

        public void safeWait(long millis) throws InterruptedException {
            final long timeOutTime = java.lang.System.currentTimeMillis() + millis;
            synchronized (this) {
                while (!mQuit) {
                    final long timeToWait = timeOutTime - java.lang.System.currentTimeMillis();
                    if (timeToWait < 0) { break; }
                    this.wait(timeToWait);
                }
            }
        }
    }

    private static final class MyBlockingIntentReceiver extends BroadcastReceiver {
        private final SafeWaitObject mLock = new SafeWaitObject();
        // state protected by mLock
        private boolean mIntentReceived = false;

        @Override
        public void onReceive(Context context, Intent intent) {
            synchronized (mLock) {
                mIntentReceived = true;
                mLock.safeNotify();
            }
        }

        public boolean waitForMicMuteChanged(long timeOutMs) {
            synchronized (mLock) {
                try {
                    mLock.safeWait(timeOutMs);
                } catch (InterruptedException e) { }
                return mIntentReceived;
            }
        }
    }

    public void testSoundEffects() throws Exception {
        Settings.System.putInt(mContext.getContentResolver(), SOUND_EFFECTS_ENABLED, 1);

        // should hear sound after loadSoundEffects() called.
        mAudioManager.loadSoundEffects();
        Thread.sleep(TIME_TO_PLAY);
        float volume = 13;
        mAudioManager.playSoundEffect(SoundEffectConstants.CLICK);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT);

        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT, volume);

        // won't hear sound after unloadSoundEffects() called();
        mAudioManager.unloadSoundEffects();
        mAudioManager.playSoundEffect(AudioManager.FX_KEY_CLICK);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT);

        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_UP, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_DOWN, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_LEFT, volume);
        mAudioManager.playSoundEffect(AudioManager.FX_FOCUS_NAVIGATION_RIGHT, volume);
    }

    public void testCheckingZenModeBlockDoesNotRequireNotificationPolicyAccess() throws Exception {
        if (!mSupportNotificationPolicyAccess) {
            return;
        }
        try {
            // set zen mode to priority only, so playSoundEffect will check notification policy
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    true);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            Settings.System.putInt(mContext.getContentResolver(), SOUND_EFFECTS_ENABLED, 1);

            // take away write-notification policy access from the package
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    false);

            // playSoundEffect should NOT throw a security exception; all apps have read-access
            mAudioManager.playSoundEffect(SoundEffectConstants.CLICK);
        } finally {
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    true);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    false);
        }
    }

    public void testMusicActive() throws Exception {
        if (mAudioManager.isMusicActive()) {
            return;
        }
        MediaPlayer mp = MediaPlayer.create(mContext, MP3_TO_PLAY);
        assertNotNull(mp);
        mp.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mp.start();
        Thread.sleep(TIME_TO_PLAY);
        assertTrue(mAudioManager.isMusicActive());
        Thread.sleep(TIME_TO_PLAY);
        mp.stop();
        mp.release();
        Thread.sleep(TIME_TO_PLAY);
        assertFalse(mAudioManager.isMusicActive());
    }

    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testAccessMode() throws Exception {
        mAudioManager.setMode(MODE_RINGTONE);
        assertEquals(MODE_RINGTONE, mAudioManager.getMode());
        mAudioManager.setMode(MODE_IN_COMMUNICATION);
        assertEquals(MODE_IN_COMMUNICATION, mAudioManager.getMode());
        mAudioManager.setMode(MODE_NORMAL);
        assertEquals(MODE_NORMAL, mAudioManager.getMode());
    }

    @SuppressWarnings("deprecation")
    @AppModeFull(reason = "Instant apps cannot hold android.permission.MODIFY_AUDIO_SETTINGS")
    public void testRouting() throws Exception {
        // setBluetoothA2dpOn is a no-op, and getRouting should always return -1
        // AudioManager.MODE_CURRENT
        boolean oldA2DP = mAudioManager.isBluetoothA2dpOn();
        mAudioManager.setBluetoothA2dpOn(true);
        assertEquals(oldA2DP , mAudioManager.isBluetoothA2dpOn());
        mAudioManager.setBluetoothA2dpOn(false);
        assertEquals(oldA2DP , mAudioManager.isBluetoothA2dpOn());

        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_RINGTONE));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_NORMAL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_CALL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_COMMUNICATION));

        mAudioManager.setBluetoothScoOn(true);
        assertTrue(mAudioManager.isBluetoothScoOn());
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_RINGTONE));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_NORMAL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_CALL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_COMMUNICATION));

        mAudioManager.setBluetoothScoOn(false);
        assertFalse(mAudioManager.isBluetoothScoOn());
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_RINGTONE));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_NORMAL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_CALL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_COMMUNICATION));

        mAudioManager.setSpeakerphoneOn(true);
        assertTrue(mAudioManager.isSpeakerphoneOn());
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_CALL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_COMMUNICATION));
        mAudioManager.setSpeakerphoneOn(false);
        assertFalse(mAudioManager.isSpeakerphoneOn());
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_CALL));
        assertEquals(AudioManager.MODE_CURRENT, mAudioManager.getRouting(MODE_IN_COMMUNICATION));
    }

    public void testVibrateNotification() throws Exception {
        if (mUseFixedVolume || !mHasVibrator || !mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        // VIBRATE_SETTING_ON
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        // VIBRATE_SETTING_OFF
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        // VIBRATE_SETTING_ONLY_SILENT
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION));

        // VIBRATE_TYPE_NOTIFICATION
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF, mAudioManager
                .getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION));
    }

    public void testVibrateRinger() throws Exception {
        if (mUseFixedVolume || !mHasVibrator || !mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        // VIBRATE_TYPE_RINGER
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        // VIBRATE_SETTING_OFF
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF, mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        // Note: as of Froyo, if VIBRATE_TYPE_RINGER is set to OFF, it will
        // not vibrate, even in RINGER_MODE_VIBRATE. This allows users to
        // disable the vibration for incoming calls only.
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        // VIBRATE_SETTING_ONLY_SILENT
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertFalse(mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                mAudioManager.getRingerMode());
        assertEquals(mHasVibrator, mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER));

        // VIBRATE_TYPE_NOTIFICATION
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ON : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
        assertEquals(VIBRATE_SETTING_OFF, mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
        mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
        assertEquals(mHasVibrator ? VIBRATE_SETTING_ONLY_SILENT : VIBRATE_SETTING_OFF,
                mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER));
    }

    public void testAccessRingMode() throws Exception {
        if (!mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());

        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        // AudioService#setRingerMode() has:
        // if (isTelevision) return;
        if (mSkipRingerTests) {
            assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
        } else {
            assertEquals(RINGER_MODE_SILENT, mAudioManager.getRingerMode());
        }

        mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
        if (mSkipRingerTests) {
            assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
        } else {
            assertEquals(mHasVibrator ? RINGER_MODE_VIBRATE : RINGER_MODE_SILENT,
                    mAudioManager.getRingerMode());
        }
    }

    public void testSetRingerModePolicyAccess() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        // Apps without policy access cannot change silent -> normal or silent -> vibrate.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        assertEquals(RINGER_MODE_SILENT, mAudioManager.getRingerMode());
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);

        try {
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {
        }

        try {
            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {
        }

        // Apps without policy access cannot change normal -> silent.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);

        try {
            mAudioManager.setRingerMode(RINGER_MODE_SILENT);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {
        }
        assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());

        if (mHasVibrator) {
            // Apps without policy access cannot change vibrate -> silent.
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            assertEquals(RINGER_MODE_VIBRATE, mAudioManager.getRingerMode());
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);

            try {
                mAudioManager.setRingerMode(RINGER_MODE_SILENT);
                fail("Apps without notification policy access cannot change ringer mode");
            } catch (SecurityException e) {
            }

            // Apps without policy access can change vibrate -> normal and vice versa.
            assertEquals(RINGER_MODE_VIBRATE, mAudioManager.getRingerMode());
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            assertEquals(RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            assertEquals(RINGER_MODE_VIBRATE, mAudioManager.getRingerMode());
        }
    }

    public void testVolumeDndAffectedStream() throws Exception {
        if (mHasVibrator || mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setStreamVolume(
                AudioManager.STREAM_SYSTEM, 7, AudioManager.FLAG_ALLOW_RINGER_MODES);
        mAudioManager.setRingerMode(AudioManager.RINGER_MODE_NORMAL);
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);
        // 7 to 0, fail.
        try {
            mAudioManager.setStreamVolume(
                    AudioManager.STREAM_SYSTEM, 0, AudioManager.FLAG_ALLOW_RINGER_MODES);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {}

        // 7 to 1: success
        mAudioManager.setStreamVolume(
                AudioManager.STREAM_SYSTEM, 1, AudioManager.FLAG_ALLOW_RINGER_MODES);
        assertEquals("setStreamVolume did not change volume",
                1, mAudioManager.getStreamVolume(AudioManager.STREAM_SYSTEM));

        // 0 to non-zero: fail.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(AudioManager.RINGER_MODE_SILENT);
        mAudioManager.setStreamVolume(
                AudioManager.STREAM_SYSTEM, 0, AudioManager.FLAG_ALLOW_RINGER_MODES);
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);

        try {
            mAudioManager.setStreamVolume(
                    AudioManager.STREAM_SYSTEM, 6, AudioManager.FLAG_ALLOW_RINGER_MODES);
            fail("Apps without notification policy access cannot change ringer mode");
        } catch (SecurityException e) {}
    }

    public void testVolume() throws Exception {
        if (!mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        int volume, volumeDelta;
        int[] streams = {AudioManager.STREAM_ALARM,
                AudioManager.STREAM_MUSIC,
                AudioManager.STREAM_VOICE_CALL,
                AudioManager.STREAM_RING};

        mAudioManager.adjustVolume(ADJUST_RAISE, 0);
        // adjusting volume is aynchronous, wait before other volume checks
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        mAudioManager.adjustSuggestedStreamVolume(
                ADJUST_LOWER, USE_DEFAULT_STREAM_TYPE, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        int maxMusicVolume = mAudioManager.getStreamMaxVolume(STREAM_MUSIC);

        for (int stream : streams) {
            // set ringer mode to back normal to not interfere with volume tests
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);

            int maxVolume = mAudioManager.getStreamMaxVolume(stream);
            int minVolume = mAudioManager.getStreamMinVolume(stream);

            // validate min
            assertTrue(String.format("minVolume(%d) must be >= 0", minVolume), minVolume >= 0);
            assertTrue(String.format("minVolume(%d) must be < maxVolume(%d)", minVolume,
                    maxVolume),
                    minVolume < maxVolume);

            mAudioManager.setStreamVolume(stream, 1, 0);
            if (mUseFixedVolume) {
                assertEquals(maxVolume, mAudioManager.getStreamVolume(stream));
                continue;
            }
            assertEquals(String.format("stream=%d", stream),
                    1, mAudioManager.getStreamVolume(stream));

            if (stream == AudioManager.STREAM_MUSIC && mAudioManager.isWiredHeadsetOn()) {
                // due to new regulations, music sent over a wired headset may be volume limited
                // until the user explicitly increases the limit, so we can't rely on being able
                // to set the volume to getStreamMaxVolume(). Instead, determine the current limit
                // by increasing the volume until it won't go any higher, then use that volume as
                // the maximum for the purposes of this test
                int curvol = 0;
                int prevvol = 0;
                do {
                    prevvol = curvol;
                    mAudioManager.adjustStreamVolume(stream, ADJUST_RAISE, 0);
                    curvol = mAudioManager.getStreamVolume(stream);
                } while (curvol != prevvol);
                maxVolume = maxMusicVolume = curvol;
            }
            mAudioManager.setStreamVolume(stream, maxVolume, 0);
            mAudioManager.adjustStreamVolume(stream, ADJUST_RAISE, 0);
            assertEquals(maxVolume, mAudioManager.getStreamVolume(stream));

            volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(stream));
            mAudioManager.adjustSuggestedStreamVolume(ADJUST_LOWER, stream, 0);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            assertEquals(maxVolume - volumeDelta, mAudioManager.getStreamVolume(stream));

            // volume lower
            mAudioManager.setStreamVolume(stream, maxVolume, 0);
            volume = mAudioManager.getStreamVolume(stream);
            while (volume > minVolume) {
                volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(stream));
                mAudioManager.adjustStreamVolume(stream, ADJUST_LOWER, 0);
                assertEquals(Math.max(0, volume - volumeDelta),
                        mAudioManager.getStreamVolume(stream));
                volume = mAudioManager.getStreamVolume(stream);
            }

            mAudioManager.adjustStreamVolume(stream, ADJUST_SAME, 0);

            // volume raise
            mAudioManager.setStreamVolume(stream, 1, 0);
            volume = mAudioManager.getStreamVolume(stream);
            while (volume < maxVolume) {
                volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(stream));
                mAudioManager.adjustStreamVolume(stream, ADJUST_RAISE, 0);
                assertEquals(Math.min(volume + volumeDelta, maxVolume),
                        mAudioManager.getStreamVolume(stream));
                volume = mAudioManager.getStreamVolume(stream);
            }

            // volume same
            mAudioManager.setStreamVolume(stream, maxVolume, 0);
            for (int k = 0; k < maxVolume; k++) {
                mAudioManager.adjustStreamVolume(stream, ADJUST_SAME, 0);
                assertEquals(maxVolume, mAudioManager.getStreamVolume(stream));
            }

            mAudioManager.setStreamVolume(stream, maxVolume, 0);
        }

        if (mUseFixedVolume) {
            return;
        }

        // adjust volume
        mAudioManager.adjustVolume(ADJUST_RAISE, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);

        boolean isMusicPlayingBeforeTest = false;
        if (mAudioManager.isMusicActive()) {
            isMusicPlayingBeforeTest = true;
        }

        MediaPlayer mp = MediaPlayer.create(mContext, MP3_TO_PLAY);
        assertNotNull(mp);
        mp.setAudioStreamType(STREAM_MUSIC);
        mp.setLooping(true);
        mp.start();
        Thread.sleep(TIME_TO_PLAY);
        assertTrue(mAudioManager.isMusicActive());

        // adjust volume as ADJUST_SAME
        for (int k = 0; k < maxMusicVolume; k++) {
            mAudioManager.adjustVolume(ADJUST_SAME, 0);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            assertEquals(maxMusicVolume, mAudioManager.getStreamVolume(STREAM_MUSIC));
        }

        // adjust volume as ADJUST_RAISE
        mAudioManager.setStreamVolume(STREAM_MUSIC, 0, 0);
        volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
        mAudioManager.adjustVolume(ADJUST_RAISE, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        assertEquals(Math.min(volumeDelta, maxMusicVolume),
                mAudioManager.getStreamVolume(STREAM_MUSIC));

        // adjust volume as ADJUST_LOWER
        mAudioManager.setStreamVolume(STREAM_MUSIC, maxMusicVolume, 0);
        maxMusicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
        volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
        mAudioManager.adjustVolume(ADJUST_LOWER, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        assertEquals(Math.max(0, maxMusicVolume - volumeDelta),
                mAudioManager.getStreamVolume(STREAM_MUSIC));

        mp.stop();
        mp.release();
        Thread.sleep(TIME_TO_PLAY);
        if (!isMusicPlayingBeforeTest) {
            assertFalse(mAudioManager.isMusicActive());
        }
    }

    public void testAccessibilityVolume() throws Exception {
        if (mUseFixedVolume) {
            Log.i("AudioManagerTest", "testAccessibilityVolume() skipped: fixed volume");
            return;
        }
        final int maxA11yVol = mAudioManager.getStreamMaxVolume(STREAM_ACCESSIBILITY);
        assertTrue("Max a11yVol not strictly positive", maxA11yVol > 0);
        int currentVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);

        // changing STREAM_ACCESSIBILITY is subject to permission, shouldn't be able to change it
        // test setStreamVolume
        final int testSetVol;
        if (currentVol != maxA11yVol) {
            testSetVol = maxA11yVol;
        } else {
            testSetVol = maxA11yVol - 1;
        }
        mAudioManager.setStreamVolume(STREAM_ACCESSIBILITY, testSetVol, 0);
        Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
        currentVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);
        assertTrue("Should not be able to change A11y vol", currentVol != testSetVol);

        // test adjustStreamVolume
        //        LOWER
        currentVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);
        if (currentVol > 0) {
            mAudioManager.adjustStreamVolume(STREAM_ACCESSIBILITY, ADJUST_LOWER, 0);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            int newVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);
            assertTrue("Should not be able to lower A11y vol", currentVol == newVol);
        }
        //        RAISE
        currentVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);
        if (currentVol < maxA11yVol) {
            mAudioManager.adjustStreamVolume(STREAM_ACCESSIBILITY, ADJUST_RAISE, 0);
            Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            int newVol = mAudioManager.getStreamVolume(STREAM_ACCESSIBILITY);
            assertTrue("Should not be able to raise A11y vol", currentVol == newVol);
        }
    }

    public void testSetVoiceCallVolumeToZeroPermission() {
        // Verify that only apps with MODIFY_PHONE_STATE can set VOICE_CALL_STREAM to 0
        mAudioManager.setStreamVolume(AudioManager.STREAM_VOICE_CALL, 0, 0);
        assertTrue("MODIFY_PHONE_STATE is required in order to set voice call volume to 0",
                    mAudioManager.getStreamVolume(AudioManager.STREAM_VOICE_CALL) != 0);
    }

    public void testMuteFixedVolume() throws Exception {
        int[] streams = {
                AudioManager.STREAM_VOICE_CALL,
                AudioManager.STREAM_MUSIC,
                AudioManager.STREAM_RING,
                AudioManager.STREAM_ALARM,
                AudioManager.STREAM_NOTIFICATION,
                AudioManager.STREAM_SYSTEM};
        if (mUseFixedVolume) {
            for (int stream : streams) {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                assertFalse("Muting should not affect a fixed volume device.",
                        mAudioManager.isStreamMute(stream));

                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
                assertFalse("Toggling mute should not affect a fixed volume device.",
                        mAudioManager.isStreamMute(stream));

                mAudioManager.setStreamMute(stream, true);
                assertFalse("Muting should not affect a fixed volume device.",
                        mAudioManager.isStreamMute(stream));
            }
        }
    }

    public void testMuteDndAffectedStreams() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        int[] streams = { AudioManager.STREAM_RING };
        // Mute streams
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_SILENT);
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);
        // Verify streams cannot be unmuted without policy access.
        for (int stream : streams) {
            try {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_UNMUTE, 0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_SILENT, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE,
                        0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_SILENT, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.setStreamMute(stream, false);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_SILENT, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }
        }

        // This ensures we're out of vibrate or silent modes.
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        for (int stream : streams) {
            // ensure each stream is on and turned up.
            mAudioManager.setStreamVolume(stream,
                    mAudioManager.getStreamMaxVolume(stream),
                    0);

            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), false);
            try {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }
            try {
                mAudioManager.adjustStreamVolume(
                        stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.setStreamMute(stream, true);
                assertEquals("Apps without Notification policy access can't change ringer mode",
                        RINGER_MODE_NORMAL, mAudioManager.getRingerMode());
            } catch (SecurityException e) {
            }
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            testStreamMuting(stream);
        }
    }

    public void testMuteDndUnaffectedStreams() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        int[] streams = {
                AudioManager.STREAM_VOICE_CALL,
                AudioManager.STREAM_MUSIC,
                AudioManager.STREAM_ALARM
        };

        int muteAffectedStreams = System.getInt(mContext.getContentResolver(),
                System.MUTE_STREAMS_AFFECTED,
                // same defaults as in AudioService. Should be kept in sync.
                 (1 << STREAM_MUSIC) |
                         (1 << AudioManager.STREAM_RING) |
                         (1 << AudioManager.STREAM_NOTIFICATION) |
                         (1 << AudioManager.STREAM_SYSTEM) |
                         (1 << AudioManager.STREAM_VOICE_CALL));

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        // This ensures we're out of vibrate or silent modes.
        mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), false);
        for (int stream : streams) {
            // ensure each stream is on and turned up.
            mAudioManager.setStreamVolume(stream,
                    mAudioManager.getStreamMaxVolume(stream),
                    0);
            if (((1 << stream) & muteAffectedStreams) == 0) {
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                assertFalse("Stream " + stream + " should not be affected by mute.",
                        mAudioManager.isStreamMute(stream));
                mAudioManager.setStreamMute(stream, true);
                assertFalse("Stream " + stream + " should not be affected by mute.",
                        mAudioManager.isStreamMute(stream));
                mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE,
                        0);
                assertFalse("Stream " + stream + " should not be affected by mute.",
                        mAudioManager.isStreamMute(stream));
                continue;
            }
            testStreamMuting(stream);
        }
    }

    private void testStreamMuting(int stream) {
        // Voice call requires MODIFY_PHONE_STATE, so we should not be able to mute
        if (stream == AudioManager.STREAM_VOICE_CALL) {
            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
            assertFalse("Muting voice call stream (" + stream + ") should require "
                            + "MODIFY_PHONE_STATE.", mAudioManager.isStreamMute(stream));
        } else {
            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
            assertTrue("Muting stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_UNMUTE, 0);
            assertFalse("Unmuting stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
            assertTrue("Toggling mute on stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_TOGGLE_MUTE, 0);
            assertFalse("Toggling mute on stream " + stream + " failed.",
                    mAudioManager.isStreamMute(stream));

            mAudioManager.setStreamMute(stream, true);
            assertTrue("Muting stream " + stream + " using setStreamMute failed",
                    mAudioManager.isStreamMute(stream));

            // mute it three more times to verify the ref counting is gone.
            mAudioManager.setStreamMute(stream, true);
            mAudioManager.setStreamMute(stream, true);
            mAudioManager.setStreamMute(stream, true);

            mAudioManager.setStreamMute(stream, false);
            assertFalse("Unmuting stream " + stream + " using setStreamMute failed.",
                    mAudioManager.isStreamMute(stream));
        }
    }

    public void testSetInvalidRingerMode() {
        int ringerMode = mAudioManager.getRingerMode();
        mAudioManager.setRingerMode(-1337);
        assertEquals(ringerMode, mAudioManager.getRingerMode());

        mAudioManager.setRingerMode(-3007);
        assertEquals(ringerMode, mAudioManager.getRingerMode());
    }

    public void testAdjustVolumeInTotalSilenceMode() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_NONE);

            int musicVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            mAudioManager.adjustStreamVolume(
                    AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, 0);
            assertEquals(musicVolume, mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));

        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testAdjustVolumeInAlarmsOnlyMode() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);

            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALARMS);
            int musicVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            mAudioManager.adjustStreamVolume(
                    AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, 0);
            int volumeDelta =
                    getVolumeDelta(mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            assertEquals(musicVolume + volumeDelta,
                    mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));

        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testSetStreamVolumeInTotalSilenceMode() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);

            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_NONE);

            int musicVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 7, 0);
            assertEquals(musicVolume, mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 7, 0);
            assertEquals(7, mAudioManager.getStreamVolume(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testSetStreamVolumeInAlarmsOnlyMode() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        try {
            Utils.toggleNotificationPolicyAccess(
                    mContext.getPackageName(), getInstrumentation(), true);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALARMS);

            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 3, 0);
            assertEquals(3, mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 7, 0);
            assertEquals(7, mAudioManager.getStreamVolume(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testSetStreamVolumeInPriorityOnlyMode() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);

        try {
            // turn off zen, set stream volumes to check for later
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);

            final int testRingerVol = getTestRingerVol();
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            int musicVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            int alarmVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_ALARM);

            // disallow all sounds in priority only, turn on priority only DND, try to change volume
            mNm.setNotificationPolicy(new NotificationManager.Policy(0, 0 , 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            Thread.sleep(MUTE_DELAY); // delay for streams to get into correct mute states
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 3, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 5, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, testRingerVol, 0);

            // Turn off zen and make sure stream levels are still the same prior to zen
            // aside from ringer since ringer can exit dnd
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            assertEquals(musicVolume, mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            assertEquals(alarmVolume, mAudioManager.getStreamVolume(AudioManager.STREAM_ALARM));
            assertEquals(testRingerVol, mAudioManager.getStreamVolume(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testAdjustVolumeInPriorityOnly() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // turn off zen, set stream volumes to check for later
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            int ringVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_RING);
            int musicVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            int alarmVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_ALARM);

            // disallow all sounds in priority only, turn on priority only DND, try to change volume
            mNm.setNotificationPolicy(new NotificationManager.Policy(0, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);
            Thread.sleep(MUTE_DELAY); // delay for streams to be get into correct mute states
            mAudioManager.adjustStreamVolume(
                    AudioManager.STREAM_RING, AudioManager.ADJUST_RAISE, 0);
            mAudioManager.adjustStreamVolume(
                    AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, 0);
            mAudioManager.adjustStreamVolume(
                    AudioManager.STREAM_ALARM, AudioManager.ADJUST_RAISE, 0);

            // Turn off zen and make sure stream levels are still the same prior to zen
            // aside from ringer since ringer can exit dnd
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            assertEquals(musicVolume, mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            assertEquals(alarmVolume, mAudioManager.getStreamVolume(AudioManager.STREAM_ALARM));

            int volumeDelta =
                    getVolumeDelta(mAudioManager.getStreamVolume(AudioManager.STREAM_RING));
            assertEquals(ringVolume + volumeDelta,
                    mAudioManager.getStreamVolume(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyMuteAll() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);

            // disallow all sounds in priority only, turn on priority only DND
            mNm.setNotificationPolicy(new NotificationManager.Policy(0, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            Thread.sleep(MUTE_DELAY); // delay for streams to be get into correct mute states
            assertTrue("Music (media) stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
            assertTrue("System stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_SYSTEM));
            assertTrue("Alarm stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_ALARM));

            // Test requires that the phone's default state has no channels that can bypass dnd
            assertTrue("Ringer stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyMediaAllowed() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }
        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);

            // allow only media in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_MEDIA, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            Thread.sleep(MUTE_DELAY); // delay for streams to be get into correct mute states
            assertFalse("Music (media) stream should not be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
            assertTrue("System stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_SYSTEM));
            assertTrue("Alarm stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_ALARM));

            // Test requires that the phone's default state has no channels that can bypass dnd
            assertTrue("Ringer stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlySystemAllowed() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);

            // allow only system in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            Thread.sleep(MUTE_DELAY); // delay for streams to be get into correct mute states
            assertTrue("Music (media) stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
            assertFalse("System stream should not be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_SYSTEM));
            assertTrue("Alarm stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_ALARM));

            // Test requires that the phone's default state has no channels that can bypass dnd
            assertTrue("Ringer stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyAlarmsAllowed() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);

            // allow only alarms in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_ALARMS, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            Thread.sleep(MUTE_DELAY); // delay for streams to be get into correct mute states
            assertTrue("Music (media) stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
            assertTrue("System stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_SYSTEM));
            assertFalse("Alarm stream should not be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_ALARM));

            // Test requires that the phone's default state has no channels that can bypass dnd
            assertTrue("Ringer stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_RING));
        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyRingerAllowed() throws Exception {
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);

            // allow only reminders in priority only
            mNm.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_REMINDERS, 0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            Thread.sleep(MUTE_DELAY); // delay for streams to be muted
            assertTrue("Music (media) stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
            assertTrue("System stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_SYSTEM));
            assertTrue("Alarm stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_ALARM));
            assertFalse("Ringer stream should not be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_RING));

        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
        }
    }

    public void testPriorityOnlyChannelsCanBypassDnd() throws Exception {
        final String NOTIFICATION_CHANNEL_ID = "test_id";
        if (mSkipRingerTests || !mSupportNotificationPolicyAccess) {
            return;
        }

        Utils.toggleNotificationPolicyAccess(
                mContext.getPackageName(), getInstrumentation(), true);
        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, "TEST",
                NotificationManager.IMPORTANCE_DEFAULT);
        try {
            // ensure volume is not muted/0 to start test
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_SYSTEM, 1, 0);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, 1, 0);

            // create a channel that can bypass dnd
            channel.setBypassDnd(true);
            mNm.createNotificationChannel(channel);

            // allow nothing
            mNm.setNotificationPolicy(new NotificationManager.Policy(0,0, 0));
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_PRIORITY);

            Thread.sleep(MUTE_DELAY); // delay for streams to be muted
            assertTrue("Music (media) stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC));
            assertTrue("System stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_SYSTEM));
            assertTrue("Alarm stream should be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_ALARM));
            assertFalse("Ringer stream should not be muted",
                    mAudioManager.isStreamMute(AudioManager.STREAM_RING));

        } finally {
            setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
            mNm.deleteNotificationChannel(NOTIFICATION_CHANNEL_ID);
            Utils.toggleNotificationPolicyAccess(mContext.getPackageName(), getInstrumentation(),
                    false);

        }
    }

    public void testAdjustVolumeWithIllegalDirection() throws Exception {
        // Call the method with illegal direction. System should not reboot.
        mAudioManager.adjustVolume(37, 0);
    }

    private final int[] PUBLIC_STREAM_TYPES = { AudioManager.STREAM_VOICE_CALL,
            AudioManager.STREAM_SYSTEM, AudioManager.STREAM_RING, AudioManager.STREAM_MUSIC,
            AudioManager.STREAM_ALARM, AudioManager.STREAM_NOTIFICATION,
            AudioManager.STREAM_DTMF,  AudioManager.STREAM_ACCESSIBILITY };

    public void testGetStreamVolumeDbWithIllegalArguments() throws Exception {
        Exception ex = null;
        // invalid stream type
        try {
            float gain = mAudioManager.getStreamVolumeDb(-100 /*streamType*/, 0,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid stream type", ex);
        assertEquals("Wrong exception thrown for invalid stream type",
                ex.getClass(), IllegalArgumentException.class);

        // invalid volume index
        ex = null;
        try {
            float gain = mAudioManager.getStreamVolumeDb(AudioManager.STREAM_MUSIC, -101 /*volume*/,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid volume index", ex);
        assertEquals("Wrong exception thrown for invalid volume index",
                ex.getClass(), IllegalArgumentException.class);

        // invalid out of range volume index
        ex = null;
        try {
            final int maxVol = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            float gain = mAudioManager.getStreamVolumeDb(AudioManager.STREAM_MUSIC, maxVol + 1,
                    AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid out of range volume index", ex);
        assertEquals("Wrong exception thrown for invalid out of range volume index",
                ex.getClass(), IllegalArgumentException.class);

        // invalid device type
        ex = null;
        try {
            float gain = mAudioManager.getStreamVolumeDb(AudioManager.STREAM_MUSIC, 0,
                    -102 /*deviceType*/);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid device type", ex);
        assertEquals("Wrong exception thrown for invalid device type",
                ex.getClass(), IllegalArgumentException.class);

        // invalid input device type
        ex = null;
        try {
            float gain = mAudioManager.getStreamVolumeDb(AudioManager.STREAM_MUSIC, 0,
                    AudioDeviceInfo.TYPE_BUILTIN_MIC);
        } catch (Exception e) {
            ex = e; // expected
        }
        assertNotNull("No exception was thrown for an invalid input device type", ex);
        assertEquals("Wrong exception thrown for invalid input device type",
                ex.getClass(), IllegalArgumentException.class);
    }

    public void testGetStreamVolumeDb() throws Exception {
        for (int streamType : PUBLIC_STREAM_TYPES) {
            // verify mininum index is strictly inferior to maximum index
            final int minIndex = mAudioManager.getStreamMinVolume(streamType);
            final int maxIndex = mAudioManager.getStreamMaxVolume(streamType);
            assertTrue("Min vol index (" + minIndex + ") for stream " + streamType + " not inferior"
                    + " to max vol index (" + maxIndex + ")", minIndex <= maxIndex);
            float prevGain = Float.NEGATIVE_INFINITY;
            // verify gain increases with the volume indices
            for (int idx = minIndex ; idx <= maxIndex ; idx++) {
                float gain = mAudioManager.getStreamVolumeDb(streamType, idx,
                        AudioDeviceInfo.TYPE_BUILTIN_SPEAKER);
                assertTrue("Non-monotonically increasing gain at index " + idx + " for stream"
                        + streamType, prevGain <= gain);
                prevGain = gain;
            }
        }
    }

    public void testAdjustSuggestedStreamVolumeWithIllegalArguments() throws Exception {
        // Call the method with illegal direction. System should not reboot.
        mAudioManager.adjustSuggestedStreamVolume(37, AudioManager.STREAM_MUSIC, 0);

        // Call the method with illegal stream. System should not reboot.
        mAudioManager.adjustSuggestedStreamVolume(AudioManager.ADJUST_RAISE, 66747, 0);
    }

    public void testGetMicrophones() throws Exception {
        if (!mContext.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_MICROPHONE)) {
            return;
        }
        List<MicrophoneInfo> microphones = mAudioManager.getMicrophones();
        assertTrue(microphones.size() > 0);
        for (int i = 0; i < microphones.size(); i++) {
            MicrophoneInfo microphone = microphones.get(i);
            Log.i(TAG, "deviceId:" + microphone.getDescription());
            Log.i(TAG, "portId:" + microphone.getId());
            Log.i(TAG, "type:" + microphone.getType());
            Log.i(TAG, "address:" + microphone.getAddress());
            Log.i(TAG, "deviceLocation:" + microphone.getLocation());
            Log.i(TAG, "deviceGroup:" + microphone.getGroup()
                    + " index:" + microphone.getIndexInTheGroup());
            MicrophoneInfo.Coordinate3F position = microphone.getPosition();
            Log.i(TAG, "position:" + position.x + " " + position.y + " " + position.z);
            MicrophoneInfo.Coordinate3F orientation = microphone.getOrientation();
            Log.i(TAG, "orientation:" + orientation.x + " "
                    + orientation.y + " " + orientation.z);
            Log.i(TAG, "frequencyResponse:" + microphone.getFrequencyResponse());
            Log.i(TAG, "channelMapping:" + microphone.getChannelMapping());
            Log.i(TAG, "sensitivity:" + microphone.getSensitivity());
            Log.i(TAG, "max spl:" + microphone.getMaxSpl());
            Log.i(TAG, "min spl:" + microphone.getMinSpl());
            Log.i(TAG, "directionality:" + microphone.getDirectionality());
            Log.i(TAG, "--------------");
        }
    }

    private void setInterruptionFilter(int filter) throws Exception {
        mNm.setInterruptionFilter(filter);
        for (int i = 0; i < 5; i++) {
            if (mNm.getCurrentInterruptionFilter() == filter) {
                break;
            }
            Thread.sleep(1000);
        }
    }

    private int getVolumeDelta(int volume) {
        return 1;
    }

    private int getTestRingerVol() {
        final int currentRingVol = mAudioManager.getStreamVolume(STREAM_RING);
        final int maxRingVol = mAudioManager.getStreamMaxVolume(STREAM_RING);
        if (currentRingVol != maxRingVol) {
            return maxRingVol;
        } else {
            return maxRingVol - 1;
        }
    }

}
