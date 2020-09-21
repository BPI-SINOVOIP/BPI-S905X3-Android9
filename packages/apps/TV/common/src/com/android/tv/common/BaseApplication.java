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

package com.android.tv.common;

import android.annotation.TargetApi;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.support.annotation.VisibleForTesting;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.recording.RecordingStorageStatusManager;
import com.android.tv.common.util.Clock;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.common.util.Debug;
import com.android.tv.common.util.SystemProperties;

/** The base application class for Live TV applications. */
public abstract class BaseApplication extends Application implements BaseSingletons {
    private RecordingStorageStatusManager mRecordingStorageStatusManager;

    /**
     * An instance of {@link BaseSingletons}. Note that this can be set directly only for the test
     * purpose.
     */
    @VisibleForTesting public static BaseSingletons sSingletons;

    /** Returns the {@link BaseSingletons} using the application context. */
    public static BaseSingletons getSingletons(Context context) {
        // STOP-SHIP: changing the method to protected once the Tuner application is created.
        // No need to be "synchronized" because this doesn't create any instance.
        if (sSingletons == null) {
            sSingletons = (BaseSingletons) context.getApplicationContext();
        }
        return sSingletons;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Debug.getTimer(Debug.TAG_START_UP_TIMER).start();
        Debug.getTimer(Debug.TAG_START_UP_TIMER)
                .log("Start " + this.getClass().getSimpleName() + ".onCreate");
        CommonPreferences.initialize(this);

        // Only set StrictMode for ENG builds because the build server only produces userdebug
        // builds.
        if (BuildConfig.ENG && SystemProperties.ALLOW_STRICT_MODE.getValue()) {
            StrictMode.ThreadPolicy.Builder threadPolicyBuilder =
                    new StrictMode.ThreadPolicy.Builder().detectAll().penaltyLog();
            // TODO(b/69565157): Turn penaltyDeath on for VMPolicy when tests are fixed.
            StrictMode.VmPolicy.Builder vmPolicyBuilder =
                    new StrictMode.VmPolicy.Builder().detectAll().penaltyLog();

            if (!CommonUtils.isRunningInTest()) {
                threadPolicyBuilder.penaltyDialog();
            }
            StrictMode.setThreadPolicy(threadPolicyBuilder.build());
            StrictMode.setVmPolicy(vmPolicyBuilder.build());
        }
        if (CommonFeatures.DVR.isEnabled(this)) {
            getRecordingStorageStatusManager();
        }
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                // Fetch remote config
                getRemoteConfig().fetch(null);
                return null;
            }
        }.execute();
    }

    @Override
    public Clock getClock() {
        return Clock.SYSTEM;
    }

    /** Returns {@link RecordingStorageStatusManager}. */
    @Override
    @TargetApi(Build.VERSION_CODES.N)
    public RecordingStorageStatusManager getRecordingStorageStatusManager() {
        if (mRecordingStorageStatusManager == null) {
            mRecordingStorageStatusManager = new RecordingStorageStatusManager(this);
        }
        return mRecordingStorageStatusManager;
    }

    @Override
    public abstract Intent getTunerSetupIntent(Context context);
}
