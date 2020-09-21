/*
 * Copyright (C) 2018 Google Inc.
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
package com.android.car.settingslib.language;

import com.android.internal.app.LocaleStore;

/**
 * Listener interface that facilitates notification of changes is locale selection.
 */
public interface LocaleSelectionListener {
    /**
     * Called when a language choice has been selected.
     */
    void onLocaleSelected(LocaleStore.LocaleInfo localeInfo);

    /**
     * Called when a parent locale that has at least 2 child locales is selected. The
     * expectation here is what the PagedList in the LanguagePickerFragment will be updated to
     * display the child locales.
     */
    void onParentWithChildrenLocaleSelected(LocaleStore.LocaleInfo localeInfo);
}
