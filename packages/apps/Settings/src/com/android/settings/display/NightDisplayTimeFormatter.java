/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.settings.display;

import android.content.Context;

import com.android.internal.app.ColorDisplayController;
import com.android.settings.R;

import java.text.DateFormat;
import java.time.LocalTime;
import java.util.Calendar;
import java.util.TimeZone;

public class NightDisplayTimeFormatter {

    private DateFormat mTimeFormatter;

    NightDisplayTimeFormatter(Context context) {
        mTimeFormatter = android.text.format.DateFormat.getTimeFormat(context);
        mTimeFormatter.setTimeZone(TimeZone.getTimeZone("UTC"));
    }

    public String getFormattedTimeString(LocalTime localTime) {
        final Calendar c = Calendar.getInstance();
        c.setTimeZone(mTimeFormatter.getTimeZone());
        c.set(Calendar.HOUR_OF_DAY, localTime.getHour());
        c.set(Calendar.MINUTE, localTime.getMinute());
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        return mTimeFormatter.format(c.getTime());
    }

    public String getAutoModeTimeSummary(Context context, ColorDisplayController controller) {
        final int summaryFormatResId = controller.isActivated() ? R.string.night_display_summary_on
                : R.string.night_display_summary_off;
        return context.getString(summaryFormatResId, getAutoModeSummary(context, controller));
    }

    private String getAutoModeSummary(Context context, ColorDisplayController controller) {
        final boolean isActivated = controller.isActivated();
        final int autoMode = controller.getAutoMode();
        if (autoMode == ColorDisplayController.AUTO_MODE_CUSTOM) {
            if (isActivated) {
                return context.getString(R.string.night_display_summary_on_auto_mode_custom,
                        getFormattedTimeString(controller.getCustomEndTime()));
            } else {
                return context.getString(R.string.night_display_summary_off_auto_mode_custom,
                        getFormattedTimeString(controller.getCustomStartTime()));
            }
        } else if (autoMode == ColorDisplayController.AUTO_MODE_TWILIGHT) {
            return context.getString(isActivated
                    ? R.string.night_display_summary_on_auto_mode_twilight
                    : R.string.night_display_summary_off_auto_mode_twilight);
        } else {
            return context.getString(isActivated
                    ? R.string.night_display_summary_on_auto_mode_never
                    : R.string.night_display_summary_off_auto_mode_never);
        }
    }
}
