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

package com.android.internal.telephony;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.os.HandlerThread;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;

public class ImsSmsDispatcherTest extends TelephonyTest {
    @Mock private SmsDispatchersController mSmsDispatchersController;
    @Mock private SMSDispatcher.SmsTracker mSmsTracker;
    private ImsSmsDispatcher mImsSmsDispatcher;
    private ImsSmsDispatcherTestHandler mImsSmsDispatcherTestHandler;


    private class ImsSmsDispatcherTestHandler extends HandlerThread {

        private ImsSmsDispatcherTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mImsSmsDispatcher = spy(new ImsSmsDispatcher(mPhone, mSmsDispatchersController));
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());

        mImsSmsDispatcherTestHandler = new ImsSmsDispatcherTestHandler(TAG);
        mImsSmsDispatcherTestHandler.start();
        waitUntilReady();
    }

    @Test @SmallTest
    public void testSendSms() {
        int token = mImsSmsDispatcher.mNextToken.get();
        int trackersSize = mImsSmsDispatcher.mTrackers.size();

        mImsSmsDispatcher.sendSms(mSmsTracker);

        assertEquals(token + 1, mImsSmsDispatcher.mNextToken.get());
        assertEquals(trackersSize + 1, mImsSmsDispatcher.mTrackers.size());
    }

    @Test @SmallTest
    public void testFallbackToPstn() {
        int token = 0;
        mImsSmsDispatcher.mTrackers.put(token, mSmsTracker);
        doNothing().when(mSmsDispatchersController).sendRetrySms(mSmsTracker);

        mImsSmsDispatcher.fallbackToPstn(token, mSmsTracker);

        verify(mSmsDispatchersController).sendRetrySms(mSmsTracker);
        assertNull(mImsSmsDispatcher.mTrackers.get(token));
    }

    @After
    public void tearDown() throws Exception {
        mImsSmsDispatcher = null;
        mImsSmsDispatcherTestHandler.quit();
        super.tearDown();
    }
}
