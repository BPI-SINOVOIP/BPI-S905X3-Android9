/*
 * Copyright (C) 2018 The Android Open Source Project
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
 * limitations under the License
 */
package com.android.tv.testing.robo;

import android.content.ContentProvider;
import android.content.pm.ProviderInfo;
import org.robolectric.Robolectric;
import org.robolectric.android.controller.ContentProviderController;
import org.robolectric.shadows.ShadowContentResolver;

/** Static utilities for using content providers in tests. */
public final class ContentProviders {

    /** Builds creates and register a ContentProvider with the given authority. */
    public static <T extends ContentProvider> T register(Class<T> providerClass, String authority) {
        ProviderInfo info = new ProviderInfo();
        info.authority = authority;
        ContentProviderController<T> contentProviderController =
                Robolectric.buildContentProvider(providerClass);
        T provider = contentProviderController.create(info).get();
        provider.onCreate();
        ShadowContentResolver.registerProviderInternal(authority, provider);
        return provider;
    }

    private ContentProviders() {}
}
