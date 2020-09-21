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

import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ALLOW_DATA;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_CHANGE_SIM_PIN;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_CHANGE_SIM_PIN2;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_CONFERENCE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_DATA_REGISTRATION_STATE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_DELETE_SMS_ON_SIM;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_DEVICE_IDENTITY;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_DTMF;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ENTER_SIM_PIN;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ENTER_SIM_PIN2;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ENTER_SIM_PUK;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_ENTER_SIM_PUK2;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_ACTIVITY_INFO;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_CELL_INFO_LIST;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_CURRENT_CALLS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_HARDWARE_CONFIG;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_IMSI;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_RADIO_CAPABILITY;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_SIM_STATUS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_SMSC_ADDRESS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_HANGUP;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_IMS_REGISTRATION_STATE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_IMS_SEND_SMS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_LAST_CALL_FAIL_CAUSE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_NV_READ_ITEM;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_NV_RESET_CONFIG;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_NV_WRITE_ITEM;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_OPERATOR;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_PULL_LCEDATA;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_RADIO_POWER;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_REPORT_SMS_MEMORY_STATUS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SEND_DEVICE_STATE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SEND_SMS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SEND_SMS_EXPECT_MORE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SETUP_DATA_CALL;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SET_INITIAL_ATTACH_APN;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SET_SIM_CARD_POWER;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SET_SMSC_ADDRESS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SET_UNSOLICITED_RESPONSE_FILTER;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SHUTDOWN;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SIGNAL_STRENGTH;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SIM_AUTHENTICATION;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SIM_CLOSE_CHANNEL;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SIM_OPEN_CHANNEL;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_START_LCE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_STOP_LCE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_UDUB;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_VOICE_RADIO_TECH;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_VOICE_REGISTRATION_STATE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_WRITE_SMS_TO_SIM;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.hardware.radio.V1_0.CdmaSmsMessage;
import android.hardware.radio.V1_0.DataProfileInfo;
import android.hardware.radio.V1_0.GsmSmsMessage;
import android.hardware.radio.V1_0.IRadio;
import android.hardware.radio.V1_0.ImsSmsMessage;
import android.hardware.radio.V1_0.NvWriteItem;
import android.hardware.radio.V1_0.RadioError;
import android.hardware.radio.V1_0.RadioResponseInfo;
import android.hardware.radio.V1_0.RadioResponseType;
import android.hardware.radio.V1_0.SmsWriteArgs;
import android.hardware.radio.deprecated.V1_0.IOemHook;
import android.net.ConnectivityManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IPowerManager;
import android.os.Message;
import android.os.PowerManager;
import android.os.WorkSource;
import android.support.test.filters.FlakyTest;
import android.telephony.AccessNetworkConstants;
import android.telephony.CellIdentityCdma;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.CellSignalStrengthCdma;
import android.telephony.CellSignalStrengthGsm;
import android.telephony.CellSignalStrengthLte;
import android.telephony.CellSignalStrengthWcdma;
import android.telephony.SmsManager;
import android.telephony.TelephonyManager;
import android.telephony.data.DataProfile;

import com.android.internal.telephony.RIL.RilHandler;
import com.android.internal.telephony.dataconnection.ApnSetting;
import com.android.internal.telephony.dataconnection.DcTracker;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;

public class RILTest extends TelephonyTest {

    // refer to RIL#DEFAULT_BLOCKING_MESSAGE_RESPONSE_TIMEOUT_MS
    private static final int DEFAULT_BLOCKING_MESSAGE_RESPONSE_TIMEOUT_MS = 2000;

    // refer to RIL#DEFAULT_WAKE_LOCK_TIMEOUT_MS
    private static final int DEFAULT_WAKE_LOCK_TIMEOUT_MS = 60000;

    @Mock
    private ConnectivityManager mConnectionManager;
    @Mock
    private IRadio mRadioProxy;
    @Mock
    private IOemHook mOemHookProxy;

    private RilHandler mRilHandler;
    private RIL mRILInstance;
    private RIL mRILUnderTest;
    private RILTestHandler mTestHandler;
    ArgumentCaptor<Integer> mSerialNumberCaptor = ArgumentCaptor.forClass(Integer.class);

    // Constants
    private static final String ALPHA_LONG = "long";
    private static final String ALPHA_SHORT = "short";
    private static final int ARFCN = 690;
    private static final int BASESTATION_ID = 65531;
    private static final int BIT_ERROR_RATE = 99;
    private static final int BSIC = 8;
    private static final int CI = 268435456;
    private static final int CID = 65535;
    private static final int CQI = 2147483647;
    private static final int DBM = 74;
    private static final int EARFCN = 262140;
    private static final int BANDWIDTH = 5000;
    private static final int ECIO = 124;
    private static final String EMPTY_ALPHA_LONG = "";
    private static final String EMPTY_ALPHA_SHORT = "";
    private static final int LAC = 65535;
    private static final int LATITUDE = 1292000;
    private static final int LONGITUDE = 1295000;
    private static final int MCC = 120;
    private static final String MCC_STR = "120";
    private static final int MNC = 260;
    private static final String MNC_STR = "260";
    private static final int NETWORK_ID = 65534;
    private static final int PCI = 503;
    private static final int PSC = 500;
    private static final int RIL_TIMESTAMP_TYPE_OEM_RIL = 3;
    private static final int RSSNR = 2147483647;
    private static final int RSRP = 96;
    private static final int RSRQ = 10;
    private static final int SIGNAL_NOISE_RATIO = 6;
    private static final int SIGNAL_STRENGTH = 24;
    private static final int SYSTEM_ID = 65533;
    private static final int TAC = 65535;
    private static final int TIME_ADVANCE = 4;
    private static final long TIMESTAMP = 215924934;
    private static final int UARFCN = 690;
    private static final int TYPE_CDMA = 2;
    private static final int TYPE_GSM = 1;
    private static final int TYPE_LTE = 3;
    private static final int TYPE_WCDMA = 4;

    private static final int PROFILE_ID = 0;
    private static final String APN = "apn";
    private static final String PROTOCOL = "IPV6";
    private static final int AUTH_TYPE = 0;
    private static final String USER_NAME = "username";
    private static final String PASSWORD = "password";
    private static final int TYPE = 0;
    private static final int MAX_CONNS_TIME = 1;
    private static final int MAX_CONNS = 3;
    private static final int WAIT_TIME = 10;
    private static final boolean APN_ENABLED = true;
    private static final int SUPPORTED_APNT_YPES_BITMAP = 123456;
    private static final String ROAMING_PROTOCOL = "IPV6";
    private static final int BEARER_BITMAP = 123123;
    private static final int MTU = 1234;
    private static final String MVNO_TYPE = "";
    private static final String MVNO_MATCH_DATA = "";
    private static final boolean MODEM_COGNITIVE = true;

    private class RILTestHandler extends HandlerThread {

        RILTestHandler(String name) {
            super(name);
        }

        @Override
        protected void onLooperPrepared() {
            super.onLooperPrepared();
            createTelephonyDevController();
            Context context = new ContextFixture().getTestDouble();
            doReturn(true).when(mConnectionManager)
                    .isNetworkSupported(ConnectivityManager.TYPE_MOBILE);
            doReturn(mConnectionManager).when(context)
                    .getSystemService(Context.CONNECTIVITY_SERVICE);
            PowerManager powerManager = createPowerManager(context);
            doReturn(powerManager).when(context).getSystemService(Context.POWER_SERVICE);
            doReturn(new ApplicationInfo()).when(context).getApplicationInfo();

            mRILInstance = new RIL(context, RILConstants.PREFERRED_NETWORK_MODE,
                    Phone.PREFERRED_CDMA_SUBSCRIPTION, 0);
            mRILUnderTest = spy(mRILInstance);
            doReturn(mRadioProxy).when(mRILUnderTest).getRadioProxy(any());
            doReturn(mOemHookProxy).when(mRILUnderTest).getOemHookProxy(any());

            mRilHandler = mRILUnderTest.getRilHandler();

            setReady(true);
        }

        private PowerManager createPowerManager(Context context) {
            return new PowerManager(context, mock(IPowerManager.class), new Handler(getLooper()));
        }

        private void createTelephonyDevController() {
            try {
                TelephonyDevController.create();
            } catch (RuntimeException e) {
            }
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(RILTest.class.getSimpleName());
        MockitoAnnotations.initMocks(this);
        mTestHandler = new RILTestHandler(getClass().getSimpleName());
        mTestHandler.start();
        waitUntilReady();
    }

    @After
    public void tearDown() throws Exception {
        mTestHandler.quit();
        super.tearDown();
    }

    @FlakyTest
    @Test
    public void testGetIccCardStatus() throws Exception {
        mRILUnderTest.getIccCardStatus(obtainMessage());
        verify(mRadioProxy).getIccCardStatus(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_SIM_STATUS);
    }

    @FlakyTest
    @Test
    public void testSupplyIccPinForApp() throws Exception {
        String pin = "1234";
        String aid = "2345";
        mRILUnderTest.supplyIccPinForApp(pin, aid, obtainMessage());
        verify(mRadioProxy).supplyIccPinForApp(mSerialNumberCaptor.capture(), eq(pin), eq(aid));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_ENTER_SIM_PIN);
    }

    @FlakyTest
    @Test
    public void testSupplyIccPukForApp() throws Exception {
        String puk = "pukcode";
        String newPin = "1314";
        String aid = "2345";
        mRILUnderTest.supplyIccPukForApp(puk, newPin, aid, obtainMessage());
        verify(mRadioProxy)
                .supplyIccPukForApp(mSerialNumberCaptor.capture(), eq(puk), eq(newPin), eq(aid));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_ENTER_SIM_PUK);
    }

    @FlakyTest
    @Test
    public void testSupplyIccPin2ForApp() throws Exception {
        String pin = "1234";
        String aid = "2345";
        mRILUnderTest.supplyIccPin2ForApp(pin, aid, obtainMessage());
        verify(mRadioProxy).supplyIccPin2ForApp(
                mSerialNumberCaptor.capture(), eq(pin), eq(aid));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_ENTER_SIM_PIN2);
    }

    @FlakyTest
    @Test
    public void testSupplyIccPuk2ForApp() throws Exception {
        String puk = "pukcode";
        String newPin = "1314";
        String aid = "2345";
        mRILUnderTest.supplyIccPuk2ForApp(puk, newPin, aid, obtainMessage());
        verify(mRadioProxy)
                .supplyIccPuk2ForApp(mSerialNumberCaptor.capture(), eq(puk), eq(newPin), eq(aid));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_ENTER_SIM_PUK2);
    }

    @FlakyTest
    @Test
    public void testChangeIccPinForApp() throws Exception {
        String oldPin = "1234";
        String newPin = "1314";
        String aid = "2345";
        mRILUnderTest.changeIccPinForApp(oldPin, newPin, aid, obtainMessage());
        verify(mRadioProxy).changeIccPinForApp(
                mSerialNumberCaptor.capture(), eq(oldPin), eq(newPin), eq(aid));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_CHANGE_SIM_PIN);
    }

    @FlakyTest
    @Test
    public void testChangeIccPin2ForApp() throws Exception {
        String oldPin2 = "1234";
        String newPin2 = "1314";
        String aid = "2345";
        mRILUnderTest.changeIccPin2ForApp(oldPin2, newPin2, aid, obtainMessage());
        verify(mRadioProxy).changeIccPin2ForApp(
                mSerialNumberCaptor.capture(), eq(oldPin2), eq(newPin2), eq(aid));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_CHANGE_SIM_PIN2);
    }

    @FlakyTest
    @Test
    public void testSupplyNetworkDepersonalization() throws Exception {
        String netpin = "1234";
        mRILUnderTest.supplyNetworkDepersonalization(netpin, obtainMessage());
        verify(mRadioProxy).supplyNetworkDepersonalization(
                mSerialNumberCaptor.capture(), eq(netpin));
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION);
    }

    @FlakyTest
    @Test
    public void testGetCurrentCalls() throws Exception {
        mRILUnderTest.getCurrentCalls(obtainMessage());
        verify(mRadioProxy).getCurrentCalls(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_CURRENT_CALLS);
    }

    @FlakyTest
    @Test
    public void testGetIMSIForApp() throws Exception {
        String aid = "1234";
        mRILUnderTest.getIMSIForApp(aid, obtainMessage());
        verify(mRadioProxy).getImsiForApp(mSerialNumberCaptor.capture(), eq(aid));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_IMSI);
    }

    @FlakyTest
    @Test
    public void testHangupWaitingOrBackground() throws Exception {
        mRILUnderTest.hangupWaitingOrBackground(obtainMessage());
        verify(mRadioProxy).hangupWaitingOrBackground(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND);
    }

    @FlakyTest
    @Test
    public void testHangupForegroundResumeBackground() throws Exception {
        mRILUnderTest.hangupForegroundResumeBackground(obtainMessage());
        verify(mRadioProxy).hangupForegroundResumeBackground(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND);
    }

    @FlakyTest
    @Test
    public void testHangupConnection() throws Exception {
        int gsmIndex = 0;
        mRILUnderTest.hangupConnection(gsmIndex, obtainMessage());
        verify(mRadioProxy).hangup(mSerialNumberCaptor.capture(), eq(gsmIndex));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_HANGUP);
    }

    @FlakyTest
    @Test
    public void testSwitchWaitingOrHoldingAndActive() throws Exception {
        mRILUnderTest.switchWaitingOrHoldingAndActive(obtainMessage());
        verify(mRadioProxy).switchWaitingOrHoldingAndActive(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE);
    }

    @FlakyTest
    @Test
    public void testConference() throws Exception {
        mRILUnderTest.conference(obtainMessage());
        verify(mRadioProxy).conference(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_CONFERENCE);
    }

    @FlakyTest
    @Test
    public void testRejectCall() throws Exception {
        mRILUnderTest.rejectCall(obtainMessage());
        verify(mRadioProxy).rejectCall(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_UDUB);
    }

    @FlakyTest
    @Test
    public void testGetLastCallFailCause() throws Exception {
        mRILUnderTest.getLastCallFailCause(obtainMessage());
        verify(mRadioProxy).getLastCallFailCause(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_LAST_CALL_FAIL_CAUSE);
    }

    @FlakyTest
    @Test
    public void testGetSignalStrength() throws Exception {
        mRILUnderTest.getSignalStrength(obtainMessage());
        verify(mRadioProxy).getSignalStrength(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SIGNAL_STRENGTH);
    }

    @FlakyTest
    @Test
    public void testGetVoiceRegistrationState() throws Exception {
        mRILUnderTest.getVoiceRegistrationState(obtainMessage());
        verify(mRadioProxy).getVoiceRegistrationState(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_VOICE_REGISTRATION_STATE);
    }

    @FlakyTest
    @Test
    public void testGetDataRegistrationState() throws Exception {
        mRILUnderTest.getDataRegistrationState(obtainMessage());
        verify(mRadioProxy).getDataRegistrationState(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_DATA_REGISTRATION_STATE);
    }

    @FlakyTest
    @Test
    public void testGetOperator() throws Exception {
        mRILUnderTest.getOperator(obtainMessage());
        verify(mRadioProxy).getOperator(mSerialNumberCaptor.capture());
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_OPERATOR);
    }

    @FlakyTest
    @Test
    public void testSetRadioPower() throws Exception {
        boolean on = true;
        mRILUnderTest.setRadioPower(on, obtainMessage());
        verify(mRadioProxy).setRadioPower(mSerialNumberCaptor.capture(), eq(on));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_RADIO_POWER);
    }

    @FlakyTest
    @Test
    public void testSendDtmf() throws Exception {
        char c = 'c';
        mRILUnderTest.sendDtmf(c, obtainMessage());
        verify(mRadioProxy).sendDtmf(mSerialNumberCaptor.capture(), eq(c + ""));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_DTMF);
    }

    @FlakyTest
    @Test
    public void testSendSMS() throws Exception {
        String smscPdu = "smscPdu";
        String pdu = "pdu";
        GsmSmsMessage msg = new GsmSmsMessage();
        msg.smscPdu = smscPdu;
        msg.pdu = pdu;
        mRILUnderTest.sendSMS(smscPdu, pdu, obtainMessage());
        verify(mRadioProxy).sendSms(mSerialNumberCaptor.capture(), eq(msg));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SEND_SMS);
    }

    @FlakyTest
    @Test
    public void testSendSMSExpectMore() throws Exception {
        String smscPdu = "smscPdu";
        String pdu = "pdu";
        GsmSmsMessage msg = new GsmSmsMessage();
        msg.smscPdu = smscPdu;
        msg.pdu = pdu;
        mRILUnderTest.sendSMSExpectMore(smscPdu, pdu, obtainMessage());
        verify(mRadioProxy).sendSMSExpectMore(mSerialNumberCaptor.capture(), eq(msg));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SEND_SMS_EXPECT_MORE);
    }

    @FlakyTest
    @Test
    public void testWriteSmsToSim() throws Exception {
        String smscPdu = "smscPdu";
        String pdu = "pdu";
        int status = SmsManager.STATUS_ON_ICC_READ;
        SmsWriteArgs args = new SmsWriteArgs();
        args.status = 1;
        args.smsc = smscPdu;
        args.pdu = pdu;
        mRILUnderTest.writeSmsToSim(status, smscPdu, pdu, obtainMessage());
        verify(mRadioProxy).writeSmsToSim(mSerialNumberCaptor.capture(), eq(args));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_WRITE_SMS_TO_SIM);
    }

    @FlakyTest
    @Test
    public void testDeleteSmsOnSim() throws Exception {
        int index = 0;
        mRILUnderTest.deleteSmsOnSim(index, obtainMessage());
        verify(mRadioProxy).deleteSmsOnSim(mSerialNumberCaptor.capture(), eq(index));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_DELETE_SMS_ON_SIM);
    }

    @FlakyTest
    @Test
    public void testGetDeviceIdentity() throws Exception {
        mRILUnderTest.getDeviceIdentity(obtainMessage());
        verify(mRadioProxy).getDeviceIdentity(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_DEVICE_IDENTITY);
    }

    @FlakyTest
    @Test
    public void testExitEmergencyCallbackMode() throws Exception {
        mRILUnderTest.exitEmergencyCallbackMode(obtainMessage());
        verify(mRadioProxy).exitEmergencyCallbackMode(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE);
    }

    @FlakyTest
    @Test
    public void testGetSmscAddress() throws Exception {
        mRILUnderTest.getSmscAddress(obtainMessage());
        verify(mRadioProxy).getSmscAddress(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_SMSC_ADDRESS);
    }

    @FlakyTest
    @Test
    public void testSetSmscAddress() throws Exception {
        String address = "address";
        mRILUnderTest.setSmscAddress(address, obtainMessage());
        verify(mRadioProxy).setSmscAddress(mSerialNumberCaptor.capture(), eq(address));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SET_SMSC_ADDRESS);
    }

    @FlakyTest
    @Test
    public void testReportSmsMemoryStatus() throws Exception {
        boolean available = true;
        mRILUnderTest.reportSmsMemoryStatus(available, obtainMessage());
        verify(mRadioProxy).reportSmsMemoryStatus(mSerialNumberCaptor.capture(), eq(available));
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_REPORT_SMS_MEMORY_STATUS);
    }

    @FlakyTest
    @Test
    public void testReportStkServiceIsRunning() throws Exception {
        mRILUnderTest.reportStkServiceIsRunning(obtainMessage());
        verify(mRadioProxy).reportStkServiceIsRunning(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING);
    }

    @FlakyTest
    @Test
    public void testGetCdmaSubscriptionSource() throws Exception {
        mRILUnderTest.getCdmaSubscriptionSource(obtainMessage());
        verify(mRadioProxy).getCdmaSubscriptionSource(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE);
    }

    @FlakyTest
    @Test
    public void testAcknowledgeIncomingGsmSmsWithPdu() throws Exception {
        boolean success = true;
        String ackPdu = "ackPdu";
        mRILUnderTest.acknowledgeIncomingGsmSmsWithPdu(success, ackPdu, obtainMessage());
        verify(mRadioProxy).acknowledgeIncomingGsmSmsWithPdu(
                mSerialNumberCaptor.capture(), eq(success), eq(ackPdu));
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU);
    }

    @FlakyTest
    @Test
    public void testGetVoiceRadioTechnology() throws Exception {
        mRILUnderTest.getVoiceRadioTechnology(obtainMessage());
        verify(mRadioProxy).getVoiceRadioTechnology(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_VOICE_RADIO_TECH);
    }

    @FlakyTest
    @Test
    public void testGetCellInfoList() throws Exception {
        mRILUnderTest.getCellInfoList(obtainMessage(), null);
        verify(mRadioProxy).getCellInfoList(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_CELL_INFO_LIST);
    }

    @FlakyTest
    @Test
    public void testSetCellInfoListRate() throws Exception {
        int rateInMillis = 1000;
        mRILUnderTest.setCellInfoListRate(rateInMillis, obtainMessage(), null);
        verify(mRadioProxy).setCellInfoListRate(mSerialNumberCaptor.capture(), eq(rateInMillis));
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE);
    }

    @FlakyTest
    @Test
    public void testSetInitialAttachApn() throws Exception {
        ApnSetting apnSetting = new ApnSetting(
                -1, "22210", "Vodafone IT", "web.omnitel.it", "", "",
                "", "", "", "", "", 0, new String[]{"DUN"}, "IP", "IP", true, 0, 0,
                0, false, 0, 0, 0, 0, "", "");
        DataProfile dataProfile = DcTracker.createDataProfile(apnSetting, apnSetting.profileId);
        boolean isRoaming = false;

        mRILUnderTest.setInitialAttachApn(dataProfile, isRoaming, obtainMessage());
        verify(mRadioProxy).setInitialAttachApn(
                mSerialNumberCaptor.capture(),
                eq((DataProfileInfo) invokeMethod(
                        mRILInstance,
                        "convertToHalDataProfile",
                        new Class<?>[] {DataProfile.class},
                        new Object[] {dataProfile})),
                eq(dataProfile.isModemCognitive()),
                eq(isRoaming));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SET_INITIAL_ATTACH_APN);
    }

    @FlakyTest
    @Test
    public void testGetImsRegistrationState() throws Exception {
        mRILUnderTest.getImsRegistrationState(obtainMessage());
        verify(mRadioProxy).getImsRegistrationState(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_IMS_REGISTRATION_STATE);
    }

    @FlakyTest
    @Test
    public void testSendRetryImsGsmSms() throws Exception {
        String smscPdu = "smscPdu";
        String pdu = "pdu";
        GsmSmsMessage gsmMsg = new GsmSmsMessage();
        gsmMsg.smscPdu = smscPdu;
        gsmMsg.pdu = pdu;

        ImsSmsMessage firstMsg = new ImsSmsMessage();
        firstMsg.tech = RILConstants.GSM_PHONE;
        firstMsg.retry = false;
        firstMsg.messageRef = 0;
        firstMsg.gsmMessage.add(gsmMsg);

        ImsSmsMessage retryMsg = new ImsSmsMessage();
        retryMsg.tech = RILConstants.GSM_PHONE;
        retryMsg.retry = true;
        retryMsg.messageRef = 0;
        retryMsg.gsmMessage.add(gsmMsg);

        int maxRetryCount = 3;
        int firstTransmission = 0;
        for (int i = 0; i <= maxRetryCount; i++) {
            mRILUnderTest.sendImsGsmSms(smscPdu, pdu, i, 0, obtainMessage());
            if (i == firstTransmission) {
                verify(mRadioProxy, times(1)).sendImsSms(mSerialNumberCaptor.capture(),
                        eq(firstMsg));
                verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(),
                        RIL_REQUEST_IMS_SEND_SMS);
            } else {
                verify(mRadioProxy, times(i)).sendImsSms(mSerialNumberCaptor.capture(),
                        eq(retryMsg));
                verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(),
                        RIL_REQUEST_IMS_SEND_SMS);
            }
        }
    }

    @FlakyTest
    @Test
    public void testSendRetryImsCdmaSms() throws Exception {
        CdmaSmsMessage cdmaMsg = new CdmaSmsMessage();

        ImsSmsMessage firstMsg = new ImsSmsMessage();
        firstMsg.tech = RILConstants.CDMA_PHONE;
        firstMsg.retry = false;
        firstMsg.messageRef = 0;
        firstMsg.cdmaMessage.add(cdmaMsg);

        ImsSmsMessage retryMsg = new ImsSmsMessage();
        retryMsg.tech = RILConstants.CDMA_PHONE;
        retryMsg.retry = true;
        retryMsg.messageRef = 0;
        retryMsg.cdmaMessage.add(cdmaMsg);

        int maxRetryCount = 3;
        int firstTransmission = 0;
        byte pdu[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        for (int i = 0; i <= maxRetryCount; i++) {
            mRILUnderTest.sendImsCdmaSms(pdu, i, 0, obtainMessage());
            if (i == firstTransmission) {
                verify(mRadioProxy, times(1)).sendImsSms(mSerialNumberCaptor.capture(),
                        eq(firstMsg));
                verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(),
                        RIL_REQUEST_IMS_SEND_SMS);
            } else {
                verify(mRadioProxy, times(i)).sendImsSms(mSerialNumberCaptor.capture(),
                        eq(retryMsg));
                verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(),
                        RIL_REQUEST_IMS_SEND_SMS);
            }
        }
    }

    @FlakyTest
    @Test
    public void testIccOpenLogicalChannel() throws Exception {
        String aid = "aid";
        int p2 = 0;
        mRILUnderTest.iccOpenLogicalChannel(aid, p2, obtainMessage());
        verify(mRadioProxy).iccOpenLogicalChannel(mSerialNumberCaptor.capture(), eq(aid), eq(p2));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SIM_OPEN_CHANNEL);
    }

    @FlakyTest
    @Test
    public void testIccCloseLogicalChannel() throws Exception {
        int channel = 1;
        mRILUnderTest.iccCloseLogicalChannel(channel, obtainMessage());
        verify(mRadioProxy).iccCloseLogicalChannel(mSerialNumberCaptor.capture(), eq(channel));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SIM_CLOSE_CHANNEL);
    }

    @FlakyTest
    @Test
    public void testNvWriteItem() throws Exception {
        int itemId = 1;
        String itemValue = "value";
        mRILUnderTest.nvWriteItem(itemId, itemValue, obtainMessage());
        NvWriteItem item = new NvWriteItem();
        item.itemId = itemId;
        item.value = itemValue;
        verify(mRadioProxy).nvWriteItem(mSerialNumberCaptor.capture(), eq(item));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_NV_WRITE_ITEM);
    }

    @FlakyTest
    @Test
    public void testNvReadItem() throws Exception {
        int itemId = 1;
        mRILUnderTest.nvReadItem(itemId, obtainMessage());
        verify(mRadioProxy).nvReadItem(mSerialNumberCaptor.capture(), eq(itemId));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_NV_READ_ITEM);
    }

    @FlakyTest
    @Test
    public void testNvResetConfig() throws Exception {
        int resetType = 1;
        mRILUnderTest.nvResetConfig(resetType, obtainMessage());
        verify(mRadioProxy).nvResetConfig(
                mSerialNumberCaptor.capture(),
                eq((Integer) invokeMethod(
                        mRILInstance,
                        "convertToHalResetNvType",
                        new Class<?>[] {Integer.TYPE},
                        new Object[] {resetType})));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_NV_RESET_CONFIG);
    }

    @FlakyTest
    @Test
    public void testSetDataAllowed() throws Exception {
        boolean allowed = true;
        mRILUnderTest.setDataAllowed(allowed, obtainMessage());
        verify(mRadioProxy).setDataAllowed(mSerialNumberCaptor.capture(), eq(allowed));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_ALLOW_DATA);
    }

    @FlakyTest
    @Test
    public void testGetHardwareConfig() throws Exception {
        mRILUnderTest.getHardwareConfig(obtainMessage());
        verify(mRadioProxy).getHardwareConfig(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_HARDWARE_CONFIG);
    }

    @FlakyTest
    @Test
    public void testRequestIccSimAuthentication() throws Exception {
        int authContext = 1;
        String data = "data";
        String aid = "aid";
        mRILUnderTest.requestIccSimAuthentication(authContext, data, aid, obtainMessage());
        verify(mRadioProxy).requestIccSimAuthentication(
                mSerialNumberCaptor.capture(), eq(authContext), eq(data), eq(aid));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SIM_AUTHENTICATION);
    }

    @FlakyTest
    @Test
    public void testRequestShutdown() throws Exception {
        mRILUnderTest.requestShutdown(obtainMessage());
        verify(mRadioProxy).requestShutdown(mSerialNumberCaptor.capture());
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SHUTDOWN);
    }

    @FlakyTest
    @Test
    public void testGetRadioCapability() throws Exception {
        mRILUnderTest.getRadioCapability(obtainMessage());
        verify(mRadioProxy).getRadioCapability(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_RADIO_CAPABILITY);
    }

    @FlakyTest
    @Test
    public void testStartLceService() throws Exception {
        int reportIntervalMs = 1000;
        boolean pullMode = false;
        mRILUnderTest.startLceService(reportIntervalMs, pullMode, obtainMessage());
        verify(mRadioProxy).startLceService(
                mSerialNumberCaptor.capture(), eq(reportIntervalMs), eq(pullMode));
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_START_LCE);
    }

    @FlakyTest
    @Test
    public void testStopLceService() throws Exception {
        mRILUnderTest.stopLceService(obtainMessage());
        verify(mRadioProxy).stopLceService(mSerialNumberCaptor.capture());
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_STOP_LCE);
    }

    @FlakyTest
    @Test
    public void testPullLceData() throws Exception {
        mRILUnderTest.pullLceData(obtainMessage());
        verify(mRadioProxy).pullLceData(mSerialNumberCaptor.capture());
        verifyRILResponse(mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_PULL_LCEDATA);
    }

    @FlakyTest
    @Test
    public void testGetModemActivityInfo() throws Exception {
        mRILUnderTest.getModemActivityInfo(obtainMessage());
        verify(mRadioProxy).getModemActivityInfo(mSerialNumberCaptor.capture());
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_GET_ACTIVITY_INFO);
    }

    @FlakyTest
    @Test
    public void testGetModemActivityInfoTimeout() {
        mRILUnderTest.getModemActivityInfo(obtainMessage());
        assertEquals(1, mRILUnderTest.getRilRequestList().size());
        waitForHandlerActionDelayed(mRilHandler, 10, DEFAULT_BLOCKING_MESSAGE_RESPONSE_TIMEOUT_MS);
        assertEquals(0, mRILUnderTest.getRilRequestList().size());
    }

    @FlakyTest
    @Test
    public void testSendDeviceState() throws Exception {
        int stateType = 1;
        boolean state = false;
        mRILUnderTest.sendDeviceState(stateType, state, obtainMessage());
        verify(mRadioProxy).sendDeviceState(
                mSerialNumberCaptor.capture(), eq(stateType), eq(state));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SEND_DEVICE_STATE);
    }

    @FlakyTest
    @Test
    public void testSetUnsolResponseFilter() throws Exception {
        int filter = 1;
        mRILUnderTest.setUnsolResponseFilter(filter, obtainMessage());
        verify(mRadioProxy).setIndicationFilter(mSerialNumberCaptor.capture(), eq(filter));
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_SET_UNSOLICITED_RESPONSE_FILTER);
    }

    @FlakyTest
    @Test
    public void testSetSimCardPowerForPowerDownState() throws Exception {
        mRILUnderTest.setSimCardPower(TelephonyManager.CARD_POWER_DOWN, obtainMessage());
        verify(mRadioProxy).setSimCardPower(mSerialNumberCaptor.capture(), eq(false));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SET_SIM_CARD_POWER);
    }

    @FlakyTest
    @Test
    public void testSetSimCardPowerForPowerUpState() throws Exception {
        mRILUnderTest.setSimCardPower(TelephonyManager.CARD_POWER_UP, obtainMessage());
        verify(mRadioProxy).setSimCardPower(mSerialNumberCaptor.capture(), eq(true));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SET_SIM_CARD_POWER);
    }

    @FlakyTest
    @Test
    public void testHandleCallSetupRequestFromSim() throws Exception {
        boolean accept = true;
        mRILUnderTest.handleCallSetupRequestFromSim(accept, obtainMessage());
        verify(mRadioProxy).handleStkCallSetupRequestFromSim(
                mSerialNumberCaptor.capture(), eq(accept));
        verifyRILResponse(
                mRILUnderTest,
                mSerialNumberCaptor.getValue(),
                RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM);
    }

    @FlakyTest
    @Test
    public void testWakeLockTimeout() throws Exception {
        invokeMethod(
                mRILInstance,
                "obtainRequest",
                new Class<?>[] {Integer.TYPE, Message.class, WorkSource.class},
                new Object[] {RIL_REQUEST_GET_SIM_STATUS, obtainMessage(), null});

        // The wake lock should be held when obtain a RIL request.
        assertTrue(mRILInstance.getWakeLock(RIL.FOR_WAKELOCK).isHeld());

        waitForHandlerActionDelayed(mRilHandler, 10, DEFAULT_WAKE_LOCK_TIMEOUT_MS);

        // The wake lock should be released after processed the time out event.
        assertFalse(mRILInstance.getWakeLock(RIL.FOR_WAKELOCK).isHeld());
    }

    @Test
    public void testInvokeOemRilRequestStrings() throws Exception {
        String[] strings = new String[]{"a", "b", "c"};
        mRILUnderTest.invokeOemRilRequestStrings(strings, obtainMessage());
        verify(mOemHookProxy).sendRequestStrings(
                mSerialNumberCaptor.capture(), eq(new ArrayList<>(Arrays.asList(strings))));
    }

    @Test
    public void testInvokeOemRilRequestRaw() throws Exception {
        byte[] data = new byte[]{1, 2, 3};
        mRILUnderTest.invokeOemRilRequestRaw(data, obtainMessage());
        verify(mOemHookProxy).sendRequestRaw(
                mSerialNumberCaptor.capture(), eq(mRILUnderTest.primitiveArrayToArrayList(data)));
    }

    private Message obtainMessage() {
        return mTestHandler.getThreadHandler().obtainMessage();
    }

    private static void verifyRILResponse(RIL ril, int serial, int requestType) {
        RadioResponseInfo responseInfo =
                createFakeRadioResponseInfo(serial, RadioError.NONE, RadioResponseType.SOLICITED);

        RILRequest rr = ril.processResponse(responseInfo);
        assertNotNull(rr);

        assertEquals(serial, rr.getSerial());
        assertEquals(requestType, rr.getRequest());
        assertTrue(ril.getWakeLock(RIL.FOR_WAKELOCK).isHeld());

        ril.processResponseDone(rr, responseInfo, null);
        assertEquals(0, ril.getRilRequestList().size());
        assertFalse(ril.getWakeLock(RIL.FOR_WAKELOCK).isHeld());
    }

    private static RadioResponseInfo createFakeRadioResponseInfo(int serial, int error, int type) {
        RadioResponseInfo respInfo = new RadioResponseInfo();
        respInfo.serial = serial;
        respInfo.error = error;
        respInfo.type = type;
        return respInfo;
    }

    @Test
    public void testConvertHalCellInfoListForLTE() throws Exception {
        android.hardware.radio.V1_0.CellInfoLte lte = new android.hardware.radio.V1_0.CellInfoLte();
        lte.cellIdentityLte.ci = CI;
        lte.cellIdentityLte.pci = PCI;
        lte.cellIdentityLte.tac = TAC;
        lte.cellIdentityLte.earfcn = EARFCN;
        lte.cellIdentityLte.mcc = MCC_STR;
        lte.cellIdentityLte.mnc = MNC_STR;
        lte.signalStrengthLte.signalStrength = SIGNAL_STRENGTH;
        lte.signalStrengthLte.rsrp = RSRP;
        lte.signalStrengthLte.rsrq = RSRQ;
        lte.signalStrengthLte.rssnr = RSSNR;
        lte.signalStrengthLte.cqi = CQI;
        lte.signalStrengthLte.timingAdvance = TIME_ADVANCE;
        android.hardware.radio.V1_0.CellInfo record = new android.hardware.radio.V1_0.CellInfo();
        record.cellInfoType = TYPE_LTE;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.lte.add(lte);
        ArrayList<android.hardware.radio.V1_0.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_0.CellInfo>();
        records.add(record);

        ArrayList<CellInfo> ret = RIL.convertHalCellInfoList(records);

        assertEquals(1, ret.size());
        CellInfoLte cellInfoLte = (CellInfoLte) ret.get(0);
        CellInfoLte expected = new CellInfoLte();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityLte cil = new CellIdentityLte(CI, PCI, TAC, EARFCN, Integer.MAX_VALUE, MCC_STR,
                MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthLte css = new CellSignalStrengthLte(
                SIGNAL_STRENGTH, -RSRP, -RSRQ, RSSNR, CQI, TIME_ADVANCE);
        expected.setCellIdentity(cil);
        expected.setCellSignalStrength(css);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_UNKNOWN);
        assertEquals(expected, cellInfoLte);
    }

    @Test
    public void testConvertHalCellInfoListForGSM() throws Exception {
        android.hardware.radio.V1_0.CellInfoGsm cellinfo =
                new android.hardware.radio.V1_0.CellInfoGsm();
        cellinfo.cellIdentityGsm.lac = LAC;
        cellinfo.cellIdentityGsm.cid = CID;
        cellinfo.cellIdentityGsm.bsic = BSIC;
        cellinfo.cellIdentityGsm.arfcn = ARFCN;
        cellinfo.cellIdentityGsm.mcc = MCC_STR;
        cellinfo.cellIdentityGsm.mnc = MNC_STR;
        cellinfo.signalStrengthGsm.signalStrength = SIGNAL_STRENGTH;
        cellinfo.signalStrengthGsm.bitErrorRate = BIT_ERROR_RATE;
        cellinfo.signalStrengthGsm.timingAdvance = TIME_ADVANCE;
        android.hardware.radio.V1_0.CellInfo record = new android.hardware.radio.V1_0.CellInfo();
        record.cellInfoType = TYPE_GSM;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.gsm.add(cellinfo);
        ArrayList<android.hardware.radio.V1_0.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_0.CellInfo>();
        records.add(record);

        ArrayList<CellInfo> ret = RIL.convertHalCellInfoList(records);

        assertEquals(1, ret.size());
        CellInfoGsm cellInfoGsm = (CellInfoGsm) ret.get(0);
        CellInfoGsm expected = new CellInfoGsm();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityGsm ci = new CellIdentityGsm(
                LAC, CID, ARFCN, BSIC, MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthGsm cs = new CellSignalStrengthGsm(
                SIGNAL_STRENGTH, BIT_ERROR_RATE, TIME_ADVANCE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_UNKNOWN);
        assertEquals(expected, cellInfoGsm);
    }

    @Test
    public void testConvertHalCellInfoListForWcdma() throws Exception {
        android.hardware.radio.V1_0.CellInfoWcdma cellinfo =
                new android.hardware.radio.V1_0.CellInfoWcdma();
        cellinfo.cellIdentityWcdma.lac = LAC;
        cellinfo.cellIdentityWcdma.cid = CID;
        cellinfo.cellIdentityWcdma.psc = PSC;
        cellinfo.cellIdentityWcdma.uarfcn = UARFCN;
        cellinfo.cellIdentityWcdma.mcc = MCC_STR;
        cellinfo.cellIdentityWcdma.mnc = MNC_STR;
        cellinfo.signalStrengthWcdma.signalStrength = SIGNAL_STRENGTH;
        cellinfo.signalStrengthWcdma.bitErrorRate = BIT_ERROR_RATE;
        android.hardware.radio.V1_0.CellInfo record = new android.hardware.radio.V1_0.CellInfo();
        record.cellInfoType = TYPE_WCDMA;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.wcdma.add(cellinfo);
        ArrayList<android.hardware.radio.V1_0.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_0.CellInfo>();
        records.add(record);

        ArrayList<CellInfo> ret = RIL.convertHalCellInfoList(records);

        assertEquals(1, ret.size());
        CellInfoWcdma cellInfoWcdma = (CellInfoWcdma) ret.get(0);
        CellInfoWcdma expected = new CellInfoWcdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityWcdma ci = new CellIdentityWcdma(
                LAC, CID, PSC, UARFCN, MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthWcdma cs = new CellSignalStrengthWcdma(SIGNAL_STRENGTH, BIT_ERROR_RATE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_UNKNOWN);
        assertEquals(expected, cellInfoWcdma);
    }

    @Test
    public void testConvertHalCellInfoListForCdma() throws Exception {
        android.hardware.radio.V1_0.CellInfoCdma cellinfo =
                new android.hardware.radio.V1_0.CellInfoCdma();
        cellinfo.cellIdentityCdma.networkId = NETWORK_ID;
        cellinfo.cellIdentityCdma.systemId = SYSTEM_ID;
        cellinfo.cellIdentityCdma.baseStationId = BASESTATION_ID;
        cellinfo.cellIdentityCdma.longitude = LONGITUDE;
        cellinfo.cellIdentityCdma.latitude = LATITUDE;
        cellinfo.signalStrengthCdma.dbm = DBM;
        cellinfo.signalStrengthCdma.ecio = ECIO;
        cellinfo.signalStrengthEvdo.dbm = DBM;
        cellinfo.signalStrengthEvdo.ecio = ECIO;
        cellinfo.signalStrengthEvdo.signalNoiseRatio = SIGNAL_NOISE_RATIO;
        android.hardware.radio.V1_0.CellInfo record = new android.hardware.radio.V1_0.CellInfo();
        record.cellInfoType = TYPE_CDMA;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.cdma.add(cellinfo);
        ArrayList<android.hardware.radio.V1_0.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_0.CellInfo>();
        records.add(record);

        ArrayList<CellInfo> ret = RIL.convertHalCellInfoList(records);

        assertEquals(1, ret.size());
        CellInfoCdma cellInfoCdma = (CellInfoCdma) ret.get(0);
        CellInfoCdma expected = new CellInfoCdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityCdma ci = new CellIdentityCdma(
                NETWORK_ID, SYSTEM_ID, BASESTATION_ID, LONGITUDE, LATITUDE,
                EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthCdma cs = new CellSignalStrengthCdma(
                DBM, ECIO, DBM, ECIO, SIGNAL_NOISE_RATIO);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_UNKNOWN);
        assertEquals(expected, cellInfoCdma);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForLTE() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForLTE(MCC_STR, MNC_STR, ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoLte cellInfoLte = (CellInfoLte) ret.get(0);
        CellInfoLte expected = new CellInfoLte();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityLte cil = new CellIdentityLte(
                CI, PCI, TAC, EARFCN, BANDWIDTH, MCC_STR, MNC_STR, ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthLte css = new CellSignalStrengthLte(
                SIGNAL_STRENGTH, -RSRP, -RSRQ, RSSNR, CQI, TIME_ADVANCE);
        expected.setCellIdentity(cil);
        expected.setCellSignalStrength(css);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoLte);
    }

    @Test
    public void testConvertHalCellInfoList_1_2_ForLTEWithEmptyOperatorInfo() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForLTE(
                MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoLte cellInfoLte = (CellInfoLte) ret.get(0);
        CellInfoLte expected = new CellInfoLte();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityLte cil = new CellIdentityLte(CI, PCI, TAC, EARFCN, BANDWIDTH, MCC_STR, MNC_STR,
                EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthLte css = new CellSignalStrengthLte(
                SIGNAL_STRENGTH, -RSRP, -RSRQ, RSSNR, CQI, TIME_ADVANCE);
        expected.setCellIdentity(cil);
        expected.setCellSignalStrength(css);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoLte);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForLTEWithEmptyMccMnc() throws Exception {
        // MCC/MNC will be set as INT_MAX if unknown
        ArrayList<CellInfo> ret = getCellInfoListForLTE(
                String.valueOf(Integer.MAX_VALUE), String.valueOf(Integer.MAX_VALUE),
                ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoLte cellInfoLte = (CellInfoLte) ret.get(0);
        CellInfoLte expected = new CellInfoLte();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityLte cil = new CellIdentityLte(
                CI, PCI, TAC, EARFCN, BANDWIDTH, null, null, ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthLte css = new CellSignalStrengthLte(
                SIGNAL_STRENGTH, -RSRP, -RSRQ, RSSNR, CQI, TIME_ADVANCE);
        expected.setCellIdentity(cil);
        expected.setCellSignalStrength(css);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoLte);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForGSM() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForGSM(MCC_STR, MNC_STR, ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoGsm cellInfoGsm = (CellInfoGsm) ret.get(0);
        CellInfoGsm expected = new CellInfoGsm();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityGsm ci = new CellIdentityGsm(
                LAC, CID, ARFCN, BSIC, MCC_STR, MNC_STR, ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthGsm cs = new CellSignalStrengthGsm(
                SIGNAL_STRENGTH, BIT_ERROR_RATE, TIME_ADVANCE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoGsm);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForGSMWithEmptyOperatorInfo() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForGSM(
                MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoGsm cellInfoGsm = (CellInfoGsm) ret.get(0);
        CellInfoGsm expected = new CellInfoGsm();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityGsm ci = new CellIdentityGsm(
                LAC, CID, ARFCN, BSIC, MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthGsm cs = new CellSignalStrengthGsm(
                SIGNAL_STRENGTH, BIT_ERROR_RATE, TIME_ADVANCE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoGsm);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForGSMWithEmptyMccMnc() throws Exception {
        // MCC/MNC will be set as INT_MAX if unknown
        ArrayList<CellInfo> ret = getCellInfoListForGSM(
                String.valueOf(Integer.MAX_VALUE), String.valueOf(Integer.MAX_VALUE),
                ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoGsm cellInfoGsm = (CellInfoGsm) ret.get(0);
        CellInfoGsm expected = new CellInfoGsm();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityGsm ci = new CellIdentityGsm(
                LAC, CID, ARFCN, BSIC, null, null, ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthGsm cs = new CellSignalStrengthGsm(
                SIGNAL_STRENGTH, BIT_ERROR_RATE, TIME_ADVANCE);
        expected.setCellIdentity(ci);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        expected.setCellSignalStrength(cs);
        assertEquals(expected, cellInfoGsm);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForWcdma() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForWcdma(
                MCC_STR, MNC_STR, ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoWcdma cellInfoWcdma = (CellInfoWcdma) ret.get(0);
        CellInfoWcdma expected = new CellInfoWcdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityWcdma ci = new CellIdentityWcdma(
                LAC, CID, PSC, UARFCN, MCC_STR, MNC_STR, ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthWcdma cs = new CellSignalStrengthWcdma(SIGNAL_STRENGTH, BIT_ERROR_RATE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoWcdma);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForWcdmaWithEmptyOperatorInfo() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForWcdma(
                MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoWcdma cellInfoWcdma = (CellInfoWcdma) ret.get(0);
        CellInfoWcdma expected = new CellInfoWcdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityWcdma ci = new CellIdentityWcdma(
                LAC, CID, PSC, UARFCN, MCC_STR, MNC_STR, EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthWcdma cs = new CellSignalStrengthWcdma(SIGNAL_STRENGTH, BIT_ERROR_RATE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoWcdma);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForWcdmaWithEmptyMccMnc() throws Exception {
        // MCC/MNC will be set as INT_MAX if unknown
        ArrayList<CellInfo> ret = getCellInfoListForWcdma(
                String.valueOf(Integer.MAX_VALUE), String.valueOf(Integer.MAX_VALUE),
                ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoWcdma cellInfoWcdma = (CellInfoWcdma) ret.get(0);
        CellInfoWcdma expected = new CellInfoWcdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityWcdma ci = new CellIdentityWcdma(
                LAC, CID, PSC, UARFCN, null, null, ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthWcdma cs = new CellSignalStrengthWcdma(SIGNAL_STRENGTH, BIT_ERROR_RATE);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoWcdma);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForCdma() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForCdma(ALPHA_LONG, ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoCdma cellInfoCdma = (CellInfoCdma) ret.get(0);
        CellInfoCdma expected = new CellInfoCdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityCdma ci = new CellIdentityCdma(
                NETWORK_ID, SYSTEM_ID, BASESTATION_ID, LONGITUDE, LATITUDE,
                ALPHA_LONG, ALPHA_SHORT);
        CellSignalStrengthCdma cs = new CellSignalStrengthCdma(
                DBM, ECIO, DBM, ECIO, SIGNAL_NOISE_RATIO);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoCdma);
    }

    @Test
    public void testConvertHalCellInfoList_1_2ForCdmaWithEmptyOperatorInfo() throws Exception {
        ArrayList<CellInfo> ret = getCellInfoListForCdma(EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);

        assertEquals(1, ret.size());
        CellInfoCdma cellInfoCdma = (CellInfoCdma) ret.get(0);
        CellInfoCdma expected = new CellInfoCdma();
        expected.setRegistered(false);
        expected.setTimeStamp(TIMESTAMP);
        expected.setTimeStampType(RIL_TIMESTAMP_TYPE_OEM_RIL);
        CellIdentityCdma ci = new CellIdentityCdma(
                NETWORK_ID, SYSTEM_ID, BASESTATION_ID, LONGITUDE, LATITUDE,
                EMPTY_ALPHA_LONG, EMPTY_ALPHA_SHORT);
        CellSignalStrengthCdma cs = new CellSignalStrengthCdma(
                DBM, ECIO, DBM, ECIO, SIGNAL_NOISE_RATIO);
        expected.setCellIdentity(ci);
        expected.setCellSignalStrength(cs);
        expected.setCellConnectionStatus(CellInfo.CONNECTION_NONE);
        assertEquals(expected, cellInfoCdma);
    }

    @Test
    public void testGetWorksourceClientId() {
        RILRequest request = RILRequest.obtain(0, null, null);
        assertEquals(null, request.getWorkSourceClientId());

        request = RILRequest.obtain(0, null, new WorkSource());
        assertEquals(null, request.getWorkSourceClientId());

        WorkSource ws = new WorkSource();
        ws.add(100);
        request = RILRequest.obtain(0, null, ws);
        assertEquals("100:null", request.getWorkSourceClientId());

        ws = new WorkSource();
        ws.add(100, "foo");
        request = RILRequest.obtain(0, null, ws);
        assertEquals("100:foo", request.getWorkSourceClientId());

        ws = new WorkSource();
        ws.createWorkChain().addNode(100, "foo").addNode(200, "bar");
        request = RILRequest.obtain(0, null, ws);
        assertEquals("100:foo", request.getWorkSourceClientId());
    }

    private ArrayList<CellInfo> getCellInfoListForLTE(
            String mcc, String mnc, String alphaLong, String alphaShort) {
        android.hardware.radio.V1_2.CellInfoLte lte = new android.hardware.radio.V1_2.CellInfoLte();
        lte.cellIdentityLte.base.ci = CI;
        lte.cellIdentityLte.base.pci = PCI;
        lte.cellIdentityLte.base.tac = TAC;
        lte.cellIdentityLte.base.earfcn = EARFCN;
        lte.cellIdentityLte.bandwidth = BANDWIDTH;
        lte.cellIdentityLte.base.mcc = mcc;
        lte.cellIdentityLte.base.mnc = mnc;
        lte.cellIdentityLte.operatorNames.alphaLong = alphaLong;
        lte.cellIdentityLte.operatorNames.alphaShort = alphaShort;
        lte.signalStrengthLte.signalStrength = SIGNAL_STRENGTH;
        lte.signalStrengthLte.rsrp = RSRP;
        lte.signalStrengthLte.rsrq = RSRQ;
        lte.signalStrengthLte.rssnr = RSSNR;
        lte.signalStrengthLte.cqi = CQI;
        lte.signalStrengthLte.timingAdvance = TIME_ADVANCE;
        android.hardware.radio.V1_2.CellInfo record = new android.hardware.radio.V1_2.CellInfo();
        record.cellInfoType = TYPE_LTE;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.lte.add(lte);
        record.connectionStatus = 0;
        ArrayList<android.hardware.radio.V1_2.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_2.CellInfo>();
        records.add(record);
        return RIL.convertHalCellInfoList_1_2(records);
    }

    private ArrayList<CellInfo> getCellInfoListForGSM(
            String mcc, String mnc, String alphaLong, String alphaShort) {
        android.hardware.radio.V1_2.CellInfoGsm cellinfo =
                new android.hardware.radio.V1_2.CellInfoGsm();
        cellinfo.cellIdentityGsm.base.lac = LAC;
        cellinfo.cellIdentityGsm.base.cid = CID;
        cellinfo.cellIdentityGsm.base.bsic = BSIC;
        cellinfo.cellIdentityGsm.base.arfcn = ARFCN;
        cellinfo.cellIdentityGsm.base.mcc = mcc;
        cellinfo.cellIdentityGsm.base.mnc = mnc;
        cellinfo.cellIdentityGsm.operatorNames.alphaLong = alphaLong;
        cellinfo.cellIdentityGsm.operatorNames.alphaShort = alphaShort;
        cellinfo.signalStrengthGsm.signalStrength = SIGNAL_STRENGTH;
        cellinfo.signalStrengthGsm.bitErrorRate = BIT_ERROR_RATE;
        cellinfo.signalStrengthGsm.timingAdvance = TIME_ADVANCE;
        android.hardware.radio.V1_2.CellInfo record = new android.hardware.radio.V1_2.CellInfo();
        record.cellInfoType = TYPE_GSM;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.gsm.add(cellinfo);
        record.connectionStatus = 0;
        ArrayList<android.hardware.radio.V1_2.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_2.CellInfo>();
        records.add(record);

        return RIL.convertHalCellInfoList_1_2(records);
    }

    private ArrayList<CellInfo> getCellInfoListForWcdma(
            String mcc, String mnc, String alphaLong, String alphaShort) {
        android.hardware.radio.V1_2.CellInfoWcdma cellinfo =
                new android.hardware.radio.V1_2.CellInfoWcdma();
        cellinfo.cellIdentityWcdma.base.lac = LAC;
        cellinfo.cellIdentityWcdma.base.cid = CID;
        cellinfo.cellIdentityWcdma.base.psc = PSC;
        cellinfo.cellIdentityWcdma.base.uarfcn = UARFCN;
        cellinfo.cellIdentityWcdma.base.mcc = mcc;
        cellinfo.cellIdentityWcdma.base.mnc = mnc;
        cellinfo.cellIdentityWcdma.operatorNames.alphaLong = alphaLong;
        cellinfo.cellIdentityWcdma.operatorNames.alphaShort = alphaShort;
        cellinfo.signalStrengthWcdma.base.signalStrength = SIGNAL_STRENGTH;
        cellinfo.signalStrengthWcdma.base.bitErrorRate = BIT_ERROR_RATE;
        cellinfo.signalStrengthWcdma.rscp = 10;
        cellinfo.signalStrengthWcdma.ecno = 5;
        android.hardware.radio.V1_2.CellInfo record = new android.hardware.radio.V1_2.CellInfo();
        record.cellInfoType = TYPE_WCDMA;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.wcdma.add(cellinfo);
        record.connectionStatus = 0;
        ArrayList<android.hardware.radio.V1_2.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_2.CellInfo>();
        records.add(record);

        return RIL.convertHalCellInfoList_1_2(records);
    }

    private ArrayList<CellInfo> getCellInfoListForCdma(String alphaLong, String alphaShort) {
        android.hardware.radio.V1_2.CellInfoCdma cellinfo =
                new android.hardware.radio.V1_2.CellInfoCdma();
        cellinfo.cellIdentityCdma.base.networkId = NETWORK_ID;
        cellinfo.cellIdentityCdma.base.systemId = SYSTEM_ID;
        cellinfo.cellIdentityCdma.base.baseStationId = BASESTATION_ID;
        cellinfo.cellIdentityCdma.base.longitude = LONGITUDE;
        cellinfo.cellIdentityCdma.base.latitude = LATITUDE;
        cellinfo.cellIdentityCdma.operatorNames.alphaLong = alphaLong;
        cellinfo.cellIdentityCdma.operatorNames.alphaShort = alphaShort;
        cellinfo.signalStrengthCdma.dbm = DBM;
        cellinfo.signalStrengthCdma.ecio = ECIO;
        cellinfo.signalStrengthEvdo.dbm = DBM;
        cellinfo.signalStrengthEvdo.ecio = ECIO;
        cellinfo.signalStrengthEvdo.signalNoiseRatio = SIGNAL_NOISE_RATIO;
        android.hardware.radio.V1_2.CellInfo record = new android.hardware.radio.V1_2.CellInfo();
        record.cellInfoType = TYPE_CDMA;
        record.registered = false;
        record.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
        record.timeStamp = TIMESTAMP;
        record.cdma.add(cellinfo);
        record.connectionStatus = 0;
        ArrayList<android.hardware.radio.V1_2.CellInfo> records =
                new ArrayList<android.hardware.radio.V1_2.CellInfo>();
        records.add(record);

        return RIL.convertHalCellInfoList_1_2(records);
    }

    public android.telephony.SignalStrength getTdScdmaSignalStrength_1_0(int tdscdmaNegDbm) {
        android.hardware.radio.V1_0.SignalStrength halSs =
                new android.hardware.radio.V1_0.SignalStrength();
        halSs.lte.signalStrength = SIGNAL_STRENGTH;
        halSs.lte.rsrp = RSRP;
        halSs.lte.rsrq = RSRQ;
        halSs.lte.rssnr = RSSNR;
        halSs.gw.signalStrength = SIGNAL_STRENGTH;
        halSs.gw.bitErrorRate = BIT_ERROR_RATE;
        halSs.cdma.dbm = DBM;
        halSs.cdma.ecio = ECIO;
        halSs.evdo.dbm = DBM;
        halSs.evdo.ecio = ECIO;
        halSs.evdo.signalNoiseRatio = SIGNAL_NOISE_RATIO;
        halSs.tdScdma.rscp = tdscdmaNegDbm;
        android.telephony.SignalStrength ss = RIL.convertHalSignalStrength(halSs);
        // FIXME: We should not need to call validateInput here b/74115980.
        ss.validateInput();
        return ss;
    }

    public android.telephony.SignalStrength getTdScdmaSignalStrength_1_2(int tdscdmaAsu) {
        android.hardware.radio.V1_2.SignalStrength halSs =
                new android.hardware.radio.V1_2.SignalStrength();
        halSs.lte.signalStrength = SIGNAL_STRENGTH;
        halSs.lte.rsrp = RSRP;
        halSs.lte.rsrq = RSRQ;
        halSs.lte.rssnr = RSSNR;
        halSs.gsm.signalStrength = SIGNAL_STRENGTH;
        halSs.gsm.bitErrorRate = BIT_ERROR_RATE;
        halSs.cdma.dbm = DBM;
        halSs.cdma.ecio = ECIO;
        halSs.evdo.dbm = DBM;
        halSs.evdo.ecio = ECIO;
        halSs.evdo.signalNoiseRatio = SIGNAL_NOISE_RATIO;
        halSs.wcdma.base.signalStrength = 99;
        halSs.wcdma.rscp = 255;
        halSs.tdScdma.rscp = tdscdmaAsu;
        android.telephony.SignalStrength ss = RIL.convertHalSignalStrength_1_2(halSs);
        // FIXME: We should not need to call validateInput here b/74115980
        // but unless we call it, we have to pass Integer.MAX_VALUE for wcdma RSCP,
        // which is outside the allowable range for the HAL. This value is being
        // coerced inside SignalStrength.validateInput().
        ss.validateInput();
        return ss;
    }

    @Test
    public void testHalSignalStrengthTdScdma() throws Exception {
        // Check that the minimum value is the same.
        assertEquals(getTdScdmaSignalStrength_1_0(120), getTdScdmaSignalStrength_1_2(0));
        // Check that the maximum common value is the same.
        assertEquals(getTdScdmaSignalStrength_1_0(25), getTdScdmaSignalStrength_1_2(95));
        // Check that an invalid value is the same.
        assertEquals(getTdScdmaSignalStrength_1_0(-1), getTdScdmaSignalStrength_1_2(255));
    }

    @Test
    public void testSetupDataCall() throws Exception {

        DataProfile dp = new DataProfile(PROFILE_ID, APN, PROTOCOL, AUTH_TYPE, USER_NAME, PASSWORD,
                TYPE, MAX_CONNS_TIME, MAX_CONNS, WAIT_TIME, APN_ENABLED, SUPPORTED_APNT_YPES_BITMAP,
                ROAMING_PROTOCOL, BEARER_BITMAP, MTU, MVNO_TYPE, MVNO_MATCH_DATA, MODEM_COGNITIVE);
        mRILUnderTest.setupDataCall(AccessNetworkConstants.AccessNetworkType.EUTRAN, dp, false,
                false, 0, null, obtainMessage());
        ArgumentCaptor<DataProfileInfo> dpiCaptor = ArgumentCaptor.forClass(DataProfileInfo.class);
        verify(mRadioProxy).setupDataCall(
                mSerialNumberCaptor.capture(), eq(AccessNetworkConstants.AccessNetworkType.EUTRAN),
                dpiCaptor.capture(), eq(true), eq(false), eq(false));
        verifyRILResponse(
                mRILUnderTest, mSerialNumberCaptor.getValue(), RIL_REQUEST_SETUP_DATA_CALL);
        DataProfileInfo dpi = dpiCaptor.getValue();
        assertEquals(PROFILE_ID, dpi.profileId);
        assertEquals(APN, dpi.apn);
        assertEquals(PROTOCOL, dpi.protocol);
        assertEquals(AUTH_TYPE, dpi.authType);
        assertEquals(USER_NAME, dpi.user);
        assertEquals(PASSWORD, dpi.password);
        assertEquals(TYPE, dpi.type);
        assertEquals(MAX_CONNS_TIME, dpi.maxConnsTime);
        assertEquals(MAX_CONNS, dpi.maxConns);
        assertEquals(WAIT_TIME, dpi.waitTime);
        assertEquals(APN_ENABLED, dpi.enabled);
        assertEquals(SUPPORTED_APNT_YPES_BITMAP, dpi.supportedApnTypesBitmap);
        assertEquals(ROAMING_PROTOCOL, dpi.protocol);
        assertEquals(BEARER_BITMAP, dpi.bearerBitmap);
        assertEquals(MTU, dpi.mtu);
        assertEquals(0, dpi.mvnoType);
        assertEquals(MVNO_MATCH_DATA, dpi.mvnoMatchData);
    }
}
