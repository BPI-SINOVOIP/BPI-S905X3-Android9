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

import static com.android.tv.testing.uihelper.Constants.CHANNEL_BANNER;
import static com.android.tv.testing.uihelper.Constants.FOCUSED_VIEW;
import static com.android.tv.testing.uihelper.Constants.MENU;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;

import android.support.test.filters.SmallTest;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.view.KeyEvent;
import com.android.tv.R;
import com.android.tv.testing.testinput.TvTestInputConstants;
import com.android.tv.testing.uihelper.Constants;
import com.android.tv.testing.uihelper.DialogHelper;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@SmallTest
@RunWith(JUnit4.class)
public class PlayControlsRowViewTest {
    private static final String BUTTON_ID_PLAY_PAUSE = Constants.TV_APP_PACKAGE + ":id/play_pause";
    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();

    private BySelector mBySettingsSidePanel;

    @Before
    public void setUp() throws Exception {

        controller.liveChannelsHelper.assertAppStarted();
        controller.pressKeysForChannel(TvTestInputConstants.CH_2);
        // Wait until KeypadChannelSwitchView closes.
        controller.assertWaitForCondition(Until.hasObject(CHANNEL_BANNER));
        // Tune to a new channel to ensure that the channel is changed.
        controller.pressDPadUp();
        controller.waitForIdleSync();
        mBySettingsSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_settings);
    }

    /** Test the normal case. The play/pause button should have focus initially. */
    @Ignore("b/72154153")
    @Test
    public void testFocusedViewInNormalCase() {
        controller.menuHelper.showMenu();
        controller.menuHelper.assertNavigateToPlayControlsRow();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
        controller.pressBack();
    }

    /**
     * Tests the case when the forwarding action is disabled. In this case, the button corresponding
     * to the action is disabled, so play/pause button should have the focus.
     */
    @Test
    public void testFocusedViewWithDisabledActionForward() {
        // Fast forward button
        controller.pressKeyCode(KeyEvent.KEYCODE_MEDIA_FAST_FORWARD);
        controller.menuHelper.assertWaitForMenu();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
        controller.pressBack();

        // Next button
        controller.pressKeyCode(KeyEvent.KEYCODE_MEDIA_NEXT);
        controller.menuHelper.assertWaitForMenu();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
        controller.pressBack();
    }

    @Test
    public void testFocusedViewInMenu() {
        controller.menuHelper.showMenu();
        controller.pressKeyCode(KeyEvent.KEYCODE_MEDIA_PLAY);
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
        controller.menuHelper.assertNavigateToRow(R.string.menu_title_channels);
        controller.pressKeyCode(KeyEvent.KEYCODE_MEDIA_NEXT);
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
    }

    @Ignore("b/72154153")
    @Test
    public void testKeepPausedWhileParentalControlChange() {
        // Pause the playback.
        controller.pressKeyCode(KeyEvent.KEYCODE_MEDIA_PAUSE);
        controller.menuHelper.assertWaitForMenu();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
        // Show parental controls fragment.
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));
        controller.sidePanelHelper.assertNavigateToItem(R.string.settings_parental_controls);
        controller.pressDPadCenter();
        DialogHelper dialogHelper =
                new DialogHelper(controller.getUiDevice(), controller.getTargetResources());
        dialogHelper.assertWaitForPinDialogOpen();
        dialogHelper.enterPinCodes();
        dialogHelper.assertWaitForPinDialogClose();
        BySelector bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.menu_parental_controls);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        controller.pressEnter();
        controller.pressEnter();
        controller.pressBack();
        controller.pressBack();
        // Return to the main menu.
        controller.menuHelper.assertWaitForMenu();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
    }

    // TODO("b/70727167"): fix tests
    @Test
    public void testKeepPausedAfterVisitingHome() {
        // Pause the playback.
        controller.pressKeyCode(KeyEvent.KEYCODE_MEDIA_PAUSE);
        controller.menuHelper.assertWaitForMenu();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
        // Press HOME twice to visit the home screen and return to Live TV.
        controller.pressHome();
        // Wait until home screen is shown.
        controller.waitForIdle();
        controller.pressHome();
        // Wait until TV is resumed.
        controller.waitForIdle();
        // Return to the main menu.
        controller.menuHelper.assertWaitForMenu();
        assertButtonHasFocus(BUTTON_ID_PLAY_PAUSE);
    }

    private void assertButtonHasFocus(String buttonId) {
        UiObject2 menu = controller.getUiDevice().findObject(MENU);
        UiObject2 focusedView = menu.findObject(FOCUSED_VIEW);
        assertNotNull("Play controls row doesn't have a focused child.", focusedView);
        UiObject2 focusedButtonGroup = focusedView.getParent();
        assertNotNull("The focused item should have parent", focusedButtonGroup);
        assertEquals(buttonId, focusedButtonGroup.getResourceName());
    }
}
