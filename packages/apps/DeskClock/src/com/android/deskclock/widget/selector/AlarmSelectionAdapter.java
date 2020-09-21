/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.deskclock.widget.selector;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.android.deskclock.R;
import com.android.deskclock.data.DataModel;
import com.android.deskclock.data.Weekdays;
import com.android.deskclock.provider.Alarm;
import com.android.deskclock.widget.TextTime;

import java.util.Calendar;
import java.util.List;

public class AlarmSelectionAdapter extends ArrayAdapter<AlarmSelection> {

    public AlarmSelectionAdapter(Context context, int id, List<AlarmSelection> alarms) {
        super(context, id, alarms);
    }

    @Override
    public @NonNull View getView(int position, @Nullable View convertView,
            @NonNull ViewGroup parent) {
        final Context context = getContext();
        View row = convertView;
        if (row == null) {
            final LayoutInflater inflater = LayoutInflater.from(context);
            row = inflater.inflate(R.layout.alarm_row, parent, false);
        }

        final AlarmSelection selection = getItem(position);
        final Alarm alarm = selection.getAlarm();

        final TextTime alarmTime = (TextTime) row.findViewById(R.id.digital_clock);
        alarmTime.setTime(alarm.hour, alarm.minutes);

        final TextView alarmLabel = (TextView) row.findViewById(R.id.label);
        alarmLabel.setText(alarm.label);

        // find days when alarm is firing
        final String daysOfWeek;
        if (!alarm.daysOfWeek.isRepeating()) {
            daysOfWeek = Alarm.isTomorrow(alarm, Calendar.getInstance()) ?
                    context.getResources().getString(R.string.alarm_tomorrow) :
                    context.getResources().getString(R.string.alarm_today);
        } else {
            final Weekdays.Order weekdayOrder = DataModel.getDataModel().getWeekdayOrder();
            daysOfWeek = alarm.daysOfWeek.toString(context, weekdayOrder);
        }

        final TextView daysOfWeekView = (TextView) row.findViewById(R.id.daysOfWeek);
        daysOfWeekView.setText(daysOfWeek);

        return row;
    }
}