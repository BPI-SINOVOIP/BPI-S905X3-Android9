/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.settings.wifi.p2p;

import android.content.Context;
import android.net.wifi.p2p.WifiP2pGroup;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceViewHolder;

public class WifiP2pPersistentGroup extends Preference {

    public WifiP2pGroup mGroup;

    public WifiP2pPersistentGroup(Context context, WifiP2pGroup group) {
        super(context);
        mGroup = group;
        setTitle(mGroup.getNetworkName());
    }

    int getNetworkId() {
        return mGroup.getNetworkId();
    }

    String getGroupName() {
        return mGroup.getNetworkName();
    }
}
