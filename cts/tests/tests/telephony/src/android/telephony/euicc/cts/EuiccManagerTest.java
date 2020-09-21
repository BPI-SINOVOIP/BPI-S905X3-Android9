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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.euicc.DownloadableSubscription;
import android.telephony.euicc.EuiccInfo;
import android.telephony.euicc.EuiccManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
public class EuiccManagerTest {

    private static final int REQUEST_CODE = 0;
    private static final int CALLBACK_TIMEOUT_MILLIS = 2000;
    // starting activities might take extra time
    private static final int ACTIVITY_CALLBACK_TIMEOUT_MILLIS = 5000;
    private static final String ACTION_DOWNLOAD_SUBSCRIPTION = "cts_download_subscription";
    private static final String ACTION_DELETE_SUBSCRIPTION = "cts_delete_subscription";
    private static final String ACTION_SWITCH_TO_SUBSCRIPTION = "cts_switch_to_subscription";
    private static final String ACTION_START_TEST_RESOLUTION_ACTIVITY =
            "cts_start_test_resolution_activity";
    private static final String ACTIVATION_CODE = "1$LOCALHOST$04386-AGYFT-A74Y8-3F815";

    private static final String[] sCallbackActions =
            new String[]{
                    ACTION_DOWNLOAD_SUBSCRIPTION,
                    ACTION_DELETE_SUBSCRIPTION,
                    ACTION_SWITCH_TO_SUBSCRIPTION,
                    ACTION_START_TEST_RESOLUTION_ACTIVITY,
            };

    private EuiccManager mEuiccManager;
    private CallbackReceiver mCallbackReceiver;

    @Before
    public void setUp() throws Exception {
        mEuiccManager = (EuiccManager) getContext().getSystemService(Context.EUICC_SERVICE);
    }

    @After
    public void tearDown() throws Exception {
        if (mCallbackReceiver != null) {
            getContext().unregisterReceiver(mCallbackReceiver);
        }
    }

    @Test
    public void testGetEid() {
        // test disabled state only for now
        if (mEuiccManager.isEnabled()) {
            return;
        }

        // call getEid()
        String eid = mEuiccManager.getEid();

        // verify result is null
        assertNull(eid);
    }

    @Test
    public void testDownloadSubscription() {
        // test disabled state only for now
        if (mEuiccManager.isEnabled()) {
            return;
        }

        // set up CountDownLatch and receiver
        CountDownLatch countDownLatch = new CountDownLatch(1);
        mCallbackReceiver = new CallbackReceiver(countDownLatch);
        getContext()
                .registerReceiver(
                        mCallbackReceiver, new IntentFilter(ACTION_DOWNLOAD_SUBSCRIPTION));

        // call downloadSubscription()
        DownloadableSubscription subscription = createDownloadableSubscription();
        PendingIntent callbackIntent = createCallbackIntent(ACTION_DOWNLOAD_SUBSCRIPTION);
        mEuiccManager.downloadSubscription(
                subscription, false /* switchAfterDownload */, callbackIntent);

        // wait for callback
        try {
            countDownLatch.await(CALLBACK_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        // verify correct result code is received
        assertEquals(
                EuiccManager.EMBEDDED_SUBSCRIPTION_RESULT_ERROR, mCallbackReceiver.getResultCode());
    }

    @Test
    public void testGetEuiccInfo() {
        // test disabled state only for now
        if (mEuiccManager.isEnabled()) {
            return;
        }

        // call getEuiccInfo()
        EuiccInfo euiccInfo = mEuiccManager.getEuiccInfo();

        // verify result is null
        assertNull(euiccInfo);
    }

    @Test
    public void testDeleteSubscription() {
        // test disabled state only for now
        if (mEuiccManager.isEnabled()) {
            return;
        }

        // set up CountDownLatch and receiver
        CountDownLatch countDownLatch = new CountDownLatch(1);
        mCallbackReceiver = new CallbackReceiver(countDownLatch);
        getContext()
                .registerReceiver(mCallbackReceiver, new IntentFilter(ACTION_DELETE_SUBSCRIPTION));

        // call deleteSubscription()
        PendingIntent callbackIntent = createCallbackIntent(ACTION_DELETE_SUBSCRIPTION);
        mEuiccManager.deleteSubscription(3, callbackIntent);

        // wait for callback
        try {
            countDownLatch.await(CALLBACK_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        // verify correct result code is received
        assertEquals(
                EuiccManager.EMBEDDED_SUBSCRIPTION_RESULT_ERROR, mCallbackReceiver.getResultCode());
    }

    @Test
    public void testSwitchToSubscription() {
        // test disabled state only for now
        if (mEuiccManager.isEnabled()) {
            return;
        }

        // set up CountDownLatch and receiver
        CountDownLatch countDownLatch = new CountDownLatch(1);
        mCallbackReceiver = new CallbackReceiver(countDownLatch);
        getContext()
                .registerReceiver(
                        mCallbackReceiver, new IntentFilter(ACTION_SWITCH_TO_SUBSCRIPTION));

        // call deleteSubscription()
        PendingIntent callbackIntent = createCallbackIntent(ACTION_SWITCH_TO_SUBSCRIPTION);
        mEuiccManager.switchToSubscription(4, callbackIntent);

        // wait for callback
        try {
            countDownLatch.await(CALLBACK_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        // verify correct result code is received
        assertEquals(
                EuiccManager.EMBEDDED_SUBSCRIPTION_RESULT_ERROR, mCallbackReceiver.getResultCode());
    }

    @Test
    public void testStartResolutionActivity() {
        // set up CountDownLatch and receiver
        CountDownLatch countDownLatch = new CountDownLatch(1);
        mCallbackReceiver = new CallbackReceiver(countDownLatch);
        getContext()
                .registerReceiver(
                        mCallbackReceiver, new IntentFilter(ACTION_START_TEST_RESOLUTION_ACTIVITY));

        /*
         * Start EuiccTestResolutionActivity to test EuiccManager#startResolutionActivity(), since
         * it requires a foreground activity. EuiccTestResolutionActivity will report the test
         * result to the callback receiver.
         */
        Intent testResolutionActivityIntent =
                new Intent(getContext(), EuiccTestResolutionActivity.class);
        PendingIntent callbackIntent = createCallbackIntent(ACTION_START_TEST_RESOLUTION_ACTIVITY);
        testResolutionActivityIntent.putExtra(
                EuiccTestResolutionActivity.EXTRA_ACTIVITY_CALLBACK_INTENT, callbackIntent);
        testResolutionActivityIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getContext().startActivity(testResolutionActivityIntent);

        // wait for callback
        try {
            countDownLatch.await(ACTIVITY_CALLBACK_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        // verify test result reported by EuiccTestResolutionActivity
        assertEquals(
                EuiccTestResolutionActivity.RESULT_CODE_TEST_PASSED,
                mCallbackReceiver.getResultCode());
    }

    private Context getContext() {
        return InstrumentationRegistry.getContext();
    }

    private DownloadableSubscription createDownloadableSubscription() {
        return DownloadableSubscription.forActivationCode(ACTIVATION_CODE);
    }

    private PendingIntent createCallbackIntent(String action) {
        Intent intent = new Intent(action);
        return PendingIntent.getBroadcast(
                getContext(), REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    private static class CallbackReceiver extends BroadcastReceiver {

        private CountDownLatch mCountDownLatch;

        public CallbackReceiver(CountDownLatch latch) {
            mCountDownLatch = latch;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            for (String callbackAction : sCallbackActions) {
                if (callbackAction.equals(intent.getAction())) {
                    int resultCode = getResultCode();
                    mCountDownLatch.countDown();
                    break;
                }
            }
        }
    }
}
