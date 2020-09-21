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

package com.android.settings.applications.defaultapps;

import android.content.Context;
import android.os.UserHandle;

import com.android.settings.Utils;


public class DefaultWorkBrowserPreferenceController extends DefaultBrowserPreferenceController {

    public static final String KEY = "work_default_browser";
    private final UserHandle mUserHandle;

    public DefaultWorkBrowserPreferenceController(Context context) {
        super(context);
        mUserHandle = Utils.getManagedProfile(mUserManager);
        if (mUserHandle != null) {
            mUserId = mUserHandle.getIdentifier();
        }
    }

    @Override
    public String getPreferenceKey() {
        return KEY;
    }

    @Override
    public boolean isAvailable() {
        if (mUserHandle == null) {
            return false;
        }
        return super.isAvailable();
    }
}
