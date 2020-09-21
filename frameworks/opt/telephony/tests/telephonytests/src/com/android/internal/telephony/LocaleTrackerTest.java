/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.internal.telephony;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.isNull;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.AsyncResult;
import android.os.HandlerThread;
import android.telephony.CellIdentityGsm;
import android.telephony.CellInfoGsm;
import android.telephony.ServiceState;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Arrays;

public class LocaleTrackerTest extends TelephonyTest {

    private static final String US_MCC = "310";
    private static final String FAKE_MNC = "123";
    private static final String US_COUNTRY_CODE = "us";
    private static final String COUNTRY_CODE_UNAVAILABLE = "";

    private LocaleTracker mLocaleTracker;
    private LocaleTrackerTestHandler mLocaleTrackerTestHandler;

    private CellInfoGsm mCellInfo;
    private WifiManager mWifiManager;

    private class LocaleTrackerTestHandler extends HandlerThread {

        private LocaleTrackerTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mLocaleTracker = new LocaleTracker(mPhone, this.getLooper());
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        logd("LocaleTrackerTest +Setup!");
        super.setUp(getClass().getSimpleName());

        // This is a workaround to bypass setting system properties, which causes access violation.
        doReturn(-1).when(mPhone).getPhoneId();
        mWifiManager = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);

        mCellInfo = new CellInfoGsm();
        mCellInfo.setCellIdentity(new CellIdentityGsm(Integer.parseInt(US_MCC),
                Integer.parseInt(FAKE_MNC), 0, 0));
        doReturn(Arrays.asList(mCellInfo)).when(mPhone).getAllCellInfo(isNull());
        doReturn(true).when(mSST).getDesiredPowerState();

        mLocaleTrackerTestHandler = new LocaleTrackerTestHandler(getClass().getSimpleName());
        mLocaleTrackerTestHandler.start();
        waitUntilReady();
        logd("LocaleTrackerTest -Setup!");
    }

    @After
    public void tearDown() throws Exception {
        mLocaleTracker.removeCallbacksAndMessages(null);
        mLocaleTrackerTestHandler.quit();
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testUpdateOperatorNumericSync() throws Exception {
        mLocaleTracker.updateOperatorNumericSync(US_MCC + FAKE_MNC);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(US_COUNTRY_CODE);
    }

    @Test
    @SmallTest
    public void testUpdateOperatorNumericAsync() throws Exception {
        mLocaleTracker.updateOperatorNumericAsync(US_MCC + FAKE_MNC);
        waitForMs(100);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(US_COUNTRY_CODE);
    }

    @Test
    @SmallTest
    public void testNoSim() throws Exception {
        mLocaleTracker.updateOperatorNumericAsync("");
        waitForHandlerAction(mLocaleTracker, 100);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(US_COUNTRY_CODE);
    }

    @Test
    @SmallTest
    public void testBootupInAirplaneModeOn() throws Exception {
        doReturn(false).when(mSST).getDesiredPowerState();
        mLocaleTracker.updateOperatorNumericAsync("");
        waitForHandlerAction(mLocaleTracker, 100);
        assertEquals(COUNTRY_CODE_UNAVAILABLE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(COUNTRY_CODE_UNAVAILABLE);
    }

    @Test
    @SmallTest
    public void testTogglingAirplaneMode() throws Exception {
        mLocaleTracker.updateOperatorNumericSync(US_MCC + FAKE_MNC);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(US_COUNTRY_CODE);

        doReturn(false).when(mSST).getDesiredPowerState();
        mLocaleTracker.updateOperatorNumericAsync("");
        waitForHandlerAction(mLocaleTracker, 100);
        assertEquals(COUNTRY_CODE_UNAVAILABLE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(COUNTRY_CODE_UNAVAILABLE);

        doReturn(true).when(mSST).getDesiredPowerState();
        mLocaleTracker.updateOperatorNumericSync(US_MCC + FAKE_MNC);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager, times(2)).setCountryCode(US_COUNTRY_CODE);
    }

    @Test
    @SmallTest
    public void testCellInfoUnavailableRetry() throws Exception {
        doReturn(null).when(mPhone).getAllCellInfo(isNull());
        mLocaleTracker.updateOperatorNumericAsync("");
        waitForHandlerAction(mLocaleTracker, 100);
        assertEquals(COUNTRY_CODE_UNAVAILABLE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(COUNTRY_CODE_UNAVAILABLE);

        doReturn(Arrays.asList(mCellInfo)).when(mPhone).getAllCellInfo(isNull());
        waitForHandlerActionDelayed(mLocaleTracker, 100, 2500);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(US_COUNTRY_CODE);
    }

    @Test
    @SmallTest
    public void testOutOfAirplaneMode() throws Exception {
        doReturn(null).when(mPhone).getAllCellInfo(isNull());
        mLocaleTracker.updateOperatorNumericAsync("");
        waitForHandlerAction(mLocaleTracker, 100);
        assertEquals(COUNTRY_CODE_UNAVAILABLE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(COUNTRY_CODE_UNAVAILABLE);

        doReturn(Arrays.asList(mCellInfo)).when(mPhone).getAllCellInfo(isNull());
        ServiceState ss = new ServiceState();
        ss.setState(ServiceState.STATE_IN_SERVICE);
        AsyncResult ar = new AsyncResult(null, ss, null);
        mLocaleTracker.sendMessage(mLocaleTracker.obtainMessage(3, ar));
        waitForHandlerAction(mLocaleTracker, 100);
        assertEquals(US_COUNTRY_CODE, mLocaleTracker.getCurrentCountry());
        verify(mWifiManager).setCountryCode(US_COUNTRY_CODE);
    }
}
