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

package com.android.documentsui;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import com.android.documentsui.prefs.ScopedAccessLocalPreferences;

/**
 * Clean up {@link ScopedAccessLocalPreferences} when packages are removed.
 */
public class ScopedAccessPackageReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        final Uri data = intent.getData();
        final String packageName = data == null ? null : data.getSchemeSpecificPart();

        if (Intent.ACTION_PACKAGE_FULLY_REMOVED.equals(action)) {
            if (packageName != null) {
                ScopedAccessLocalPreferences.clearPackagePreferences(context, packageName);
            }
        } else if (Intent.ACTION_PACKAGE_DATA_CLEARED.equals(action)) {
            if (packageName != null) {
                ScopedAccessLocalPreferences.clearPackagePreferences(context, packageName);
            }
        }
    }
}
