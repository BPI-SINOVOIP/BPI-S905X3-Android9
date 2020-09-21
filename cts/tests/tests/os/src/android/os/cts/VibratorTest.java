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

package android.os.cts;

import android.content.Context;
import android.media.AudioAttributes;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.fail;

@RunWith(AndroidJUnit4.class)
@LargeTest
public class VibratorTest {
    private static final AudioAttributes AUDIO_ATTRIBUTES =
            new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                .build();

    private Vibrator mVibrator;

    @Before
    public void setUp() {
        mVibrator = InstrumentationRegistry.getContext().getSystemService(Vibrator.class);
    }

    @Test
    public void testVibratorCancel() {
        mVibrator.vibrate(1000);
        sleep(500);
        mVibrator.cancel();
    }

    @Test
    public void testVibratePattern() {
        long[] pattern = {100, 200, 400, 800, 1600};
        mVibrator.vibrate(pattern, 3);
        try {
            mVibrator.vibrate(pattern, 10);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException expected) { }
        mVibrator.cancel();
    }

    @Test
    public void testVibrateMultiThread() {
        new Thread(new Runnable() {
            public void run() {
                try {
                    mVibrator.vibrate(100);
                } catch (Exception e) {
                    fail("MultiThread fail1");
                }
            }
        }).start();
        new Thread(new Runnable() {
            public void run() {
                try {
                    // This test only get two threads to run vibrator at the same time
                    // for a functional test,
                    // but it can not verify if the second thread get the precedence.
                    mVibrator.vibrate(1000);
                } catch (Exception e) {
                    fail("MultiThread fail2");
                }
            }
        }).start();
        sleep(1500);
    }

    @Test
    public void testVibrateOneShot() {
        VibrationEffect oneShot =
                VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE);
        mVibrator.vibrate(oneShot);
        sleep(100);

        oneShot = VibrationEffect.createOneShot(500, 255 /* Max amplitude */);
        mVibrator.vibrate(oneShot);
        sleep(100);
        mVibrator.cancel();

        oneShot = VibrationEffect.createOneShot(100, 1 /* Min amplitude */);
        mVibrator.vibrate(oneShot, AUDIO_ATTRIBUTES);
        sleep(100);
    }

    @Test
    public void testVibrateWaveform() {
        final long[] timings = new long[] {100, 200, 300, 400, 500};
        final int[] amplitudes = new int[] {64, 128, 255, 128, 64};
        VibrationEffect waveform = VibrationEffect.createWaveform(timings, amplitudes, -1);
        mVibrator.vibrate(waveform);
        sleep(1500);

        waveform = VibrationEffect.createWaveform(timings, amplitudes, 0);
        mVibrator.vibrate(waveform, AUDIO_ATTRIBUTES);
        sleep(2000);
        mVibrator.cancel();
    }

    @Test
    public void testVibratorHasAmplitudeControl() {
        // Just make sure it doesn't crash when this is called; we don't really have a way to test
        // if the amplitude control works or not.
        mVibrator.hasAmplitudeControl();
    }

    private static void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException ignored) { }
    }
}
