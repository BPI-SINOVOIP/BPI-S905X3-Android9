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

import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.Until;
import com.android.tv.guide.ProgramGuide;
import com.android.tv.testing.uihelper.Constants;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link ProgramGuide}. */
@MediumTest
@RunWith(JUnit4.class)
public class ProgramGuideTest {
    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();

    @Test
    public void testCancel() {
        controller.liveChannelsHelper.assertAppStarted();
        controller.menuHelper.assertPressProgramGuide();
        controller.assertWaitForCondition(Until.hasObject(Constants.PROGRAM_GUIDE));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(Constants.PROGRAM_GUIDE));
        controller.assertHas(Constants.MENU, false);
    }
}
