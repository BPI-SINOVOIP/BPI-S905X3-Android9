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

public interface IAutoOverviewHelper extends IAppHelper {
    /**
     * Setup expectations: in home (overview) screen.
     *
     * This method is used to open settings.
     */
    void openSettings();

    /**
     * Setup expectations: in home (overview) screen.
     *
     * This method is used to start voice assistant.
     */
    void startVoiceAssistant();

    /**
     * Setup expectations:
     * 1.Play media from Media player/ Radio.
     * 2.Select Home button.
     * 3.Media/Radio card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to click next track/next station on Media/Radio card.
     */
    void clickNextTrack();

    /**
     * Setup expectations:
     * 1.Play media from Media player/ Radio.
     * 2.Select Home button.
     * 3.Media/Radio card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to click previous track/previous station on Media/Radio card.
     */
    void clickPreviousTrack();

    /**
     * Setup expectations:
     * 1.Play media from Media player/ Radio.
     * 2.Select Home button.
     * 3.Media/Radio card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to play media on Media/Radio card.
     */
    void playMedia();

    /**
     * Setup expectations:
     * 1.Play media from Media player/ Radio.
     * 2.Select Home button.
     * 3.Media/Radio card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to pause media on Media/Radio card.
     */
    void pauseMedia();

    /**
     * Setup expectations:
     * 1.Play media from Media player/ Radio.
     * 2.Select Home button.
     * 3.Media/Radio card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to open active media card app.
     * ( Radio,Bluetooth Media, Local Media player).
     */
    void openMediaApp();

    /**
     * Setup expectations:
     * 1.Dial call from Dial app .
     * 2.Select Home button.
     * 3.Dial card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to end call on Dial card.
     */
    void endCall();

    /**
     * Setup expectations:
     * 1.Dial call from Dial app .
     * 2.Select Home button.
     * 3.Dial card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to mute on going call.
     */
    void muteCall();

    /**
     * Setup expectations:
     * 1.Dial call from Dial app and end call.
     * 2.Select Home button.
     * 3.Recent Dial card shown in home (overview) screen.
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to dial recent call activity.
     */
    void dialRecentCall();

    /**
     * Setup expectations:
     * 1.Play media from Media player
     * 2.Select Home button.
     * 3.Media card shown on Home screen
     *
     * else throws UnknownUiException if element not found.
     *
     * This method is used to get the media-title/trackname on the media card.
     *
     * @return current media-title/trackname
     */
    String getMediaTitle();
}
