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
package com.android.tv.tests.jank;

import android.support.test.filters.MediumTest;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import com.android.tv.testing.uihelper.MenuHelper;

/** Jank tests for the program guide. */
@MediumTest
public class MenuJankTest extends LiveChannelsTestCase {
    private static final String STARTING_CHANNEL = "1";

    /**
     * The minimum number of frames expected during each jank test. If there is less the test will
     * fail. To be safe we loop the action in each test to create twice this many frames under
     * normal conditions.
     *
     * <p>200 is chosen so there will be enough frame for the 90th, 95th, and 98th percentile
     * measurements are significant.
     *
     * @see <a href="http://go/janktesthelper-best-practices">Jank Test Helper Best Practices</a>
     */
    private static final int EXPECTED_FRAMES = 200;

    protected MenuHelper mMenuHelper;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMenuHelper = new MenuHelper(mDevice, mTargetResources);
        Utils.pressKeysForChannelNumber(STARTING_CHANNEL, mDevice);
    }

    @JankTest(expectedFrames = EXPECTED_FRAMES, beforeTest = "fillTheMenuRowWithPreviousChannels")
    @GfxMonitor(processName = Utils.LIVE_CHANNELS_PROCESS_NAME)
    public void testShowMenu() {
        int frames = 40; // measured by hand.
        int repeat = EXPECTED_FRAMES * 2 / frames;
        for (int i = 0; i < repeat; i++) {
            mMenuHelper.showMenu();
            mDevice.pressBack();
            mDevice.waitForIdle();
        }
    }

    public void fillTheMenuRowWithPreviousChannels() {
        int cardViewCount = 6;
        for (int i = 0; i < cardViewCount; i++) {
            mDevice.pressDPadUp();
            mDevice.waitForIdle();
        }
    }
}
