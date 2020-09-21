/**
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.phone;

import android.app.ActionBar;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.telecom.PhoneAccountHandle;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;

/**
 * Helper for manipulating intents or components with subscription-related information.
 *
 * In settings, subscription ids and labels are passed along to indicate that settings
 * are being changed for particular subscriptions. This helper provides functions for
 * helping extract this info and perform common operations using this info.
 */
public class SubscriptionInfoHelper {

    // Extra on intent containing the id of a subscription.
    public static final String SUB_ID_EXTRA =
            "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionId";
    // Extra on intent containing the label of a subscription.
    private static final String SUB_LABEL_EXTRA =
            "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionLabel";

    private Context mContext;

    private int mSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    private String mSubLabel;

    /**
     * Instantiates the helper, by extracting the subscription id and label from the intent.
     */
    public SubscriptionInfoHelper(Context context, Intent intent) {
        mContext = context;
        PhoneAccountHandle phoneAccountHandle =
                intent.getParcelableExtra(TelephonyManager.EXTRA_PHONE_ACCOUNT_HANDLE);
        if (phoneAccountHandle != null) {
            mSubId = PhoneUtils.getSubIdForPhoneAccountHandle(phoneAccountHandle);
        }
        if (mSubId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            mSubId = intent.getIntExtra(SUB_ID_EXTRA, SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        }
        mSubLabel = intent.getStringExtra(SUB_LABEL_EXTRA);
    }

    /**
     * @param newActivityClass The class of the activity for the intent to start.
     * @return Intent containing extras for the subscription id and label if they exist.
     */
    public Intent getIntent(Class newActivityClass) {
        Intent intent = new Intent(mContext, newActivityClass);

        if (hasSubId()) {
            intent.putExtra(SUB_ID_EXTRA, mSubId);
        }

        if (!TextUtils.isEmpty(mSubLabel)) {
            intent.putExtra(SUB_LABEL_EXTRA, mSubLabel);
        }

        return intent;
    }

    public static void addExtrasToIntent(Intent intent, SubscriptionInfo subscription) {
        if (subscription == null) {
            return;
        }

        intent.putExtra(SubscriptionInfoHelper.SUB_ID_EXTRA, subscription.getSubscriptionId());
        intent.putExtra(
                SubscriptionInfoHelper.SUB_LABEL_EXTRA, subscription.getDisplayName().toString());
    }

    /**
     * @return Phone object. If a subscription id exists, it returns the phone for the id.
     */
    public Phone getPhone() {
        return hasSubId()
                ? PhoneFactory.getPhone(SubscriptionManager.getPhoneId(mSubId))
                : PhoneGlobals.getPhone();
    }

    /**
     * Sets the action bar title to the string specified by the given resource id, formatting
     * it with the subscription label. This assumes the resource string is formattable with a
     * string-type specifier.
     *
     * If the subscription label does not exists, leave the existing title.
     */
    public void setActionBarTitle(ActionBar actionBar, Resources res, int resId) {
        if (actionBar == null || TextUtils.isEmpty(mSubLabel)) {
            return;
        }

        if (!TelephonyManager.from(mContext).isMultiSimEnabled()) {
            return;
        }

        String title = String.format(res.getString(resId), mSubLabel);
        actionBar.setTitle(title);
    }

    public boolean hasSubId() {
        return mSubId != SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    }

    public int getSubId() {
        return mSubId;
    }
}
