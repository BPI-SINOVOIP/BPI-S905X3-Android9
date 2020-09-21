
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

public interface IAutoRadioHelper extends IAppHelper {
    /**
     * Setup expectations: Radio app is open
     *
     * This method is used to play Radio.
     */
    void playRadio();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to pause radio.
     */
    void pauseRadio();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to select next station.
     */
    void clickNextStation();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to select previous station.
     */
    void clickPreviousStation();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to save current station.
     */
    void saveCurrentStation();

    /**
     * Setup expectations: Radio app is open
     *
     * This method is used to unsave current station.
     */
    void unsaveCurrentStation();

    /**
     * Setup expectations: Radio app is open
     *
     * This method is used to open saved station list.
     */
    void openSavedStationList();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to select AM from menu.
     */
    void clickAmFromMenu();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to select FM from menu.
     */
    void clickFmFromMenu();

    /**
     * Setup expectations: Radio app is open.
     *
     * This method is used to tune station manually.
     *
     * @param stationType - to select AM or FM.
     * @param band        - band to tune in.
     */
    void setStation(String stationType, double band);

    /**
     * Setup expectations: Radio app is open.
     *
     * @return to get current playing station band with Am or Fm.
     */
    String getStationBand();

    /**
     * Setup expectations: Radio app is open and
     * Favourite/saved station list should be open.
     *
     * This method is used to pause radio from current station card
     */
    void pauseCurrentStationCard();

    /**
     * Setup expectations: Radio app is open and
     * Favourite/saved station list should be open
     *
     * This method is used to play radio from current station card
     */
    void playCurrentStationCard();

    /**
     * Setup expectations: Radio app is open and
     * Favourite/saved station list should be open
     *
     * This method is used to exit current station card
     */
    void exitCurrentStationCard();

    /**
     * Setup expectations: Radio app is open and
     * Favourite/saved station list should be open
     *
     * This method is used to find and play the provided channelName from the list of saved stations
     *
     * @param channelName : the channel name to be played
     */
    void playFavoriteStation(String channelName);
}
