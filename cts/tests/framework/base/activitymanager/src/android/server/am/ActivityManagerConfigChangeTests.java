/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static android.server.am.ActivityManagerState.STATE_RESUMED;
import static android.server.am.ComponentNameUtils.getLogTag;
import static android.server.am.Components.FONT_SCALE_ACTIVITY;
import static android.server.am.Components.FONT_SCALE_NO_RELAUNCH_ACTIVITY;
import static android.server.am.Components.NO_RELAUNCH_ACTIVITY;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logE;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.content.ComponentName;
import android.platform.test.annotations.Presubmit;
import android.provider.Settings;
import android.server.am.settings.SettingsSession;
import android.support.test.filters.FlakyTest;

import org.junit.Test;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerConfigChangeTests
 */
public class ActivityManagerConfigChangeTests extends ActivityManagerTestBase {

    private static final float EXPECTED_FONT_SIZE_SP = 10.0f;

    @Test
    public void testRotation90Relaunch() throws Exception{
        // Should relaunch on every rotation and receive no onConfigurationChanged()
        testRotation(TEST_ACTIVITY, 1, 1, 0);
    }

    @Test
    public void testRotation90NoRelaunch() throws Exception {
        // Should receive onConfigurationChanged() on every rotation and no relaunch
        testRotation(NO_RELAUNCH_ACTIVITY, 1, 0, 1);
    }

    @Test
    public void testRotation180Relaunch() throws Exception {
        // Should receive nothing
        testRotation(TEST_ACTIVITY, 2, 0, 0);
    }

    @Test
    public void testRotation180NoRelaunch() throws Exception {
        // Should receive nothing
        testRotation(NO_RELAUNCH_ACTIVITY, 2, 0, 0);
    }

    @FlakyTest(bugId = 73701185)
    @Presubmit
    @Test
    public void testChangeFontScaleRelaunch() throws Exception {
        // Should relaunch and receive no onConfigurationChanged()
        testChangeFontScale(FONT_SCALE_ACTIVITY, true /* relaunch */);
    }

    @FlakyTest(bugId = 73812451)
    @Presubmit
    @Test
    public void testChangeFontScaleNoRelaunch() throws Exception {
        // Should receive onConfigurationChanged() and no relaunch
        testChangeFontScale(FONT_SCALE_NO_RELAUNCH_ACTIVITY, false /* relaunch */);
    }

    private void testRotation(ComponentName activityName, int rotationStep, int numRelaunch,
            int numConfigChange) throws Exception {
        launchActivity(activityName);

        mAmWmState.computeState(activityName);

        final int initialRotation = 4 - rotationStep;
        try (final RotationSession rotationSession = new RotationSession()) {
            rotationSession.set(initialRotation);
            mAmWmState.computeState(activityName);
            final int actualStackId =
                    mAmWmState.getAmState().getTaskByActivity(activityName).mStackId;
            final int displayId = mAmWmState.getAmState().getStackById(actualStackId).mDisplayId;
            final int newDeviceRotation = getDeviceRotation(displayId);
            if (newDeviceRotation == INVALID_DEVICE_ROTATION) {
                logE("Got an invalid device rotation value. "
                        + "Continuing the test despite of that, but it is likely to fail.");
            } else if (newDeviceRotation != initialRotation) {
                log("This device doesn't support user rotation "
                        + "mode. Not continuing the rotation checks.");
                return;
            }

            for (int rotation = 0; rotation < 4; rotation += rotationStep) {
                final LogSeparator logSeparator = separateLogs();
                rotationSession.set(rotation);
                mAmWmState.computeState(activityName);
                assertRelaunchOrConfigChanged(activityName, numRelaunch, numConfigChange,
                        logSeparator);
            }
        }
    }

    /** Helper class to save, set, and restore font_scale preferences. */
    private static class FontScaleSession extends SettingsSession<Float> {
        FontScaleSession() {
            super(Settings.System.getUriFor(Settings.System.FONT_SCALE),
                    Settings.System::getFloat,
                    Settings.System::putFloat);
        }
    }

    private void testChangeFontScale(
            ComponentName activityName, boolean relaunch) throws Exception {
        try (final FontScaleSession fontScaleSession = new FontScaleSession()) {
            fontScaleSession.set(1.0f);
            LogSeparator logSeparator = separateLogs();
            launchActivity(activityName);
            mAmWmState.computeState(activityName);

            final int densityDpi = getActivityDensityDpi(activityName, logSeparator);

            for (float fontScale = 0.85f; fontScale <= 1.3f; fontScale += 0.15f) {
                logSeparator = separateLogs();
                fontScaleSession.set(fontScale);
                mAmWmState.computeState(activityName);
                assertRelaunchOrConfigChanged(activityName, relaunch ? 1 : 0, relaunch ? 0 : 1,
                        logSeparator);

                // Verify that the display metrics are updated, and therefore the text size is also
                // updated accordingly.
                assertExpectedFontPixelSize(activityName,
                        scaledPixelsToPixels(EXPECTED_FONT_SIZE_SP, fontScale, densityDpi),
                        logSeparator);
            }
        }
    }

    /**
     * Test updating application info when app is running. An activity with matching package name
     * must be recreated and its asset sequence number must be incremented.
     */
    @Test
    public void testUpdateApplicationInfo() throws Exception {
        final LogSeparator firstLogSeparator = separateLogs();

        // Launch an activity that prints applied config.
        launchActivity(TEST_ACTIVITY);
        final int assetSeq = readAssetSeqNumber(TEST_ACTIVITY, firstLogSeparator);

        final LogSeparator logSeparator = separateLogs();
        // Update package info.
        executeShellCommand("am update-appinfo all " + TEST_ACTIVITY.getPackageName());
        mAmWmState.waitForWithAmState((amState) -> {
            // Wait for activity to be resumed and asset seq number to be updated.
            try {
                return readAssetSeqNumber(TEST_ACTIVITY, logSeparator) == assetSeq + 1
                        && amState.hasActivityState(TEST_ACTIVITY, STATE_RESUMED);
            } catch (Exception e) {
                logE("Error waiting for valid state: " + e.getMessage());
                return false;
            }
        }, "Waiting asset sequence number to be updated and for activity to be resumed.");

        // Check if activity is relaunched and asset seq is updated.
        assertRelaunchOrConfigChanged(TEST_ACTIVITY, 1 /* numRelaunch */,
                0 /* numConfigChange */, logSeparator);
        final int newAssetSeq = readAssetSeqNumber(TEST_ACTIVITY, logSeparator);
        assertEquals("Asset sequence number must be incremented.", assetSeq + 1, newAssetSeq);
    }

    private static final Pattern sConfigurationPattern = Pattern.compile(
            "(.+): Configuration: \\{(.*) as.(\\d+)(.*)\\}");

    /** Read asset sequence number in last applied configuration from logs. */
    private int readAssetSeqNumber(ComponentName activityName, LogSeparator logSeparator)
            throws Exception {
        final String[] lines = getDeviceLogsForComponents(logSeparator, getLogTag(activityName));
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            final Matcher matcher = sConfigurationPattern.matcher(line);
            if (matcher.matches()) {
                final String assetSeqNumber = matcher.group(3);
                try {
                    return Integer.valueOf(assetSeqNumber);
                } catch (NumberFormatException e) {
                    // Ignore, asset seq number is not printed when not set.
                }
            }
        }
        return 0;
    }

    // Calculate the scaled pixel size just like the device is supposed to.
    private static int scaledPixelsToPixels(float sp, float fontScale, int densityDpi) {
        final int DEFAULT_DENSITY = 160;
        float f = densityDpi * (1.0f / DEFAULT_DENSITY) * fontScale * sp;
        return (int) ((f >= 0) ? (f + 0.5f) : (f - 0.5f));
    }

    private static Pattern sDeviceDensityPattern = Pattern.compile("^(.+): fontActivityDpi=(.+)$");

    private int getActivityDensityDpi(ComponentName activityName, LogSeparator logSeparator)
            throws Exception {
        final String[] lines = getDeviceLogsForComponents(logSeparator, getLogTag(activityName));
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            final Matcher matcher = sDeviceDensityPattern.matcher(line);
            if (matcher.matches()) {
                return Integer.parseInt(matcher.group(2));
            }
        }
        fail("No fontActivityDpi reported from activity " + activityName);
        return -1;
    }

    private static final Pattern sFontSizePattern = Pattern.compile("^(.+): fontPixelSize=(.+)$");

    /** Read the font size in the last log line. */
    private void assertExpectedFontPixelSize(ComponentName activityName, int fontPixelSize,
            LogSeparator logSeparator) throws Exception {
        final String[] lines = getDeviceLogsForComponents(logSeparator, getLogTag(activityName));
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            final Matcher matcher = sFontSizePattern.matcher(line);
            if (matcher.matches()) {
                assertEquals("Expected font pixel size does not match", fontPixelSize,
                        Integer.parseInt(matcher.group(2)));
                return;
            }
        }
        fail("No fontPixelSize reported from activity " + activityName);
    }
}
