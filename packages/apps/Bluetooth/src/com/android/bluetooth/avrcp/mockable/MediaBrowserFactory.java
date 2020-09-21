/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.bluetooth.avrcp;

import android.content.ComponentName;
import android.content.Context;
import android.os.Bundle;

import com.android.internal.annotations.VisibleForTesting;

/**
 * Provide a method to inject custom MediaBrowser objects for testing. By using the factory
 * methods instead of calling the constructor of MediaBrowser directly, we can inject a custom
 * MediaBrowser that can be used with JUnit and Mockito to set expectations and validate
 * behaviour in tests.
 */
public final class MediaBrowserFactory {
    private static MediaBrowser sInjectedBrowser;

    static MediaBrowser wrap(android.media.browse.MediaBrowser delegate) {
        if (sInjectedBrowser != null) return sInjectedBrowser;
        return (delegate != null) ? new MediaBrowser(delegate) : null;
    }

    static MediaBrowser make(Context context, ComponentName serviceComponent,
            MediaBrowser.ConnectionCallback callback, Bundle rootHints) {
        if (sInjectedBrowser != null) {
            sInjectedBrowser.testInit(context, serviceComponent, callback, rootHints);
            return sInjectedBrowser;
        }
        return new MediaBrowser(context, serviceComponent, callback, rootHints);
    }

    @VisibleForTesting
    static void inject(MediaBrowser browser) {
        sInjectedBrowser = browser;
    }
}
