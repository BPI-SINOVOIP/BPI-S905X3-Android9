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

package com.android.tv.tests.ui.dvr;

import static com.android.tv.testing.uihelper.UiDeviceAsserts.assertWaitUntilFocused;

import android.os.Build;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SdkSuppress;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Until;
import com.android.tv.R;
import com.android.tv.testing.uihelper.ByResource;
import com.android.tv.testing.uihelper.Constants;
import com.android.tv.tests.ui.LiveChannelsTestController;
import java.util.regex.Pattern;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test the DVR library UI */
@MediumTest
@RunWith(JUnit4.class)
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.N)
public class DvrLibraryTest {
    private static final String PROGRAM_NAME_PREFIX = "Title(";

    @Rule public final LiveChannelsTestController controller = new LiveChannelsTestController();

    private BySelector mRecentRow;
    private BySelector mScheduledRow;
    private BySelector mSeriesRow;
    private BySelector mFullScheduleCard;

    @Before
    public void setUp() throws Exception {

        mRecentRow =
                By.hasDescendant(
                        ByResource.text(controller.getTargetResources(), R.string.dvr_main_recent));
        mScheduledRow =
                By.hasDescendant(
                        ByResource.text(
                                controller.getTargetResources(), R.string.dvr_main_scheduled));
        mSeriesRow =
                By.hasDescendant(
                        ByResource.text(controller.getTargetResources(), R.string.dvr_main_series));
        mFullScheduleCard =
                By.focusable(true)
                        .hasDescendant(
                                ByResource.text(
                                        controller.getTargetResources(),
                                        R.string.dvr_full_schedule_card_view_title));
        controller.liveChannelsHelper.assertAppStarted();
    }

    @Test
    public void placeholder() {
        // TODO(b/72153742): three must be at least one test
    }

    @Ignore("b/72153742")
    @Test
    public void testCancel() {
        controller.menuHelper.assertPressDvrLibrary();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
        controller.assertHas(Constants.MENU, false);
    }

    @Ignore("b/72153742")
    @Test
    public void testEmptyLibrary() {
        controller.menuHelper.assertPressDvrLibrary();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));

        // DVR Library is empty, only Scheduled row and Full schedule card should be displayed.
        controller.assertHas(mRecentRow, false);
        controller.assertHas(mScheduledRow, true);
        controller.assertHas(mSeriesRow, false);

        controller.pressDPadCenter();
        assertWaitUntilFocused(controller.getUiDevice(), mFullScheduleCard);
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));

        // Empty schedules screen should be shown.
        controller.assertHas(Constants.DVR_SCHEDULES, true);
        controller.assertHas(
                ByResource.text(
                        controller.getTargetResources(), R.string.dvr_schedules_empty_state),
                true);

        // Close the DVR library.
        controller.pressBack();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
    }

    @Ignore("b/72153742")
    @Test
    public void testScheduleRecordings() {
        BySelector newScheduleCard =
                By.focusable(true)
                        .hasDescendant(By.textStartsWith(PROGRAM_NAME_PREFIX))
                        .hasDescendant(By.textEndsWith("today"));
        BySelector seriesCardWithOneSchedule =
                By.focusable(true)
                        .hasDescendant(By.textStartsWith(PROGRAM_NAME_PREFIX))
                        .hasDescendant(
                                By.text(
                                        controller
                                                .getTargetResources()
                                                .getQuantityString(
                                                        R.plurals.dvr_count_scheduled_recordings,
                                                        1,
                                                        1)));
        BySelector seriesCardWithOneRecordedProgram =
                By.focusable(true)
                        .hasDescendant(By.textStartsWith(PROGRAM_NAME_PREFIX))
                        .hasDescendant(
                                By.text(
                                        controller
                                                .getTargetResources()
                                                .getQuantityString(
                                                        R.plurals.dvr_count_new_recordings, 1, 1)));
        Pattern watchButton =
                Pattern.compile(
                        "^"
                                + controller
                                        .getTargetResources()
                                        .getString(R.string.dvr_detail_watch)
                                        .toUpperCase()
                                + "\n.*$");

        controller.menuHelper.showMenu();
        controller.menuHelper.assertNavigateToPlayControlsRow();
        controller.pressDPadRight();
        controller.pressDPadCenter();
        controller.assertWaitForCondition(
                Until.hasObject(
                        ByResource.text(
                                controller.getTargetResources(),
                                R.string.dvr_action_record_episode)));
        controller.pressDPadCenter();
        controller.assertWaitForCondition(
                Until.gone(
                        ByResource.text(
                                controller.getTargetResources(),
                                R.string.dvr_action_record_episode)));

        controller.menuHelper.assertPressDvrLibrary();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));

        // Schedule should be automatically added to the series.
        controller.assertHas(mRecentRow, false);
        controller.assertHas(mScheduledRow, true);
        controller.assertHas(mSeriesRow, true);
        String programName =
                controller
                        .getUiDevice()
                        .findObject(By.textStartsWith(PROGRAM_NAME_PREFIX))
                        .getText();

        // Move to scheduled row, there should be one new schedule and one full schedule card.
        controller.pressDPadRight();
        controller.assertWaitUntilFocused(newScheduleCard);
        controller.pressDPadRight();
        controller.assertWaitUntilFocused(mFullScheduleCard);

        // Enters the full schedule, there should be one schedule in the full schedule.
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
        controller.assertHas(Constants.DVR_SCHEDULES, true);
        controller.assertHas(
                ByResource.text(
                        controller.getTargetResources(), R.string.dvr_schedules_empty_state),
                false);
        controller.assertHas(By.textStartsWith(programName), true);

        // Moves to the series card, clicks it, the detail page should be shown with "View schedule"
        // button.
        controller.pressBack();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.pressDPadLeft();
        controller.assertWaitUntilFocused(newScheduleCard);
        controller.pressDPadDown();
        controller.assertWaitUntilFocused(seriesCardWithOneSchedule);
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_view_schedule)
                                .toUpperCase()),
                true);
        controller.assertHas(By.text(watchButton), false);
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_series_delete)
                                .toUpperCase()),
                false);

        // Clicks the new schedule, the detail page should be shown with "Stop recording" button.
        controller.pressBack();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.assertWaitUntilFocused(seriesCardWithOneSchedule);
        controller.pressDPadUp();
        controller.assertWaitUntilFocused(newScheduleCard);
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_stop_recording)
                                .toUpperCase()),
                true);

        // Stops the recording
        controller.pressDPadCenter();
        controller.assertWaitForCondition(
                Until.hasObject(
                        ByResource.text(
                                controller.getTargetResources(), R.string.dvr_action_stop)));
        controller.pressDPadCenter();
        controller.assertWaitForCondition(
                Until.gone(
                        ByResource.text(
                                controller.getTargetResources(), R.string.dvr_action_stop)));
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.assertWaitUntilFocused(mFullScheduleCard);

        // Moves to series' detail page again, now it should have two more buttons
        controller.pressDPadDown();
        controller.assertWaitUntilFocused(seriesCardWithOneRecordedProgram);
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
        controller.assertHas(By.text(watchButton), true);
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_view_schedule)
                                .toUpperCase()),
                true);
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_series_delete)
                                .toUpperCase()),
                true);

        // Moves to the recent row and clicks the recent recorded program.
        controller.pressBack();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.assertWaitUntilFocused(seriesCardWithOneRecordedProgram);
        controller.pressDPadUp();
        controller.assertWaitUntilFocused(mFullScheduleCard);
        controller.pressDPadUp();
        controller.assertWaitUntilFocused(By.focusable(true).hasDescendant(By.text(programName)));
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_watch)
                                .toUpperCase()),
                true);
        controller.assertHas(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_delete)
                                .toUpperCase()),
                true);

        // Moves to the delete button and clicks to remove the recorded program.
        controller.pressDPadRight();
        controller.assertWaitUntilFocused(
                By.text(
                        controller
                                .getTargetResources()
                                .getString(R.string.dvr_detail_delete)
                                .toUpperCase()));
        controller.pressDPadCenter();
        controller.assertWaitForCondition(Until.hasObject(Constants.DVR_LIBRARY));
        controller.assertWaitUntilFocused(mFullScheduleCard);

        // DVR Library should be empty now.
        controller.assertHas(mRecentRow, false);
        controller.assertHas(mScheduledRow, true);
        controller.assertHas(mSeriesRow, false);

        // Close the DVR library.
        controller.pressBack();
        controller.assertWaitForCondition(Until.gone(Constants.DVR_LIBRARY));
    }
}
