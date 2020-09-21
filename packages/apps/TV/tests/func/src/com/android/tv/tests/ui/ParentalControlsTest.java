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

package com.android.tv.tests.ui;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

import android.support.test.filters.MediumTest;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import com.android.tv.R;
import com.android.tv.testing.uihelper.ByResource;
import com.android.tv.testing.uihelper.DialogHelper;
import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests for {@link com.android.tv.ui.sidepanel.parentalcontrols.ParentalControlsFragment} */
@MediumTest
@RunWith(JUnit4.class)
public class ParentalControlsTest {
    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();
    private BySelector mBySettingsSidePanel;

    @Before
    public void setUp() throws Exception {

        controller.liveChannelsHelper.assertAppStarted();
        mBySettingsSidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.side_panel_title_settings);
        // TODO(b/72154681): prepareParentalControl();
    }

    @After
    public void tearDown() throws Exception {
        switchParentalControl(R.string.option_toggle_parental_controls_on);
    }

    @Test
    public void placeHolder() {
        // there must be at least one test.
    }

    @Ignore("b/72154681")
    @Test
    public void testRatingDependentSelect() {
        // Show ratings fragment.
        BySelector bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.option_program_restrictions);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        controller.sidePanelHelper.assertNavigateToItem(R.string.option_ratings);
        controller.pressDPadCenter();
        // Block rating 6 and rating 12. Check if dependent select works well.
        bySidePanel = controller.sidePanelHelper.bySidePanelTitled(R.string.option_ratings);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        // Test on blocking and unblocking Japanese rating.
        int blockAge = 6;
        int unBlockAge = 12;
        int maxAge = 20;
        int minAge = 4;
        for (int age = minAge; age <= maxAge; age++) {
            UiObject2 ratingCheckBox =
                    controller
                            .sidePanelHelper
                            .assertNavigateToItem(String.valueOf(age))
                            .findObject(
                                    ByResource.id(controller.getTargetResources(), R.id.check_box));
            if (ratingCheckBox.isChecked()) {
                controller.pressDPadCenter();
            }
        }
        controller.sidePanelHelper.assertNavigateToItem(String.valueOf(blockAge));
        controller.pressDPadCenter();
        assertRatingViewIsChecked(minAge, maxAge, blockAge, true);
        controller.sidePanelHelper.assertNavigateToItem(String.valueOf(unBlockAge));
        controller.pressDPadCenter();
        assertRatingViewIsChecked(minAge, maxAge, unBlockAge, false);
        controller.pressBack();
        controller.pressBack();
        controller.waitForIdleSync();
    }

    private void assertRatingViewIsChecked(
            int minAge, int maxAge, int selectedAge, boolean expectedValue) {
        for (int age = minAge; age <= maxAge; age++) {
            UiObject2 ratingCheckBox =
                    controller
                            .sidePanelHelper
                            .assertNavigateToItem(String.valueOf(age))
                            .findObject(
                                    ByResource.id(controller.getTargetResources(), R.id.check_box));
            if (age < selectedAge) {
                assertTrue("The lower rating age should be unblocked", !ratingCheckBox.isChecked());
            } else if (age > selectedAge) {
                assertTrue("The higher rating age should be blocked", ratingCheckBox.isChecked());
            } else {
                assertEquals(
                        "The rating for age " + selectedAge + " isBlocked ",
                        expectedValue,
                        ratingCheckBox.isChecked());
            }
        }
    }

    /**
     * Prepare the need for testRatingDependentSelect. 1. Turn on parental control if it's off. 2.
     * Make sure Japan rating system is selected.
     */
    private void prepareParentalControl() {
        showParentalControl();
        switchParentalControl(R.string.option_toggle_parental_controls_off);
        // Show all rating systems.
        controller.sidePanelHelper.assertNavigateToItem(R.string.option_program_restrictions);
        controller.pressDPadCenter();
        BySelector bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.option_program_restrictions);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        controller.sidePanelHelper.assertNavigateToItem(R.string.option_country_rating_systems);
        controller.pressDPadCenter();
        bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(
                        R.string.option_country_rating_systems);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
        controller.sidePanelHelper.assertNavigateToItem(R.string.option_see_all_rating_systems);
        controller.pressDPadCenter();
        // Make sure Japan rating system is selected.
        UiObject2 ratingSystemCheckBox =
                controller
                        .sidePanelHelper
                        .assertNavigateToItem("Japan")
                        .findObject(ByResource.id(controller.getTargetResources(), R.id.check_box));
        if (!ratingSystemCheckBox.isChecked()) {
            controller.pressDPadCenter();
            controller.waitForIdleSync();
        }
        controller.pressBack();
    }

    private void switchParentalControl(int oppositeStateResId) {
        BySelector bySidePanel = controller.sidePanelHelper.byViewText(oppositeStateResId);
        if (controller.getUiDevice().hasObject(bySidePanel)) {
            controller.sidePanelHelper.assertNavigateToItem(oppositeStateResId);
            controller.pressDPadCenter();
            controller.waitForIdleSync();
        }
    }

    private void showParentalControl() {
        // Show menu and select parental controls.
        controller.menuHelper.showMenu();
        controller.menuHelper.assertPressOptionsSettings();
        controller.assertWaitForCondition(Until.hasObject(mBySettingsSidePanel));
        controller.sidePanelHelper.assertNavigateToItem(R.string.settings_parental_controls);
        controller.pressDPadCenter();
        // Enter pin code.
        DialogHelper dialogHelper =
                new DialogHelper(controller.getUiDevice(), controller.getTargetResources());
        dialogHelper.assertWaitForPinDialogOpen();
        dialogHelper.enterPinCodes();
        dialogHelper.assertWaitForPinDialogClose();
        BySelector bySidePanel =
                controller.sidePanelHelper.bySidePanelTitled(R.string.menu_parental_controls);
        controller.assertWaitForCondition(Until.hasObject(bySidePanel));
    }
}
