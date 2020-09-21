/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning.manageduser;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;

import com.android.managedprovisioning.common.ProvisionLogger;

/**
 * This receiver is invoked after a managed user is created.
 */
public class ManagedUserCreationListener extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        if (DevicePolicyManager.ACTION_MANAGED_USER_CREATED.equals(intent.getAction())) {
            final int userId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, UserHandle.USER_NULL);
            final boolean leaveAllSystemAppsEnabled = intent.getBooleanExtra(
                    DevicePolicyManager.EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED, false);
            ProvisionLogger.logd("ACTION_MANAGED_USER_CREATED received for user " + userId);
            final PendingResult result = goAsync();
            Thread thread = new Thread(() -> {
                new ManagedUserCreationController(userId, leaveAllSystemAppsEnabled, context).run();
                result.finish();
            });
            thread.setPriority(Thread.MAX_PRIORITY);
            thread.start();
        } else {
            ProvisionLogger.logw("Unexpected intent action: " + intent.getAction());
        }
    }
}
