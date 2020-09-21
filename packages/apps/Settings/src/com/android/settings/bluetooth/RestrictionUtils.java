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

package com.android.settings.bluetooth;

import android.content.Context;
import android.os.UserHandle;

import com.android.settingslib.RestrictedLockUtils;
import com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;

/**
 * A utility class to aid testing.
 */
public class RestrictionUtils {

    public RestrictionUtils() {}

    /**
     *  Utility method to check if user restriction is enforced on the current user.
     *
     * <p> It helps with testing - override it to avoid calling static method which calls system
     * API.
     */
    public EnforcedAdmin checkIfRestrictionEnforced(Context context, String restriction) {
        return RestrictedLockUtils.checkIfRestrictionEnforced(
                context, restriction, UserHandle.myUserId());
    }

}
