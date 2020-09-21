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

import android.content.Context;
import android.provider.Settings;

import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.common.BaseFragment.FragmentController;
import com.android.settingslib.datetime.ZoneGetter;

import java.util.Calendar;

/**
 * A LineItem that displays and sets time zone.
 */
class SetTimeZoneLineItem extends TextListItem
        implements DatetimeSettingsFragment.ListRefreshObserver {

    private final Context mContext;
    private final FragmentController mFragmentController;

    public SetTimeZoneLineItem(Context context, BaseFragment.FragmentController fragmentController) {
        super(context);
        mContext = context;
        mFragmentController = fragmentController;
        setTitle(context.getString(R.string.date_time_set_timezone));
        setBody(getDesc());
        updateLineItemData();
    }

    @Override
    public void onPreRefresh() {
        updateLineItemData();
    }

    private void updateLineItemData() {
        if (isEnabled()) {
            setSupplementalIcon(R.drawable.ic_chevron_right, /* showDivider= */ false);
            setOnClickListener(v ->
                    mFragmentController.launchFragment(TimeZonePickerFragment.getInstance()));
        } else {
            setSupplementalIcon(null, /* showDivider= */ false);
            setOnClickListener(null);
        }
    }

    private String getDesc() {
        Calendar now = Calendar.getInstance();
        return ZoneGetter.getTimeZoneOffsetAndName(mContext, now.getTimeZone(), now.getTime())
                .toString();
    }

    private boolean isEnabled() {
        return Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AUTO_TIME_ZONE, 0) <= 0;
    }
}
