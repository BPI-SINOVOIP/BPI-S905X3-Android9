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

package com.android.cts.managedprofile;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Invoke lockNow() with a flag to evict the CE key of the profile. Used by the hostside test to
 * lock the profile with key eviction. This is triggered via a broadcast instead of a normal
 * test case, since the test process will be killed after calling lockNow() which will result in
 * a test failure if this were run as a test case.
 */
public class LockProfileReceiver extends BroadcastReceiver {

    private static final String ACTION_LOCK_PROFILE = "com.android.cts.managedprofile.LOCK_PROFILE";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (ACTION_LOCK_PROFILE.equals(intent.getAction())) {
            final DevicePolicyManager dpm = context.getSystemService(DevicePolicyManager.class);
            dpm.lockNow(DevicePolicyManager.FLAG_EVICT_CREDENTIAL_ENCRYPTION_KEY);
        }
    }
}
