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
 * limitations under the License.
 */

package android.server.wm;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;

/**
 * A class that sends log data to {@link TestLogService}.
 */
public class TestLogClient {

    public static final String EXTRA_LOG_TAG = "logtag";
    public static final String EXTRA_KEY = "key";
    public static final String EXTRA_VALUE = "value";

    private static final String TEST_LOGGER_PACKAGE_NAME = "android.server.cts.wm";
    private static final String TEST_LOGGER_SERVICE_NAME = "android.server.wm.TestLogService";

    private final Context mContext;
    private final String mLogTag;

    public TestLogClient(Context context, String logtag) {
        mContext = context;
        mLogTag = logtag;
    }

    public void record(String key, String value) {
        Intent intent = new Intent();
        intent.setComponent(
                new ComponentName(TEST_LOGGER_PACKAGE_NAME, TEST_LOGGER_SERVICE_NAME));
        intent.putExtra(EXTRA_LOG_TAG, mLogTag);
        intent.putExtra(EXTRA_KEY, key);
        intent.putExtra(EXTRA_VALUE, value);
        mContext.startService(intent);
    }
}
