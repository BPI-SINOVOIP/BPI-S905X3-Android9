/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.settings.datausage;

import android.net.INetworkStatsService;
import android.net.NetworkPolicyManager;
import android.net.NetworkTemplate;
import android.os.INetworkManagementService;
import android.os.UserManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import com.android.settingslib.NetworkPolicyEditor;

public interface TemplatePreference {

    void setTemplate(NetworkTemplate template, int subId, NetworkServices services);

    class NetworkServices {
        INetworkManagementService mNetworkService;
        INetworkStatsService mStatsService;
        NetworkPolicyManager mPolicyManager;
        TelephonyManager mTelephonyManager;
        SubscriptionManager mSubscriptionManager;
        UserManager mUserManager;
        NetworkPolicyEditor mPolicyEditor;
    }

}
