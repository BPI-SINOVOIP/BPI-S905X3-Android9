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
package android.os.cts.batterysaving.common;

import android.content.ComponentName;

import java.security.SecureRandom;

public class Values {
    private static final SecureRandom sRng = new SecureRandom();

    public static final String APP_CURRENT_PACKAGE =
            "android.os.cts.batterysaving.app_target_api_current";

    public static final String APP_25_PACKAGE =
            "android.os.cts.batterysaving.app_target_api_25";

    public static final String COMM_RECEIVER = "android.os.cts.batterysaving.app.CommReceiver";

    public static final String TEST_SERVICE = "android.os.cts.batterysaving.app.TestService";

    public static final String KEY_REQUEST_FOREGROUND = "KEY_REQUEST_FOREGROUND";

    public static ComponentName getCommReceiver(String packageName) {
        return new ComponentName(packageName, COMM_RECEIVER);
    }

    public static ComponentName getTestService(String packageName) {
        return new ComponentName(packageName, TEST_SERVICE);
    }

    public static int getRandomInt() {
        return sRng.nextInt();
    }
}
