package com.google.android.car.defaultstoragemonitoringcompanionapp;

import android.car.storagemonitoring.CarStorageMonitoringManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class ExcessiveIoIntentReceiver extends BroadcastReceiver {
    private final static String TAG = ExcessiveIoIntentReceiver.class.getSimpleName();

    @Override
    public void onReceive(Context context, Intent intent) {
        if (CarStorageMonitoringManager.INTENT_EXCESSIVE_IO.equals(intent.getAction())) {
            Log.d(TAG, "excessive I/O activity detected.");
        } else {
            Log.w(TAG, "unexpected intent received: " + intent);
        }
    }
}
