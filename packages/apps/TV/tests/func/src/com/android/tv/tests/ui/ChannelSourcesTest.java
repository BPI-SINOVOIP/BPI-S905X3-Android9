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
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Until;
import com.android.tv.R;
import com.android.tv.testing.uihelper.ByResource;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for channel sources. */
@MediumTest
@RunWith(JUnit4.class)
public class ChannelSourcesTest {
    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();
    private BySelector mBySettingsSidePanel;

    @Before
    public void before() throws Exception {
        mBySettingsSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_settings);
    }

    // TODO: create a cancelable test channel setup.

    @Test
    public void testSetup_cancel() {
        controller.liveChannelsHelper.assertAppStarted();
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));

        controller.sidePanelHelper.assertNavigateToItem(
                R.string.settings_channel_source_item_setup);
        controller.pressDPadCenter();

        controller.assertWaitForCondition(
                Until.hasObject(
                        ByResource.text(
                                controller.getTargetResources(), R.string.setup_sources_text)));
        controller.pressBack();
    }

    // SetupSourcesFragment should have no errors if side fragment item is clicked multiple times.
    @Test
    public void testSetupTwice_cancel() {
        controller.liveChannelsHelper.assertAppStarted();
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));

        controller.sidePanelHelper.assertNavigateToItem(
                R.string.settings_channel_source_item_setup);
        controller.pressDPadCenter(2);

        controller.assertWaitForCondition(
                Until.hasObject(
                        ByResource.text(
                                controller.getTargetResources(), R.string.setup_sources_text)));
        controller.pressBack();
    }
}
