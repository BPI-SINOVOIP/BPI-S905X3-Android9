/*
 * Copyright 2018, The Android Open Source Project
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

package com.android.managedprovisioning.task;

import android.content.Context;
import android.os.Handler;
import android.provider.Settings;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.task.wifi.NetworkMonitor;

/**
 * A task that enables mobile data and waits for it to successfully connect. If connection times out
 * {@link #error(int)} will be called.
 */
public class ConnectMobileNetworkTask extends AbstractProvisioningTask
        implements NetworkMonitor.NetworkConnectedCallback {
    private static final int RECONNECT_TIMEOUT_MS = 60000;

    private final NetworkMonitor mNetworkMonitor;

    private Handler mHandler;
    private boolean mTaskDone = false;

    private final Utils mUtils;
    private Runnable mTimeoutRunnable;

    public ConnectMobileNetworkTask(
            Context context,
            ProvisioningParams provisioningParams,
            Callback callback) {
        super(context, provisioningParams, callback);
        mNetworkMonitor = new NetworkMonitor(context);
        mUtils = new Utils();
    }

    @Override
    public void run(int userId) {
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.DEVICE_PROVISIONING_MOBILE_DATA_ENABLED, 1);

        if (mUtils.isConnectedToNetwork(mContext)) {
            success();
            return;
        }

        mTaskDone = false;
        mHandler = new Handler();
        mNetworkMonitor.startListening(this);

        // NetworkMonitor will call onNetworkConnected.
        // Post time out event in case the NetworkMonitor doesn't call back.
        mTimeoutRunnable = () -> finishTask(false);
        mHandler.postDelayed(mTimeoutRunnable, RECONNECT_TIMEOUT_MS);
    }

    @Override
    public int getStatusMsgId() {
        return R.string.progress_connect_to_mobile_network;
    }

    @Override
    public void onNetworkConnected() {
        ProvisionLogger.logd("onNetworkConnected");
        if (mUtils.isConnectedToNetwork(mContext)) {
            ProvisionLogger.logd("Connected to the mobile network");
            finishTask(true);
            // Remove time out callback.
            mHandler.removeCallbacks(mTimeoutRunnable);
        }
    }

    private synchronized void finishTask(boolean isSuccess) {
        if (mTaskDone) {
            return;
        }

        mTaskDone = true;
        mNetworkMonitor.stopListening();
        if (isSuccess) {
            success();
        } else {
            error(0);
        }
    }
}
