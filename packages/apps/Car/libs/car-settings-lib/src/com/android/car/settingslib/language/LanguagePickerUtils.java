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

import android.app.ActivityManager;
import android.content.Context;
import android.os.RemoteException;
import android.support.annotation.Nullable;

import com.android.internal.app.LocaleHelper;
import com.android.internal.app.LocaleStore;
import com.android.internal.app.SuggestedLocaleAdapter;

import java.util.Locale;
import java.util.Set;

/**
 * Utility class that supports the language/locale picker flows.
 */
public final class LanguagePickerUtils {
    /**
     * Creates an instance of {@link SuggestedLocaleAdapter} with a locale
     * {@link LocaleStore.LocaleInfo} that is scoped to a parent locale if a parent locale is
     * provided.
     */
    public static SuggestedLocaleAdapter createSuggestedLocaleAdapter(
            Context context,
            Set<LocaleStore.LocaleInfo> localeInfoSet,
            @Nullable LocaleStore.LocaleInfo parent) {
        boolean countryMode = (parent != null);
        Locale displayLocale = countryMode ? parent.getLocale() : Locale.getDefault();
        SuggestedLocaleAdapter adapter = new SuggestedLocaleAdapter(localeInfoSet, countryMode);
        LocaleHelper.LocaleInfoComparator comp =
                new LocaleHelper.LocaleInfoComparator(displayLocale, countryMode);
        adapter.sort(comp);
        adapter.setDisplayLocale(context, displayLocale);
        return adapter;
    }

    /**
     * Returns the locale from current system configuration, or the default locale if no system
     * locale is available.
     */
    public static Locale getConfiguredLocale() {
        try {
            Locale configLocale =
                    ActivityManager.getService().getConfiguration().getLocales().get(0);
            return configLocale != null ? configLocale : Locale.getDefault();
        } catch (RemoteException e) {
            return Locale.getDefault();
        }
    }

    private LanguagePickerUtils() {}
}
