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
 * limitations under the License.
 */

package com.android.tv.tests.ui.sidepanel;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;

import android.graphics.Point;
import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import com.android.tv.R;
import com.android.tv.testing.uihelper.Constants;
import com.android.tv.tests.ui.LiveChannelsTestController;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for @{link {@link com.android.tv.ui.sidepanel.CustomizeChannelListFragment} */
@MediumTest
@RunWith(JUnit4.class)
public class CustomizeChannelListFragmentTest {

    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();
    private BySelector mBySettingsSidePanel;
    private UiObject2 mTvView;
    private Point mNormalTvViewCenter;

    @Before
    public void setUp() throws Exception {

        controller.liveChannelsHelper.assertAppStarted();
        mTvView = controller.getUiDevice().findObject(Constants.TV_VIEW);
        mNormalTvViewCenter = mTvView.getVisibleCenter();
        assertNotNull(mNormalTvViewCenter);
        controller.pressKeysForChannel(com.android.tv.testing.testinput.TvTestInputConstants.CH_2);
        // Wait until KeypadChannelSwitchView closes.
        controller.assertWaitForCondition(Until.hasObject(Constants.CHANNEL_BANNER));
        mBySettingsSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_settings);
    }

    private void assertShrunkenTvView(boolean shrunkenExpected) {
        Point currentTvViewCenter = mTvView.getVisibleCenter();
        if (shrunkenExpected) {
            assertFalse(mNormalTvViewCenter.equals(currentTvViewCenter));
        } else {
            assertTrue(mNormalTvViewCenter.equals(currentTvViewCenter));
        }
    }

    @Test
    public void testCustomizeChannelList_noraml() {
        // Show customize channel list fragment
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));
        controller.sidePanelHelper.assertNavigateToItem(
                R.string.settings_channel_source_item_customize_channels);
        controller.pressDPadCenter();
        BySelector bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.side_panel_title_edit_channels_for_an_input);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        assertShrunkenTvView(true);

        // Show group by fragment
        controller.sidePanelHelper.assertNavigateToItem(
                R.string.edit_channels_item_group_by, Direction.UP);
        controller.pressDPadCenter();
        bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_group_by);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        assertShrunkenTvView(true);

        // Back to customize channel list fragment
        controller.pressBack();
        bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.side_panel_title_edit_channels_for_an_input);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        assertShrunkenTvView(true);

        // Return to the main menu.
        controller.pressBack();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));
        assertShrunkenTvView(false);
    }

    @Ignore("b/73727914")
    @Test
    public void testCustomizeChannelList_timeout() {
        // Show customize channel list fragment
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));
        controller.sidePanelHelper.assertNavigateToItem(
                R.string.settings_channel_source_item_customize_channels);
        controller.pressDPadCenter();
        BySelector bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.side_panel_title_edit_channels_for_an_input);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        assertShrunkenTvView(true);

        // Show group by fragment
        controller.sidePanelHelper.assertNavigateToItem(
                R.string.edit_channels_item_group_by, Direction.UP);
        controller.pressDPadCenter();
        bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_group_by);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        assertShrunkenTvView(true);

        // Wait for time-out to return to the main menu.
        controller.assertWaitForCondition(
                Until.gone(bySidePanel),
                controller.getTargetResources().getInteger(R.integer.side_panel_show_duration));
        assertShrunkenTvView(false);
    }
}
