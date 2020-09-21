/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.service.ims.presence;

import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.CarrierConfigManager;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

import static junit.framework.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;

/**
 * Unit Tests for PollingService.
 */
@RunWith(AndroidJUnit4.class)
public class PollingServiceTests extends PresencePollingTestBase {

    @Mock BroadcastReceiver mReceiver;

    private PollingService mService;

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        mService = new PollingService();
        mService.attach(mTestContext,null, null, null, mApp, null);
    }

    @After
    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        mService = null;
    }

    @SmallTest
    @Test
    public void testRegisterCarrierConfigReceiver() throws Exception {
        mService.setBroadcastReceiver(mReceiver);
        mService.onCreate();
        ArgumentCaptor<IntentFilter> captor = ArgumentCaptor.forClass(IntentFilter.class);
        verify(mTestContext).registerReceiver(eq(mReceiver), captor.capture());
        IntentFilter f = captor.getValue();
        assertEquals(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED, f.getAction(0));
    }

    @SmallTest
    @Test
    public void testUnregisterCarrierConfigReceiver() throws Exception {
        mService.setBroadcastReceiver(mReceiver);
        mService.onDestroy();
        verify(mTestContext).unregisterReceiver(eq(mReceiver));
    }
}
