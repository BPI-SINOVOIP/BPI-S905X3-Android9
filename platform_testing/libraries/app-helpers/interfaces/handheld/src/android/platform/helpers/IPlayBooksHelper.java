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

public interface IPlayBooksHelper extends IAppHelper {

    /**
     * Setup expectations: PlayBooks is open on any screen.
     *
     * Navigates to "My Library" and selects the "ALL BOOKS" tab.
     */
    public void goToAllBooksTab();

    /**
     * Setup expectations: PlayBooks is open on "My Library - ALL BOOKS" screen.
     *
     * Selects the first Book and start reading.
     */
    public void openBook();

    /**
     * Setup expectations: PlayBooks is on a page of a book.
     *
     * Exits reading mode.
     */
    public void exitReadingMode();

    /**
     * Setup expectations: PlayBooks is on a full-screen page of a book.
     *
     * Goes to the next page by clicking the right side of the page.
     */
    public void goToNextPage();

    /**
     * Setup expectations: PlayBooks is on a full-screen page of a book.
     *
     * Goes to the previous page by clicking the left side of the page.
     */
    public void goToPreviousPage();

    /**
     * Setup expectations: PlayBooks is on a full-screen page of a book.
     *
     * Goes to the next page by scrolling leftwards.
     */
    public void scrollToNextPage();

    /**
     * Setup expectations: PlayBooks is on a full-screen page of a book.
     *
     * Goes to the previous page by scrolling rightwards.
     */
    public void scrollToPreviousPage();
}
