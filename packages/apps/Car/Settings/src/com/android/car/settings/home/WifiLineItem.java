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

package com.android.car.settings.home;

import android.annotation.DrawableRes;
import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Switch;

import com.android.car.list.IconToggleLineItem;
import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.wifi.CarWifiManager;
import com.android.car.settings.wifi.WifiSettingsFragment;
import com.android.car.settings.wifi.WifiUtil;


/**
 * Represents the wifi line item on settings home page.
 */
public class WifiLineItem extends IconToggleLineItem {
    private final Context mContext;
    private final CarWifiManager mCarWifiManager;
    private BaseFragment.FragmentController mFragmentController;

    public WifiLineItem(
            Context context,
            CarWifiManager carWifiManager,
            BaseFragment.FragmentController fragmentController) {
        super(context.getText(R.string.wifi_settings), context);
        mContext = context;
        mCarWifiManager = carWifiManager;
        mFragmentController = fragmentController;
    }

    @Override
    public boolean onToggleTouched(Switch toggleSwitch, MotionEvent event) {
        // intercept touch event, so we can process the request and update the switch
        // state accordingly
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            mCarWifiManager.setWifiEnabled(!isChecked());
        }
        return true;
    }

    @Override
    public void onClick(View view) {
        mFragmentController.launchFragment(WifiSettingsFragment.newInstance());
    }

    @Override
    public CharSequence getDesc() {
        return mContext.getText(R.string.wifi_settings_summary);
    }

    @Override
    public boolean isChecked() {
        return mCarWifiManager.isWifiEnabled();
    }

    @Override
    public boolean isExpandable() {
        return true;
    }

    @Override
    public @DrawableRes int getIcon() {
        return WifiUtil.getIconRes(mCarWifiManager.getWifiState());
    }

    public void onWifiStateChanged(int state) {
        if (mIconUpdateListener != null) {
            mIconUpdateListener.onUpdateIcon(WifiUtil.getIconRes(state));
        }
        if (mSwitchStateUpdateListener != null) {
            mSwitchStateUpdateListener.onToggleChanged(WifiUtil.isWifiOn(state));
        }
    }

}
