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

public interface IAutoDialHelper extends IAppHelper {
    /**
     * Setup expectations: The app is open and the drawer is open
     *
     * This method is used to dial the phonenumber on dialpad
     *
     * @param phoneNumber  phone number to dial.
     */
    void dialANumber(String phoneNumber);

    /**
     * Setup expectations: The app is open and there is an ongoing call.
     *
     * This method is used to end call.
     */
    void endCall();

    /**
     * Setup expectations: The app is open and the drawer is open.
     *
     * This method is used to open call history details.
     */
    void openCallHistory();

    /**
     * Setup expectations: The app is open and the drawer is open.
     *
     * This method is used to open missed call details.
     */
    void openMissedCall();

    /**
     * Setup expectations: The app is open and in "Dial a number" drawer option
     *
     * This method is used to delete the number entered on dialpad using backspace
     */
    void deleteDialedNumber();

    /**
     * Setup expectations: The app is open and in "Dial a number" drawer option
     *
     * This method is used to get the number entered on dialpad
     */
    String getDialedNumber();

    /**
     * Setup expectations: The app is open and there is an ongoing call.
     *
     * This method is used to get the name of the contact for the ongoing call
     */
    String getDialedContactName();

    /**
     * Setup expectations: The app is open and phonenumber is entered on the dialpad
     *
     * This method is used to make a call
     *
     */
    void makeCall();

    /**
     * Setup expectations: The app is open
     *
     * This method is used to dial a number from call history, missed call[s], recent
     * call[s] list
     *
     * @param phoneNumber  phoneNumber to be dialed
     */
    void dialNumberFromList(String phoneNumber);

    /**
     * Setup expectations: The app is open and there is an ongoing call
     *
     * This method is used to enter number on the in-call dialpad
     */
    void inCallDialPad(String phoneNumber);

    /**
     * Setup expectations: The app is open and there is an ongoing call
     *
     * This method is used to mute the ongoing call
     */
    void muteCall();

    /**
     * Setup expectations: The app is open and there is an ongoing call
     *
     * This method is used to unmute the ongoing call
     */
    void unmuteCall();
}
