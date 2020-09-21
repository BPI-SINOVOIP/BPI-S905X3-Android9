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
package com.android.internal.telephony.uicc;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.support.test.filters.SmallTest;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.TelephonyTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

public class UiccControllerTest extends TelephonyTest {
    private UiccController mUiccControllerUT;
    private UiccControllerHandlerThread mUiccControllerHandlerThread;
    private static final int PHONE_COUNT = 1;
    private static int ICC_CHANGED_EVENT = 0;
    @Mock
    private Handler mMockedHandler;
    @Mock
    private IccCardStatus mIccCardStatus;

    private class UiccControllerHandlerThread extends HandlerThread {

        private UiccControllerHandlerThread(String name) {
            super(name);
        }
        @Override
        public void onLooperPrepared() {
            /* create a new UICC Controller associated with the simulated Commands */
            mUiccControllerUT = UiccController.make(mContext,
                    new CommandsInterface[]{mSimulatedCommands});
            setReady(true);
        }
    }

    private IccCardApplicationStatus composeUiccApplicationStatus(
            IccCardApplicationStatus.AppType appType,
            IccCardApplicationStatus.AppState appState, String aid) {
        IccCardApplicationStatus mIccCardAppStatus = new IccCardApplicationStatus();
        mIccCardAppStatus.aid = aid;
        mIccCardAppStatus.app_type = appType;
        mIccCardAppStatus.app_state = appState;
        mIccCardAppStatus.pin1 = mIccCardAppStatus.pin2 =
                IccCardStatus.PinState.PINSTATE_ENABLED_VERIFIED;
        return mIccCardAppStatus;
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(this.getClass().getSimpleName());

        doReturn(PHONE_COUNT).when(mTelephonyManager).getPhoneCount();
        doReturn(PHONE_COUNT).when(mTelephonyManager).getSimCount();
        // set number of slots to 1
        mContextFixture.putIntResource(com.android.internal.R.integer.config_num_physical_slots, 1);

        replaceInstance(UiccController.class, "mInstance", null, null);

        /* null Application associated with any FAM */
        mIccCardStatus.mApplications = new IccCardApplicationStatus[]{};
        mIccCardStatus.mCdmaSubscriptionAppIndex =
                mIccCardStatus.mImsSubscriptionAppIndex =
                        mIccCardStatus.mGsmUmtsSubscriptionAppIndex = -1;
        mSimulatedCommands.setIccCardStatus(mIccCardStatus);
        // slotIndex should be invalid when testing with older versions (before 1.2) of hal
        mIccCardStatus.physicalSlotIndex = UiccController.INVALID_SLOT_ID;
        mUiccControllerHandlerThread = new UiccControllerHandlerThread(TAG);
        mUiccControllerHandlerThread.start();
        waitUntilReady();
        /* expected to get new UiccCards being created
        wait till the async result and message delay */
        waitForMs(100);
    }

    @After
    public void tearDown() throws Exception {
        mUiccControllerHandlerThread.quit();
        super.tearDown();
    }

    @Test @SmallTest
    public void testSanity() {
        // radio power is expected to be on which should trigger icc card and slot status requests
        verify(mSimulatedCommandsVerifier, times(1)).getIccCardStatus(any(Message.class));
        verify(mMockRadioConfig, times(1)).getSimSlotsStatus(any(Message.class));

        // response to getIccCardStatus should create mUiccSlots[0] and UiccCard for it, and update
        // phoneId to slotId mapping
        UiccSlot uiccSlot = mUiccControllerUT.getUiccSlot(0);
        UiccCard uiccCard = mUiccControllerUT.getUiccCardForSlot(0);
        assertNotNull(uiccSlot);
        // this assert verifies that phoneId 0 maps to slotId 0, since UiccCard object for both are
        // same
        assertEquals(uiccCard, mUiccControllerUT.getUiccCardForPhone(0));

        assertNotNull(mUiccControllerUT.getUiccCard(0));
        assertEquals(mSimRecords, mUiccControllerUT.getIccRecords(0, UiccController.APP_FAM_3GPP));
        assertEquals(mRuimRecords, mUiccControllerUT.getIccRecords(0,
                UiccController.APP_FAM_3GPP2));
        assertEquals(mIsimUiccRecords, mUiccControllerUT.getIccRecords(0,
                UiccController.APP_FAM_IMS));
        assertNull(mUiccControllerUT.getIccFileHandler(0, UiccController.APP_FAM_3GPP));
        assertNull(mUiccControllerUT.getIccFileHandler(0, UiccController.APP_FAM_3GPP2));
        assertNull(mUiccControllerUT.getIccFileHandler(0, UiccController.APP_FAM_IMS));
    }

    @Test @SmallTest
    public void testPowerOff() {
        /* Uicc Controller registered for event off to unavail */
        logd("radio power state transition from off to unavail, dispose UICC Card");
        testSanity();
        mSimulatedCommands.requestShutdown(null);
        waitForMs(50);
        assertNull(mUiccControllerUT.getUiccCard(0));
        assertEquals(CommandsInterface.RadioState.RADIO_UNAVAILABLE,
                mSimulatedCommands.getRadioState());
    }

    @Test@SmallTest
    public void testPowerOn() {
        mSimulatedCommands.setRadioPower(true, null);
        waitForMs(500);
        assertNotNull(mUiccControllerUT.getUiccCard(0));
        assertEquals(CommandsInterface.RadioState.RADIO_ON, mSimulatedCommands.getRadioState());
    }

    @Test @SmallTest
    public void testPowerOffPowerOnWithApp() {
         /* update app status and index */
        IccCardApplicationStatus cdmaApp = composeUiccApplicationStatus(
                IccCardApplicationStatus.AppType.APPTYPE_CSIM,
                IccCardApplicationStatus.AppState.APPSTATE_UNKNOWN, "0xA0");
        IccCardApplicationStatus imsApp = composeUiccApplicationStatus(
                IccCardApplicationStatus.AppType.APPTYPE_ISIM,
                IccCardApplicationStatus.AppState.APPSTATE_UNKNOWN, "0xA1");
        IccCardApplicationStatus umtsApp = composeUiccApplicationStatus(
                IccCardApplicationStatus.AppType.APPTYPE_USIM,
                IccCardApplicationStatus.AppState.APPSTATE_UNKNOWN, "0xA2");
        mIccCardStatus.mApplications = new IccCardApplicationStatus[]{cdmaApp, imsApp, umtsApp};
        mIccCardStatus.mCdmaSubscriptionAppIndex = 0;
        mIccCardStatus.mImsSubscriptionAppIndex = 1;
        mIccCardStatus.mGsmUmtsSubscriptionAppIndex = 2;

        mSimulatedCommands.setIccCardStatus(mIccCardStatus);
        logd("radio power state transition from off to unavail");
        testPowerOff();
        /* UICC controller registered for event unavailable to on */
        logd("radio power state transition from unavail to on, update IccCardStatus with app");
        testPowerOn();

        logd("validate Card status with Applications on it followed by Power on");
        assertNotNull(mUiccControllerUT.getUiccCard(0));
        assertNotNull(mUiccControllerUT.getIccRecords(0, UiccController.APP_FAM_3GPP));
        assertNotNull(mUiccControllerUT.getIccRecords(0, UiccController.APP_FAM_3GPP2));
        assertNotNull(mUiccControllerUT.getIccRecords(0, UiccController.APP_FAM_IMS));
        // null because getIccFileHandler() has not been mocked for mocked applications
        assertNull(mUiccControllerUT.getIccFileHandler(0, UiccController.APP_FAM_3GPP));
        assertNull(mUiccControllerUT.getIccFileHandler(0, UiccController.APP_FAM_3GPP2));
        assertNull(mUiccControllerUT.getIccFileHandler(0, UiccController.APP_FAM_IMS));
    }

    @Test @SmallTest
    public void testIccChangedListener() {
        mUiccControllerUT.registerForIccChanged(mMockedHandler, ICC_CHANGED_EVENT, null);
        testPowerOff();
        ArgumentCaptor<Message> mCaptorMessage = ArgumentCaptor.forClass(Message.class);
        ArgumentCaptor<Long> mCaptorLong = ArgumentCaptor.forClass(Long.class);
        verify(mMockedHandler, atLeast(1)).sendMessageDelayed(mCaptorMessage.capture(),
                mCaptorLong.capture());
        assertEquals(ICC_CHANGED_EVENT, mCaptorMessage.getValue().what);
    }
}
