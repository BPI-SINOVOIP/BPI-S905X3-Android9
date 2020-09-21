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

package com.android.settings.deviceinfo;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.BidiFormatter;
import android.text.TextDirectionHeuristics;
import android.text.TextUtils;

import com.android.settings.R;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.DeviceInfoUtils;
import com.android.settingslib.core.AbstractPreferenceController;

import java.util.ArrayList;
import java.util.List;

public class PhoneNumberPreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin {

    private final static String KEY_PHONE_NUMBER = "phone_number";

    private final TelephonyManager mTelephonyManager;
    private final SubscriptionManager mSubscriptionManager;
    private final List<Preference> mPreferenceList = new ArrayList<>();

    public PhoneNumberPreferenceController(Context context) {
        super(context);
        mTelephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        mSubscriptionManager = (SubscriptionManager) context.getSystemService(
                Context.TELEPHONY_SUBSCRIPTION_SERVICE);
    }

    @Override
    public String getPreferenceKey() {
        return KEY_PHONE_NUMBER;
    }

    @Override
    public boolean isAvailable() {
        return mTelephonyManager.isVoiceCapable();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        final Preference preference = screen.findPreference(getPreferenceKey());
        mPreferenceList.add(preference);

        final int phonePreferenceOrder = preference.getOrder();
        // Add additional preferences for each sim in the device
        for (int simSlotNumber = 1; simSlotNumber < mTelephonyManager.getPhoneCount();
                simSlotNumber++) {
            final Preference multiSimPreference = createNewPreference(screen.getContext());
            multiSimPreference.setOrder(phonePreferenceOrder + simSlotNumber);
            multiSimPreference.setKey(KEY_PHONE_NUMBER + simSlotNumber);
            screen.addPreference(multiSimPreference);
            mPreferenceList.add(multiSimPreference);
        }
    }

    @Override
    public void updateState(Preference preference) {
        for (int simSlotNumber = 0; simSlotNumber < mPreferenceList.size(); simSlotNumber++) {
            final Preference simStatusPreference = mPreferenceList.get(simSlotNumber);
            simStatusPreference.setTitle(getPreferenceTitle(simSlotNumber));
            simStatusPreference.setSummary(getPhoneNumber(simSlotNumber));
        }
    }

    private CharSequence getPhoneNumber(int simSlot) {
        final SubscriptionInfo subscriptionInfo = getSubscriptionInfo(simSlot);
        if (subscriptionInfo == null) {
            return mContext.getString(R.string.device_info_default);
        }

        return getFormattedPhoneNumber(subscriptionInfo);
    }

    private CharSequence getPreferenceTitle(int simSlot) {
        return mTelephonyManager.getPhoneCount() > 1 ? mContext.getString(
                R.string.status_number_sim_slot, simSlot + 1) : mContext.getString(
                R.string.status_number);
    }

    @VisibleForTesting
    SubscriptionInfo getSubscriptionInfo(int simSlot) {
        final List<SubscriptionInfo> subscriptionInfoList =
                mSubscriptionManager.getActiveSubscriptionInfoList();
        if (subscriptionInfoList != null) {
            for (SubscriptionInfo info : subscriptionInfoList) {
                if (info.getSimSlotIndex() == simSlot) {
                    return info;
                }
            }
        }
        return null;
    }

    @VisibleForTesting
    CharSequence getFormattedPhoneNumber(SubscriptionInfo subscriptionInfo) {
        final String phoneNumber = DeviceInfoUtils.getFormattedPhoneNumber(mContext,
                subscriptionInfo);
        return TextUtils.isEmpty(phoneNumber) ? mContext.getString(R.string.device_info_default)
                : BidiFormatter.getInstance().unicodeWrap(phoneNumber, TextDirectionHeuristics.LTR);
    }

    @VisibleForTesting
    Preference createNewPreference(Context context) {
        return new Preference(context);
    }
}
