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

package com.android.cts.verifier.security;

import android.content.DialogInterface;
import android.hardware.biometrics.BiometricPrompt;
import android.os.CancellationSignal;
import android.os.Handler;
import android.os.Looper;

import java.util.concurrent.Executor;

public class BiometricPromptBoundKeysTest extends FingerprintBoundKeysTest {

    private DialogCallback mDialogCallback;
    private BiometricPrompt mBiometricPrompt;
    private CancellationSignal mCancellationSignal;

    private final Handler mHandler = new Handler(Looper.getMainLooper());

    private final Executor mExecutor = (runnable) -> {
        mHandler.post(runnable);
    };

    private final Runnable mNegativeButtonRunnable = () -> {
        showToast("Authentication canceled by user");
    };

    private class DialogCallback extends
            BiometricPrompt.AuthenticationCallback {
        @Override
        public void onAuthenticationError(int errMsgId, CharSequence errString) {
            showToast(errString.toString());
        }

        @Override
        public void onAuthenticationSucceeded(BiometricPrompt.AuthenticationResult result) {
            if (tryEncrypt()) {
                showToast("Test passed.");
                getPassButton().setEnabled(true);
            } else {
                showToast("Test failed. Key not accessible after auth");
            }
        }
    }

    @Override
    protected void showAuthenticationScreen() {
        mCancellationSignal = new CancellationSignal();
        mDialogCallback = new DialogCallback();
        mBiometricPrompt = new BiometricPrompt.Builder(getApplicationContext())
                .setTitle("Authenticate with fingerprint")
                .setNegativeButton("Cancel", mExecutor,
                        (DialogInterface dialogInterface, int which) -> {
                            if (which == DialogInterface.BUTTON_NEGATIVE) {
                                mHandler.post(mNegativeButtonRunnable);
                            }
                        })
                .build();
        mBiometricPrompt.authenticate(
                new BiometricPrompt
                .CryptoObject(getCipher()),
                mCancellationSignal, mExecutor, mDialogCallback);
    }
}
