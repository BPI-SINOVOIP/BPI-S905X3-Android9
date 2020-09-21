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

package android.autofillservice.cts.common;

import static android.autofillservice.cts.common.SettingsHelper.NAMESPACE_GLOBAL;
import static android.autofillservice.cts.common.SettingsHelper.NAMESPACE_SECURE;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Helper used to block tests until a secure settings value has been updated.
 */
public final class OneTimeSettingsListener extends ContentObserver {
    private final CountDownLatch mLatch = new CountDownLatch(1);
    private final ContentResolver mResolver;
    private final String mKey;

    public OneTimeSettingsListener(Context context, String key) {
        this(context, NAMESPACE_SECURE, key);
    }

    public OneTimeSettingsListener(Context context, String namespace, String key) {
        super(new Handler(Looper.getMainLooper()));
        mKey = key;
        mResolver = context.getContentResolver();
        final Uri uri;
        switch (namespace) {
            case NAMESPACE_SECURE:
                uri = Settings.Secure.getUriFor(key);
                break;
            case NAMESPACE_GLOBAL:
                uri = Settings.Global.getUriFor(key);
                break;
            default:
                throw new IllegalArgumentException("invalid namespace: " + namespace);
        }
        mResolver.registerContentObserver(uri, false, this);
    }

    @Override
    public void onChange(boolean selfChange, Uri uri) {
        mResolver.unregisterContentObserver(this);
        mLatch.countDown();
    }

    /**
     * Blocks for a few seconds until it's called.
     *
     * @throws IllegalStateException if it's not called.
     */
    public void assertCalled() {
        try {
            final boolean updated = mLatch.await(5, TimeUnit.SECONDS);
            if (!updated) {
                throw new IllegalStateException("Settings " + mKey + " not called in 5s");
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new IllegalStateException("Interrupted", e);
        }
    }
}
