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

import android.support.test.uiautomator.Direction;

public interface IPhotosHelper extends IAppHelper {
    /**
     * Setup expectations: Photos is open and on the main or a device folder's screen.
     *
     * This method will check if there are videos that can be opened on current screen
     * by looking for any view objects with content-desc that starts with "Video"
     *
     * @return true if video clips are found, false otherwise
     */
    public boolean searchForVideoClip();

    /**
     * Setup expectations: Photos is open and on the main or a device folder's screen.
     *
     * This method will select the first clip to open and play. This will block until the clip
     * begins to plays.
     */
    public void openFirstClip();

    /**
     * Setup expectations: Photos is open and a clip is currently playing.
     *
     * This method will pause the current clip and block until paused.
     */
    public void pauseClip();

    /**
     * Setup expectations: Photos is open and a clip is currently paused in the foreground.
     *
     * This method will play the current clip and block until it is playing.
     */
    public void playClip();

    /**
     * Setup expectations: Photos is open.
     *
     * This method will go to the main screen.
     */
    public void goToMainScreen();

    /**
     * Setup expectations: Photos is open.
     *
     * This method will go to device folder screen.
     */
    public void goToDeviceFolderScreen();

    /**
     * Setup expectations:
     *   1. Photos is open
     *   2. on device folder screen
     *   3. the first device folder is shown on the screen
     *
     * This method will search for user-specified device folder in device folders.
     * If the device folder is found, the function will return with the device
     * folder on current screen.
     *
     * @param folderName  User-specified device folder name
     * @return true if device folder is found, false otherwise
     */
    public boolean searchForDeviceFolder(String folderName);

    /**
     * Setup expectations:
     *   1. Photos is open
     *   2. on device folder screen
     *   3. user-specified device folder is currently on screen
     *
     * This method will open the user-specified device folder.
     *
     * @param folderName  User-specified device folder name
     */
    public void openDeviceFolder(String folderName);

    /**
     * Setup expectations: Photos is open and on the main or a device folder's screen.
     *
     * This method will check if there are pictures that can be opened on current screen
     * by looking for any view objects with content-desc that starts with "Photo"
     *
     * @return true if pictures are found
     */
    public boolean searchForPicture();

    /**
     * Setup expectations: Photos is open and on the main or a device folder's screen.
     *
     * This method will open the picture at the specified index.
     *
     * @param index The index of the picture to open
     */
    public void openPicture(int index);

    /**
     * Setup expectations: Photos is open and a picture album is open.
     *
     * This method will scroll the picture album in the specified direction.
     *
     * @param direction The direction to scroll, must be LEFT or RIGHT.
     * @return Returns whether album can be still scrolled in the given direction
     */
    public boolean scrollAlbum(Direction direction);

    /**
     * Setup expectations: Photos is open and a picture folder is open.
     *
     * This method will scroll the Photos grid view in the specified direction.
     *
     * @param direction The direction of the scroll, must be UP or DOWN.
     * @return Returns whether the object can still scroll in the given direction
     */
    public boolean scrollGridView(Direction direction);
}
