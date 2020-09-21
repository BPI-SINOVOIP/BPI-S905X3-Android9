/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.mediacenter.airplay.util;
import com.droidlogic.mediacenter.R;
import com.droidlogic.mediacenter.SettingsFragment;
import com.droidlogic.mediacenter.airplay.setting.SettingsPreferences;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;

public class Utils {

        // SDK Version
        public static int getSDKVersion() {
            // 14 : 4.0,4.0.1,4.0.2
            // 15 : 4.0.3,4.0.4
            // 16 : 4.1,4.1.1
            // 17 : 4.2,4.2.2
            return Build.VERSION.SDK_INT;
        }

        // Network
        public static boolean isNetworkConnected ( Context context ) {
            ConnectivityManager cm = ( ConnectivityManager ) context
                                     .getSystemService ( Context.CONNECTIVITY_SERVICE );
            NetworkInfo info = cm.getActiveNetworkInfo();
            if ( info != null ) {
                int type = info.getType();
                if ( type == ConnectivityManager.TYPE_WIFI ||
                        type == ConnectivityManager.TYPE_ETHERNET ) {
                    return info.isAvailable();
                }
            }
            return false;
        }

        public static SharedPreferences getSharedPreferences ( Context context ) {
            return PreferenceManager.getDefaultSharedPreferences ( context );
        }

        public static String getSavedDeviceName ( Context context ) {
            SharedPreferences prefs = Utils.getSharedPreferences ( context );
            return "AirPlay-" + prefs.getString ( SettingsFragment.KEY_DEVICE_NAME,
                                                  context.getString ( R.string.config_default_name ) );
        }
}
