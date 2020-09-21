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

package com.android.car.settings.system;

import android.content.Context;
import android.content.Intent;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.text.TextUtils;
import android.view.View;

import androidx.car.widget.TextListItem;

import com.android.car.settings.R;


/**
 * A LineItem that links to system update.
 */
class SystemUpdatesListItem extends TextListItem {
    private final Context mContext;
    private final Intent mSettingsIntent;

    SystemUpdatesListItem(Context context, Intent settingsIntent) {
        super(context);
        mContext = context;
        mSettingsIntent = settingsIntent;
        setTitle(context.getString(R.string.system_update_settings_list_item_title));
        setPrimaryActionIcon(R.drawable.ic_system_update, /*useLargeIcon=*/ false);
        setSupplementalIcon(R.drawable.ic_chevron_right, /*showDivider=*/ false);
        setOnClickListener(this::onClick);
    }

    private void onClick(View view) {
        mContext.startActivity(mSettingsIntent);

        // copy what the phone setting is doing, sending out a carrier defined intent
        CarrierConfigManager configManager =
                (CarrierConfigManager) mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle b = configManager.getConfig();
        if (b == null || !b.getBoolean(CarrierConfigManager.KEY_CI_ACTION_ON_SYS_UPDATE_BOOL)) {
            return;
        }
        String intentStr = b.getString(CarrierConfigManager
                .KEY_CI_ACTION_ON_SYS_UPDATE_INTENT_STRING);
        if (!TextUtils.isEmpty(intentStr)) {
            String extra = b.getString(CarrierConfigManager
                    .KEY_CI_ACTION_ON_SYS_UPDATE_EXTRA_STRING);
            Intent intent = new Intent(intentStr);
            if (!TextUtils.isEmpty(extra)) {
                String extraVal = b.getString(CarrierConfigManager
                        .KEY_CI_ACTION_ON_SYS_UPDATE_EXTRA_VAL_STRING);
                intent.putExtra(extra, extraVal);
            }
            mContext.getApplicationContext().sendBroadcast(intent);
        }
    }
}
