/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.server.telecom.tests;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.telecom.TelecomManager;
import android.telephony.CarrierConfigManager;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.telecom.Call;
import com.android.server.telecom.CallerInfoLookupHelper;
import com.android.server.telecom.callfiltering.AsyncBlockCheckFilter;
import com.android.server.telecom.callfiltering.BlockCheckerAdapter;
import com.android.server.telecom.callfiltering.CallFilterResultCallback;
import com.android.server.telecom.callfiltering.CallFilteringResult;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

import java.util.concurrent.CountDownLatch;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(JUnit4.class)
public class AsyncBlockCheckFilterTest extends TelecomTestCase {
    @Mock private Context mContext;
    @Mock private BlockCheckerAdapter mBlockCheckerAdapter;
    @Mock private Call mCall;
    @Mock private CallFilterResultCallback mCallback;
    @Mock private CallerInfoLookupHelper mCallerInfoLookupHelper;
    @Mock private CarrierConfigManager mCarrierConfigManager;

    private AsyncBlockCheckFilter mFilter;
    private static final CallFilteringResult BLOCK_RESULT = new CallFilteringResult(
            false, // shouldAllowCall
            true, //shouldReject
            false, //shouldAddToCallLog
            false // shouldShowNotification
    );

    private static final CallFilteringResult PASS_RESULT = new CallFilteringResult(
            true, // shouldAllowCall
            false, // shouldReject
            true, // shouldAddToCallLog
            true // shouldShowNotification
    );

    private static final Uri TEST_HANDLE = Uri.parse("tel:1235551234");
    private static final int TEST_TIMEOUT = 100;

    @Override
    @Before
    public void setUp() throws Exception {
        super.setUp();
        when(mCall.getHandle()).thenReturn(TEST_HANDLE);
        mFilter = new AsyncBlockCheckFilter(mContext, mBlockCheckerAdapter,
                mCallerInfoLookupHelper);
    }

    @SmallTest
    @Test
    public void testBlockNumber() {
        final CountDownLatch latch = new CountDownLatch(1);
        doAnswer(invocation -> {
            latch.countDown();
            return true;
        }).when(mBlockCheckerAdapter)
                .isBlocked(any(Context.class), eq(TEST_HANDLE.getSchemeSpecificPart()),
                        any(Bundle.class));

        setEnhancedBlockingEnabled(false);
        mFilter.startFilterLookup(mCall, mCallback);

        waitOnLatch(latch);
        verify(mCallback, timeout(TEST_TIMEOUT))
                .onCallFilteringComplete(eq(mCall), eq(BLOCK_RESULT));
    }

    @SmallTest
    @Test
    public void testBlockNumber_enhancedBlockingEnabled() {
        final CountDownLatch latch = new CountDownLatch(1);
        doAnswer(invocation -> {
            latch.countDown();
            return true;
        }).when(mBlockCheckerAdapter)
                .isBlocked(any(Context.class), eq(TEST_HANDLE.getSchemeSpecificPart()),
                        any(Bundle.class));

        setEnhancedBlockingEnabled(true);
        CallerInfoLookupHelper.OnQueryCompleteListener queryListener = verifyEnhancedLookupStart();
        queryListener.onCallerInfoQueryComplete(TEST_HANDLE, null);

        waitOnLatch(latch);
        verify(mCallback, timeout(TEST_TIMEOUT))
                .onCallFilteringComplete(eq(mCall), eq(BLOCK_RESULT));
    }

    @SmallTest
    @Test
    public void testDontBlockNumber() {
        final CountDownLatch latch = new CountDownLatch(1);
        doAnswer(invocation -> {
            latch.countDown();
            return false;
        }).when(mBlockCheckerAdapter)
                .isBlocked(any(Context.class), eq(TEST_HANDLE.getSchemeSpecificPart()),
                        any(Bundle.class));

        setEnhancedBlockingEnabled(false);
        mFilter.startFilterLookup(mCall, mCallback);

        waitOnLatch(latch);
        verify(mCallback, timeout(TEST_TIMEOUT))
                .onCallFilteringComplete(eq(mCall), eq(PASS_RESULT));
    }

    @SmallTest
    @Test
    public void testDontBlockNumber_enhancedBlockingEnabled() {
        final CountDownLatch latch = new CountDownLatch(1);
        doAnswer(invocation -> {
            latch.countDown();
            return false;
        }).when(mBlockCheckerAdapter)
                .isBlocked(any(Context.class), eq(TEST_HANDLE.getSchemeSpecificPart()),
                        any(Bundle.class));

        setEnhancedBlockingEnabled(true);
        CallerInfoLookupHelper.OnQueryCompleteListener queryListener = verifyEnhancedLookupStart();
        queryListener.onCallerInfoQueryComplete(TEST_HANDLE, null);

        waitOnLatch(latch);
        verify(mCallback, timeout(TEST_TIMEOUT))
                .onCallFilteringComplete(eq(mCall), eq(PASS_RESULT));
    }

    private void setEnhancedBlockingEnabled(Boolean value) {
        PersistableBundle bundle = new PersistableBundle();
        bundle.putBoolean(CarrierConfigManager.KEY_SUPPORT_ENHANCED_CALL_BLOCKING_BOOL,
                value);
        when(mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE))
                .thenReturn(mCarrierConfigManager);
        when(mCarrierConfigManager.getConfig()).thenReturn(bundle);
    }

    private CallerInfoLookupHelper.OnQueryCompleteListener verifyEnhancedLookupStart() {
        // The enhanced lookup will only be excuted when enhanced blocking enabled and the
        // presentation is PRESENTATION_ALLOWED.
        when(mCall.getHandlePresentation()).thenReturn(TelecomManager.PRESENTATION_ALLOWED);
        mFilter.startFilterLookup(mCall, mCallback);
        ArgumentCaptor<CallerInfoLookupHelper.OnQueryCompleteListener> captor =
                ArgumentCaptor.forClass(CallerInfoLookupHelper.OnQueryCompleteListener.class);
        verify(mCallerInfoLookupHelper).startLookup(eq(TEST_HANDLE), captor.capture());
        return captor.getValue();
    }

    private void waitOnLatch(CountDownLatch latch) {
        while (latch.getCount() > 0) {
            try {
                latch.await();
            } catch (InterruptedException e) {
                // do nothing
            }
        }
    }
}
