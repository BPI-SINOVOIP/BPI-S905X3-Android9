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

package com.android.car.list;

import android.annotation.DrawableRes;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;

/**
 * Represents the a line item that can launch another activity.
 */
public class LaunchAppLineItem extends IconTextLineItem {
    private static final String TAG = "LaunchAppLineItem";

    private final CharSequence mDesc;
    private final Context mContext;
    private final Icon mIcon;
    private final Intent mLaunchIntent;

    public LaunchAppLineItem(
            CharSequence title,
            Icon icon,
            Context context,
            CharSequence desc,
            Intent launchIntent) {
        super(title);
        mDesc = desc;
        mContext = context;
        mIcon = icon;
        mLaunchIntent = launchIntent;
    }

    @Override
    public void setIcon(ImageView iconView) {
        if (mIcon == null) {
            Log.d(TAG, "not icon set.");
            return;
        }
        iconView.setImageIcon(mIcon);
    }

    @Override
    public boolean isExpandable() {
        return true;
    }

    @Override
    public CharSequence getDesc() {
        return mDesc;
    }

    @Override
    public void onClick(View view) {
        mContext.startActivity(mLaunchIntent);
    }
}
