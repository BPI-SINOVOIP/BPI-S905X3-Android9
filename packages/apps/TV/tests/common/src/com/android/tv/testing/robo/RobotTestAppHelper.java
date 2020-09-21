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
 * limitations under the License
 */
package com.android.tv.testing.robo;

import android.media.tv.TvContract;
import com.android.tv.testing.FakeTvProvider;
import com.android.tv.testing.TestSingletonApp;
import com.android.tv.testing.testdata.TestData;
import java.util.concurrent.TimeUnit;
import org.robolectric.Robolectric;

/** Static utilities for using {@link TestSingletonApp} in roboletric tests. */
public final class RobotTestAppHelper {

    public static void loadTestData(TestSingletonApp app, TestData testData) {
        ContentProviders.register(FakeTvProvider.class, TvContract.AUTHORITY);
        app.loadTestData(testData, TimeUnit.DAYS.toMillis(1));
        Robolectric.flushBackgroundThreadScheduler();
        Robolectric.flushForegroundThreadScheduler();
    }

    private RobotTestAppHelper() {}
}
