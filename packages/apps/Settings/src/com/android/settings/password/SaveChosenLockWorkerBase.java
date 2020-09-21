/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.settings.password;

import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.UserManager;

import com.android.internal.widget.LockPatternUtils;

/**
 * An invisible retained worker fragment to track the AsyncWork that saves (and optionally
 * verifies if a challenge is given) the chosen lock credential (pattern/pin/password).
 */
abstract class SaveChosenLockWorkerBase extends Fragment {

    private Listener mListener;
    private boolean mFinished;
    private Intent mResultData;

    protected LockPatternUtils mUtils;
    protected boolean mHasChallenge;
    protected long mChallenge;
    protected boolean mWasSecureBefore;
    protected int mUserId;

    private boolean mBlocking;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
    }

    public void setListener(Listener listener) {
        if (mListener == listener) {
            return;
        }

        mListener = listener;
        if (mFinished && mListener != null) {
            mListener.onChosenLockSaveFinished(mWasSecureBefore, mResultData);
        }
    }

    protected void prepare(LockPatternUtils utils, boolean credentialRequired,
            boolean hasChallenge, long challenge, int userId) {
        mUtils = utils;
        mUserId = userId;

        mHasChallenge = hasChallenge;
        mChallenge = challenge;
        // This will be a no-op for non managed profiles.
        mWasSecureBefore = mUtils.isSecure(mUserId);

        Context context = getContext();
        // If context is null, we're being invoked to change the setCredentialRequiredToDecrypt,
        // and we made sure that this is the primary user already.
        if (context == null || UserManager.get(context).getUserInfo(mUserId).isPrimary()) {
            mUtils.setCredentialRequiredToDecrypt(credentialRequired);
        }

        mFinished = false;
        mResultData = null;
    }

    protected void start() {
        if (mBlocking) {
            finish(saveAndVerifyInBackground());
        } else {
            new Task().execute();
        }
    }

    /**
     * Executes the save and verify work in background.
     * @return Intent with challenge token or null.
     */
    protected abstract Intent saveAndVerifyInBackground();

    protected void finish(Intent resultData) {
        mFinished = true;
        mResultData = resultData;
        if (mListener != null) {
            mListener.onChosenLockSaveFinished(mWasSecureBefore, mResultData);
        }
    }

    public void setBlocking(boolean blocking) {
        mBlocking = blocking;
    }

    private class Task extends AsyncTask<Void, Void, Intent> {
        @Override
        protected Intent doInBackground(Void... params){
            return saveAndVerifyInBackground();
        }

        @Override
        protected void onPostExecute(Intent resultData) {
            finish(resultData);
        }
    }

    interface Listener {
        public void onChosenLockSaveFinished(boolean wasSecureBefore, Intent resultData);
    }
}
