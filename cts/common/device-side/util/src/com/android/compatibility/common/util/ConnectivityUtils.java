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
package com.android.compatibility.common.util;

import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class ConnectivityUtils {
    private ConnectivityUtils() {
    }

    /** @return true when the device has a network connection. */
    public static boolean isNetworkConnected(Context context) {
        final NetworkInfo networkInfo = context.getSystemService(ConnectivityManager.class)
                .getActiveNetworkInfo();
        return (networkInfo != null) && networkInfo.isConnected();
    }

    /** Assert that the device has a network connection. */
    public static void assertNetworkConnected(Context context) {
        assertTrue("Network must be connected", isNetworkConnected(context));
    }
}
