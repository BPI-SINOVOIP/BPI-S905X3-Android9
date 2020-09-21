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

public interface IDownloadsHelper extends IAppHelper {

    public static enum Category {
        AUDIO,
        IMAGES,
        RECENT,
        VIDEOS
    }

    /**
     * Setup expectation: Downloads app's Navigation Drawer is open
     * <p>
     * This method will select an item from the navigation drawer's list
     *
     * @param category - menu item to select (click)
     */
    public void selectMenuCategory(Category category);

    /**
     * Setup expectation: Item has been selected from Navigation Drawer's list
     * <p>
     * This method opens a directory from the directories list
     *
     * @param directoryName - name of directory to open
     */
    public void selectDirectory(String directoryName);

    /**
     * Setup expectation: Navigated to the right folder
     * <p>
     * This method clicks a specific file with name 'filename'
     *
     * @param filename - name of file to open
     */
    public void openFile(String filename);

    /**
     * Setup expectation: Video is playing
     * <p>
     * This method will wait for the video to stop playing or until timeoutInSeconds occur,
     * whichever comes first. Function will just exit, no test failure in either case.
     *
     * @param timeoutInSeconds - timeout value in seconds the test will wait for video to end
     */
    public void waitForVideoToStopPlaying(long timeoutInSeconds);

    /**
     * Setup expectation: Audio is playing
     * <p>
     * This method will wait for the audio to stop playing or until timeoutInSeconds occur,
     * whichever comes first. Function will just exit, no test failure in either case.
     *
     * @param timeoutInSeconds - timeout value in seconds the test will wait for audio to end
     */
    public void waitForAudioToStopPlaying(long timeoutInSeconds);

    /**
     * Setup expectation: Video is playing
     * <p>
     * This method will enable or disable video looping. It will bring up the options menu and
     * check the "Loop video" option
     *
     * @param enableVideoLooping - true for continuous looping video, false for not looping video
     */
    public void enableVideoLooping(boolean enableVideoLooping);
}
