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

import android.content.Context;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.lifecycle.Lifecycle;

public class ZenModeBehaviorMsgEventReminderPreferenceController extends
        AbstractZenModePreferenceController implements PreferenceControllerMixin {

    protected static final String KEY_BEHAVIOR_SETTINGS = "zen_mode_msg_event_reminder_settings";
    private final ZenModeSettings.SummaryBuilder mSummaryBuilder;

    public ZenModeBehaviorMsgEventReminderPreferenceController(Context context,
            Lifecycle lifecycle) {
        super(context, KEY_BEHAVIOR_SETTINGS, lifecycle);
        mSummaryBuilder = new ZenModeSettings.SummaryBuilder(context);
    }

    @Override
    public String getPreferenceKey() {
        return KEY_BEHAVIOR_SETTINGS;
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public void updateState(Preference preference) {
        super.updateState(preference);

        preference.setSummary(mSummaryBuilder.getMsgEventReminderSettingSummary(getPolicy()));
    }
}
