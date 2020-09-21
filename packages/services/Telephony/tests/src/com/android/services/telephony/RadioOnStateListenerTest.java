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
 * limitations under the License.
 */

package com.android.services.telephony;

import android.os.AsyncResult;
import android.os.Handler;
import android.telephony.ServiceState;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.filters.FlakyTest;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.TelephonyTestBase;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.ServiceStateTracker;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;

import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Matchers.isNull;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.when;

/**
 * Tests the RadioOnStateListener, which listens to one Phone and waits until its service
 * state changes to accepting emergency calls or in service. If it can not find a tower to camp onto
 * for emergency calls, then it will fail after a timeout period.
 */
@RunWith(AndroidJUnit4.class)
public class RadioOnStateListenerTest extends TelephonyTestBase {

    private static final long TIMEOUT_MS = 100;

    @Mock Phone mMockPhone;
    @Mock RadioOnStateListener.Callback mCallback;
    RadioOnStateListener mListener;

    @Before
    public void setUp() throws Exception {
        super.setUp();
        mListener = new RadioOnStateListener();
    }

    @After
    public void tearDown() throws Exception {
        mListener.getHandler().removeCallbacksAndMessages(null);
        super.tearDown();
    }

    /**
     * Ensure that we successfully register for the ServiceState changed messages in Telephony.
     */
    @Test
    @SmallTest
    public void testRegisterForCallback() {
        mListener.waitForRadioOn(mMockPhone, mCallback);

        waitForHandlerAction(mListener.getHandler(), TIMEOUT_MS);

        verify(mMockPhone).unregisterForServiceStateChanged(any(Handler.class));
        verify(mMockPhone).registerForServiceStateChanged(any(Handler.class),
                eq(RadioOnStateListener.MSG_SERVICE_STATE_CHANGED), isNull());
    }

    /**
     * {@link RadioOnStateListener.Callback#isOkToCall(int)} returns true, so we are expecting
     * {@link RadioOnStateListener.Callback#onComplete(boolean)} to return true.
     */
    @Test
    @SmallTest
    public void testPhoneChangeState_OkToCallTrue() {
        ServiceState state = new ServiceState();
        state.setState(ServiceState.STATE_IN_SERVICE);
        when(mMockPhone.getState()).thenReturn(PhoneConstants.State.IDLE);
        when(mCallback.isOkToCall(eq(mMockPhone), anyInt())).thenReturn(true);
        mListener.waitForRadioOn(mMockPhone, mCallback);
        waitForHandlerAction(mListener.getHandler(), TIMEOUT_MS);

        mListener.getHandler().obtainMessage(RadioOnStateListener.MSG_SERVICE_STATE_CHANGED,
                new AsyncResult(null, state, null)).sendToTarget();

        waitForHandlerAction(mListener.getHandler(), TIMEOUT_MS);
        verify(mCallback).onComplete(eq(mListener), eq(true));
    }

    /**
     * We never receive a {@link RadioOnStateListener.Callback#onComplete(boolean)} because
     * {@link RadioOnStateListener.Callback#isOkToCall(int)} returns false.
     */
    @Test
    @SmallTest
    public void testPhoneChangeState_NoOkToCall_Timeout() {
        ServiceState state = new ServiceState();
        state.setState(ServiceState.STATE_OUT_OF_SERVICE);
        when(mMockPhone.getState()).thenReturn(PhoneConstants.State.IDLE);
        when(mCallback.isOkToCall(eq(mMockPhone), anyInt())).thenReturn(false);
        when(mMockPhone.getServiceState()).thenReturn(state);
        mListener.waitForRadioOn(mMockPhone, mCallback);
        waitForHandlerAction(mListener.getHandler(), TIMEOUT_MS);

        mListener.getHandler().obtainMessage(RadioOnStateListener.MSG_SERVICE_STATE_CHANGED,
                new AsyncResult(null, state, null)).sendToTarget();

        waitForHandlerAction(mListener.getHandler(), TIMEOUT_MS);
        verify(mCallback, never()).onComplete(any(RadioOnStateListener.class), anyBoolean());
    }

    /**
     * Tests {@link RadioOnStateListener.Callback#isOkToCall(int)} returning false and hitting the
     * max number of retries. This should result in
     * {@link RadioOnStateListener.Callback#onComplete(boolean)} returning false.
     */
    @Test
    @FlakyTest
    public void testTimeout_RetryFailure() {
        ServiceState state = new ServiceState();
        state.setState(ServiceState.STATE_POWER_OFF);
        when(mMockPhone.getState()).thenReturn(PhoneConstants.State.IDLE);
        when(mMockPhone.getServiceState()).thenReturn(state);
        when(mCallback.isOkToCall(eq(mMockPhone), anyInt())).thenReturn(false);
        mListener.setTimeBetweenRetriesMillis(50);
        mListener.setMaxNumRetries(2);

        // Wait for the timer to expire and check state manually in onRetryTimeout
        mListener.waitForRadioOn(mMockPhone, mCallback);
        waitForHandlerActionDelayed(mListener.getHandler(), TIMEOUT_MS, 500);

        verify(mCallback).onComplete(eq(mListener), eq(false));
        verify(mMockPhone, times(2)).setRadioPower(eq(true));
    }

}
