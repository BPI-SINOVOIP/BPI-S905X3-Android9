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
import android.annotation.StringRes;
import android.content.Context;
import android.widget.ImageView;

/**
 * Represents the basic line item with Icon and text.
 */
public class SimpleIconLineItem extends IconTextLineItem {
    private final CharSequence mDesc;
    private final Context mContext;
    @DrawableRes
    private final int mIconRes;

    public SimpleIconLineItem(
            @StringRes int title,
            @DrawableRes int iconRes,
            Context context,
            CharSequence desc) {
        super(context.getText(title));
        mDesc = desc;
        mContext = context;
        mIconRes = iconRes;
    }

    @Override
    public void setIcon(ImageView iconView) {
        iconView.setImageResource(mIconRes);
    }

    @Override
    public boolean isExpandable() {
        return true;
    }

    @Override
    public CharSequence getDesc() {
        return mDesc;
    }
}
