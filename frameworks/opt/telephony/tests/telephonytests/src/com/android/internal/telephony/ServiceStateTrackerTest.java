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

package com.android.internal.telephony;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.nullable;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.IAlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.ServiceInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Parcel;
import android.os.PersistableBundle;
import android.os.Process;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.WorkSource;
import android.support.test.filters.FlakyTest;
import android.telephony.AccessNetworkConstants.AccessNetworkType;
import android.telephony.CarrierConfigManager;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellInfo;
import android.telephony.CellInfoGsm;
import android.telephony.NetworkRegistrationState;
import android.telephony.NetworkService;
import android.telephony.PhysicalChannelConfig;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.gsm.GsmCellLocation;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Pair;

import com.android.internal.R;
import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.test.SimulatedCommands;
import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.internal.telephony.util.TimeStampedValue;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

public class ServiceStateTrackerTest extends TelephonyTest {

    @Mock
    private DcTracker mDct;
    @Mock
    private ProxyController mProxyController;
    @Mock
    private Handler mTestHandler;
    @Mock
    protected IAlarmManager mAlarmManager;

    CellularNetworkService mCellularNetworkService;

    private ServiceStateTracker sst;
    private ServiceStateTrackerTestHandler mSSTTestHandler;
    private PersistableBundle mBundle;

    private static final int EVENT_REGISTERED_TO_NETWORK = 1;
    private static final int EVENT_SUBSCRIPTION_INFO_READY = 2;
    private static final int EVENT_DATA_ROAMING_ON = 3;
    private static final int EVENT_DATA_ROAMING_OFF = 4;
    private static final int EVENT_DATA_CONNECTION_ATTACHED = 5;
    private static final int EVENT_DATA_CONNECTION_DETACHED = 6;
    private static final int EVENT_DATA_RAT_CHANGED = 7;
    private static final int EVENT_PS_RESTRICT_ENABLED = 8;
    private static final int EVENT_PS_RESTRICT_DISABLED = 9;
    private static final int EVENT_VOICE_ROAMING_ON = 10;
    private static final int EVENT_VOICE_ROAMING_OFF = 11;

    private class ServiceStateTrackerTestHandler extends HandlerThread {

        private ServiceStateTrackerTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            sst = new ServiceStateTracker(mPhone, mSimulatedCommands);
            setReady(true);
        }
    }

    private void addNetworkService() {
        mCellularNetworkService = new CellularNetworkService();
        ServiceInfo serviceInfo =  new ServiceInfo();
        serviceInfo.packageName = "com.android.phone";
        serviceInfo.permission = "android.permission.BIND_NETWORK_SERVICE";
        IntentFilter filter = new IntentFilter();
        mContextFixture.addService(
                NetworkService.NETWORK_SERVICE_INTERFACE,
                null,
                "com.android.phone",
                mCellularNetworkService.mBinder,
                serviceInfo,
                filter);
    }

    @Before
    public void setUp() throws Exception {

        logd("ServiceStateTrackerTest +Setup!");
        super.setUp("ServiceStateTrackerTest");

        mContextFixture.putResource(R.string.config_wwan_network_service_package,
                "com.android.phone");
        addNetworkService();

        doReturn(true).when(mDct).isDisconnected();
        mPhone.mDcTracker = mDct;

        replaceInstance(ProxyController.class, "sProxyController", null, mProxyController);
        mBundle = mContextFixture.getCarrierConfigBundle();
        mBundle.putStringArray(
                CarrierConfigManager.KEY_ROAMING_OPERATOR_STRING_ARRAY, new String[]{"123456"});

        mBundle.putStringArray(
                CarrierConfigManager.KEY_NON_ROAMING_OPERATOR_STRING_ARRAY, new String[]{"123456"});

        mBundle.putStringArray(CarrierConfigManager.KEY_RATCHET_RAT_FAMILIES,
                // UMTS < GPRS < EDGE
                new String[]{"3,1,2"});

        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setVoiceRadioTech(ServiceState.RIL_RADIO_TECHNOLOGY_HSPA);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRadioTech(ServiceState.RIL_RADIO_TECHNOLOGY_HSPA);

        int dds = SubscriptionManager.getDefaultDataSubscriptionId();
        doReturn(dds).when(mPhone).getSubId();

        mSSTTestHandler = new ServiceStateTrackerTestHandler(getClass().getSimpleName());
        mSSTTestHandler.start();
        waitUntilReady();
        waitForMs(600);
        logd("ServiceStateTrackerTest -Setup!");
    }

    @After
    public void tearDown() throws Exception {
        sst = null;
        mSSTTestHandler.quit();
        super.tearDown();
    }

    @Test
    @MediumTest
    public void testSetRadioPower() {
        boolean oldState = mSimulatedCommands.getRadioState().isOn();
        sst.setRadioPower(!oldState);
        waitForMs(100);
        assertTrue(oldState != mSimulatedCommands.getRadioState().isOn());
    }

    @Test
    @MediumTest
    public void testSetRadioPowerFromCarrier() {
        // Carrier disable radio power
        sst.setRadioPowerFromCarrier(false);
        waitForMs(100);
        assertFalse(mSimulatedCommands.getRadioState().isOn());
        assertTrue(sst.getDesiredPowerState());
        assertFalse(sst.getPowerStateFromCarrier());

        // User toggle radio power will not overrides carrier settings
        sst.setRadioPower(true);
        waitForMs(100);
        assertFalse(mSimulatedCommands.getRadioState().isOn());
        assertTrue(sst.getDesiredPowerState());
        assertFalse(sst.getPowerStateFromCarrier());

        // Carrier re-enable radio power
        sst.setRadioPowerFromCarrier(true);
        waitForMs(100);
        assertTrue(mSimulatedCommands.getRadioState().isOn());
        assertTrue(sst.getDesiredPowerState());
        assertTrue(sst.getPowerStateFromCarrier());

        // User toggle radio power off (airplane mode) and set carrier on
        sst.setRadioPower(false);
        sst.setRadioPowerFromCarrier(true);
        waitForMs(100);
        assertFalse(mSimulatedCommands.getRadioState().isOn());
        assertFalse(sst.getDesiredPowerState());
        assertTrue(sst.getPowerStateFromCarrier());
    }

    @Test
    @MediumTest
    public void testRilTrafficAfterSetRadioPower() {
        sst.setRadioPower(true);
        final int getOperatorCallCount = mSimulatedCommands.getGetOperatorCallCount();
        final int getDataRegistrationStateCallCount =
                mSimulatedCommands.getGetDataRegistrationStateCallCount();
        final int getVoiceRegistrationStateCallCount =
                mSimulatedCommands.getGetVoiceRegistrationStateCallCount();
        final int getNetworkSelectionModeCallCount =
                mSimulatedCommands.getGetNetworkSelectionModeCallCount();
        sst.setRadioPower(false);

        waitForMs(500);
        sst.pollState();
        waitForMs(250);

        // This test was meant to be for *no* ril traffic. However, RADIO_STATE_CHANGED is
        // considered a modem triggered action and that causes a pollState() to be done
        assertEquals(getOperatorCallCount + 1, mSimulatedCommands.getGetOperatorCallCount());
        assertEquals(getDataRegistrationStateCallCount + 1,
                mSimulatedCommands.getGetDataRegistrationStateCallCount());
        assertEquals(getVoiceRegistrationStateCallCount + 1,
                mSimulatedCommands.getGetVoiceRegistrationStateCallCount());
        assertEquals(getNetworkSelectionModeCallCount + 1,
                mSimulatedCommands.getGetNetworkSelectionModeCallCount());

        // Note that if the poll is triggered by a network change notification
        // and the modem is supposed to be off, we should still do the poll
        mSimulatedCommands.notifyNetworkStateChanged();
        waitForMs(250);

        assertEquals(getOperatorCallCount + 2 , mSimulatedCommands.getGetOperatorCallCount());
        assertEquals(getDataRegistrationStateCallCount + 2,
                mSimulatedCommands.getGetDataRegistrationStateCallCount());
        assertEquals(getVoiceRegistrationStateCallCount + 2,
                mSimulatedCommands.getGetVoiceRegistrationStateCallCount());
        assertEquals(getNetworkSelectionModeCallCount + 2,
                mSimulatedCommands.getGetNetworkSelectionModeCallCount());
    }

    @FlakyTest
    @Ignore
    @Test
    @MediumTest
    public void testSpnUpdateShowPlmnOnly() {
        doReturn(0x02).when(mSimRecords).getDisplayRule(new ServiceState());
        doReturn(IccCardApplicationStatus.AppState.APPSTATE_UNKNOWN).
                when(mUiccCardApplication3gpp).getState();

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_NETWORK_STATE_CHANGED, null));

        waitForMs(750);

        ArgumentCaptor<Intent> intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContextFixture.getTestDouble(), times(3))
                .sendStickyBroadcastAsUser(intentArgumentCaptor.capture(), eq(UserHandle.ALL));

        // We only want to verify the intent SPN_STRINGS_UPDATED_ACTION.
        List<Intent> intents = intentArgumentCaptor.getAllValues();
        logd("Total " + intents.size() + " intents");
        for (Intent intent : intents) {
            logd("  " + intent.getAction());
        }
        Intent intent = intents.get(2);
        assertEquals(TelephonyIntents.SPN_STRINGS_UPDATED_ACTION, intent.getAction());

        Bundle b = intent.getExtras();

        // For boolean we need to make sure the key exists first
        assertTrue(b.containsKey(TelephonyIntents.EXTRA_SHOW_SPN));
        assertFalse(b.getBoolean(TelephonyIntents.EXTRA_SHOW_SPN));

        assertEquals(null, b.getString(TelephonyIntents.EXTRA_SPN));
        assertEquals(null, b.getString(TelephonyIntents.EXTRA_DATA_SPN));

        // For boolean we need to make sure the key exists first
        assertTrue(b.containsKey(TelephonyIntents.EXTRA_SHOW_PLMN));
        assertTrue(b.getBoolean(TelephonyIntents.EXTRA_SHOW_PLMN));

        assertEquals(SimulatedCommands.FAKE_LONG_NAME, b.getString(TelephonyIntents.EXTRA_PLMN));

        ArgumentCaptor<Integer> intArgumentCaptor = ArgumentCaptor.forClass(Integer.class);
        verify(mTelephonyManager).setDataNetworkTypeForPhone(anyInt(), intArgumentCaptor.capture());
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_HSPA,
                intArgumentCaptor.getValue().intValue());
    }

    @Test
    @MediumTest
    public void testCellInfoList() {
        Parcel p = Parcel.obtain();
        p.writeInt(1);
        p.writeInt(1);
        p.writeInt(2);
        p.writeLong(1453510289108L);
        p.writeInt(310);
        p.writeInt(260);
        p.writeInt(123);
        p.writeInt(456);
        p.writeInt(99);
        p.writeInt(3);
        p.setDataPosition(0);

        CellInfoGsm cellInfo = CellInfoGsm.CREATOR.createFromParcel(p);

        ArrayList<CellInfo> list = new ArrayList();
        list.add(cellInfo);
        mSimulatedCommands.setCellInfoList(list);

        WorkSource workSource = new WorkSource(Process.myUid(),
                mContext.getPackageName());
        assertEquals(sst.getAllCellInfo(workSource), list);
    }

    @Test
    @MediumTest
    public void testImsRegState() {
        // Simulate IMS registered
        mSimulatedCommands.setImsRegistrationState(new int[]{1, PhoneConstants.PHONE_TYPE_GSM});

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_IMS_STATE_CHANGED, null));
        waitForMs(200);

        assertTrue(sst.isImsRegistered());

        // Simulate IMS unregistered
        mSimulatedCommands.setImsRegistrationState(new int[]{0, PhoneConstants.PHONE_TYPE_GSM});

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_IMS_STATE_CHANGED, null));
        waitForMs(200);

        assertFalse(sst.isImsRegistered());
    }

    @Test
    public void testOnImsServiceStateChanged() {
        // The service state of GsmCdmaPhone is STATE_OUT_OF_SERVICE, and IMS is unregistered.
        ServiceState ss = new ServiceState();
        ss.setVoiceRegState(ServiceState.STATE_OUT_OF_SERVICE);
        sst.mSS = ss;

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_IMS_SERVICE_STATE_CHANGED));
        waitForMs(200);

        // The listener will be notified that the service state was changed.
        verify(mPhone).notifyServiceStateChanged(any(ServiceState.class));

        // The service state of GsmCdmaPhone is STATE_IN_SERVICE, and IMS is registered.
        ss = new ServiceState();
        ss.setVoiceRegState(ServiceState.STATE_IN_SERVICE);
        sst.mSS = ss;

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_IMS_SERVICE_STATE_CHANGED));
        waitForMs(200);

        // Nothing happened because the IMS service state was not affected the merged service state.
        verify(mPhone, times(1)).notifyServiceStateChanged(any(ServiceState.class));
    }

    @Test
    @MediumTest
    public void testSignalStrength() {
        SignalStrength ss = new SignalStrength(
                30, // gsmSignalStrength
                0,  // gsmBitErrorRate
                -1, // cdmaDbm
                -1, // cdmaEcio
                -1, // evdoDbm
                -1, // evdoEcio
                -1, // evdoSnr
                99, // lteSignalStrength
                SignalStrength.INVALID,     // lteRsrp
                SignalStrength.INVALID,     // lteRsrq
                SignalStrength.INVALID,     // lteRssnr
                SignalStrength.INVALID,     // lteCqi
                SignalStrength.INVALID      // tdScdmaRscp
        );

        mSimulatedCommands.setSignalStrength(ss);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().isGsm(), true);

        // switch to CDMA
        doReturn(false).when(mPhone).isPhoneTypeGsm();
        doReturn(true).when(mPhone).isPhoneTypeCdmaLte();
        sst.updatePhoneType();
        sst.mSS.setRilDataRadioTechnology(ServiceState.RIL_RADIO_TECHNOLOGY_LTE);

        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().isGsm(), true);

        // notify signal strength again, but this time data RAT is not LTE
        sst.mSS.setRilVoiceRadioTechnology(ServiceState.RIL_RADIO_TECHNOLOGY_1xRTT);
        sst.mSS.setRilDataRadioTechnology(ServiceState.RIL_RADIO_TECHNOLOGY_EHRPD);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().isGsm(), false);
    }

    @Test
    public void testSetsNewSignalStrengthReportingCriteria() {
        int[] wcdmaThresholds = {
                -110, /* SIGNAL_STRENGTH_POOR */
                -100, /* SIGNAL_STRENGTH_MODERATE */
                -90, /* SIGNAL_STRENGTH_GOOD */
                -80  /* SIGNAL_STRENGTH_GREAT */
        };
        mBundle.putIntArray(CarrierConfigManager.KEY_WCDMA_RSCP_THRESHOLDS_INT_ARRAY,
                wcdmaThresholds);

        int[] lteThresholds = {
                -130, /* SIGNAL_STRENGTH_POOR */
                -120, /* SIGNAL_STRENGTH_MODERATE */
                -110, /* SIGNAL_STRENGTH_GOOD */
                -100,  /* SIGNAL_STRENGTH_GREAT */
        };
        mBundle.putIntArray(CarrierConfigManager.KEY_LTE_RSRP_THRESHOLDS_INT_ARRAY,
                lteThresholds);

        CarrierConfigManager mockConfigManager = Mockito.mock(CarrierConfigManager.class);
        when(mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE))
                .thenReturn(mockConfigManager);
        when(mockConfigManager.getConfigForSubId(anyInt())).thenReturn(mBundle);

        Intent intent = new Intent().setAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        mContext.sendBroadcast(intent);
        waitForMs(300);

        verify(mPhone).setSignalStrengthReportingCriteria(eq(wcdmaThresholds),
                eq(AccessNetworkType.UTRAN));
        verify(mPhone).setSignalStrengthReportingCriteria(eq(lteThresholds),
                eq(AccessNetworkType.EUTRAN));
    }

    @Test
    @MediumTest
    public void testSignalLevelWithWcdmaRscpThresholds() {
        mBundle.putIntArray(CarrierConfigManager.KEY_WCDMA_RSCP_THRESHOLDS_INT_ARRAY,
                new int[] {
                        -110, /* SIGNAL_STRENGTH_POOR */
                        -100, /* SIGNAL_STRENGTH_MODERATE */
                        -90, /* SIGNAL_STRENGTH_GOOD */
                        -80  /* SIGNAL_STRENGTH_GREAT */
                });
        mBundle.putString(
                CarrierConfigManager.KEY_WCDMA_DEFAULT_SIGNAL_STRENGTH_MEASUREMENT_STRING,
                "rscp");

        SignalStrength ss = new SignalStrength(
                30, // gsmSignalStrength
                0,  // gsmBitErrorRate
                -1, // cdmaDbm
                -1, // cdmaEcio
                -1, // evdoDbm
                -1, // evdoEcio
                -1, // evdoSnr
                99, // lteSignalStrength
                SignalStrength.INVALID,     // lteRsrp
                SignalStrength.INVALID,     // lteRsrq
                SignalStrength.INVALID,     // lteRssnr
                SignalStrength.INVALID,     // lteCqi
                SignalStrength.INVALID,     // tdScdmaRscp
                99,                         // wcdmaSignalStrength
                45                         // wcdmaRscpAsu
        );
        mSimulatedCommands.setSignalStrength(ss);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().getWcdmaLevel(), SignalStrength.SIGNAL_STRENGTH_GREAT);
        assertEquals(sst.getSignalStrength().getWcdmaAsuLevel(), 45);
        assertEquals(sst.getSignalStrength().getWcdmaDbm(), -75);

        ss = new SignalStrength(
                30, // gsmSignalStrength
                0,  // gsmBitErrorRate
                -1, // cdmaDbm
                -1, // cdmaEcio
                -1, // evdoDbm
                -1, // evdoEcio
                -1, // evdoSnr
                99, // lteSignalStrength
                SignalStrength.INVALID,     // lteRsrp
                SignalStrength.INVALID,     // lteRsrq
                SignalStrength.INVALID,     // lteRssnr
                SignalStrength.INVALID,     // lteCqi
                SignalStrength.INVALID,     // tdScdmaRscp
                99,                         // wcdmaSignalStrength
                35                          // wcdmaRscpAsu
        );
        mSimulatedCommands.setSignalStrength(ss);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().getWcdmaLevel(), SignalStrength.SIGNAL_STRENGTH_GOOD);
        assertEquals(sst.getSignalStrength().getWcdmaAsuLevel(), 35);
        assertEquals(sst.getSignalStrength().getWcdmaDbm(), -85);

        ss = new SignalStrength(
                30, // gsmSignalStrength
                0,  // gsmBitErrorRate
                -1, // cdmaDbm
                -1, // cdmaEcio
                -1, // evdoDbm
                -1, // evdoEcio
                -1, // evdoSnr
                99, // lteSignalStrength
                SignalStrength.INVALID,     // lteRsrp
                SignalStrength.INVALID,     // lteRsrq
                SignalStrength.INVALID,     // lteRssnr
                SignalStrength.INVALID,     // lteCqi
                SignalStrength.INVALID,     // tdScdmaRscp
                99,                         // wcdmaSignalStrength
                25                          // wcdmaRscpAsu
        );
        mSimulatedCommands.setSignalStrength(ss);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().getWcdmaLevel(),
                SignalStrength.SIGNAL_STRENGTH_MODERATE);
        assertEquals(sst.getSignalStrength().getWcdmaAsuLevel(), 25);
        assertEquals(sst.getSignalStrength().getWcdmaDbm(), -95);

        ss = new SignalStrength(
                30, // gsmSignalStrength
                0,  // gsmBitErrorRate
                -1, // cdmaDbm
                -1, // cdmaEcio
                -1, // evdoDbm
                -1, // evdoEcio
                -1, // evdoSnr
                99, // lteSignalStrength
                SignalStrength.INVALID,     // lteRsrp
                SignalStrength.INVALID,     // lteRsrq
                SignalStrength.INVALID,     // lteRssnr
                SignalStrength.INVALID,     // lteCqi
                SignalStrength.INVALID,     // tdScdmaRscp
                99,                         // wcdmaSignalStrength
                15                          // wcdmaRscpAsu
        );
        mSimulatedCommands.setSignalStrength(ss);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().getWcdmaLevel(), SignalStrength.SIGNAL_STRENGTH_POOR);
        assertEquals(sst.getSignalStrength().getWcdmaAsuLevel(), 15);
        assertEquals(sst.getSignalStrength().getWcdmaDbm(), -105);

        ss = new SignalStrength(
                30, // gsmSignalStrength
                0,  // gsmBitErrorRate
                -1, // cdmaDbm
                -1, // cdmaEcio
                -1, // evdoDbm
                -1, // evdoEcio
                -1, // evdoSnr
                99, // lteSignalStrength
                SignalStrength.INVALID,     // lteRsrp
                SignalStrength.INVALID,     // lteRsrq
                SignalStrength.INVALID,     // lteRssnr
                SignalStrength.INVALID,     // lteCqi
                SignalStrength.INVALID,     // tdScdmaRscp
                99,                         // wcdmaSignalStrength
                5                           // wcdmaRscpAsu
        );
        mSimulatedCommands.setSignalStrength(ss);
        mSimulatedCommands.notifySignalStrength();
        waitForMs(300);
        assertEquals(sst.getSignalStrength(), ss);
        assertEquals(sst.getSignalStrength().getWcdmaLevel(),
                SignalStrength.SIGNAL_STRENGTH_NONE_OR_UNKNOWN);
        assertEquals(sst.getSignalStrength().getWcdmaAsuLevel(), 5);
        assertEquals(sst.getSignalStrength().getWcdmaDbm(), -115);
    }

    @Test
    @MediumTest
    public void testGsmCellLocation() {
        CellIdentityGsm cellIdentityGsm = new CellIdentityGsm(0, 0, 2, 3);
        NetworkRegistrationState result = new NetworkRegistrationState(
                0, 0, 0, 0, 0, false, null, cellIdentityGsm);

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_GET_LOC_DONE,
                new AsyncResult(null, result, null)));

        waitForMs(200);
        WorkSource workSource = new WorkSource(Process.myUid(), mContext.getPackageName());
        GsmCellLocation cl = (GsmCellLocation) sst.getCellLocation(workSource);
        assertEquals(2, cl.getLac());
        assertEquals(3, cl.getCid());
    }

    @Test
    @MediumTest
    public void testUpdatePhoneType() {
        doReturn(false).when(mPhone).isPhoneTypeGsm();
        doReturn(true).when(mPhone).isPhoneTypeCdmaLte();
        doReturn(CdmaSubscriptionSourceManager.SUBSCRIPTION_FROM_RUIM).when(mCdmaSSM).
                getCdmaSubscriptionSource();

        logd("Calling updatePhoneType");
        // switch to CDMA
        sst.updatePhoneType();

        ArgumentCaptor<Integer> integerArgumentCaptor = ArgumentCaptor.forClass(Integer.class);
        verify(mRuimRecords).registerForRecordsLoaded(eq(sst), integerArgumentCaptor.capture(),
                nullable(Object.class));

        // response for mRuimRecords.registerForRecordsLoaded()
        Message msg = Message.obtain();
        msg.what = integerArgumentCaptor.getValue();
        msg.obj = new AsyncResult(null, null, null);
        sst.sendMessage(msg);
        waitForMs(100);

        // on RUIM_RECORDS_LOADED, sst is expected to call following apis
        verify(mRuimRecords, times(1)).isProvisioned();

        // switch back to GSM
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        doReturn(false).when(mPhone).isPhoneTypeCdmaLte();

        // response for mRuimRecords.registerForRecordsLoaded() can be sent after switching to GSM
        msg = Message.obtain();
        msg.what = integerArgumentCaptor.getValue();
        msg.obj = new AsyncResult(null, null, null);
        sst.sendMessage(msg);

        // There's no easy way to check if the msg was handled or discarded. Wait to make sure sst
        // did not crash, and then verify that the functions called records loaded are not called
        // again
        waitForMs(200);

        verify(mRuimRecords, times(1)).isProvisioned();
    }

    @Test
    @MediumTest
    public void testRegAndUnregForVoiceRoamingOn() throws Exception {
        sst.registerForVoiceRoamingOn(mTestHandler, EVENT_DATA_ROAMING_ON, null);

        // Enable roaming and trigger events to notify handler registered
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_ROAMING_ON, messageArgumentCaptor.getValue().what);

        // Disable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForVoiceRoamingOn(mTestHandler);

        // Enable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndUnregForVoiceRoamingOff() throws Exception {
        // Enable roaming
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        sst.registerForVoiceRoamingOff(mTestHandler, EVENT_DATA_ROAMING_OFF, null);

        // Disable roaming
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_ROAMING_OFF, messageArgumentCaptor.getValue().what);

        // Enable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForVoiceRoamingOff(mTestHandler);

        // Disable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndUnregForDataRoamingOn() throws Exception {
        sst.registerForDataRoamingOn(mTestHandler, EVENT_DATA_ROAMING_ON, null);

        // Enable roaming and trigger events to notify handler registered
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_ROAMING_ON, messageArgumentCaptor.getValue().what);

        // Disable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForDataRoamingOn(mTestHandler);

        // Enable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndUnregForDataRoamingOff() throws Exception {
        // Enable roaming
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        sst.registerForDataRoamingOff(mTestHandler, EVENT_DATA_ROAMING_OFF, null, true);

        // Disable roaming
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_ROAMING_OFF, messageArgumentCaptor.getValue().what);

        // Enable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForDataRoamingOff(mTestHandler);

        // Disable roaming
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_HOME);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndInvalidregForDataConnAttach() throws Exception {
        // Initially set service state out of service
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(23);
        mSimulatedCommands.setDataRegState(23);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        sst.registerForDataConnectionAttached(mTestHandler, EVENT_DATA_CONNECTION_ATTACHED, null);

        // set service state in service and trigger events to post message on handler
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_CONNECTION_ATTACHED, messageArgumentCaptor.getValue().what);

        // set service state out of service
        mSimulatedCommands.setVoiceRegState(-1);
        mSimulatedCommands.setDataRegState(-1);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForDataConnectionAttached(mTestHandler);

        // set service state in service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndUnregForDataConnAttach() throws Exception {
        // Initially set service state out of service
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        sst.registerForDataConnectionAttached(mTestHandler, EVENT_DATA_CONNECTION_ATTACHED, null);

        // set service state in service and trigger events to post message on handler
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_CONNECTION_ATTACHED, messageArgumentCaptor.getValue().what);

        // set service state out of service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForDataConnectionAttached(mTestHandler);

        // set service state in service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndUnregForDataConnDetach() throws Exception {
        // Initially set service state in service
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        sst.registerForDataConnectionDetached(mTestHandler, EVENT_DATA_CONNECTION_DETACHED, null);

        // set service state out of service and trigger events to post message on handler
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_CONNECTION_DETACHED, messageArgumentCaptor.getValue().what);

        // set service state in service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForDataConnectionDetached(mTestHandler);

        // set service state out of service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegisterForDataRegStateOrRatChange() {
        int drs = NetworkRegistrationState.REG_STATE_HOME;
        int rat = sst.mSS.RIL_RADIO_TECHNOLOGY_LTE;
        sst.mSS.setRilDataRadioTechnology(rat);
        sst.mSS.setDataRegState(drs);
        sst.registerForDataRegStateOrRatChanged(mTestHandler, EVENT_DATA_RAT_CHANGED, null);

        waitForMs(100);

        // Verify if message was posted to handler and value of result
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_DATA_RAT_CHANGED, messageArgumentCaptor.getValue().what);
        assertEquals(new Pair<Integer, Integer>(drs, rat),
                ((AsyncResult)messageArgumentCaptor.getValue().obj).result);
    }

    @Test
    @MediumTest
    public void testRegAndUnregForNetworkAttached() throws Exception {
        // Initially set service state out of service
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        sst.registerForNetworkAttached(mTestHandler, EVENT_REGISTERED_TO_NETWORK, null);

        // set service state in service and trigger events to post message on handler
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_REGISTERED_TO_NETWORK, messageArgumentCaptor.getValue().what);

        // set service state out of service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_UNKNOWN);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForNetworkAttached(mTestHandler);

        // set service state in service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify that no new message posted to handler
        verify(mTestHandler, times(1)).sendMessageAtTime(any(Message.class), anyLong());
    }

    @Test
    @MediumTest
    public void testRegAndInvalidRegForNetworkAttached() throws Exception {
        // Initially set service state out of service
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        mSimulatedCommands.setVoiceRegState(23);
        mSimulatedCommands.setDataRegState(23);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        sst.registerForNetworkAttached(mTestHandler, EVENT_REGISTERED_TO_NETWORK, null);

        // set service state in service and trigger events to post message on handler
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_REGISTERED_TO_NETWORK, messageArgumentCaptor.getValue().what);

        // set service state out of service
        mSimulatedCommands.setVoiceRegState(-1);
        mSimulatedCommands.setDataRegState(-1);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // Unregister registrant
        sst.unregisterForNetworkAttached(mTestHandler);


        waitForMs(100);

        sst.registerForNetworkAttached(mTestHandler, EVENT_REGISTERED_TO_NETWORK, null);

        // set service state in service
        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(100);

        // verify if registered handler has message posted to it
        messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler, times(2)).sendMessageAtTime(messageArgumentCaptor.capture(),
                anyLong());
        assertEquals(EVENT_REGISTERED_TO_NETWORK, messageArgumentCaptor.getValue().what);
    }

    @Test
    @MediumTest
    public void testRegisterForPsRestrictedEnabled() throws Exception {
        sst.mRestrictedState.setPsRestricted(true);
        // Since PsRestricted is set to true, registerForPsRestrictedEnabled will
        // also post message to handler
        sst.registerForPsRestrictedEnabled(mTestHandler, EVENT_PS_RESTRICT_ENABLED, null);

        waitForMs(100);

        // verify posted message
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_PS_RESTRICT_ENABLED, messageArgumentCaptor.getValue().what);
    }

    @Test
    @MediumTest
    public void testRegisterForPsRestrictedDisabled() throws Exception {
        sst.mRestrictedState.setPsRestricted(true);
        // Since PsRestricted is set to true, registerForPsRestrictedDisabled will
        // also post message to handler
        sst.registerForPsRestrictedDisabled(mTestHandler, EVENT_PS_RESTRICT_DISABLED, null);

        waitForMs(100);

        // verify posted message
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_PS_RESTRICT_DISABLED, messageArgumentCaptor.getValue().what);
    }

    @Test
    @MediumTest
    public void testOnRestrictedStateChanged() throws Exception {
        ServiceStateTracker spySst = spy(sst);
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        doReturn(IccCardApplicationStatus.AppState.APPSTATE_READY).when(
                mUiccCardApplication3gpp).getState();

        ArgumentCaptor<Integer> intArgumentCaptor = ArgumentCaptor.forClass(Integer.class);
        verify(mSimulatedCommandsVerifier).setOnRestrictedStateChanged(any(Handler.class),
                intArgumentCaptor.capture(), eq(null));
        // Since spy() creates a copy of sst object we need to call
        // setOnRestrictedStateChanged() explicitly.
        mSimulatedCommands.setOnRestrictedStateChanged(spySst,
                intArgumentCaptor.getValue().intValue(), null);

        // Combination of restricted state and expected notification type.
        final int CS_ALL[] = {RILConstants.RIL_RESTRICTED_STATE_CS_ALL,
                ServiceStateTracker.CS_ENABLED};
        final int CS_NOR[] = {RILConstants.RIL_RESTRICTED_STATE_CS_NORMAL,
                ServiceStateTracker.CS_NORMAL_ENABLED};
        final int CS_EME[] = {RILConstants.RIL_RESTRICTED_STATE_CS_EMERGENCY,
                ServiceStateTracker.CS_EMERGENCY_ENABLED};
        final int CS_NON[] = {RILConstants.RIL_RESTRICTED_STATE_NONE,
                ServiceStateTracker.CS_DISABLED};
        final int PS_ALL[] = {RILConstants.RIL_RESTRICTED_STATE_PS_ALL,
                ServiceStateTracker.PS_ENABLED};
        final int PS_NON[] = {RILConstants.RIL_RESTRICTED_STATE_NONE,
                ServiceStateTracker.PS_DISABLED};

        int notifyCount = 0;
        // cs not restricted -> cs emergency/normal restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_ALL);
        // cs emergency/normal restricted -> cs normal restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_NOR);
        // cs normal restricted -> cs emergency restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_EME);
        // cs emergency restricted -> cs not restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_NON);
        // cs not restricted -> cs normal restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_NOR);
        // cs normal restricted -> cs emergency/normal restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_ALL);
        // cs emergency/normal restricted -> cs emergency restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_EME);
        // cs emergency restricted -> cs emergency/normal restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_ALL);
        // cs emergency/normal restricted -> cs not restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_NON);
        // cs not restricted -> cs emergency restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_EME);
        // cs emergency restricted -> cs normal restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_NOR);
        // cs normal restricted -> cs not restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, CS_NON);

        // ps not restricted -> ps restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, PS_ALL);
        // ps restricted -> ps not restricted
        internalCheckForRestrictedStateChange(spySst, ++notifyCount, PS_NON);
    }

    private void internalCheckForRestrictedStateChange(ServiceStateTracker serviceStateTracker,
                int times, int[] restrictedState) {
        mSimulatedCommands.triggerRestrictedStateChanged(restrictedState[0]);
        waitForMs(100);
        ArgumentCaptor<Integer> intArgumentCaptor = ArgumentCaptor.forClass(Integer.class);
        verify(serviceStateTracker, times(times)).setNotification(intArgumentCaptor.capture());
        assertEquals(intArgumentCaptor.getValue().intValue(), restrictedState[1]);
    }

    private boolean notificationHasTitleSet(Notification n) {
        // Notification has no methods to check the actual title, but #toString() includes the
        // word "tick" if the title is set so we check this as a workaround
        return n.toString().contains("tick");
    }

    private String getNotificationTitle(Notification n) {
        return n.extras.getString(Notification.EXTRA_TITLE);
    }

    @Test
    @SmallTest
    public void testSetPsNotifications() {
        sst.mSubId = 1;
        final NotificationManager nm = (NotificationManager)
                mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mContextFixture.putBooleanResource(
                R.bool.config_user_notification_of_restrictied_mobile_access, true);
        doReturn(new ApplicationInfo()).when(mContext).getApplicationInfo();
        Drawable mockDrawable = mock(Drawable.class);
        Resources mockResources = mContext.getResources();
        when(mockResources.getDrawable(anyInt(), any())).thenReturn(mockDrawable);

        mContextFixture.putResource(com.android.internal.R.string.RestrictedOnDataTitle, "test1");
        sst.setNotification(ServiceStateTracker.PS_ENABLED);
        ArgumentCaptor<Notification> notificationArgumentCaptor =
                ArgumentCaptor.forClass(Notification.class);
        verify(nm).notify(anyString(), anyInt(), notificationArgumentCaptor.capture());
        // if the postedNotification has title set then it must have been the correct notification
        Notification postedNotification = notificationArgumentCaptor.getValue();
        assertTrue(notificationHasTitleSet(postedNotification));
        assertEquals("test1", getNotificationTitle(postedNotification));

        sst.setNotification(ServiceStateTracker.PS_DISABLED);
        verify(nm).cancel(anyString(), anyInt());
    }

    @Test
    @SmallTest
    public void testSetCsNotifications() {
        sst.mSubId = 1;
        final NotificationManager nm = (NotificationManager)
                mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mContextFixture.putBooleanResource(
                R.bool.config_user_notification_of_restrictied_mobile_access, true);
        doReturn(new ApplicationInfo()).when(mContext).getApplicationInfo();
        Drawable mockDrawable = mock(Drawable.class);
        Resources mockResources = mContext.getResources();
        when(mockResources.getDrawable(anyInt(), any())).thenReturn(mockDrawable);

        mContextFixture.putResource(com.android.internal.R.string.RestrictedOnAllVoiceTitle,
                "test2");
        sst.setNotification(ServiceStateTracker.CS_ENABLED);
        ArgumentCaptor<Notification> notificationArgumentCaptor =
                ArgumentCaptor.forClass(Notification.class);
        verify(nm).notify(anyString(), anyInt(), notificationArgumentCaptor.capture());
        // if the postedNotification has title set then it must have been the correct notification
        Notification postedNotification = notificationArgumentCaptor.getValue();
        assertTrue(notificationHasTitleSet(postedNotification));
        assertEquals("test2", getNotificationTitle(postedNotification));

        sst.setNotification(ServiceStateTracker.CS_DISABLED);
        verify(nm).cancel(anyString(), anyInt());
    }

    @Test
    @SmallTest
    public void testSetCsNormalNotifications() {
        sst.mSubId = 1;
        final NotificationManager nm = (NotificationManager)
                mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mContextFixture.putBooleanResource(
                R.bool.config_user_notification_of_restrictied_mobile_access, true);
        doReturn(new ApplicationInfo()).when(mContext).getApplicationInfo();
        Drawable mockDrawable = mock(Drawable.class);
        Resources mockResources = mContext.getResources();
        when(mockResources.getDrawable(anyInt(), any())).thenReturn(mockDrawable);

        mContextFixture.putResource(com.android.internal.R.string.RestrictedOnNormalTitle, "test3");
        sst.setNotification(ServiceStateTracker.CS_NORMAL_ENABLED);
        ArgumentCaptor<Notification> notificationArgumentCaptor =
                ArgumentCaptor.forClass(Notification.class);
        verify(nm).notify(anyString(), anyInt(), notificationArgumentCaptor.capture());
        // if the postedNotification has title set then it must have been the correct notification
        Notification postedNotification = notificationArgumentCaptor.getValue();
        assertTrue(notificationHasTitleSet(postedNotification));
        assertEquals("test3", getNotificationTitle(postedNotification));

        sst.setNotification(ServiceStateTracker.CS_DISABLED);
        verify(nm).cancel(anyString(), anyInt());
    }

    @Test
    @SmallTest
    public void testSetCsEmergencyNotifications() {
        sst.mSubId = 1;
        final NotificationManager nm = (NotificationManager)
                mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        mContextFixture.putBooleanResource(
                R.bool.config_user_notification_of_restrictied_mobile_access, true);
        doReturn(new ApplicationInfo()).when(mContext).getApplicationInfo();
        Drawable mockDrawable = mock(Drawable.class);
        Resources mockResources = mContext.getResources();
        when(mockResources.getDrawable(anyInt(), any())).thenReturn(mockDrawable);

        mContextFixture.putResource(com.android.internal.R.string.RestrictedOnEmergencyTitle,
                "test4");
        sst.setNotification(ServiceStateTracker.CS_EMERGENCY_ENABLED);
        ArgumentCaptor<Notification> notificationArgumentCaptor =
                ArgumentCaptor.forClass(Notification.class);
        verify(nm).notify(anyString(), anyInt(), notificationArgumentCaptor.capture());
        // if the postedNotification has title set then it must have been the correct notification
        Notification postedNotification = notificationArgumentCaptor.getValue();
        assertTrue(notificationHasTitleSet(postedNotification));
        assertEquals("test4", getNotificationTitle(postedNotification));

        sst.setNotification(ServiceStateTracker.CS_DISABLED);
        verify(nm).cancel(anyString(), anyInt());
        sst.setNotification(ServiceStateTracker.CS_REJECT_CAUSE_ENABLED);
    }

    @Test
    @MediumTest
    public void testRegisterForSubscriptionInfoReady() {
        sst.registerForSubscriptionInfoReady(mTestHandler, EVENT_SUBSCRIPTION_INFO_READY, null);

        // Call functions which would trigger posting of message on test handler
        doReturn(false).when(mPhone).isPhoneTypeGsm();
        sst.updatePhoneType();
        mSimulatedCommands.notifyOtaProvisionStatusChanged();

        waitForMs(200);

        // verify posted message
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler).sendMessageAtTime(messageArgumentCaptor.capture(), anyLong());
        assertEquals(EVENT_SUBSCRIPTION_INFO_READY, messageArgumentCaptor.getValue().what);
    }

    @Test
    @MediumTest
    public void testRoamingPhoneTypeSwitch() {
        // Enable roaming
        doReturn(true).when(mPhone).isPhoneTypeGsm();

        mSimulatedCommands.setVoiceRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.setDataRegState(NetworkRegistrationState.REG_STATE_ROAMING);
        mSimulatedCommands.notifyNetworkStateChanged();

        waitForMs(200);

        sst.registerForDataRoamingOff(mTestHandler, EVENT_DATA_ROAMING_OFF, null, true);
        sst.registerForVoiceRoamingOff(mTestHandler, EVENT_VOICE_ROAMING_OFF, null);
        sst.registerForDataConnectionDetached(mTestHandler, EVENT_DATA_CONNECTION_DETACHED, null);

        // Call functions which would trigger posting of message on test handler
        doReturn(false).when(mPhone).isPhoneTypeGsm();
        sst.updatePhoneType();

        // verify if registered handler has message posted to it
        ArgumentCaptor<Message> messageArgumentCaptor = ArgumentCaptor.forClass(Message.class);
        verify(mTestHandler, atLeast(3)).sendMessageAtTime(
                messageArgumentCaptor.capture(), anyLong());
        HashSet<Integer> messageSet = new HashSet<>();
        for (Message m : messageArgumentCaptor.getAllValues()) {
            messageSet.add(m.what);
        }

        assertTrue(messageSet.contains(EVENT_DATA_ROAMING_OFF));
        assertTrue(messageSet.contains(EVENT_VOICE_ROAMING_OFF));
        assertTrue(messageSet.contains(EVENT_DATA_CONNECTION_DETACHED));
    }

    @Test
    @SmallTest
    public void testGetDesiredPowerState() {
        sst.setRadioPower(true);
        assertEquals(sst.getDesiredPowerState(), true);
    }

    @Test
    @MediumTest
    public void testEnableLocationUpdates() throws Exception {
        sst.enableLocationUpdates();
        verify(mSimulatedCommandsVerifier, times(1)).setLocationUpdates(eq(true),
                any(Message.class));
    }

    @Test
    @SmallTest
    public void testDisableLocationUpdates() throws Exception {
        sst.disableLocationUpdates();
        verify(mSimulatedCommandsVerifier, times(1)).setLocationUpdates(eq(false),
                nullable(Message.class));
    }

    @Test
    @SmallTest
    public void testGetCurrentDataRegState() throws Exception {
        sst.mSS.setDataRegState(ServiceState.STATE_OUT_OF_SERVICE);
        assertEquals(sst.getCurrentDataConnectionState(), ServiceState.STATE_OUT_OF_SERVICE);
    }

    @Test
    @SmallTest
    public void testIsConcurrentVoiceAndDataAllowed() {
        doReturn(false).when(mPhone).isPhoneTypeGsm();
        sst.mSS.setCssIndicator(1);
        assertEquals(true, sst.isConcurrentVoiceAndDataAllowed());
        sst.mSS.setCssIndicator(0);
        assertEquals(false, sst.isConcurrentVoiceAndDataAllowed());

        doReturn(true).when(mPhone).isPhoneTypeGsm();
        sst.mSS.setRilDataRadioTechnology(sst.mSS.RIL_RADIO_TECHNOLOGY_HSPA);
        assertEquals(true, sst.isConcurrentVoiceAndDataAllowed());
        sst.mSS.setRilDataRadioTechnology(sst.mSS.RIL_RADIO_TECHNOLOGY_GPRS);
        assertEquals(false, sst.isConcurrentVoiceAndDataAllowed());
        sst.mSS.setCssIndicator(1);
        assertEquals(true, sst.isConcurrentVoiceAndDataAllowed());
    }

    @Test
    @MediumTest
    public void testIsImsRegistered() throws Exception {
        mSimulatedCommands.setImsRegistrationState(new int[]{1, PhoneConstants.PHONE_TYPE_GSM});
        mSimulatedCommands.notifyImsNetworkStateChanged();
        waitForMs(200);
        assertEquals(sst.isImsRegistered(), true);
    }

    @Test
    @SmallTest
    public void testIsDeviceShuttingDown() throws Exception {
        sst.requestShutdown();
        assertEquals(true, sst.isDeviceShuttingDown());
    }

    @Test
    @SmallTest
    public void testShuttingDownRequest() throws Exception {
        sst.setRadioPower(true);
        waitForMs(100);

        sst.requestShutdown();
        waitForMs(100);
        assertFalse(mSimulatedCommands.getRadioState().isAvailable());
    }

    @Test
    @SmallTest
    public void testShuttingDownRequestWithRadioPowerFailResponse() throws Exception {
        sst.setRadioPower(true);
        waitForMs(100);

        // Simulate RIL fails the radio power settings.
        mSimulatedCommands.setRadioPowerFailResponse(true);
        sst.setRadioPower(false);
        waitForMs(100);
        assertTrue(mSimulatedCommands.getRadioState().isOn());
        sst.requestShutdown();
        waitForMs(100);
        assertFalse(mSimulatedCommands.getRadioState().isAvailable());
    }

    @Test
    @SmallTest
    public void testSetTimeFromNITZStr() throws Exception {
        {
            // Mock sending incorrect nitz str from RIL
            mSimulatedCommands.triggerNITZupdate("38/06/20,00:00:00+0");
            waitForMs(200);
            verify(mNitzStateMachine, times(0)).handleNitzReceived(any());
        }
        {
            // Mock sending correct nitz str from RIL
            String nitzStr = "15/06/20,00:00:00+0";
            NitzData expectedNitzData = NitzData.parse(nitzStr);
            mSimulatedCommands.triggerNITZupdate(nitzStr);
            waitForMs(200);

            ArgumentCaptor<TimeStampedValue<NitzData>> argumentsCaptor =
                    ArgumentCaptor.forClass(TimeStampedValue.class);
            verify(mNitzStateMachine, times(1))
                    .handleNitzReceived(argumentsCaptor.capture());

            // Confirm the argument was what we expected.
            TimeStampedValue<NitzData> actualNitzSignal = argumentsCaptor.getValue();
            assertEquals(expectedNitzData, actualNitzSignal.mValue);
            assertTrue(actualNitzSignal.mElapsedRealtime <= SystemClock.elapsedRealtime());
        }
    }

    // Edge and GPRS are grouped under the same family and Edge has higher rate than GPRS.
    // Expect no rat update when move from E to G.
    @Test
    public void testRatRatchet() throws Exception {
        CellIdentityGsm cellIdentity = new CellIdentityGsm(-1, -1, -1, -1, -1, -1);
        NetworkRegistrationState dataResult = new NetworkRegistrationState(
                0, 0, 1, 2, 0, false, null, cellIdentity, 1);
        sst.mPollingContext[0] = 2;
        // update data reg state to be in service
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        NetworkRegistrationState voiceResult = new NetworkRegistrationState(
                0, 0, 1, 2, 0, false, null, cellIdentity, false, 0, 0, 0);
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_EDGE, sst.mSS.getRilVoiceRadioTechnology());

        // EDGE -> GPRS
        voiceResult = new NetworkRegistrationState(
                0, 0, 1, 1, 0, false, null,
                cellIdentity, false, 0, 0, 0);
        sst.mPollingContext[0] = 2;
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_EDGE, sst.mSS.getRilVoiceRadioTechnology());
    }

    // Edge and GPRS are grouped under the same family and Edge has higher rate than GPRS.
    // Bypass rat rachet when cell id changed. Expect rat update from E to G
    @Test
    public void testRatRatchetWithCellChange() throws Exception {
        CellIdentityGsm cellIdentity = new CellIdentityGsm(-1, -1, -1, -1, -1, -1);
        NetworkRegistrationState dataResult = new NetworkRegistrationState(
                0, 0, 1, 2, 0, false, null, cellIdentity, 1);
        sst.mPollingContext[0] = 2;
        // update data reg state to be in service
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        NetworkRegistrationState voiceResult = new NetworkRegistrationState(
                0, 0, 1, 2, 0, false, null, cellIdentity, false, 0, 0, 0);
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_EDGE, sst.mSS.getRilVoiceRadioTechnology());

        // RAT: EDGE -> GPRS cell ID: -1 -> 5
        cellIdentity = new CellIdentityGsm(-1, -1, -1, 5, -1, -1);
        voiceResult = new NetworkRegistrationState(
                0, 0, 1, 1, 0, false, null, cellIdentity, false, 0, 0, 0);
        sst.mPollingContext[0] = 2;
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_GPRS, sst.mSS.getRilVoiceRadioTechnology());
    }

    // Edge, GPRS and UMTS are grouped under the same family where Edge > GPRS > UMTS  .
    // Expect no rat update from E to G immediately following cell id change.
    // Expect ratratchet (from G to UMTS) for the following rat update within the cell location.
    @Test
    public void testRatRatchetWithCellChangeBeforeRatChange() throws Exception {
        // cell ID update
        CellIdentityGsm cellIdentity = new CellIdentityGsm(-1, -1, -1, 5, -1, -1);
        NetworkRegistrationState dataResult = new NetworkRegistrationState(
                0, 0, 1, 2, 0, false, null, cellIdentity, 1);
        sst.mPollingContext[0] = 2;
        // update data reg state to be in service
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        NetworkRegistrationState voiceResult = new NetworkRegistrationState(
                0, 0, 1, 2, 0, false, null, cellIdentity, false, 0, 0, 0);
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_EDGE, sst.mSS.getRilVoiceRadioTechnology());

        // RAT: EDGE -> GPRS, cell ID unchanged. Expect no rat ratchet following cell Id change.
        voiceResult = new NetworkRegistrationState(
                0, 0, 1, 1, 0, false, null, cellIdentity, false, 0, 0, 0);
        sst.mPollingContext[0] = 2;
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_GPRS, sst.mSS.getRilVoiceRadioTechnology());

        // RAT: GPRS -> UMTS
        voiceResult = new NetworkRegistrationState(
                0, 0, 1, 3, 0, false, null, cellIdentity, false, 0, 0, 0);
        sst.mPollingContext[0] = 2;
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.STATE_IN_SERVICE, mSST.getCurrentDataConnectionState());

        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertEquals(ServiceState.RIL_RADIO_TECHNOLOGY_GPRS, sst.mSS.getRilVoiceRadioTechnology());
    }

    private void sendPhyChanConfigChange(int[] bandwidths) {
        ArrayList<PhysicalChannelConfig> pc = new ArrayList<>();
        int ssType = PhysicalChannelConfig.CONNECTION_PRIMARY_SERVING;
        for (int bw : bandwidths) {
            pc.add(new PhysicalChannelConfig(ssType, bw));

            // All cells after the first are secondary serving cells.
            ssType = PhysicalChannelConfig.CONNECTION_SECONDARY_SERVING;
        }
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_PHYSICAL_CHANNEL_CONFIG,
                new AsyncResult(null, pc, null)));
        waitForMs(100);
    }

    private void sendRegStateUpdateForLteCellId(CellIdentityLte cellId) {
        NetworkRegistrationState dataResult = new NetworkRegistrationState(
                1, 2, 1, TelephonyManager.NETWORK_TYPE_LTE, 0, false, null, cellId, 1);
        NetworkRegistrationState voiceResult = new NetworkRegistrationState(
                1, 1, 1, TelephonyManager.NETWORK_TYPE_LTE, 0, false, null, cellId,
                false, 0, 0, 0);
        sst.mPollingContext[0] = 2;
        // update data reg state to be in service
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
    }

    @Test
    public void testPhyChanBandwidthUpdatedOnDataRegState() throws Exception {
        // Cell ID change should trigger hasLocationChanged.
        CellIdentityLte cellIdentity5 =
                new CellIdentityLte(1, 1, 5, 1, 5000, "001", "01", "test", "tst");

        sendPhyChanConfigChange(new int[] {10000});
        sendRegStateUpdateForLteCellId(cellIdentity5);
        assertTrue(Arrays.equals(new int[] {5000}, sst.mSS.getCellBandwidths()));
    }

    @Test
    public void testPhyChanBandwidthNotUpdatedWhenInvalidInCellIdentity() throws Exception {
        // Cell ID change should trigger hasLocationChanged.
        CellIdentityLte cellIdentityInv =
                new CellIdentityLte(1, 1, 5, 1, 12345, "001", "01", "test", "tst");

        sendPhyChanConfigChange(new int[] {10000});
        sendRegStateUpdateForLteCellId(cellIdentityInv);
        assertTrue(Arrays.equals(new int[] {10000}, sst.mSS.getCellBandwidths()));
    }

    @Test
    public void testPhyChanBandwidthPrefersCarrierAggregationReport() throws Exception {
        // Cell ID change should trigger hasLocationChanged.
        CellIdentityLte cellIdentity10 =
                new CellIdentityLte(1, 1, 5, 1, 10000, "001", "01", "test", "tst");

        sendPhyChanConfigChange(new int[] {10000, 5000});
        sendRegStateUpdateForLteCellId(cellIdentity10);
        assertTrue(Arrays.equals(new int[] {10000, 5000}, sst.mSS.getCellBandwidths()));
    }

    @Test
    public void testPhyChanBandwidthRatchetedOnPhyChanBandwidth() throws Exception {
        // LTE Cell with bandwidth = 10000
        CellIdentityLte cellIdentity10 =
                new CellIdentityLte(1, 1, 1, 1, 10000, "1", "1", "test", "tst");

        sendRegStateUpdateForLteCellId(cellIdentity10);
        assertTrue(Arrays.equals(new int[] {10000}, sst.mSS.getCellBandwidths()));
        sendPhyChanConfigChange(new int[] {10000, 5000});
        assertTrue(Arrays.equals(new int[] {10000, 5000}, sst.mSS.getCellBandwidths()));
    }

    @Test
    public void testPhyChanBandwidthResetsOnOos() throws Exception {
        testPhyChanBandwidthRatchetedOnPhyChanBandwidth();
        NetworkRegistrationState dataResult = new NetworkRegistrationState(
                1, 2, 0, TelephonyManager.NETWORK_TYPE_UNKNOWN, 0, false, null, null, 1);
        NetworkRegistrationState voiceResult = new NetworkRegistrationState(
                1, 1, 0, TelephonyManager.NETWORK_TYPE_UNKNOWN, 0, false, null, null,
                false, 0, 0, 0);
        sst.mPollingContext[0] = 2;
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_GPRS,
                new AsyncResult(sst.mPollingContext, dataResult, null)));
        waitForMs(200);
        sst.sendMessage(sst.obtainMessage(ServiceStateTracker.EVENT_POLL_STATE_REGISTRATION,
                new AsyncResult(sst.mPollingContext, voiceResult, null)));
        waitForMs(200);
        assertTrue(Arrays.equals(new int[0], sst.mSS.getCellBandwidths()));
    }
}
