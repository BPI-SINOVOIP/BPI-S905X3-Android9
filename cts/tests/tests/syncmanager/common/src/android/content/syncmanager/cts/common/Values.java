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
package android.content.syncmanager.cts.common;

import android.accounts.Account;
import android.content.ComponentName;

public class Values {
    public static final String APP1_PACKAGE = "android.content.syncmanager.cts.app1";
    public static final String APP2_PACKAGE = "android.content.syncmanager.cts.app2";

    public static final Account ACCOUNT_1_A = new Account("accountA", APP1_PACKAGE);

    public static final String APP1_AUTHORITY = "android.content.syncmanager.cts.app.provider1";
    public static final String APP2_AUTHORITY = "android.content.syncmanager.cts.app.provider2";

    public static final String COMM_RECEIVER = "android.content.syncmanager.cts.app.CommReceiver";

    public static final ComponentName APP1_RECEIVER = getCommReceiver(APP1_PACKAGE);
    public static final ComponentName APP2_RECEIVER = getCommReceiver(APP2_PACKAGE);

    public static ComponentName getCommReceiver(String packageName) {
        return new ComponentName(packageName, COMM_RECEIVER);
    }
}
