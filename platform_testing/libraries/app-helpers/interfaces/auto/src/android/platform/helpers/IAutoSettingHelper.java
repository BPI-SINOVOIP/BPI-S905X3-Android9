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

/**
 * Helper class for functional tests of Settings facet
 */
public interface IAutoSettingHelper extends IAppHelper {

    /**
     * enum for changing(increasing, decreasing) value.
     */
    enum ChangeType{
        INCREASE,
        DECREASE
    }

     /**
     * Setup expectations: The app is open and the settings facet is open
     *
     * @param setting  option to open.
     */
    void openSetting(String setting);

    /**
     * Setup expectations: The app is open and wifi setting options is selected
     *
     * @param option to turn on/off wifi
     */
    void turnOnOffWifi(boolean turnOn);

    /**
     * Setup expectations: The app is open and bluetooth setting options is selected
     *
     * @param option to turn on/off bluetooth
     */
    void turnOnOffBluetooth(boolean turnOn);

    /**
     * Setup expectations: The app is open.
     *
     * Checks if the wifi is enabled.
     */
    boolean isWifiOn();

    /**
     * Setup expectations: The app is open.
     *
     * Checks if the bluetooth is enabled.
     */
    boolean isBluetoothOn();

    /**
     * Setup expectations: The app is open and the settings facet is open
     */
    void goBackToSettingsScreen();

    /**
     * Force stops the settings application
     */
    void stopSettingsApplication();

    /**
     * Setup expectations: settings app is open.
     *
     * This method is used to open Settings Menu with menuOptions.
     * Example - Settings->App info->Calandar->Permissions
     *           openMenuWith("App info", "Calandar", "Permissions");
     *
     * @param - menuOptions used to pass multiple level of menu options in one go.
     *
     */
    void openMenuWith(String... menuOptions);

    /**
     * Setup expectations: settings app is open and settings menu is selected
     *
     * Checks if the toggle switch for the given index is checked.
     * @param index of toggle switch.
     * index should be passed as 0 if only one toggle switch is present on screen.
     */
    boolean isToggleSwitchChecked(int index);

    /**
     * Setup expectations: settings app is open and settings menu is selected
     *
     * Clicks the toggle switch for the given index
     * @param index of toggle switch.
     * index should be passed as 0 if only one toggle switch is present on screen.
     */
    void clickToggleSwitch(int index);

    /**
     * Setup expectations: settings app is open.
     *
     * gets the value of the setting.
     * @param setting should be passed. example for setting is screen_brightness.
     */
    int getValue(String setting);

    /**
     * Setup expectations: settings app is open
     *
     * sets the value of the setting.
     * @param setting should be passed. example for setting is screen_brightness.
     */
    void setValue(String setting, int value);

    /**
     * Setup expectations: settings app is open and a seekbar is visible on the screen
     *
     * changes setting level of seekbar for the given index.
     * @param index of seekbar. should be passed as 0 if only one seekbar is present on screen.
     * @param changeType determines to increase or decrease the value of setting.
     */
    void changeSeekbarLevel(int index, ChangeType changeType);



}
