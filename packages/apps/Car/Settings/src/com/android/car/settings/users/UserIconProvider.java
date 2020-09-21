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

package com.android.car.settings.users;

import android.car.user.CarUserManagerHelper;
import android.content.Context;
import android.content.pm.UserInfo;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawable;
import android.support.v4.graphics.drawable.RoundedBitmapDrawableFactory;

import com.android.car.settings.R;

/**
 * Simple class for providing icons for users in Settings.
 */
public class UserIconProvider {
    private final CarUserManagerHelper mCarUserManagerHelper;

    public UserIconProvider(CarUserManagerHelper userManagerHelper) {
        mCarUserManagerHelper = userManagerHelper;
    }

    /**
     * Gets the icon for the given user to use in settings.
     * If icon has not been assigned to this user, it defaults to a generic user icon.
     *
     * @param userInfo User for which the icon is requested.
     *
     * @return Drawable representing the icon for the user.
     */
    public Drawable getUserIcon(UserInfo userInfo, Context context) {
        Bitmap icon = mCarUserManagerHelper.getUserIcon(userInfo);
        if (icon == null) {
            // Return default user icon.
            return context.getDrawable(R.drawable.ic_user);
        }
        Resources res = context.getResources();
        BitmapDrawable scaledIcon = (BitmapDrawable) mCarUserManagerHelper.scaleUserIcon(icon, res
                .getDimensionPixelSize(R.dimen.car_primary_icon_size));

        // Enforce that the icon is circular
        RoundedBitmapDrawable circleIcon = RoundedBitmapDrawableFactory
                .create(res, scaledIcon.getBitmap());
        circleIcon.setCircular(true);
        return circleIcon;
    }

    /**
     * Scales passed in bitmap to the appropriate user icon size.
     *
     * @param bitmap Bitmap to scale.
     *
     * @return Drawable scaled to the user icon size.
     */
    public static Drawable scaleUserIcon(Bitmap bitmap, CarUserManagerHelper userManagerHelper,
            Context context) {
        return userManagerHelper.scaleUserIcon(bitmap, context.getResources()
                .getDimensionPixelSize(R.dimen.car_primary_icon_size));
    }
}
