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
import com.android.tv.R;
import com.android.tv.testing.uihelper.Constants;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link com.android.tv.ui.ChannelBannerView} */
@MediumTest
@RunWith(JUnit4.class)
public class ChannelBannerViewTest {
    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();

    // Channel banner show duration with the grace period.
    private long mShowDurationMillis;

    @Before
    public void setUp() throws Exception {
        controller.liveChannelsHelper.assertAppStarted();
        mShowDurationMillis =
                controller.getTargetResources().getInteger(R.integer.channel_banner_show_duration)
                        + Constants.MAX_SHOW_DELAY_MILLIS;
    }

    @Ignore("b/73727914")
    @Test
    public void testChannelBannerAppearDisappear() {
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.hasObject(Constants.CHANNEL_BANNER));
        controller.assertWaitForCondition(
                Until.gone(Constants.CHANNEL_BANNER), mShowDurationMillis);
    }

    @Test
    public void testChannelBannerShownWhenTune() {
        controller.pressDPadDown();
        controller.assertWaitForCondition(Until.hasObject(Constants.CHANNEL_BANNER));
        controller.pressDPadUp();
        controller.assertWaitForCondition(Until.hasObject(Constants.CHANNEL_BANNER));
    }
}
