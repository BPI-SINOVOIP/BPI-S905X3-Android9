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

package com.android.settings.datetime.timezone;

import android.content.Context;
import android.icu.impl.OlsonTimeZone;
import android.icu.text.DateFormat;
import android.icu.text.DisplayContext;
import android.icu.text.SimpleDateFormat;
import android.icu.util.Calendar;
import android.icu.util.TimeZone;
import android.icu.util.TimeZoneTransition;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;

import com.android.settings.R;
import com.android.settingslib.widget.FooterPreference;

import java.util.Date;

public class TimeZoneInfoPreferenceController extends BaseTimeZonePreferenceController {

    private static final String PREFERENCE_KEY = FooterPreference.KEY_FOOTER;
    @VisibleForTesting
    Date mDate;
    private TimeZoneInfo mTimeZoneInfo;
    private final DateFormat mDateFormat;

    public TimeZoneInfoPreferenceController(Context context) {
        super(context, PREFERENCE_KEY);
        mDateFormat = DateFormat.getDateInstance(SimpleDateFormat.LONG);
        mDateFormat.setContext(DisplayContext.CAPITALIZATION_NONE);
        mDate = new Date();
    }

    @Override
    public int getAvailabilityStatus() {
        return AVAILABLE;
    }

    @Override
    public void updateState(Preference preference) {
        CharSequence formattedTimeZone = mTimeZoneInfo == null ? "" : formatInfo(mTimeZoneInfo);
        preference.setTitle(formattedTimeZone);
        preference.setVisible(mTimeZoneInfo != null);
    }

    public void setTimeZoneInfo(TimeZoneInfo timeZoneInfo) {
        mTimeZoneInfo = timeZoneInfo;
    }

    public TimeZoneInfo getTimeZoneInfo() {
        return mTimeZoneInfo;
    }

    private CharSequence formatOffsetAndName(TimeZoneInfo item) {
        String name = item.getGenericName();
        if (name == null) {
            if (item.getTimeZone().inDaylightTime(mDate)) {
                name = item.getDaylightName();
            } else {
                name = item.getStandardName();
            }
        }
        if (name == null) {
            return item.getGmtOffset().toString();
        } else {
            return SpannableUtil.getResourcesText(mContext.getResources(),
                    R.string.zone_info_offset_and_name, item.getGmtOffset(),
                    name);
        }
    }

    private CharSequence formatInfo(TimeZoneInfo item) {
        final CharSequence offsetAndName = formatOffsetAndName(item);
        final TimeZone timeZone = item.getTimeZone();
        if (!timeZone.observesDaylightTime()) {
            return mContext.getString(R.string.zone_info_footer_no_dst, offsetAndName);
        }

        final TimeZoneTransition nextDstTransition = findNextDstTransition(timeZone);
        if (nextDstTransition == null) {
            return null;
        }
        final boolean toDst = nextDstTransition.getTo().getDSTSavings() != 0;
        String timeType = toDst ? item.getDaylightName() : item.getStandardName();
        if (timeType == null) {
            // Fall back to generic "summer time" and "standard time" if the time zone has no
            // specific names.
            timeType = toDst ?
                    mContext.getString(R.string.zone_time_type_dst) :
                    mContext.getString(R.string.zone_time_type_standard);

        }
        final Calendar transitionTime = Calendar.getInstance(timeZone);
        transitionTime.setTimeInMillis(nextDstTransition.getTime());
        final String date = mDateFormat.format(transitionTime);
        return SpannableUtil.getResourcesText(mContext.getResources(),
                R.string.zone_info_footer, offsetAndName, timeType, date);
    }

    private TimeZoneTransition findNextDstTransition(TimeZone timeZone) {
        if (!(timeZone instanceof OlsonTimeZone)) {
            return null;
        }
        final OlsonTimeZone olsonTimeZone = (OlsonTimeZone) timeZone;
        TimeZoneTransition transition = olsonTimeZone.getNextTransition(
                mDate.getTime(), /* inclusive */ false);
        do {
            if (transition.getTo().getDSTSavings() != transition.getFrom().getDSTSavings()) {
                break;
            }
            transition = olsonTimeZone.getNextTransition(
                    transition.getTime(), /*inclusive */ false);
        } while (transition != null);
        return transition;
    }

}
