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

package com.android.settings.applications;

import android.content.Context;
import android.support.annotation.VisibleForTesting;

import com.android.settings.core.BasePreferenceController;
import com.android.settings.R;

public class EnabledVrListenersController extends BasePreferenceController {

    @VisibleForTesting static final String KEY_ENABLED_VR_LISTENERS = "enabled_vr_listeners";

    public EnabledVrListenersController(Context context) {
        super(context, KEY_ENABLED_VR_LISTENERS);
    }

    @AvailabilityStatus
    public int getAvailabilityStatus() {
        return mContext.getResources().getBoolean(R.bool.config_show_enabled_vr_listeners)
                ? AVAILABLE
                : UNSUPPORTED_ON_DEVICE;
    }
}