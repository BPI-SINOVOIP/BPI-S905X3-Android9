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

package android.server.am;

import static android.content.pm.PackageManager.FEATURE_ACTIVITIES_ON_SECONDARY_DISPLAYS;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.Components.VIRTUAL_DISPLAY_ACTIVITY;
import static android.server.am.Components.VirtualDisplayActivity.COMMAND_CREATE_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.COMMAND_DESTROY_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.COMMAND_RESIZE_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity
        .KEY_CAN_SHOW_WITH_INSECURE_KEYGUARD;
import static android.server.am.Components.VirtualDisplayActivity.KEY_COMMAND;
import static android.server.am.Components.VirtualDisplayActivity.KEY_COUNT;
import static android.server.am.Components.VirtualDisplayActivity.KEY_DENSITY_DPI;
import static android.server.am.Components.VirtualDisplayActivity.KEY_LAUNCH_TARGET_COMPONENT;
import static android.server.am.Components.VirtualDisplayActivity.KEY_PUBLIC_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.KEY_RESIZE_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.VIRTUAL_DISPLAY_PREFIX;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logAlways;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.hasSize;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.ComponentName;
import android.content.res.Configuration;
import android.os.SystemClock;
import android.provider.Settings;
import android.server.am.ActivityManagerState.ActivityDisplay;
import android.server.am.settings.SettingsSession;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.util.Size;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Base class for ActivityManager display tests.
 *
 * @see ActivityManagerDisplayTests
 * @see ActivityManagerDisplayLockedKeyguardTests
 */
class ActivityManagerDisplayTestBase extends ActivityManagerTestBase {

    static final int CUSTOM_DENSITY_DPI = 222;
    private static final int INVALID_DENSITY_DPI = -1;

    ActivityDisplay getDisplayState(List<ActivityDisplay> displays, int displayId) {
        for (ActivityDisplay display : displays) {
            if (display.mId == displayId) {
                return display;
            }
        }
        return null;
    }

    /** Return the display state with width, height, dpi. Always not default display. */
    ActivityDisplay getDisplayState(List<ActivityDisplay> displays, int width, int height,
            int dpi) {
        for (ActivityDisplay display : displays) {
            if (display.mId == DEFAULT_DISPLAY) {
                continue;
            }
            final Configuration config = display.mFullConfiguration;
            if (config.densityDpi == dpi && config.screenWidthDp == width
                    && config.screenHeightDp == height) {
                return display;
            }
        }
        return null;
    }

    List<ActivityDisplay> getDisplaysStates() {
        mAmWmState.getAmState().computeState();
        return mAmWmState.getAmState().getDisplays();
    }

    /** Find the display that was not originally reported in oldDisplays and added in newDisplays */
    List<ActivityDisplay> findNewDisplayStates(List<ActivityDisplay> oldDisplays,
            List<ActivityDisplay> newDisplays) {
        final ArrayList<ActivityDisplay> result = new ArrayList<>();

        for (ActivityDisplay newDisplay : newDisplays) {
            if (oldDisplays.stream().noneMatch(d -> d.mId == newDisplay.mId)) {
                result.add(newDisplay);
            }
        }

        return result;
    }

    static class ReportedDisplayMetrics {
        private static final String WM_SIZE = "wm size";
        private static final String WM_DENSITY = "wm density";
        private static final Pattern PHYSICAL_SIZE =
                Pattern.compile("Physical size: (\\d+)x(\\d+)");
        private static final Pattern OVERRIDE_SIZE =
                Pattern.compile("Override size: (\\d+)x(\\d+)");
        private static final Pattern PHYSICAL_DENSITY =
                Pattern.compile("Physical density: (\\d+)");
        private static final Pattern OVERRIDE_DENSITY =
                Pattern.compile("Override density: (\\d+)");

        @NonNull
        final Size physicalSize;
        final int physicalDensity;

        @Nullable
        final Size overrideSize;
        @Nullable
        final Integer overrideDensity;

        /** Get physical and override display metrics from WM. */
        static ReportedDisplayMetrics getDisplayMetrics() {
            return new ReportedDisplayMetrics(
                    executeShellCommand(WM_SIZE) + executeShellCommand(WM_DENSITY));
        }

        void setDisplayMetrics(final Size size, final int density) {
            setSize(size);
            setDensity(density);
        }

        void restoreDisplayMetrics() {
            if (overrideSize != null) {
                setSize(overrideSize);
            } else {
                executeShellCommand(WM_SIZE + " reset");
            }
            if (overrideDensity != null) {
                setDensity(overrideDensity);
            } else {
                executeShellCommand(WM_DENSITY + " reset");
            }
        }

        private void setSize(final Size size) {
            executeShellCommand(WM_SIZE + " " + size.getWidth() + "x" + size.getHeight());
        }

        private void setDensity(final int density) {
            executeShellCommand(WM_DENSITY + " " + density);
        }

        /** Get display size that WM operates with. */
        Size getSize() {
            return overrideSize != null ? overrideSize : physicalSize;
        }

        /** Get density that WM operates with. */
        int getDensity() {
            return overrideDensity != null ? overrideDensity : physicalDensity;
        }

        private ReportedDisplayMetrics(final String lines) {
            Matcher matcher = PHYSICAL_SIZE.matcher(lines);
            assertTrue("Physical display size must be reported", matcher.find());
            log(matcher.group());
            physicalSize = new Size(
                    Integer.parseInt(matcher.group(1)), Integer.parseInt(matcher.group(2)));

            matcher = PHYSICAL_DENSITY.matcher(lines);
            assertTrue("Physical display density must be reported", matcher.find());
            log(matcher.group());
            physicalDensity = Integer.parseInt(matcher.group(1));

            matcher = OVERRIDE_SIZE.matcher(lines);
            if (matcher.find()) {
                log(matcher.group());
                overrideSize = new Size(
                        Integer.parseInt(matcher.group(1)), Integer.parseInt(matcher.group(2)));
            } else {
                overrideSize = null;
            }

            matcher = OVERRIDE_DENSITY.matcher(lines);
            if (matcher.find()) {
                log(matcher.group());
                overrideDensity = Integer.parseInt(matcher.group(1));
            } else {
                overrideDensity = null;
            }
        }
    }

    protected class VirtualDisplaySession implements AutoCloseable {
        private int mDensityDpi = CUSTOM_DENSITY_DPI;
        private boolean mLaunchInSplitScreen = false;
        private boolean mCanShowWithInsecureKeyguard = false;
        private boolean mPublicDisplay = false;
        private boolean mResizeDisplay = true;
        private ComponentName mLaunchActivity = null;
        private boolean mSimulateDisplay = false;
        private boolean mMustBeCreated = true;

        private boolean mVirtualDisplayCreated = false;
        private final OverlayDisplayDevicesSession mOverlayDisplayDeviceSession =
                new OverlayDisplayDevicesSession();

        VirtualDisplaySession setDensityDpi(int densityDpi) {
            mDensityDpi = densityDpi;
            return this;
        }

        VirtualDisplaySession setLaunchInSplitScreen(boolean launchInSplitScreen) {
            mLaunchInSplitScreen = launchInSplitScreen;
            return this;
        }

        VirtualDisplaySession setCanShowWithInsecureKeyguard(boolean canShowWithInsecureKeyguard) {
            mCanShowWithInsecureKeyguard = canShowWithInsecureKeyguard;
            return this;
        }

        VirtualDisplaySession setPublicDisplay(boolean publicDisplay) {
            mPublicDisplay = publicDisplay;
            return this;
        }

        VirtualDisplaySession setResizeDisplay(boolean resizeDisplay) {
            mResizeDisplay = resizeDisplay;
            return this;
        }

        VirtualDisplaySession setLaunchActivity(ComponentName launchActivity) {
            mLaunchActivity = launchActivity;
            return this;
        }

        VirtualDisplaySession setSimulateDisplay(boolean simulateDisplay) {
            mSimulateDisplay = simulateDisplay;
            return this;
        }

        VirtualDisplaySession setMustBeCreated(boolean mustBeCreated) {
            mMustBeCreated = mustBeCreated;
            return this;
        }

        @Nullable
        ActivityDisplay createDisplay() throws Exception {
            return createDisplays(1).stream().findFirst().orElse(null);
        }

        @NonNull
        List<ActivityDisplay> createDisplays(int count) throws Exception {
            if (mSimulateDisplay) {
                return simulateDisplay();
            } else {
                return createVirtualDisplays(count);
            }
        }

        void resizeDisplay() {
            executeShellCommand(getAmStartCmd(VIRTUAL_DISPLAY_ACTIVITY)
                    + " -f 0x20000000" + " --es " + KEY_COMMAND + " " + COMMAND_RESIZE_DISPLAY);
        }

        @Override
        public void close() throws Exception {
            mOverlayDisplayDeviceSession.close();
            if (mVirtualDisplayCreated) {
                destroyVirtualDisplays();
                mVirtualDisplayCreated = false;
            }
        }

        /**
         * Simulate new display.
         * <pre>
         * <code>mDensityDpi</code> provide custom density for the display.
         * </pre>
         * @return {@link ActivityDisplay} of newly created display.
         */
        private List<ActivityDisplay> simulateDisplay() throws Exception {
            final List<ActivityDisplay> originalDs = getDisplaysStates();

            // Create virtual display with custom density dpi.
            mOverlayDisplayDeviceSession.set("1024x768/" + mDensityDpi);

            return assertAndGetNewDisplays(1, originalDs);
        }

        /**
         * Create new virtual display.
         * <pre>
         * <code>mDensityDpi</code> provide custom density for the display.
         * <code>mLaunchInSplitScreen</code> start {@link VirtualDisplayActivity} to side from
         *     {@link LaunchingActivity} on primary display.
         * <code>mCanShowWithInsecureKeyguard</code>  allow showing content when device is
         *     showing an insecure keyguard.
         * <code>mMustBeCreated</code> should assert if the display was or wasn't created.
         * <code>mPublicDisplay</code> make display public.
         * <code>mResizeDisplay</code> should resize display when surface size changes.
         * <code>LaunchActivity</code> should launch test activity immediately after display
         *     creation.
         * </pre>
         * @param displayCount number of displays to be created.
         * @return A list of {@link ActivityDisplay} that represent newly created displays.
         * @throws Exception
         */
        private List<ActivityDisplay> createVirtualDisplays(int displayCount) {
            // Start an activity that is able to create virtual displays.
            if (mLaunchInSplitScreen) {
                getLaunchActivityBuilder()
                        .setToSide(true)
                        .setTargetActivity(VIRTUAL_DISPLAY_ACTIVITY)
                        .execute();
            } else {
                launchActivity(VIRTUAL_DISPLAY_ACTIVITY);
            }
            mAmWmState.computeState(false /* compareTaskAndStackBounds */,
                    new WaitForValidActivityState(VIRTUAL_DISPLAY_ACTIVITY));
            mAmWmState.assertVisibility(VIRTUAL_DISPLAY_ACTIVITY, true /* visible */);
            mAmWmState.assertFocusedActivity("Focus must be on virtual display host activity",
                    VIRTUAL_DISPLAY_ACTIVITY);
            final List<ActivityDisplay> originalDS = getDisplaysStates();

            // Create virtual display with custom density dpi.
            final StringBuilder createVirtualDisplayCommand = new StringBuilder(
                    getAmStartCmd(VIRTUAL_DISPLAY_ACTIVITY))
                    .append(" -f 0x20000000")
                    .append(" --es " + KEY_COMMAND + " " + COMMAND_CREATE_DISPLAY);
            if (mDensityDpi != INVALID_DENSITY_DPI) {
                createVirtualDisplayCommand
                        .append(" --ei " + KEY_DENSITY_DPI + " ")
                        .append(mDensityDpi);
            }
            createVirtualDisplayCommand.append(" --ei " + KEY_COUNT + " ").append(displayCount)
                    .append(" --ez " + KEY_CAN_SHOW_WITH_INSECURE_KEYGUARD + " ")
                    .append(mCanShowWithInsecureKeyguard)
                    .append(" --ez " + KEY_PUBLIC_DISPLAY + " ").append(mPublicDisplay)
                    .append(" --ez " + KEY_RESIZE_DISPLAY + " ").append(mResizeDisplay);
            if (mLaunchActivity != null) {
                createVirtualDisplayCommand
                        .append(" --es " + KEY_LAUNCH_TARGET_COMPONENT + " ")
                        .append(getActivityName(mLaunchActivity));
            }
            executeShellCommand(createVirtualDisplayCommand.toString());
            mVirtualDisplayCreated = true;

            return assertAndGetNewDisplays(mMustBeCreated ? displayCount : -1, originalDS);
        }

        /**
         * Destroy existing virtual display.
         */
        void destroyVirtualDisplays() {
            final String destroyVirtualDisplayCommand = getAmStartCmd(VIRTUAL_DISPLAY_ACTIVITY)
                    + " -f 0x20000000"
                    + " --es " + KEY_COMMAND + " " + COMMAND_DESTROY_DISPLAY;
            executeShellCommand(destroyVirtualDisplayCommand);
            waitForDisplaysDestroyed();
        }

        private void waitForDisplaysDestroyed() {
            for (int retry = 1; retry <= 5; retry++) {
                if (!isHostedVirtualDisplayPresent()) {
                    return;
                }
                logAlways("Waiting for hosted displays destruction... retry=" + retry);
                SystemClock.sleep(500);
            }
            fail("Waiting for hosted displays destruction failed.");
        }

        private boolean isHostedVirtualDisplayPresent() {
            mAmWmState.computeState(true);
            return mAmWmState.getWmState().getDisplays().stream().anyMatch(
                    d -> d.getName() != null && d.getName().contains(VIRTUAL_DISPLAY_PREFIX));
        }

        /**
         * Wait for desired number of displays to be created and get their properties.
         * @param newDisplayCount expected display count, -1 if display should not be created.
         * @param originalDS display states before creation of new display(s).
         * @return list of new displays, empty list if no new display is created.
         */
        private List<ActivityDisplay> assertAndGetNewDisplays(int newDisplayCount,
                List<ActivityDisplay> originalDS) {
            final int originalDisplayCount = originalDS.size();

            // Wait for the display(s) to be created and get configurations.
            final List<ActivityDisplay> ds = getDisplayStateAfterChange(
                    originalDisplayCount + newDisplayCount);
            if (newDisplayCount != -1) {
                assertEquals("New virtual display(s) must be created",
                        originalDisplayCount + newDisplayCount, ds.size());
            } else {
                assertEquals("New virtual display must not be created",
                        originalDisplayCount, ds.size());
                return Collections.emptyList();
            }

            // Find the newly added display(s).
            final List<ActivityDisplay> newDisplays = findNewDisplayStates(originalDS, ds);
            assertThat("New virtual display must be created",
                    newDisplays, hasSize(newDisplayCount));

            return newDisplays;
        }
    }

    /** Helper class to save, set, and restore overlay_display_devices preference. */
    private static class OverlayDisplayDevicesSession extends SettingsSession<String> {
        OverlayDisplayDevicesSession() {
            super(Settings.Global.getUriFor(Settings.Global.OVERLAY_DISPLAY_DEVICES),
                    Settings.Global::getString,
                    Settings.Global::putString);
        }
    }

    /** Wait for provided number of displays and report their configurations. */
    List<ActivityDisplay> getDisplayStateAfterChange(int expectedDisplayCount) {
        List<ActivityDisplay> ds = getDisplaysStates();

        int retriesLeft = 5;
        while (!areDisplaysValid(ds, expectedDisplayCount) && retriesLeft-- > 0) {
            log("***Waiting for the correct number of displays...");
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                log(e.toString());
            }
            ds = getDisplaysStates();
        }

        return ds;
    }

    private boolean areDisplaysValid(List<ActivityDisplay> displays, int expectedDisplayCount) {
        if (displays.size() != expectedDisplayCount) {
            return false;
        }
        for (ActivityDisplay display : displays) {
            if (display.mOverrideConfiguration.densityDpi == 0) {
                return false;
            }
        }
        return true;
    }

    /** Checks if the device supports multi-display. */
    boolean supportsMultiDisplay() {
        return hasDeviceFeature(FEATURE_ACTIVITIES_ON_SECONDARY_DISPLAYS);
    }
}
