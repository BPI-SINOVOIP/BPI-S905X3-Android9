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

package android.platform.helpers;

import java.io.IOException;

public interface IGoogleFitHelper extends IAppHelper {
    /**
     * Setup expectations: Google Fit is open and idle.
     *
     * This method will open the "Home" page of Fit
     */
    public void goToHome();

    /**
     * Setup expectations: Google Fit is open and idle.
     *
     * This method will open the "Timeline" page of Fit
     */
    public void goToTimeline();

    /**
     * Setup expectations: Google Fit is open and idle.
     *
     * This method will open the "Challenges" page of Fit
     */
    public void goToChallenges();

    /**
     * Setup expectations: Google Fit is open and idle on the home page.
     *
     * The method will add a goal with the supplied characteristics and return to the home page
     * @param mode the goal type by steps (1), duration (2), or run times (3)
     * @param value the method will set values for each type of goal
     */
    public void addGoal(int mode, int value);

    /**
     * Setup expectations: Google Fit is open and idle on the home page or timeline page.
     *
     * This method will add an untracked activity record and set descriptions
     * @param activity case-insensitive activity types, like: running, biking, walking
     * @param duration value of time duration in minutes
     * @param calories value of Calories
     * @param steps value of Steps
     * @param distance value of Distance in mile
     */
    public void addActivity(String activity, int duration, int calories,
                                     int steps, int distance);
    /**
     * Setup expectations: Google Fit is open and idle on the home page or timeline page.
     *
     * This method will select an activity type and start an activity session
     * @param activity case-insensitive activity types, like: running, biking, walking
     */
    public void startActivity(String activity) throws IOException;

    /**
     * Setup expectations: An activity session is started.
     *
     * This method will pause the current activity and end on the activity page.
     */
    public void pauseActivity();

    /**
     * Setup expectations: An activity session is paused.
     *
     * This method will resume the current paused activity and end on the activity page.
     */
    public void resumeActivity();

    /**
     * Setup expectations: An activity session is paused.
     *
     * This method will terminate and save the current activity and end on the activity summary page
     */
    public void saveActivity();

    /**
     * Setup expectations: An activity session is started.
     *
     * This method will stop the current activity and end on the activity page.
     */
    public void stopActivity() throws IOException;

    /**
     * Setup expectation: On the summary page
     *
     * Check summary after an activity
     */
    public void checkSummary();
}
