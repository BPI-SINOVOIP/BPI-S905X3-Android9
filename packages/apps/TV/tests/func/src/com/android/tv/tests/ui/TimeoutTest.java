/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.tv.tests.ui;

import android.support.test.filters.LargeTest;
import android.support.test.uiautomator.Until;
import com.android.tv.R;
import com.android.tv.testing.uihelper.Constants;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Test timeout events like the menu despairing after no input.
 *
 * <p><b>WARNING</b> some of these timeouts are 60 seconds. These tests will take a long time
 * complete.
 */
@LargeTest
@RunWith(JUnit4.class)
public class TimeoutTest {

    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();

    @Test
    public void placeholder() {
        // There must be at least one test
    }

    @Ignore("b/73727914")
    @Test
    public void testMenu() {
        controller.liveChannelsHelper.assertAppStarted();
        controller.pressMenu();

        controller.assertWaitForCondition(Until.hasObject(Constants.MENU));
        controller.assertWaitForCondition(
                Until.gone(Constants.MENU),
                controller.getTargetResources().getInteger(R.integer.menu_show_duration));
    }

    @Ignore("b/73727914")
    @Test
    public void testProgramGuide() {
        controller.liveChannelsHelper.assertAppStarted();
        controller.menuHelper.assertPressProgramGuide();
        controller.assertWaitForCondition(Until.hasObject(Constants.PROGRAM_GUIDE));
        controller.assertWaitForCondition(
                Until.gone(Constants.PROGRAM_GUIDE),
                controller.getTargetResources().getInteger(R.integer.program_guide_show_duration));
        controller.assertHas(Constants.MENU, false);
    }
}
