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
 * limitations under the License.
 */

package com.android.internal.telephony;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.isA;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.HandlerThread;
import android.os.Message;
import android.os.PersistableBundle;
import android.provider.Settings;
import android.telephony.CarrierConfigManager;
import android.telephony.ServiceState;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockitoAnnotations;

import java.util.Map;

/**
 * Unit tests for {@link com.android.internal.telephony.CarrierServiceStateTracker}.
 */
public class CarrierServiceStateTrackerTest extends TelephonyTest {
    public static final String LOG_TAG = "CSST";
    public static final int TEST_TIMEOUT = 5000;

    private CarrierServiceStateTracker mSpyCarrierSST;
    private CarrierServiceStateTracker mCarrierSST;
    private CarrierServiceStateTrackerTestHandler mCarrierServiceStateTrackerTestHandler;
    private FakeContentResolver mFakeContentResolver;

    NotificationManager mNotificationManager;
    PersistableBundle mBundle;

    private class FakeContentResolver extends MockContentResolver {
        @Override
        public void notifyChange(Uri uri, ContentObserver observer, boolean syncToNetwork) {
            super.notifyChange(uri, observer, syncToNetwork);
            logd("onChanged(uri=" + uri + ")" + observer);
            if (observer != null) {
                observer.dispatchChange(false, uri);
            }
        }
    }

    private class CarrierServiceStateTrackerTestHandler extends HandlerThread {
        private CarrierServiceStateTrackerTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mCarrierSST = new CarrierServiceStateTracker(mPhone, mSST);
            mSpyCarrierSST = spy(mCarrierSST);
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        logd(LOG_TAG + "Setup!");
        super.setUp(getClass().getSimpleName());
        mBundle = mContextFixture.getCarrierConfigBundle();
        when(mPhone.getSubId()).thenReturn(1);
        mCarrierServiceStateTrackerTestHandler =
                new CarrierServiceStateTrackerTestHandler(getClass().getSimpleName());
        mCarrierServiceStateTrackerTestHandler.start();
        mFakeContentResolver = new CarrierServiceStateTrackerTest.FakeContentResolver();

        when(mPhone.getContext().getContentResolver()).thenReturn(mFakeContentResolver);

        doReturn(new ApplicationInfo()).when(mContext).getApplicationInfo();

        mNotificationManager = (NotificationManager) mContext.getSystemService(
                Context.NOTIFICATION_SERVICE);

        setDefaultValues();
        waitUntilReady();
    }

    private void setDefaultValues() {
        mBundle.putInt(CarrierConfigManager.KEY_PREF_NETWORK_NOTIFICATION_DELAY_INT,
                0);
        mBundle.putInt(CarrierConfigManager.KEY_EMERGENCY_NOTIFICATION_DELAY_INT,
                0);
    }

    @After
    public void tearDown() throws Exception {
        mCarrierServiceStateTrackerTestHandler.quit();
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testCancelBothNotifications() {
        logd(LOG_TAG + ":testCancelBothNotifications()");
        Message notificationMsg = mSpyCarrierSST.obtainMessage(
                CarrierServiceStateTracker.CARRIER_EVENT_DATA_REGISTRATION, null);
        doReturn(false).when(mSpyCarrierSST).evaluateSendingMessage(any());
        doReturn(mNotificationManager).when(mSpyCarrierSST).getNotificationManager(any());
        mSpyCarrierSST.handleMessage(notificationMsg);
        waitForHandlerAction(mSpyCarrierSST, TEST_TIMEOUT);
        verify(mNotificationManager).cancel(
                CarrierServiceStateTracker.NOTIFICATION_EMERGENCY_NETWORK);
        verify(mNotificationManager).cancel(
                CarrierServiceStateTracker.NOTIFICATION_PREF_NETWORK);
    }

    @Test
    @SmallTest
    public void testSendBothNotifications() {
        logd(LOG_TAG + ":testSendBothNotifications()");
        Notification.Builder mNotificationBuilder = new Notification.Builder(mContext);
        Message notificationMsg = mSpyCarrierSST.obtainMessage(
                CarrierServiceStateTracker.CARRIER_EVENT_DATA_DEREGISTRATION, null);
        doReturn(true).when(mSpyCarrierSST).evaluateSendingMessage(any());
        doReturn(false).when(mSpyCarrierSST).isRadioOffOrAirplaneMode();
        doReturn(0).when(mSpyCarrierSST).getDelay(any());
        doReturn(mNotificationBuilder).when(mSpyCarrierSST).getNotificationBuilder(any());
        doReturn(mNotificationManager).when(mSpyCarrierSST).getNotificationManager(any());
        mSpyCarrierSST.handleMessage(notificationMsg);
        waitForHandlerAction(mSpyCarrierSST, TEST_TIMEOUT);
        verify(mNotificationManager).notify(
                eq(CarrierServiceStateTracker.NOTIFICATION_PREF_NETWORK), isA(Notification.class));
        verify(mNotificationManager).notify(
                eq(CarrierServiceStateTracker.NOTIFICATION_EMERGENCY_NETWORK), any());
    }

    @Test
    @SmallTest
    public void testSendPrefNetworkNotification() {
        logd(LOG_TAG + ":testSendPrefNetworkNotification()");
        Intent intent = new Intent().setAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        mContext.sendBroadcast(intent);
        waitForMs(300);

        Map<Integer, CarrierServiceStateTracker.NotificationType> notificationTypeMap =
                mCarrierSST.getNotificationTypeMap();
        CarrierServiceStateTracker.NotificationType prefNetworkNotification =
                notificationTypeMap.get(CarrierServiceStateTracker.NOTIFICATION_PREF_NETWORK);
        CarrierServiceStateTracker.NotificationType spyPrefNetworkNotification = spy(
                prefNetworkNotification);
        notificationTypeMap.put(CarrierServiceStateTracker.NOTIFICATION_PREF_NETWORK,
                spyPrefNetworkNotification);
        Notification.Builder mNotificationBuilder = new Notification.Builder(mContext);
        doReturn(ServiceState.STATE_OUT_OF_SERVICE).when(mSST.mSS).getVoiceRegState();
        doReturn(ServiceState.STATE_OUT_OF_SERVICE).when(mSST.mSS).getDataRegState();
        doReturn(true).when(mSST).isRadioOn();
        doReturn(mNotificationBuilder).when(spyPrefNetworkNotification).getNotificationBuilder();

        String prefNetworkMode = Settings.Global.PREFERRED_NETWORK_MODE + mPhone.getSubId();
        Settings.Global.putInt(mFakeContentResolver, prefNetworkMode,
                RILConstants.NETWORK_MODE_LTE_CDMA_EVDO);
        mFakeContentResolver.notifyChange(
                Settings.Global.getUriFor(prefNetworkMode), mSpyCarrierSST.getContentObserver());
        waitForMs(500);
        verify(mNotificationManager).notify(
                eq(CarrierServiceStateTracker.NOTIFICATION_PREF_NETWORK), isA(Notification.class));

        Settings.Global.putInt(mFakeContentResolver, prefNetworkMode,
                RILConstants.NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA);
        mFakeContentResolver.notifyChange(
                Settings.Global.getUriFor(prefNetworkMode), mSpyCarrierSST.getContentObserver());
        waitForMs(500);
        verify(mNotificationManager, atLeast(1)).cancel(
                CarrierServiceStateTracker.NOTIFICATION_PREF_NETWORK);
    }
}
