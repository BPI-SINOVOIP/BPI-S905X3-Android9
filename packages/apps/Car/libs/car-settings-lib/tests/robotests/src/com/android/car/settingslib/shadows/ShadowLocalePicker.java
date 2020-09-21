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

package com.android.car.settingslib.shadows;

import android.content.Context;

import com.android.internal.app.LocalePicker;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.Resetter;

import java.util.Locale;


/**
 * Shadow class for {@link LocalePicker} intended to help provide a defined set of locales that can
 * be set and changed within the test.
 */
@Implements(LocalePicker.class)
public class ShadowLocalePicker {

    static Locale sUpdatedLocale;
    private static String[] sSystemAssetLocales;
    private static String[] sSupportedLocales;

    @Implementation
    public static void updateLocale(Locale locale) {
        sUpdatedLocale = locale;
    }

    @Implementation
    public static String[] getSystemAssetLocales() {
        if (sSystemAssetLocales == null) {
            return new String[0];
        }
        return sSystemAssetLocales;
    }

    @Implementation
    public static String[] getSupportedLocales(Context context) {
        if (sSupportedLocales == null) {
            return new String[0];
        }
        return sSupportedLocales;
    }

    public static void setSystemAssetLocales(String... locales) {
        sSystemAssetLocales = locales;
    }

    public static void setSupportedLocales(String... locales) {
        sSupportedLocales = locales;
    }

    @Resetter
    public static void reset() {
        sUpdatedLocale = null;
        sSystemAssetLocales = null;
        sSupportedLocales = null;
    }
}
