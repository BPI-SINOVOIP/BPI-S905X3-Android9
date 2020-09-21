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
 * limitations under the License
 */

package com.android.car.settings.datetime;

import android.annotation.NonNull;
import android.app.AlarmManager;
import android.content.Context;

import androidx.car.widget.TextListItem;

import com.android.settingslib.datetime.ZoneGetter;

import java.util.Map;

/**
 * A LineItem that displays available time zone.
 */
class TimeZoneLineItem extends TextListItem {

    public interface TimeZoneChangeListener {
        void onTimeZoneChanged();
    }

    public TimeZoneLineItem(
            Context context,
            @NonNull TimeZoneChangeListener listener,
            Map<String, Object> timeZone) {
        super(context);
        setTitle(timeZone.get(ZoneGetter.KEY_DISPLAYNAME).toString());
        setBody(timeZone.get(ZoneGetter.KEY_GMT).toString());
        setOnClickListener(v -> {
            AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
            am.setTimeZone((String) timeZone.get(ZoneGetter.KEY_ID));
            listener.onTimeZoneChanged();
        });
    }
}
