/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.app.ActivityManager;
import android.app.UiAutomation;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.platform.test.annotations.AppModeFull;
import android.provider.Settings;
import android.test.InstrumentationTestCase;
import android.util.Log;

@AppModeFull(reason = "TODO: evaluate and port to instant")
public class RingtoneTest extends InstrumentationTestCase {
    private static final String TAG = "RingtoneTest";

    private Context mContext;
    private Ringtone mRingtone;
    private AudioManager mAudioManager;
    private int mOriginalVolume;
    private int mOriginalRingerMode;
    private int mOriginalStreamType;
    private Uri mDefaultRingUri;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        enableAppOps();
        mContext = getInstrumentation().getContext();
        mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        mRingtone = RingtoneManager.getRingtone(mContext, Settings.System.DEFAULT_RINGTONE_URI);
        // backup ringer settings
        mOriginalRingerMode = mAudioManager.getRingerMode();
        mOriginalVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_RING);
        mOriginalStreamType = mRingtone.getStreamType();

        int maxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_RING);

        if (mAudioManager.getRingerMode() == AudioManager.RINGER_MODE_VIBRATE) {
            mAudioManager.setRingerMode(AudioManager.RINGER_MODE_NORMAL);
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, maxVolume / 2,
                    AudioManager.FLAG_ALLOW_RINGER_MODES);
        } else if (mAudioManager.getRingerMode() == AudioManager.RINGER_MODE_NORMAL) {
            mAudioManager.setStreamVolume(AudioManager.STREAM_RING, maxVolume / 2,
                    AudioManager.FLAG_ALLOW_RINGER_MODES);
        } else if (!ActivityManager.isLowRamDeviceStatic()) {
            try {
                Utils.toggleNotificationPolicyAccess(
                        mContext.getPackageName(), getInstrumentation(), true);
                // set ringer to a reasonable volume
                mAudioManager.setStreamVolume(AudioManager.STREAM_RING, maxVolume / 2,
                        AudioManager.FLAG_ALLOW_RINGER_MODES);
                // make sure that we are not in silent mode
                mAudioManager.setRingerMode(AudioManager.RINGER_MODE_NORMAL);
            } finally {
                Utils.toggleNotificationPolicyAccess(
                        mContext.getPackageName(), getInstrumentation(), false);
            }
        }

        mDefaultRingUri = RingtoneManager.getActualDefaultRingtoneUri(mContext,
                RingtoneManager.TYPE_RINGTONE);
    }

    private void enableAppOps() {
        StringBuilder cmd = new StringBuilder();
        cmd.append("appops set ");
        cmd.append(getInstrumentation().getContext().getPackageName());
        cmd.append(" android:write_settings allow");
        getInstrumentation().getUiAutomation().executeShellCommand(cmd.toString());
        try {
            Thread.sleep(2200);
        } catch (InterruptedException e) {
        }
    }

    @Override
    protected void tearDown() throws Exception {
        // restore original settings
        if (mRingtone != null) {
            if (mRingtone.isPlaying()) mRingtone.stop();
            mRingtone.setStreamType(mOriginalStreamType);
        }
        if (mAudioManager != null && !ActivityManager.isLowRamDeviceStatic()) {
            try {
                Utils.toggleNotificationPolicyAccess(
                        mContext.getPackageName(), getInstrumentation(), true);
                mAudioManager.setRingerMode(mOriginalRingerMode);
                mAudioManager.setStreamVolume(AudioManager.STREAM_RING, mOriginalVolume,
                        AudioManager.FLAG_ALLOW_RINGER_MODES);
            } finally {
                Utils.toggleNotificationPolicyAccess(
                        mContext.getPackageName(), getInstrumentation(), false);
            }
        }
        RingtoneManager.setActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_RINGTONE,
                mDefaultRingUri);
        super.tearDown();
    }

    private boolean hasAudioOutput() {
        return getInstrumentation().getContext().getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_AUDIO_OUTPUT);
    }

    private boolean isTV() {
        return getInstrumentation().getContext().getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_LEANBACK_ONLY);
    }

    public void testRingtone() {
        if (isTV()) {
            return;
        }
        if (!hasAudioOutput()) {
            Log.i(TAG, "Skipping testRingtone(): device doesn't have audio output.");
            return;
        }

        assertNotNull(mRingtone.getTitle(mContext));
        assertTrue(mOriginalStreamType >= 0);

        mRingtone.setStreamType(AudioManager.STREAM_MUSIC);
        assertEquals(AudioManager.STREAM_MUSIC, mRingtone.getStreamType());
        mRingtone.setStreamType(AudioManager.STREAM_ALARM);
        assertEquals(AudioManager.STREAM_ALARM, mRingtone.getStreamType());
        // make sure we play on STREAM_RING because we the volume on this stream is not 0
        mRingtone.setStreamType(AudioManager.STREAM_RING);
        assertEquals(AudioManager.STREAM_RING, mRingtone.getStreamType());

        // test both the "None" ringtone and an actual ringtone
        RingtoneManager.setActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_RINGTONE, null);
        mRingtone = RingtoneManager.getRingtone(mContext, Settings.System.DEFAULT_RINGTONE_URI);
        assertTrue(mRingtone.getStreamType() == AudioManager.STREAM_RING);
        mRingtone.play();
        assertFalse(mRingtone.isPlaying());

        Uri uri = RingtoneManager.getValidRingtoneUri(mContext);
        assertNotNull("ringtone was unexpectedly null", uri);
        RingtoneManager.setActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_RINGTONE, uri);
        mRingtone = RingtoneManager.getRingtone(mContext, Settings.System.DEFAULT_RINGTONE_URI);
        assertTrue(mRingtone.getStreamType() == AudioManager.STREAM_RING);
        mRingtone.play();
        assertTrue("couldn't play ringtone " + uri, mRingtone.isPlaying());
        mRingtone.stop();
        assertFalse(mRingtone.isPlaying());
    }

    public void testLoopingVolume() {
        if (isTV()) {
            return;
        }
        if (!hasAudioOutput()) {
            Log.i(TAG, "Skipping testRingtone(): device doesn't have audio output.");
            return;
        }

        Uri uri = RingtoneManager.getValidRingtoneUri(mContext);
        assertNotNull("ringtone was unexpectedly null", uri);
        RingtoneManager.setActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_RINGTONE, uri);
        assertNotNull(mRingtone.getTitle(mContext));
        final AudioAttributes ringtoneAa = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_NOTIFICATION_RINGTONE).
                build();
        mRingtone.setAudioAttributes(ringtoneAa);
        assertEquals(ringtoneAa, mRingtone.getAudioAttributes());
        mRingtone.setLooping(true);
        mRingtone.setVolume(0.5f);
        mRingtone.play();
        assertTrue("couldn't play ringtone " + uri, mRingtone.isPlaying());
        assertTrue(mRingtone.isLooping());
        assertEquals("invalid ringtone player volume", 0.5f, mRingtone.getVolume());
        mRingtone.stop();
        assertFalse(mRingtone.isPlaying());
    }
}
