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

package com.android.car.settings.security;

import android.annotation.WorkerThread;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.support.v4.app.Fragment;

import com.android.car.settings.common.Logger;
import com.android.internal.widget.LockPatternUtils;

/**
 * An invisible retained worker fragment to track the AsyncWork that saves
 * the chosen lock credential (pattern/pin/password).
 */
abstract class SaveLockWorkerBase extends Fragment {
    /**
     * Callback when lock save has finished
     */
    interface Listener {
        void onChosenLockSaveFinished(boolean isSaveSuccessful);
    }

    private static final Logger LOG = new Logger(SaveLockWorkerBase.class);

    private Listener mListener;
    private boolean mFinished;
    private boolean mIsSaveSuccessful;
    private LockPatternUtils mUtils;
    private int mUserId;

    final LockPatternUtils getUtils() {
        return mUtils;
    }

    final int getUserId() {
        return mUserId;
    }

    final boolean isFinished() {
        return mFinished;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
    }

    final void init(int userId) {
        mUtils = new LockPatternUtils(getContext());
        mUserId = userId;
    }

    /**
     * Start executing the async task.
     */
    final void start() {
        mFinished = false;
        new Task().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    /**
     * Set the listener to get callback when finished saving the chosen lock.
     */
    public void setListener(Listener listener) {
        if (mListener != listener) {
            mListener = listener;
            if (mFinished && mListener != null) {
                mListener.onChosenLockSaveFinished(mIsSaveSuccessful);
            }
        }
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    boolean saveAndVerifyInBackground(Void... params) {
        boolean isSaveSuccessful = true;

        try {
            saveLock();
        } catch (Exception e) {
            LOG.e("Save lock exception", e);
            isSaveSuccessful = false;
        }

        return isSaveSuccessful;
    }

    private void finish(boolean isSaveSuccessful) {
        mFinished = true;
        mIsSaveSuccessful = isSaveSuccessful;
        if (mListener != null) {
            mListener.onChosenLockSaveFinished(isSaveSuccessful);
        }
    }

    /**
     * Executes the save and verify work in background.
     */
    @WorkerThread
    abstract void saveLock();

    // Save chosen lock task.
    private class Task extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... params) {
            return saveAndVerifyInBackground();
        }

        @Override
        protected void onPostExecute(Boolean isSaveSuccessful) {
            finish(isSaveSuccessful);
        }
    }
}
