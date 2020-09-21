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

import android.support.annotation.Nullable;
import android.support.test.uiautomator.Direction;

/**
 * An interface for interacting with any launcher.
 * TODO(b/77278231): Add default methods for quickstep.
 */
public interface ILauncherHelper extends IAppHelper {
    /**
     * Swipes the home page in the specified direction.
     *
     * @param dir the {@link Direction} to swipe the home page
     * @return true if swiping was possible, false otherwise
     * @throws IllegalArgumentException if the direction is UP or DOWN
     */
    public boolean swipeHome(Direction dir);

    /**
     * Opens the all apps container from the launcher home.
     */
    public void openAllApps();

    /**
     * Scrolls the all apps container in the specified direction.
     *
     * @param dir the {@link Direction} to swipe the container
     * @return true if scrolling was possible, false otherwise
     * @throws IllegalArgumentException if all apps cannot be scrolled that direction
     * @throws IllegalStateException if all apps is not already open
     */
    public boolean scrollAllApps(Direction dir);

    /**
     * Launches an application to the foreground.
     *
     * @param name the visible app name in the launcher
     * @param pkg if specified, the expected app package after launch
     */
    public void launchApp(String name, @Nullable String pkg);

    /**
     * Opens the quickstep view from any screen.
     */
    public void openQuickstep();

    /**
     * Scrolls the quickstep view in the specified direction.
     *
     * @param dir the {@link Direction} to swipe the view.
     * @return true if scrolling was possible, false otherwise
     * @throws IllegalArgumentException if the direction is UP or DOWN
     * @throws IllegalStateException if quickstep is not already open
     */
    public void scrollQuickstep(Direction dir);

    /**
     * Selects a specific app from the quickstep view.
     *
     * @param app the app name to reopen, based on ...
     * @return true if the app was found, false otherwise
     * @throws IllegalStateException if quickstep is not already open
     */
    public boolean selectQuickstepApp(String app);
}
