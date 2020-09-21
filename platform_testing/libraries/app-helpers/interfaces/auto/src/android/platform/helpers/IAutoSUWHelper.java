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
 * Helper class for functional tests of SetupWizard
 */
public interface IAutoSUWHelper {

  /**
   * enum for Activity Type.
   */
  enum ActivityType{
    NETWORK_ACTIVITY("network.NetworkActivity"),
    CAR_SUW_ACTIVITY("CarSetupWizardActivity"),
    SIM_ACTIVITY("network.SimActivity"),
    WIFI_ACTIVITY("network.WifiActivity"),
    BLUETOOTH_ACTIVITY("bluetooth.BluetoothActivity"),
    OTA_ACTIVITY("update.OtaUpdateActivity"),
    DATETIME_ACTIVITY("datetime.DateTimeActivity"),
    NAME_ACTIVITY("user.NameActivity"),
    DONE_ACTIVITY("DoneActivity"),
    CHOOSE_LOCK_TYPE_ACTIVITY("security.ChooseLockTypeActivity");

    private final String activityName;

    ActivityType(String activityName) {
      this.activityName = activityName;
    }

    public String getActivityName() {
      return activityName;
    }
  }

  /**
   * enum for Enabling Disabling.
   */

  enum EnableDisable{
    ENABLE("enable"),
    DISABLE("disable");

    private final String value;

    EnableDisable(String value) {
      this.value = value;
    }

    public String getValue() {
      return value;
    }
  }

  /**
   * enum for Available Buttons.
   */

  enum ButtonID{
    PRIMARYTOOLBARBUTTON("primary_toolbar_button"),
    SECONDARYTOOLBARBUTTON("secondary_toolbar_button"),
    BACKBUTTON("back_button");

    private final String value;

    ButtonID(String value) {
      this.value = value;
    }

    public String getID() {
      return value;
    }
  }

  /**
   * enum for Available Button Texts.
   */

  enum ButtonText{
    STARTBUTTON("Start"),
    SKIPBUTTON("Skip"),
    NEXTBUTTON("Next");

    private final String value;

    ButtonText(String value) {
      this.value = value;
    }

    public String getText() {
      return value;
    }
  }

  /**
   * Setup expectations: The SUW app is open.
   *
   * @param activityType - activity to start.
   */
  void launchActivity(ActivityType activityType);

  /**
   * Setup expectations: The SUW app is open.
   *
   * @param buttonID - ID of the button which has to be clicked
   */
  void clickButton(ButtonID buttonID);

  /**
   * Setup expectations: The SUW app is open.
   *
   * @param buttonText - text of the button which has to be clicked
   */
  void clickText(ButtonText buttonText);

  /**
   * Setup expectations: The SUW app is open.
   *
   * @param activityType - activity whose required fields need to be verified
   */
  void verifyRequiredFields(ActivityType activityType);

  /**
   * Setup expectations: The SUW app is open and user is on Add Name screen
   *
   * @param name - User's name.
   */
  void addUserName(String name);

  /**
   * enables the setup wizard
   *
   * @param enableDisable - enum enableDisable to enable/disable
   */
  void enableDisableSUW(EnableDisable enableDisable);
}
