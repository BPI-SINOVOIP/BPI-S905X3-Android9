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

public interface IContactsHelper extends IAppHelper {
    /**
     * Setup expectation: Contacts app is open
     *
     * Scroll to contact name and click
     */
    public void selectContact(String contactName);

    /**
     * Setup expectation: Contact is open
     *
     * Click the call button
     */
    public void call();

    /**
     * Setup expectation: Contact is open
     *
     * Click text button, send text message from quick reply list
     */
    public void sendText(String quickReplyText);
}
