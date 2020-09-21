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
 * Invoke wipeData(), optionally including a reason. This is used by host side tests to destroy the
 * work profile. The call is triggered via a broadcast instead of run as a normal test case, since
 * the test process will be killed after calling wipeData() which will result in a test failure if
 * this were run as a test case.
 */
public class WipeDataReceiver extends BroadcastReceiver {

    private static final String ACTION_WIPE_DATA = "com.android.cts.managedprofile.WIPE_DATA";
    private static final String ACTION_WIPE_DATA_WITH_REASON =
            "com.android.cts.managedprofile.WIPE_DATA_WITH_REASON";
    private static final String TEST_WIPE_DATA_REASON = "cts test for WipeDataWithReason";

    @Override
    public void onReceive(Context context, Intent intent) {
        final DevicePolicyManager dpm = context.getSystemService(DevicePolicyManager.class);
        if (ACTION_WIPE_DATA.equals(intent.getAction())) {
            dpm.wipeData(0);
        } else if (ACTION_WIPE_DATA_WITH_REASON.equals(intent.getAction())) {
            dpm.wipeData(0, TEST_WIPE_DATA_REASON);
        }
    }
}
