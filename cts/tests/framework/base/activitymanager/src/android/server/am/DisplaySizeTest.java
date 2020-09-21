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

import static android.content.Intent.FLAG_ACTIVITY_SINGLE_TOP;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.server.am.displaysize.Components.SMALLEST_WIDTH_ACTIVITY;
import static android.server.am.displaysize.Components.SmallestWidthActivity
        .EXTRA_LAUNCH_ANOTHER_ACTIVITY;

import static org.junit.Assert.assertTrue;

import android.os.Build;

import org.junit.After;
import org.junit.Test;

/**
 * Ensure that compatibility dialog is shown when launching an application with
 * an unsupported smallest width.
 *
 * <p>Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:DisplaySizeTest
 */
public class DisplaySizeTest extends ActivityManagerTestBase {

    /** @see com.android.server.am.UnsupportedDisplaySizeDialog */
    private static final String UNSUPPORTED_DISPLAY_SIZE_DIALOG_NAME =
            "UnsupportedDisplaySizeDialog";

    @After
    @Override
    public void tearDown() throws Exception {
        super.tearDown();

        // Ensure app process is stopped.
        stopTestPackage(SMALLEST_WIDTH_ACTIVITY);
        stopTestPackage(TEST_ACTIVITY);
    }

    @Test
    public void testCompatibilityDialog() throws Exception {
        // Launch some other app (not to perform density change on launcher).
        launchActivity(TEST_ACTIVITY);
        mAmWmState.assertActivityDisplayed(TEST_ACTIVITY);

        try (final ScreenDensitySession screenDensitySession = new ScreenDensitySession()) {
            screenDensitySession.setUnsupportedDensity();

            // Launch target app.
            launchActivity(SMALLEST_WIDTH_ACTIVITY);
            mAmWmState.assertActivityDisplayed(SMALLEST_WIDTH_ACTIVITY);
            mAmWmState.assertWindowDisplayed(UNSUPPORTED_DISPLAY_SIZE_DIALOG_NAME);
        }
    }

    @Test
    public void testCompatibilityDialogWhenFocused() throws Exception {
        launchActivity(SMALLEST_WIDTH_ACTIVITY);
        mAmWmState.assertActivityDisplayed(SMALLEST_WIDTH_ACTIVITY);

        try (final ScreenDensitySession screenDensitySession = new ScreenDensitySession()) {
            screenDensitySession.setUnsupportedDensity();

            mAmWmState.assertWindowDisplayed(UNSUPPORTED_DISPLAY_SIZE_DIALOG_NAME);
        }
    }

    @Test
    public void testCompatibilityDialogAfterReturn() throws Exception {
        // Launch target app.
        launchActivity(SMALLEST_WIDTH_ACTIVITY);
        mAmWmState.assertActivityDisplayed(SMALLEST_WIDTH_ACTIVITY);
        // Launch another activity.
        final String startActivityOnTop = String.format("%s -f 0x%x --es %s %s",
                getAmStartCmd(SMALLEST_WIDTH_ACTIVITY), FLAG_ACTIVITY_SINGLE_TOP,
                EXTRA_LAUNCH_ANOTHER_ACTIVITY, getActivityName(TEST_ACTIVITY));
        executeShellCommand(startActivityOnTop);
        mAmWmState.assertActivityDisplayed(TEST_ACTIVITY);

        try (final ScreenDensitySession screenDensitySession = new ScreenDensitySession()) {
            screenDensitySession.setUnsupportedDensity();

            pressBackButton();

            mAmWmState.assertActivityDisplayed(SMALLEST_WIDTH_ACTIVITY);
            mAmWmState.assertWindowDisplayed(UNSUPPORTED_DISPLAY_SIZE_DIALOG_NAME);
        }
    }

    private static class ScreenDensitySession implements AutoCloseable {
        private static final String DENSITY_PROP_DEVICE = "ro.sf.lcd_density";
        private static final String DENSITY_PROP_EMULATOR = "qemu.sf.lcd_density";

        void setUnsupportedDensity() {
            // Set device to 0.85 zoom. It doesn't matter that we're zooming out
            // since the feature verifies that we're in a non-default density.
            final int stableDensity = getStableDensity();
            final int targetDensity = (int) (stableDensity * 0.85);
            setDensity(targetDensity);
        }

        @Override
        public void close() throws Exception {
            resetDensity();
        }

        private int getStableDensity() {
            final String densityProp;
            if (Build.IS_EMULATOR) {
                densityProp = DENSITY_PROP_EMULATOR;
            } else {
                densityProp = DENSITY_PROP_DEVICE;
            }

            return Integer.parseInt(executeShellCommand("getprop " + densityProp).trim());
        }

        private void setDensity(int targetDensity) {
            executeShellCommand("wm density " + targetDensity);

            // Verify that the density is changed.
            final String output = executeShellCommand("wm density");
            final boolean success = output.contains("Override density: " + targetDensity);

            assertTrue("Failed to set density to " + targetDensity, success);
        }

        private void resetDensity() {
            executeShellCommand("wm density reset");
        }
    }
}
