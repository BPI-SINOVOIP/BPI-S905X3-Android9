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

package android.hardware.biometrics.cts;

import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.hardware.biometrics.BiometricPrompt;
import android.os.CancellationSignal;
import android.os.Handler;
import android.os.Looper;
import android.platform.test.annotations.Presubmit;
import android.test.AndroidTestCase;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;

/**
 * Basic test cases for BiometricPrompt
 */
public class BiometricPromptTest extends AndroidTestCase {

    private static final int AWAIT_TIMEOUT_MS = 3000;

    private final Handler mHandler = new Handler(Looper.getMainLooper());
    private final CountDownLatch mLatch = new CountDownLatch(1);

    private int mErrorReceived;

    private final Executor mExecutor = new Executor() {
        @Override
        public void execute(Runnable runnable) {
            mHandler.post(runnable);
        }
    };

    private final BiometricPrompt.AuthenticationCallback mAuthenticationCallback
            = new BiometricPrompt.AuthenticationCallback() {
        @Override
        public void onAuthenticationError(int errorCode, CharSequence errString) {
            mErrorReceived = errorCode;
            mLatch.countDown();
        }

        @Override
        public void onAuthenticationHelp(int helpCode, CharSequence helpString) {}

        @Override
        public void onAuthenticationFailed() {}

        @Override
        public void onAuthenticationSucceeded(BiometricPrompt.AuthenticationResult result) {}
    };

    /**
     * Test that we can try to start fingerprint authentication. It won't actually start since
     * there are no fingers enrolled. Cts-verifier will check the implementation.
     */
    @Presubmit
    public void test_authenticate_fingerprint() {
        final PackageManager pm = getContext().getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_FINGERPRINT)) {
            return;
        }

        boolean exceptionTaken = false;
        boolean callbackReceived = false;
        CancellationSignal cancellationSignal = new CancellationSignal();
        try {
            BiometricPrompt.Builder builder = new BiometricPrompt.Builder(getContext());
            builder.setTitle("Title");
            builder.setSubtitle("Subtitle");
            builder.setDescription("Description");
            builder.setNegativeButton("Negative", mExecutor, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {

                }
            });

            BiometricPrompt prompt = builder.build();

            prompt.authenticate(cancellationSignal, mExecutor, mAuthenticationCallback);
            mLatch.await(AWAIT_TIMEOUT_MS, TimeUnit.MILLISECONDS);
            assert(mErrorReceived == BiometricPrompt.BIOMETRIC_ERROR_NO_BIOMETRICS);
        } catch (Exception e) {
            exceptionTaken = true;
        } finally {
            assertFalse(exceptionTaken);
            cancellationSignal.cancel();
        }
    }

}
