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

public interface IAutoMediaHelper extends IAppHelper {
    /**
     * Setup expectations: media app is open
     *
     * This method is used to play media.
     *
     */
    void playMedia();

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to pause media.
     *
     */
    void pauseMedia();

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to select next track.
     *
     */
    void clickNextTrack();

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to select previous track.
     *
     */
    void clickPreviousTrack();

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to shuffle tracks.
     *
     */
    void clickShuffleAll();

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to click on nth instance among the visible menu items
     *
     * @param - instance is the index of the menu item (starts from 0)
     */
    void clickMenuItem(int instance);

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to open Folder Menu with menuOptions.
     * Example - openMenu->Folder->Mediafilename->trackName
     *           openMenuWith(Folder,mediafilename,trackName);
     *
     * @param - menuOptions used to pass multiple level of menu options in one go.
     *
     */
    void openMenuWith(String... menuOptions);

    /**
     * Setup expectations: media app is open.
     *
     * This method is used to used to open mediafilename from now playing list.
     *
     *  @param - trackName - media to be played.
     */
    void openNowPlayingWith(String trackName);

    /**
     * Setup expectations: Radio app is open.
     *
     * @return to get current playing track name.
     */
    String getMediaTrackName();

}
