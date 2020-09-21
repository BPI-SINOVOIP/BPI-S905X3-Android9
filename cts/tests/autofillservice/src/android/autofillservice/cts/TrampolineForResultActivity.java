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
package android.autofillservice.cts;

import static com.google.common.truth.Truth.assertWithMessage;

import android.content.Intent;
import android.util.Log;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Activity used to launch another activity for result.
 */
// TODO: move to common code
public class TrampolineForResultActivity extends AbstractAutoFillActivity {
    private static final String TAG = "TrampolineForResultActivity";

    private final CountDownLatch mLatch = new CountDownLatch(1);

    private int mExpectedRequestCode;
    private int mActualRequestCode;
    private int mActualResultCode;

    /**
     * Starts an activity for result.
     */
    public void startForResult(Intent intent, int requestCode) {
        mExpectedRequestCode = requestCode;
        startActivityForResult(intent, requestCode);
    }

    /**
     * Asserts the activity launched by {@link #startForResult(Intent, int)} was finished with the
     * expected result code, or fails if it times out.
     */
    public void assertResult(int expectedResultCode) throws Exception {
        final boolean called = mLatch.await(1000, TimeUnit.MILLISECONDS);
        assertWithMessage("Result not received in 1s").that(called).isTrue();
        assertWithMessage("Wrong actual code").that(mActualRequestCode)
            .isEqualTo(mExpectedRequestCode);
        assertWithMessage("Wrong result code").that(mActualResultCode)
                .isEqualTo(expectedResultCode);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult(): req=" + requestCode + ", res=" + resultCode);
        mActualRequestCode = requestCode;
        mActualResultCode = resultCode;
        mLatch.countDown();
    }
}
