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

public interface IPlayStoreHelper extends IAppHelper {
    /**
     * Setup expectations: The app is open.
     *
     * Looks for the search bar or button by scrolling up or pressing back. It then enters a query,
     * and displays the results. This method blocks until the results are selectable.
     */
    public void doSearch(String query);

    /**
     * Setup expectations: There are visible search results.
     *
     * Selects the first search result card and blocks until the app's install page is open.
     */
    public void selectFirstResult();

    /**
     * Setup expectations: An app's install page is open, but the app is not installed.
     *
     * Press the install button and dismiss any confirmation dialogs. This method will block until
     * the app starts downloading, though installation cannot be guaranteed.
     */
    public void installApp();

    /**
     * Setup expectations: An app's install page is open.
     *
     * @return true, if the app is already installed, or false if not.
     */
    public boolean isAppInstalled();

    /**
     * Setup expectations: An app's install page is open, and the app is installed.
     *
     * Press the uninstall button. This method will block until
     * the app completes uninstallation, though uninstallation cannot be guaranteed.
     */
    public void uninstallApp();
}
