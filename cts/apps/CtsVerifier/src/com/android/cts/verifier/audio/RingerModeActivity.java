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

package com.android.cts.verifier.audio;


import static android.media.AudioManager.ADJUST_LOWER;
import static android.media.AudioManager.ADJUST_RAISE;
import static android.media.AudioManager.ADJUST_SAME;
import static android.media.AudioManager.RINGER_MODE_NORMAL;
import static android.media.AudioManager.RINGER_MODE_SILENT;
import static android.media.AudioManager.RINGER_MODE_VIBRATE;
import static android.media.AudioManager.STREAM_MUSIC;
import static android.media.AudioManager.USE_DEFAULT_STREAM_TYPE;
import static android.media.AudioManager.VIBRATE_SETTING_OFF;
import static android.media.AudioManager.VIBRATE_SETTING_ON;
import static android.media.AudioManager.VIBRATE_SETTING_ONLY_SILENT;
import static android.media.AudioManager.VIBRATE_TYPE_NOTIFICATION;
import static android.media.AudioManager.VIBRATE_TYPE_RINGER;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Vibrator;
import android.provider.Settings;
import android.util.Log;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.ViewGroup;

import com.android.cts.verifier.R;
import com.android.cts.verifier.notifications.InteractiveVerifierActivity;

import java.util.ArrayList;
import java.util.List;

/**
 * Contains tests that require {@link AudioManager#getRingerMode()} to be in a particular starting
 * state. Only runs on {@link ActivityManager#isLowRamDevice()} devices that don't support
 * {@link android.service.notification.ConditionProviderService}. Otherwise the tests live in
 * AudioManagerTests in non-verifier cts.
 */
public class RingerModeActivity extends InteractiveVerifierActivity {
    private static final String TAG = "RingerModeActivity";

    private final static String PKG = "com.android.cts.verifier";
    private final static long TIME_TO_PLAY = 2000;
    private final static int MP3_TO_PLAY = R.raw.testmp3;
    private final static int ASYNC_TIMING_TOLERANCE_MS = 50;

    private AudioManager mAudioManager;
    private boolean mHasVibrator;
    private boolean mUseFixedVolume;
    private boolean mIsTelevision;
    private boolean mIsSingleVolume;
    private boolean mSkipRingerTests;

    @Override
    protected int getTitleResource() {
        return R.string.ringer_mode_tests;
    }

    @Override
    protected int getInstructionsResource() {
        return R.string.ringer_mode_info;
    }

    protected void onCreate(Bundle savedState) {
        super.onCreate(savedState);
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        Vibrator vibrator = (Vibrator) mContext.getSystemService(Context.VIBRATOR_SERVICE);
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
    }

    // Test Setup

    @Override
    protected List<InteractiveTestCase> createTestItems() {
        List<InteractiveTestCase> tests = new ArrayList<>();
        if (supportsConditionProviders()) {
            tests.add(new PassTest());
            return tests;
        }
        tests.add(new SetModeAllTest());
        tests.add(new SetModePriorityTest());
        tests.add(new TestAccessRingerModeDndOn());
        tests.add(new TestVibrateNotificationDndOn());
        tests.add(new TestVibrateRingerDndOn());
        tests.add(new TestSetRingerModePolicyAccessDndOn());
        tests.add(new TestVolumeDndAffectedStreamDndOn());
        tests.add(new TestAdjustVolumeInPriorityOnlyAllowAlarmsMediaMode());

        tests.add(new SetModeAllTest());
        tests.add(new TestAccessRingerMode());
        tests.add(new TestVibrateNotification());
        tests.add(new TestVibrateRinger());
        tests.add(new TestSetRingerModePolicyAccess());
        tests.add(new TestVolumeDndAffectedStream());
        tests.add(new TestVolume());
        tests.add(new TestMuteStreams());
        tests.add(new EnableSoundEffects());
        tests.add(new TestSoundEffects());
        return tests;
    }

    private boolean supportsConditionProviders() {
        ActivityManager am = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        return !am.isLowRamDevice()
                || mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH);
    }

    private int getVolumeDelta(int volume) {
        return 1;
    }

    private boolean hasAudioOutput() {
        return mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT);
    }

    private void testStreamMuting(int stream) {
        if (stream == AudioManager.STREAM_VOICE_CALL) {
            // Voice call requires MODIFY_PHONE_STATE, so we should not be able to mute
            mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
            assertFalse("Muting stream " + stream + " should require MODIFY_PHONE_STATE permission.",
                    mAudioManager.isStreamMute(stream));
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

    // Tests

    protected class PassTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createRetryItem(parent, R.string.ringer_mode_pass_test);
        }

        @Override
        protected void test() {
           status = PASS;
        }
    }


    protected class SetModeAllTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createRetryItem(parent, R.string.attention_filter_all);
        }

        @Override
        protected void test() {
            if (mUserVerified) {
                status = PASS;
            } else {
                status = WAIT_FOR_USER;
            }
        }

        @Override
        protected void tearDown() {
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            delay();
        }
    }

    protected class SetModePriorityTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createRetryItem(parent, R.string.attention_filter_priority_mimic_alarms_only);
        }

        @Override
        protected void test() {
            if (mUserVerified) {
                status = PASS;
            } else {
                status = WAIT_FOR_USER;
            }
        }

        @Override
        protected void tearDown() {
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
            delay();
        }
    }

    protected class EnableSoundEffects extends InteractiveTestCase {

        @Override
        protected View inflate(ViewGroup parent) {
            return createRetryItem(parent, R.string.enable_sound_effects);
        }

        @Override
        protected void test() {
            if (mUserVerified) {
                status = PASS;
            } else {
                status = WAIT_FOR_USER;
            }
        }

        @Override
        protected void tearDown() {
            delay();
        }
    }

    protected class TestSoundEffects extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_sound_effects);
        }

        @Override
        protected void test() {
            // should hear sound after loadSoundEffects() called.
            mAudioManager.loadSoundEffects();
            try {
                Thread.sleep(TIME_TO_PLAY);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
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
            status = PASS;
        }
    }

    protected class TestVibrateNotificationDndOn extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_vibrate_notification);
        }

        @Override
        protected void test() {
            if (mUseFixedVolume || !mHasVibrator) {
                status = PASS;
                return;
            }

            // VIBRATE_SETTING_ON
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
            if (VIBRATE_SETTING_ON != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_OFF
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
            if (VIBRATE_SETTING_OFF != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_ONLY_SILENT
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
            if (VIBRATE_SETTING_ONLY_SILENT
                    != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            status = PASS;
        }
    }

    protected class TestVibrateNotification extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_vibrate_notification);
        }

        @Override
        protected void test() {
            if (mUseFixedVolume || !mHasVibrator) {
                status = PASS;
                return;
            }
            // VIBRATE_SETTING_ON
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
            if (VIBRATE_SETTING_ON != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            if (!mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            if (!mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_OFF
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
            if (VIBRATE_SETTING_OFF != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_ONLY_SILENT
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
            if (VIBRATE_SETTING_ONLY_SILENT
                    != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            if (!mAudioManager.shouldVibrate(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }

            // VIBRATE_TYPE_NOTIFICATION
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ON);
            if(VIBRATE_SETTING_ON  != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_OFF);
            if(VIBRATE_SETTING_OFF != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_NOTIFICATION, VIBRATE_SETTING_ONLY_SILENT);
            if (VIBRATE_SETTING_ONLY_SILENT
                    != mAudioManager.getVibrateSetting(VIBRATE_TYPE_NOTIFICATION)) {
                setFailed();
                return;
            }
            status = PASS;
        }
    }

    protected class TestVibrateRingerDndOn extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_vibrate_ringer);
        }

        @Override
        protected void test() {
            if (mUseFixedVolume || !mHasVibrator) {
                status = PASS;
                return;
            }

            // VIBRATE_SETTING_ON
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
            if (VIBRATE_SETTING_ON != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_OFF
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
            if (VIBRATE_SETTING_OFF != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_ONLY_SILENT
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
            if (VIBRATE_SETTING_ONLY_SILENT
                    != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            status = PASS;
        }
    }

    protected class TestVibrateRinger extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_vibrate_ringer);
        }

        @Override
        protected void test() {
            if (mUseFixedVolume || !mHasVibrator) {
                status = PASS;
                return;
            }
            // VIBRATE_SETTING_ON
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
            if (VIBRATE_SETTING_ON != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            if (!mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            if (!mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_OFF
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
            if (VIBRATE_SETTING_OFF != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            // VIBRATE_SETTING_ONLY_SILENT
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
            if (VIBRATE_SETTING_ONLY_SILENT
                    != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            if (mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            if (!mAudioManager.shouldVibrate(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }

            // VIBRATE_TYPE_NOTIFICATION
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ON);
            if (VIBRATE_SETTING_ON != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_OFF);
            if (VIBRATE_SETTING_OFF != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            mAudioManager.setVibrateSetting(VIBRATE_TYPE_RINGER, VIBRATE_SETTING_ONLY_SILENT);
            if (VIBRATE_SETTING_ONLY_SILENT
                    != mAudioManager.getVibrateSetting(VIBRATE_TYPE_RINGER)) {
                setFailed();
                return;
            }
            status = PASS;
        }
    }

    protected class TestAccessRingerMode extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_access_ringer_mode);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                status = PASS;
                return;
            }
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            if (RINGER_MODE_NORMAL != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }

            mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
            if (mHasVibrator) {
                if (RINGER_MODE_VIBRATE != mAudioManager.getRingerMode()) {
                    setFailed();
                    return;
                }
            } else {
                if (RINGER_MODE_NORMAL != mAudioManager.getRingerMode()) {
                    setFailed();
                    return;
                }
            }
            status = PASS;
        }

        @Override
        protected void tearDown() {
            mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
            next();
        }
    }

    protected class TestAccessRingerModeDndOn extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_access_ringer_mode);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                status = PASS;
                return;
            }
            if (RINGER_MODE_SILENT != mAudioManager.getRingerMode()) {
                setFailed();
                return;
            }
            status = PASS;
        }
    }

    protected class TestSetRingerModePolicyAccessDndOn extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_ringer_mode_policy_access);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                status = PASS;
                return;
            }
            // This tests a subset of testSetRingerModePolicyAccess as cts verifier cannot
            // get policy access on these devices.
            try {
                mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
                status = FAIL;
                logFail("Apps without notification policy access cannot change ringer mode");
            } catch (SecurityException e) {
            }

            try {
                mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);
                status = FAIL;
                logFail("Apps without notification policy access cannot change ringer mode");
            } catch (SecurityException e) {
            }
            status = PASS;
        }
    }

    protected class TestSetRingerModePolicyAccess extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_ringer_mode_policy_access);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                status = PASS;
                return;
            }
            // This tests a subset of testSetRingerModePolicyAccess as cts verifier cannot
            // get policy access on these devices.

            try {
                mAudioManager.setRingerMode(RINGER_MODE_SILENT);
                status = FAIL;
                logFail("Apps without notification policy access cannot change ringer mode");
            } catch (SecurityException e) {
            }

            if (mHasVibrator) {
                mAudioManager.setRingerMode(RINGER_MODE_VIBRATE);

                try {
                    mAudioManager.setRingerMode(RINGER_MODE_SILENT);
                    status = FAIL;
                    logFail("Apps without notification policy access cannot change ringer mode");
                } catch (SecurityException e) {
                }
            }
            status = PASS;
        }
    }

    protected class TestVolumeDndAffectedStream extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_volume_dnd_affected_stream);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests || mHasVibrator) {
                status = PASS;
                return;
            }

            mAudioManager.setStreamVolume(
                    AudioManager.STREAM_SYSTEM, 7, AudioManager.FLAG_ALLOW_RINGER_MODES);

            // 7 to 0, fail.
            try {
                mAudioManager.setStreamVolume(
                        AudioManager.STREAM_SYSTEM, 0, AudioManager.FLAG_ALLOW_RINGER_MODES);
                status = FAIL;
                logFail("Apps without notification policy access cannot change ringer mode");
                return;
            } catch (SecurityException e) {}

            // 7 to 1: success
            mAudioManager.setStreamVolume(
                    AudioManager.STREAM_SYSTEM, 1, AudioManager.FLAG_ALLOW_RINGER_MODES);
            if (1 !=  mAudioManager.getStreamVolume(AudioManager.STREAM_SYSTEM)) {
                setFailed();
                return;
            }
            status = PASS;
        }
    }

    protected class TestVolumeDndAffectedStreamDndOn extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_volume_dnd_affected_stream);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests || mHasVibrator) {
                status = PASS;
                return;
            }

            mAudioManager.setStreamVolume(
                    AudioManager.STREAM_SYSTEM, 0, AudioManager.FLAG_ALLOW_RINGER_MODES);

            try {
                mAudioManager.setStreamVolume(
                        AudioManager.STREAM_SYSTEM, 6, AudioManager.FLAG_ALLOW_RINGER_MODES);
                status = FAIL;
                logFail("Apps without notification policy access cannot change ringer mode");
                return;
            } catch (SecurityException e) {}
            status = PASS;
        }
    }

    protected class TestVolume extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_volume_dnd_affected_stream);
        }

        @Override
        protected void test() {
            int volume, volumeDelta;
            int[] streams = {STREAM_MUSIC,
                    AudioManager.STREAM_VOICE_CALL,
                    AudioManager.STREAM_ALARM,
                    AudioManager.STREAM_RING};

            mAudioManager.adjustVolume(ADJUST_RAISE, 0);
            // adjusting volume is aynchronous, wait before other volume checks
            try {
                Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            mAudioManager.adjustSuggestedStreamVolume(
                    ADJUST_LOWER, USE_DEFAULT_STREAM_TYPE, 0);
            try {
                Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            int maxMusicVolume = mAudioManager.getStreamMaxVolume(STREAM_MUSIC);

            for (int stream : streams) {
                Log.d(TAG, "testing stream: " + stream);
                mAudioManager.setRingerMode(RINGER_MODE_NORMAL);
                int maxVolume = mAudioManager.getStreamMaxVolume(stream);
                int minVolume = 1; // mAudioManager.getStreamMinVolume(stream); is @hide

                // validate min
                assertTrue(String.format("minVolume(%d) must be >= 0", minVolume), minVolume >= 0);
                assertTrue(String.format("minVolume(%d) must be < maxVolume(%d)", minVolume,
                        maxVolume),
                        minVolume < maxVolume);

                mAudioManager.setStreamVolume(stream, 1, 0);
                if (mUseFixedVolume) {
                    assertEquals(maxVolume, mAudioManager.getStreamVolume(stream));
                    return;
                }
                assertEquals(String.format("stream=%d", stream),
                        1, mAudioManager.getStreamVolume(stream));

                if (stream == STREAM_MUSIC && mAudioManager.isWiredHeadsetOn()) {
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
                try {
                    Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
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
            try {
                Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            boolean isMusicPlayingBeforeTest = false;
            if (mAudioManager.isMusicActive()) {
                isMusicPlayingBeforeTest = true;
            }

            MediaPlayer mp = MediaPlayer.create(mContext, MP3_TO_PLAY);
            assertNotNull(mp);
            mp.setAudioStreamType(STREAM_MUSIC);
            mp.setLooping(true);
            mp.start();
            try {
                Thread.sleep(TIME_TO_PLAY);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            assertTrue(mAudioManager.isMusicActive());

            // adjust volume as ADJUST_SAME
            for (int k = 0; k < maxMusicVolume; k++) {
                mAudioManager.adjustVolume(ADJUST_SAME, 0);
                try {
                    Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                assertEquals(maxMusicVolume, mAudioManager.getStreamVolume(STREAM_MUSIC));
            }

            // adjust volume as ADJUST_RAISE
            mAudioManager.setStreamVolume(STREAM_MUSIC, 0, 0);
            volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
            mAudioManager.adjustVolume(ADJUST_RAISE, 0);
            try {
                Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            assertEquals(Math.min(volumeDelta, maxMusicVolume),
                    mAudioManager.getStreamVolume(STREAM_MUSIC));

            // adjust volume as ADJUST_LOWER
            mAudioManager.setStreamVolume(STREAM_MUSIC, maxMusicVolume, 0);
            maxMusicVolume = mAudioManager.getStreamVolume(STREAM_MUSIC);
            volumeDelta = getVolumeDelta(mAudioManager.getStreamVolume(STREAM_MUSIC));
            mAudioManager.adjustVolume(ADJUST_LOWER, 0);
            try {
                Thread.sleep(ASYNC_TIMING_TOLERANCE_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            assertEquals(Math.max(0, maxMusicVolume - volumeDelta),
                    mAudioManager.getStreamVolume(STREAM_MUSIC));

            mp.stop();
            mp.release();
            try {
                Thread.sleep(TIME_TO_PLAY);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            if (!isMusicPlayingBeforeTest) {
                assertFalse(mAudioManager.isMusicActive());
            }
            status = PASS;
        }
    }

    protected class TestMuteDndAffectedStreamsDndOn extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_mute_dnd_affected_streams);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                return;
            }
            int[] streams = { AudioManager.STREAM_RING };

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
            status = PASS;
        }
    }

    protected class TestMuteStreams extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_mute_dnd_affected_streams);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                return;
            }
            int[] streams = { AudioManager.STREAM_RING };

            for (int stream : streams) {
                // ensure each stream is on and turned up.
                mAudioManager.setStreamVolume(stream, mAudioManager.getStreamMaxVolume(stream),0);

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
                testStreamMuting(stream);
            }

            streams = new int[] {
                    AudioManager.STREAM_VOICE_CALL,
                    AudioManager.STREAM_MUSIC,
                    AudioManager.STREAM_ALARM
            };

            int muteAffectedStreams = Settings.System.getInt(mContext.getContentResolver(),
                    Settings.System.MUTE_STREAMS_AFFECTED,
                    // same defaults as in AudioService. Should be kept in sync.
                    (1 << STREAM_MUSIC) |
                            (1 << AudioManager.STREAM_RING) |
                            (1 << AudioManager.STREAM_NOTIFICATION) |
                            (1 << AudioManager.STREAM_SYSTEM) |
                            (1 << AudioManager.STREAM_VOICE_CALL));

            for (int stream : streams) {
                // ensure each stream is on and turned up.
                mAudioManager.setStreamVolume(stream, mAudioManager.getStreamMaxVolume(stream), 0);
                if (((1 << stream) & muteAffectedStreams) == 0) {
                    if (stream == AudioManager.STREAM_VOICE_CALL) {
                        // Voice call requires MODIFY_PHONE_STATE, so we should not be able to mute
                        mAudioManager.adjustStreamVolume(stream, AudioManager.ADJUST_MUTE, 0);
                        assertTrue("Voice call stream (" + stream + ") should require MODIFY_PHONE_STATE "
                                + "to mute.", mAudioManager.isStreamMute(stream));
                    } else {
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
                }
                testStreamMuting(stream);
            }
            status = PASS;
        }
    }

    protected class TestAdjustVolumeInPriorityOnlyAllowAlarmsMediaMode extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.test_volume_dnd_affected_stream);
        }

        @Override
        protected void test() {
            if (mSkipRingerTests) {
                return;
            }
            int musicVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            mAudioManager.adjustStreamVolume(
                    AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, 0);
            int volumeDelta =
                    getVolumeDelta(mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            assertEquals(musicVolume + volumeDelta,
                    mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));

            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, 3, 0);
            assertEquals(3, mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));

            status = PASS;
        }
    }
}