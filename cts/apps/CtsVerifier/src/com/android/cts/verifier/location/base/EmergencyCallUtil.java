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
 * limitations under the License
 */

package com.android.cts.verifier.location.base;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.telecom.TelecomManager;
import android.telephony.TelephonyManager;
import android.text.InputType;
import android.util.Log;
import android.widget.EditText;
import android.widget.LinearLayout;
import com.android.cts.verifier.R;
import java.lang.reflect.Method;
import java.util.concurrent.TimeUnit;

/**
 * The EmergencyCallUtil class provides util functions related to the emergency call.
 */
public class EmergencyCallUtil {
    private static final int REQUEST_CODE_SET_DEFAULT_DIALER = 1;
    private static final String TAG = "EmergencyCallUtil";
    private static final long WAIT_FOR_CONNECTION_MS = TimeUnit.SECONDS.toMillis(3);

    /*
     * This method is used to set default dialer app.
     * To dial 911, it requires to set the dialer to be the system default dial app.
     *
     * @param activity current Activity.
     * @param packageName dialer package name.
     */
    public static void setDefaultDialer(Activity activity, String packageName) {
        final Intent intent = new Intent(TelecomManager.ACTION_CHANGE_DEFAULT_DIALER);
        intent.putExtra(TelecomManager.EXTRA_CHANGE_DEFAULT_DIALER_PACKAGE_NAME, packageName);
        activity.startActivityForResult(intent, REQUEST_CODE_SET_DEFAULT_DIALER);
    }

    public static void makePhoneCall(Activity activity, int phoneNumber) {
        Intent callIntent = new Intent(Intent.ACTION_CALL);
        callIntent.setData(Uri.parse("tel:" + phoneNumber));
        try {
            callIntent.setFlags(Intent.FLAG_ACTIVITY_NO_USER_ACTION);
            activity.startActivityForResult(callIntent, REQUEST_CODE_SET_DEFAULT_DIALER);
        } catch (SecurityException ex) {
            Log.d(TAG, "Failed to make the phone call: " + ex.toString());
        }
        // sleep 3sec to make sure call is connected
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(WAIT_FOR_CONNECTION_MS);
                } catch (InterruptedException ex) {
                    Log.d(TAG, "Failed to make the phone call: " + ex.toString());
                }
            }
        });
    }

    public static void endCallWithDelay(Context context, long delayMs) {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(delayMs);
                    endCall(context);
                } catch (InterruptedException ex) {
                    Log.d(TAG, "Failed to make the phone call: " + ex.toString());
                }
            }
        };
        new Thread(runnable).start();
    }

    public static boolean isPhoneDevice(Activity activity) {
        TelephonyManager tMgr =
            (TelephonyManager) activity.getSystemService(Context.TELEPHONY_SERVICE);
        if(tMgr.getPhoneType() == TelephonyManager.PHONE_TYPE_NONE){
            return false;
        }
        return true;
    }

    private static void endCall(Context context) {
        try {
            TelephonyManager telephonyManager =
                (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);

            Class<?> classTelephony = Class.forName(telephonyManager.getClass().getName());
            Method methodGetITelephony = classTelephony.getDeclaredMethod("getITelephony");
            methodGetITelephony.setAccessible(true);

            Object telephonyInterface = methodGetITelephony.invoke(telephonyManager);

            Class<?> telephonyInterfaceClass =
                Class.forName(telephonyInterface.getClass().getName());
            Method methodEndCall = telephonyInterfaceClass.getDeclaredMethod("endCall");

            methodEndCall.invoke(telephonyInterface);

        } catch (Exception e) {
            Log.d(TAG, "Failed to cancel the call: " + e.toString());
        }
    }

    private static String getCurrentPhoneNumber(Activity activity) {
        TelephonyManager tMgr =
            (TelephonyManager)activity.getSystemService(Context.TELEPHONY_SERVICE);
        return tMgr.getLine1Number();
    }
}
