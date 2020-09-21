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
import com.android.tv.testing.testinput.ChannelStateData;
import com.android.tv.testing.testinput.TvTestInputConstants;
import com.android.tv.testing.uihelper.Constants;
import com.android.tv.testing.uihelper.DialogHelper;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Basic tests for the LiveChannels app. */
@MediumTest
@RunWith(JUnit4.class)
public class LiveChannelsAppTest {
    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();

    private BySelector mBySettingsSidePanel;

    @Before
    public void setUp() throws Exception {
        controller.liveChannelsHelper.assertAppStarted();
        controller.pressKeysForChannel(TvTestInputConstants.CH_1_DEFAULT_DONT_MODIFY);
        controller.waitForIdleSync();
        mBySettingsSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_settings);
    }

    @Test
    public void testSettingsCancel() {
        controller.menuHelper.assertPressOptionsSettings();
        BySelector byChannelSourcesSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.settings_channel_source_item_customize_channels);
        controller.assertWaitForCondition(Until.hasObject(byChannelSourcesSidePanel));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(byChannelSourcesSidePanel));
        controller.assertHas(Constants.MENU, false);
    }

    @Test
    public void testClosedCaptionsCancel() {
        controller.menuHelper.assertPressOptionsClosedCaptions();
        BySelector byClosedCaptionSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.side_panel_title_closed_caption);
        controller.assertWaitForCondition(Until.hasObject(byClosedCaptionSidePanel));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(byClosedCaptionSidePanel));
        controller.assertHas(Constants.MENU, false);
    }

    @Test
    public void testDisplayModeCancel() {
        ChannelStateData data = new ChannelStateData();
        data.mTvTrackInfos.add(com.android.tv.testing.constants.Constants.SVGA_VIDEO_TRACK);
        data.mSelectedVideoTrackId =
                com.android.tv.testing.constants.Constants.SVGA_VIDEO_TRACK.getId();
        controller.updateThenTune(data, TvTestInputConstants.CH_2);

        controller.menuHelper.assertPressOptionsDisplayMode();
        BySelector byDisplayModeSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.side_panel_title_display_mode);
        controller.assertWaitForCondition(Until.hasObject(byDisplayModeSidePanel));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(byDisplayModeSidePanel));
        controller.assertHas(Constants.MENU, false);
    }

    @Test
    public void testMenu() {
        controller.pressMenu();

        controller.assertWaitForCondition(Until.hasObject(Constants.MENU));
        controller.assertHas(controller.menuHelper.getByChannels(), true);
    }

    @Test
    public void testMultiAudioCancel() {
        ChannelStateData data = new ChannelStateData();
        data.mTvTrackInfos.add(com.android.tv.testing.constants.Constants.GENERIC_AUDIO_TRACK);
        controller.updateThenTune(data, TvTestInputConstants.CH_2);

        controller.menuHelper.assertPressOptionsMultiAudio();
        BySelector byMultiAudioSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_multi_audio);
        controller.assertWaitForCondition(Until.hasObject(byMultiAudioSidePanel));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(byMultiAudioSidePanel));
        controller.assertHas(Constants.MENU, false);
    }

    @Ignore("b/72156196")
    @Test
    public void testPinCancel() {
        controller.menuHelper.showMenu();
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));
        controller.sidePanelHelper.assertNavigateToItem(R.string.settings_parental_controls);
        controller.pressDPadCenter();
        DialogHelper dialogHelper =
                new DialogHelper(controller.getUiDevice(), controller.getTargetResources());
        dialogHelper.assertWaitForPinDialogOpen();
        controller.pressBack();
        dialogHelper.assertWaitForPinDialogClose();
        controller.assertHas(Constants.MENU, false);
    }
}
