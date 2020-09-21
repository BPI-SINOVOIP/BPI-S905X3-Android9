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

package android.media.cts;

import android.app.AppOpsManager;
import android.app.AppOpsManager.OnOpActiveChangedListener;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Process;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

import java.util.concurrent.TimeUnit;

import static android.app.AppOpsManager.OP_RECORD_AUDIO;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

/**
 * Tests for media framework behaviors related to app ops.
 */
@RunWith(AndroidJUnit4.class)
public class AudioRecordAppOpTest {
    private static final long APP_OP_CHANGE_TIMEOUT_MILLIS = TimeUnit.SECONDS.toMillis(2);

    @Test
    public void testRecordAppOps() {
        if (!hasMicrophone()) {
            return;
        }

        final String packageName = getContext().getPackageName();
        final int uid = Process.myUid();

        final AppOpsManager appOpsManager = getContext().getSystemService(AppOpsManager.class);
        final OnOpActiveChangedListener listener = mock(OnOpActiveChangedListener.class);

        AudioRecord recorder = null;
        try {
            // Setup a recorder
            final AudioRecord candidateRecorder = new AudioRecord.Builder()
                    .setAudioSource(MediaRecorder.AudioSource.MIC)
                    .setBufferSizeInBytes(1024)
                    .setAudioFormat(new AudioFormat.Builder()
                            .setSampleRate(8000)
                            .setChannelMask(AudioFormat.CHANNEL_IN_MONO)
                            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                            .build())
                    .build();

            // The app op should be reported as not started
            assertThat(appOpsManager.isOperationActive(OP_RECORD_AUDIO,
                    uid, packageName)).isFalse();

            // Start watching for app op active
            appOpsManager.startWatchingActive(new int[] {OP_RECORD_AUDIO}, listener);

            // Start recording
            candidateRecorder.startRecording();
            recorder = candidateRecorder;

            // The app op should start
            verify(listener, timeout(APP_OP_CHANGE_TIMEOUT_MILLIS)
                    .only()).onOpActiveChanged(eq(OP_RECORD_AUDIO),
                    eq(uid), eq(packageName), eq(true));

            // The app op should be reported as started
            assertThat(appOpsManager.isOperationActive(OP_RECORD_AUDIO,
                    uid, packageName)).isTrue();


            // Start with a clean slate
            Mockito.reset(listener);

            // Stop recording
            recorder.stop();
            recorder.release();
            recorder = null;

            // The app op should stop
            verify(listener, timeout(APP_OP_CHANGE_TIMEOUT_MILLIS)
                    .only()).onOpActiveChanged(eq(OP_RECORD_AUDIO),
                    eq(uid), eq(packageName), eq(false));

            // The app op should be reported as not started
            assertThat(appOpsManager.isOperationActive(OP_RECORD_AUDIO,
                    uid, packageName)).isFalse();
        } finally {
            if (recorder != null) {
                recorder.stop();
                recorder.release();
            }

            appOpsManager.stopWatchingActive(listener);
        }
    }

    private static boolean hasMicrophone() {
        return getContext().getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_MICROPHONE);
    }

    private static Context getContext() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }
}
