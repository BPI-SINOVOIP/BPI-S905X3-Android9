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

package android.server.am;

import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.UiDeviceUtils.pressHomeButton;
import static android.server.am.debuggable.Components.DEBUGGABLE_APP_ACTIVITY;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThanOrEqualTo;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.ComponentName;
import android.support.test.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;

import java.io.File;
import java.io.FileInputStream;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerAmProfileTests
 *
 * Please talk to Android Studio team first if you want to modify or delete these tests.
 */
public class ActivityManagerAmProfileTests extends ActivityManagerTestBase {

    private static final String OUTPUT_FILE_PATH = "/data/local/tmp/profile.trace";
    private static final String FIRST_WORD_NO_STREAMING = "*version\n";
    private static final String FIRST_WORD_STREAMING = "SLOW";  // Magic word set by runtime.

    private String mReadableFilePath = null;

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        mReadableFilePath = InstrumentationRegistry.getContext()
            .getExternalFilesDir(null)
            .getPath() + "/profile.trace";
    }

    /**
     * Test am profile functionality with the following 3 configurable options:
     *    starting the activity before start profiling? yes;
     *    sampling-based profiling? no;
     *    using streaming output mode? no.
     */
    @Test
    public void testAmProfileStartNoSamplingNoStreaming() throws Exception {
        // am profile start ... , and the same to the following 3 test methods.
        testProfile(true, false, false);
    }

    /**
     * The following tests are similar to testAmProfileStartNoSamplingNoStreaming(),
     * only different in the three configuration options.
     */
    @Test
    public void testAmProfileStartNoSamplingStreaming() throws Exception {
        testProfile(true, false, true);
    }

    @Test
    public void testAmProfileStartSamplingNoStreaming() throws Exception {
        testProfile(true, true, false);
    }

    @Test
    public void testAmProfileStartSamplingStreaming() throws Exception {
        testProfile(true, true, true);
    }

    @Test
    public void testAmStartStartProfilerNoSamplingNoStreaming() throws Exception {
        // am start --start-profiler ..., and the same to the following 3 test methods.
        testProfile(false, false, false);
    }

    @Test
    public void testAmStartStartProfilerNoSamplingStreaming() throws Exception {
        testProfile(false, false, true);
    }

    @Test
    public void testAmStartStartProfilerSamplingNoStreaming() throws Exception {
        testProfile(false, true, false);
    }

    @Test
    public void testAmStartStartProfilerSamplingStreaming() throws Exception {
        testProfile(false, true, true);
    }

    private void testProfile(final boolean startActivityFirst, final boolean sampling,
            final boolean streaming) throws Exception {
        if (startActivityFirst) {
            launchActivity(DEBUGGABLE_APP_ACTIVITY);
        }

        executeShellCommand(
                getStartCmd(DEBUGGABLE_APP_ACTIVITY, startActivityFirst, sampling, streaming));
        // Go to home screen and then warm start the activity to generate some interesting trace.
        pressHomeButton();
        launchActivity(DEBUGGABLE_APP_ACTIVITY);

        executeShellCommand(getStopProfileCmd(DEBUGGABLE_APP_ACTIVITY));
        // Sleep for 0.1 second (100 milliseconds) so the generation of the profiling
        // file is complete.
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            //ignored
        }
        verifyOutputFileFormat(streaming);
    }

    private static String getStartCmd(final ComponentName activityName,
            final boolean activityAlreadyStarted, final boolean sampling, final boolean streaming) {
        final StringBuilder builder = new StringBuilder("am");
        if (activityAlreadyStarted) {
            builder.append(" profile start");
        } else {
            builder.append(String.format(" start -n %s -W -S --start-profiler %s",
                    getActivityName(activityName), OUTPUT_FILE_PATH));
        }
        if (sampling) {
            builder.append(" --sampling 1000");
        }
        if (streaming) {
            builder.append(" --streaming");
        }
        if (activityAlreadyStarted) {
            builder.append(String.format(
                    " %s %s", activityName.getPackageName(), OUTPUT_FILE_PATH));
        }
        return builder.toString();
    }

    private static String getStopProfileCmd(final ComponentName activityName) {
        return "am profile stop " + activityName.getPackageName();
    }

    private void verifyOutputFileFormat(final boolean streaming) throws Exception {
        // This is a hack. The am service has to write to /data/local/tmp because it doesn't have
        // access to the sdcard but the test app can't read there
        executeShellCommand("mv " + OUTPUT_FILE_PATH + " " + mReadableFilePath);

        final String expectedFirstWord = streaming ? FIRST_WORD_STREAMING : FIRST_WORD_NO_STREAMING;
        final byte[] data = readFile(mReadableFilePath);
        assertThat("data size", data.length, greaterThanOrEqualTo(expectedFirstWord.length()));
        final String actualFirstWord = new String(data, 0, expectedFirstWord.length());
        assertEquals("Unexpected first word", expectedFirstWord, actualFirstWord);

        // Clean up.
        executeShellCommand("rm -f " + OUTPUT_FILE_PATH + " " + mReadableFilePath);
    }

    private static byte[] readFile(String clientPath) throws Exception {
        final File file = new File(clientPath);
        assertTrue("File not found on client: " + clientPath, file.isFile());
        final int size = (int) file.length();
        final byte[] bytes = new byte[size];
        try (final FileInputStream fis = new FileInputStream(file)) {
            final int readSize = fis.read(bytes, 0, bytes.length);
            assertEquals("Read all data", bytes.length, readSize);
            return bytes;
        }
    }
}
