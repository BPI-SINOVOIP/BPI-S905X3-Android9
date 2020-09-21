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

package com.android.settings.notification;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.RestrictedSwitchPreference;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnResume;

public class DndPreferenceController extends NotificationPreferenceController
        implements PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String KEY_BYPASS_DND = "bypass_dnd";

    public DndPreferenceController(Context context, NotificationBackend backend) {
        super(context, backend);
    }

    @Override
    public String getPreferenceKey() {
        return KEY_BYPASS_DND;
    }

    @Override
    public boolean isAvailable() {
        if (!super.isAvailable() || mChannel == null) {
            return false;
        }
        return true;
    }

    public void updateState(Preference preference) {
        if (mChannel != null) {
            RestrictedSwitchPreference pref = (RestrictedSwitchPreference) preference;
            pref.setDisabledByAdmin(mAdmin);
            pref.setEnabled(isChannelConfigurable() && !pref.isDisabledByAdmin());
            pref.setChecked(mChannel.canBypassDnd());
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mChannel != null) {
            final boolean bypassZenMode = (Boolean) newValue;
            mChannel.setBypassDnd(bypassZenMode);
            mChannel.lockFields(NotificationChannel.USER_LOCKED_PRIORITY);
            saveChannel();
        }
        return true;
    }
}
