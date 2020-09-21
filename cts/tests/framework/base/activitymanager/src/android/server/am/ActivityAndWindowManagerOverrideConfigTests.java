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

import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.server.am.ComponentNameUtils.getLogTag;
import static android.server.am.Components.LOG_CONFIGURATION_ACTIVITY;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logAlways;
import static android.view.Surface.ROTATION_0;
import static android.view.Surface.ROTATION_180;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.os.SystemClock;

import org.junit.Test;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityAndWindowManagerOverrideConfigTests
 */
public class ActivityAndWindowManagerOverrideConfigTests extends ActivityManagerTestBase {

    private static class ConfigurationChangeObserver {
        private static final Pattern CONFIGURATION_CHANGED_PATTERN =
            Pattern.compile("(.+)Configuration changed: (\\d+),(\\d+)");

        private ConfigurationChangeObserver() {
        }

        private boolean findConfigurationChange(
                ComponentName activityName, LogSeparator logSeparator) {
            for (int retry = 1; retry <= 5; retry++) {
                final String[] lines =
                        getDeviceLogsForComponents(logSeparator, getLogTag(activityName));
                log("Looking at logcat");
                for (int i = lines.length - 1; i >= 0; i--) {
                    final String line = lines[i].trim();
                    log(line);
                    Matcher matcher = CONFIGURATION_CHANGED_PATTERN.matcher(line);
                    if (matcher.matches()) {
                        return true;
                    }
                }
                logAlways("***Waiting configuration change of " + getLogTag(activityName)
                        + " retry=" + retry);
                SystemClock.sleep(500);
            }
            return false;
        }
    }

    @Test
    public void testReceiveOverrideConfigFromRelayout() throws Exception {
        assumeTrue("Device doesn't support freeform. Skipping test.", supportsFreeform());

        launchActivity(LOG_CONFIGURATION_ACTIVITY, WINDOWING_MODE_FREEFORM);

        try (final RotationSession rotationSession = new RotationSession()) {
            rotationSession.set(ROTATION_0);
            LogSeparator logSeparator = separateLogs();
            resizeActivityTask(LOG_CONFIGURATION_ACTIVITY, 0, 0, 100, 100);
            ConfigurationChangeObserver c = new ConfigurationChangeObserver();
            final boolean reportedSizeAfterResize = c.findConfigurationChange(
                    LOG_CONFIGURATION_ACTIVITY, logSeparator);
            assertTrue("Expected to observe configuration change when resizing",
                    reportedSizeAfterResize);

            logSeparator = separateLogs();
            rotationSession.set(ROTATION_180);
            final boolean reportedSizeAfterRotation = c.findConfigurationChange(
                    LOG_CONFIGURATION_ACTIVITY, logSeparator);
            assertFalse("Not expected to observe configuration change after flip rotation",
                    reportedSizeAfterRotation);
        }
    }
}

