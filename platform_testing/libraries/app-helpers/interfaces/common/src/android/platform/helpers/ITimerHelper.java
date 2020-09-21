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

package android.platform.helpers;

public interface ITimerHelper extends IAppHelper {
    /**
     * Setup expectation: Timer app is open on initial screen
     *
     * Create a new timer
     */
    public void createTimer();

    /**
     * Setup expectation: Timer is running
     *
     * Pause timer
     */
    public void pauseTimer();

    /**
     * Setup expectation: Timer is paused
     *
     * Resume timer
     */
    public void resumeTimer();

    /**
     * Setup expectation: Timer is paused
     *
     * Reset timer
     */
    public void resetTimer();

    /**
     * Setup expectation: Timer is paused
     *
     * Dismiss timer
     */
    public void dismissTimer();

    /**
     * Setup expectation: Timer app is open on initial screen
     *
     * Delete timer from list
     */
    public void deleteTimer();
}
