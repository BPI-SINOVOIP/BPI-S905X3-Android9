/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.work.impl.utils;

import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.MutableLiveData;
import android.content.Context;
import android.content.SharedPreferences;
import android.support.annotation.RestrictTo;

/**
 * Preferences for WorkManager.
 *
 * TODO: Migrate all preferences, including IdGenerator, to this file.
 *
 * @hide
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class Preferences {

    private static final String PREFERENCES_FILE_NAME = "androidx.work.util.preferences";

    private static final String KEY_LAST_CANCEL_ALL_TIME_MS = "last_cancel_all_time_ms";
    private static final String KEY_MIGRATE_PERSISTED_JOBS = "migrate_persisted_jobs";

    private SharedPreferences mSharedPreferences;

    public Preferences(Context context) {
        mSharedPreferences =
                context.getSharedPreferences(PREFERENCES_FILE_NAME, Context.MODE_PRIVATE);
    }

    /**
     * @return The last time (in milliseconds) a {@code cancelAll} method was called
     */
    public long getLastCancelAllTimeMillis() {
        return mSharedPreferences.getLong(KEY_LAST_CANCEL_ALL_TIME_MS, 0L);
    }

    /**
     * @return A {@link LiveData} of the last time (in milliseconds) a {@code cancelAll} method was
     *         called
     */
    public LiveData<Long> getLastCancelAllTimeMillisLiveData() {
        return new LastCancelAllLiveData(mSharedPreferences);
    }

    /**
     * Sets the last time a {@code cancelAll} method was called
     *
     * @param timeMillis The time a {@code cancelAll} method was called (in milliseconds)
     */
    public void setLastCancelAllTimeMillis(long timeMillis) {
        mSharedPreferences.edit().putLong(KEY_LAST_CANCEL_ALL_TIME_MS, timeMillis).apply();
    }

    /**
     * @return {@code true} When we should migrate from persisted jobs to non-persisted jobs in
     * {@link android.app.job.JobScheduler}
     */
    public boolean shouldMigratePersistedJobs() {
        // TODO Remove this before WorkManager 1.0 beta.
        return mSharedPreferences.getBoolean(KEY_MIGRATE_PERSISTED_JOBS, true);
    }

    /**
     * Updates the key which indicates that we have migrated all our persisted jobs in
     * {@link android.app.job.JobScheduler}.
     */
    public void setMigratedPersistedJobs() {
        mSharedPreferences.edit().putBoolean(KEY_MIGRATE_PERSISTED_JOBS, true).apply();
    }

    /**
     * A {@link android.arch.lifecycle.LiveData} that responds to changes in
     * {@link SharedPreferences} for the {@code lastCancelAllTime} value.
     */
    private static class LastCancelAllLiveData extends MutableLiveData<Long>
            implements SharedPreferences.OnSharedPreferenceChangeListener {

        private SharedPreferences mSharedPreferences;
        private long mLastCancelAllTimeMillis;

        LastCancelAllLiveData(SharedPreferences sharedPreferences) {
            mSharedPreferences = sharedPreferences;
            mLastCancelAllTimeMillis = mSharedPreferences.getLong(KEY_LAST_CANCEL_ALL_TIME_MS, 0L);
            postValue(mLastCancelAllTimeMillis);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if (KEY_LAST_CANCEL_ALL_TIME_MS.equals(key)) {
                long lastCancelAllTimeMillis = sharedPreferences.getLong(key, 0L);
                if (mLastCancelAllTimeMillis != lastCancelAllTimeMillis) {
                    mLastCancelAllTimeMillis = lastCancelAllTimeMillis;
                    setValue(mLastCancelAllTimeMillis);
                }
            }
        }

        @Override
        protected void onActive() {
            super.onActive();
            mSharedPreferences.registerOnSharedPreferenceChangeListener(this);
        }

        @Override
        protected void onInactive() {
            super.onInactive();
            mSharedPreferences.unregisterOnSharedPreferenceChangeListener(this);
        }
    }
}
