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

package android.telephony.euicc.cts;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.os.Bundle;
import android.telephony.euicc.EuiccManager;

/**
 * A dummy activity started by {@link EuiccManagerTest#testStartResolutionActivity()} for testing
 * {@link android.telephony.euicc.EuiccManager#startResolutionActivity(Activity, int, Intent,
 * PendingIntent)}. Sends {@link EuiccTestResolutionActivity#RESULT_CODE_TEST_PASSED} if the
 * resolution activity is successfully started, {@link
 * EuiccTestResolutionActivity#RESULT_CODE_TEST_FAILED} otherwise.
 */
public class EuiccTestResolutionActivity extends Activity {

    public static final String EXTRA_ACTIVITY_CALLBACK_INTENT = "extra_activity_callback_intent";
    public static final int RESULT_CODE_TEST_PASSED = 101;
    public static final int RESULT_CODE_TEST_FAILED = 102;

    private static final int REQUEST_CODE = 100;
    private static final String ACTION_START_RESOLUTION_ACTIVITY = "cts_start_resolution_activity";

    private PendingIntent mCallbackIntent;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mCallbackIntent = getIntent().getParcelableExtra(EXTRA_ACTIVITY_CALLBACK_INTENT);
    }

    @Override
    protected void onResume() {
        super.onResume();
        testStartResolutionActivity();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE) {
            sendCallbackAndFinish(
                    resultCode == RESULT_OK ? RESULT_CODE_TEST_PASSED : RESULT_CODE_TEST_FAILED);
        }
    }

    private void testStartResolutionActivity() {
        EuiccManager euiccManager = (EuiccManager) getSystemService(Context.EUICC_SERVICE);

        // specify activity to start
        Intent resolutionActivityIntent =
                new Intent(getApplicationContext(), EuiccResolutionActivity.class);
        PendingIntent resolutionIntent =
                PendingIntent.getActivity(
                        getApplicationContext(),
                        0 /* requestCode */,
                        resolutionActivityIntent,
                        PendingIntent.FLAG_ONE_SHOT);

        // add pending intent to extra
        Intent resultIntent = new Intent();
        resultIntent.putExtra(
                EuiccManager.EXTRA_EMBEDDED_SUBSCRIPTION_RESOLUTION_INTENT, resolutionIntent);

        // call startResolutionActivity()
        PendingIntent callbackIntent = createCallbackIntent(ACTION_START_RESOLUTION_ACTIVITY);
        try {
            euiccManager.startResolutionActivity(this, REQUEST_CODE, resultIntent, callbackIntent);
        } catch (IntentSender.SendIntentException e) {
            sendCallbackAndFinish(RESULT_CODE_TEST_FAILED);
        }
    }

    private PendingIntent createCallbackIntent(String action) {
        Intent intent = new Intent(action);
        return PendingIntent.getBroadcast(
                getApplicationContext(), REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    private void sendCallbackAndFinish(int resultCode) {
        if (mCallbackIntent != null) {
            try {
                mCallbackIntent.send(resultCode);
            } catch (PendingIntent.CanceledException e) {
                // Caller canceled the callback; do nothing.
            }
        }
        finish();
    }
}
