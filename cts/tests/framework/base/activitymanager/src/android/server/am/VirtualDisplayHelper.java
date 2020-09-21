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

import static android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;
import static android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION;
import static android.server.am.StateLogger.logAlways;
import static android.support.test.InstrumentationRegistry.getContext;
import static android.support.test.InstrumentationRegistry.getInstrumentation;

import static org.junit.Assert.fail;

import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.ImageReader;
import android.os.SystemClock;
import androidx.annotation.Nullable;

import com.android.compatibility.common.util.SystemUtil;

import java.io.IOException;
import java.util.Objects;
import java.util.function.Predicate;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Helper class to create virtual display.
 */
class VirtualDisplayHelper {

    private static final String VIRTUAL_DISPLAY_NAME = "CtsVirtualDisplay";
    /** See {@link DisplayManager#VIRTUAL_DISPLAY_FLAG_CAN_SHOW_WITH_INSECURE_KEYGUARD}. */
    private static final int VIRTUAL_DISPLAY_FLAG_CAN_SHOW_WITH_INSECURE_KEYGUARD = 1 << 5;

    private static final Pattern DISPLAY_DEVICE_PATTERN = Pattern.compile(
            ".*DisplayDeviceInfo\\{\"([^\"]+)\":.*, state (\\S+),.*\\}.*");
    private static final int DENSITY = 160;
    private static final int HEIGHT = 480;
    private static final int WIDTH = 800;

    private ImageReader mReader;
    private VirtualDisplay mVirtualDisplay;
    private boolean mCreated;

    void createAndWaitForDisplay(boolean requestShowWhenLocked) {
        createVirtualDisplay(requestShowWhenLocked);
        waitForDisplayState(false /* default */, true /* on */);
        mCreated = true;
    }

    void turnDisplayOff() {
        mVirtualDisplay.setSurface(null);
        waitForDisplayState(false /* default */, false /* on */);
    }

    void turnDisplayOn() {
        mVirtualDisplay.setSurface(mReader.getSurface());
        waitForDisplayState(false /* default */, true /* on */);
    }

    void releaseDisplay() {
        if (mCreated) {
            mVirtualDisplay.release();
            mReader.close();
            waitForDisplayCondition(false /* defaultDisplay */, Objects::isNull,
                    "Waiting for virtual display destroy");
        }
        mCreated = false;
    }

    private void createVirtualDisplay(boolean requestShowWhenLocked) {
        mReader = ImageReader.newInstance(WIDTH, HEIGHT, PixelFormat.RGBA_8888, 2);

        final DisplayManager displayManager = getContext().getSystemService(DisplayManager.class);

        int flags = VIRTUAL_DISPLAY_FLAG_PRESENTATION | VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;
        if (requestShowWhenLocked) {
            flags |= VIRTUAL_DISPLAY_FLAG_CAN_SHOW_WITH_INSECURE_KEYGUARD;
        }
        mVirtualDisplay = displayManager.createVirtualDisplay(
                VIRTUAL_DISPLAY_NAME, WIDTH, HEIGHT, DENSITY, mReader.getSurface(), flags);
    }

    static void waitForDefaultDisplayState(boolean wantOn) {
        waitForDisplayState(true /* default */, wantOn);
    }

    private static void waitForDisplayState(boolean defaultDisplay, boolean wantOn) {
        waitForDisplayCondition(defaultDisplay, state -> state != null && state == wantOn,
                "Waiting for " + (defaultDisplay ? "default" : "virtual") + " display "
                        + (wantOn ? "on" : "off"));
    }

    private static void waitForDisplayCondition(boolean defaultDisplay,
            Predicate<Boolean> condition, String message) {
        for (int retry = 1; retry <= 10; retry++) {
            if (condition.test(getDisplayState(defaultDisplay))) {
                return;
            }
            logAlways(message + "... retry=" + retry);
            SystemClock.sleep(500);
        }
        fail(message + " failed");
    }

    @Nullable
    private static Boolean getDisplayState(boolean defaultDisplay) {
        final String dump = executeShellCommand("dumpsys display");
        final Predicate<Matcher> displayNameMatcher = defaultDisplay
                ? m -> m.group(0).contains("FLAG_DEFAULT_DISPLAY")
                : m -> m.group(1).equals(VIRTUAL_DISPLAY_NAME);
        for (final String line : dump.split("\\n")) {
            final Matcher matcher = DISPLAY_DEVICE_PATTERN.matcher(line);
            if (matcher.matches() && displayNameMatcher.test(matcher)) {
                return "ON".equals(matcher.group(2));
            }
        }
        return null;
    }

    private static String executeShellCommand(String command) {
        try {
            return SystemUtil.runShellCommand(getInstrumentation(), command);
        } catch (IOException e) {
            //bubble it up
            throw new RuntimeException(e);
        }
    }
}
