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

/**
 * An invisible retained worker fragment to track the AsyncWork that saves the chosen lock
 * PIN/password.
 */

public class SavePasswordWorker extends SaveLockWorkerBase {

    private String mEnteredPassword;
    private String mCurrentPassword;
    private int mRequestedQuality;

    void start(int userId, String enteredPassword, String currentPassword, int requestedQuality) {
        init(userId);
        mEnteredPassword = enteredPassword;
        mCurrentPassword = currentPassword;
        mRequestedQuality = requestedQuality;
        start();
    }

    @Override
    void saveLock() {
        getUtils().saveLockPassword(mEnteredPassword, mCurrentPassword, mRequestedQuality,
                getUserId());
    }
}
