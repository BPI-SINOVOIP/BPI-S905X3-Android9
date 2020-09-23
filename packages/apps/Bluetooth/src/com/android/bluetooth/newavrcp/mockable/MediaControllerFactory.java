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

import android.content.Context;
import android.media.session.MediaSession;

import com.android.internal.annotations.VisibleForTesting;

/**
 * Provide a method to inject custom MediaController objects for testing. By using the factory
 * methods instead of calling the constructor of MediaController directly, we can inject a custom
 * MediaController that can be used with JUnit and Mockito to set expectations and validate
 * behaviour in tests.
 */
public final class MediaControllerFactory {
    private static MediaController sInjectedController;

    static MediaController wrap(android.media.session.MediaController delegate) {
        if (sInjectedController != null) return sInjectedController;
        return (delegate != null) ? new MediaController(delegate) : null;
    }

    static MediaController make(Context context, MediaSession.Token token) {
        if (sInjectedController != null) return sInjectedController;
        return new MediaController(context, token);
    }

    @VisibleForTesting
    static void inject(MediaController controller) {
        sInjectedController = controller;
    }
}
