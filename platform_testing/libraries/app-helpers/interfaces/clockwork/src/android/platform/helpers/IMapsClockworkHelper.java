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

import android.platform.helpers.IMapsHelper;
import android.support.test.uiautomator.Direction;

public interface IMapsClockworkHelper extends IMapsHelper {
    /**
     * Setup expectation: On the standard Map screen in any setup.
     *
     * Click the location button
     */
    public abstract void clickLocationButton();

    /**
     * Setup expectation: On the standard Map screen in any setup.
     *
     * Click the zoom out button
     */
    public abstract void clickZoomOut();

    /**
     * Setup expectation: On the standard Map screen in any setup.
     *
     * Click the zoom in button
     */
    public abstract void clickZoomIn();

    /**
     * Setup expectation: On the standard Map screen in any setup.
     *
     * Click the search nearby button
     */
    public abstract void clickSearchNearby();

    /**
     * Setup expectation: On the standard Map screen in any setup.
     *
     * Scroll the map
     *
     * @param d direction to swipe
     */
    public abstract void swipeMap(Direction d);

    /**
     * Setup expectation: On the standard Map screen in any setup.
     *
     * Fling the map
     *
     * @param d direction to fling
     */
    public abstract void flingMap(Direction d);
}
