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
package com.android.bluetooth.hfp;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.ServiceManager;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.bluetooth.TestUtils;
import com.android.internal.telephony.ISub;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.HashMap;

/**
 * Unit test to verify various methods in {@link HeadsetPhoneState}
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class HeadsetPhoneStateTest {
    @Mock private ISub mISub;
    @Mock private IBinder mISubBinder;
    @Mock private HeadsetService mHeadsetService;
    @Mock private TelephonyManager mTelephonyManager;
    @Mock private SubscriptionManager mSubscriptionManager;
    private HeadsetPhoneState mHeadsetPhoneState;
    private BluetoothAdapter mAdapter = BluetoothAdapter.getDefaultAdapter();
    private HandlerThread mHandlerThread;
    private HashMap<String, IBinder> mServiceManagerMockedServices = new HashMap<>();
    private Object mServiceManagerOriginalServices;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        // Mock SubscriptionManager.getDefaultSubscriptionId() to return a valid value
        when(mISub.getDefaultSubId()).thenReturn(SubscriptionManager.DEFAULT_SUBSCRIPTION_ID);
        when(mISubBinder.queryLocalInterface(anyString())).thenReturn(mISub);
        mServiceManagerMockedServices.put("isub", mISubBinder);
        mServiceManagerOriginalServices =
                TestUtils.replaceField(ServiceManager.class, "sCache", null,
                        mServiceManagerMockedServices);
        // Stub other methods
        when(mHeadsetService.getSystemService(Context.TELEPHONY_SERVICE)).thenReturn(
                mTelephonyManager);
        when(mHeadsetService.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE)).thenReturn(
                mSubscriptionManager);
        mHandlerThread = new HandlerThread("HeadsetStateMachineTestHandlerThread");
        mHandlerThread.start();
        when(mHeadsetService.getStateMachinesThreadLooper()).thenReturn(mHandlerThread.getLooper());
        mHeadsetPhoneState = new HeadsetPhoneState(mHeadsetService);
    }

    @After
    public void tearDown() throws Exception {
        mHeadsetPhoneState.cleanup();
        mHandlerThread.quit();
        if (mServiceManagerOriginalServices != null) {
            TestUtils.replaceField(ServiceManager.class, "sCache", null,
                    mServiceManagerOriginalServices);
            mServiceManagerOriginalServices = null;
        }

    }

    /**
     * Verify that {@link PhoneStateListener#LISTEN_NONE} should result in no subscription
     */
    @Test
    public void testListenForPhoneState_NoneResultsNoListen() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_NONE);
        verifyZeroInteractions(mTelephonyManager);
    }

    /**
     * Verify that {@link PhoneStateListener#LISTEN_SERVICE_STATE} should result in corresponding
     * subscription
     */
    @Test
    public void testListenForPhoneState_ServiceOnly() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE));
        verifyNoMoreInteractions(mTelephonyManager);
    }

    /**
     * Verify that {@link PhoneStateListener#LISTEN_SERVICE_STATE} and
     * {@link PhoneStateListener#LISTEN_SIGNAL_STRENGTHS} should result in corresponding
     * subscription
     */
    @Test
    public void testListenForPhoneState_ServiceAndSignalStrength() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_IGNORE_SCREEN_OFF);
        verifyNoMoreInteractions(mTelephonyManager);
    }

    /**
     * Verify that partially turnning off {@link PhoneStateListener#LISTEN_SIGNAL_STRENGTHS} update
     * should only unsubscribe signal strength update
     */
    @Test
    public void testListenForPhoneState_ServiceAndSignalStrengthUpdateTurnOffSignalStrengh() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_IGNORE_SCREEN_OFF);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_NONE));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_NORMAL);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE));
        verifyNoMoreInteractions(mTelephonyManager);
    }

    /**
     * Verify that completely disabling all updates should unsubscribe from everything
     */
    @Test
    public void testListenForPhoneState_ServiceAndSignalStrengthUpdateTurnOffAll() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_IGNORE_SCREEN_OFF);
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_NONE);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_NONE));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_NORMAL);
        verifyNoMoreInteractions(mTelephonyManager);
    }

    /**
     * Verify that when multiple devices tries to subscribe to the same indicator, the same
     * subscription is not triggered twice. Also, when one of the device is unsubsidised from an
     * indicator, the other device still remain subscribed.
     */
    @Test
    public void testListenForPhoneState_MultiDevice_AllUpAllDown() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        BluetoothDevice device2 = TestUtils.getTestDevice(mAdapter, 2);
        // Enabling updates from first device should trigger subscription
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_IGNORE_SCREEN_OFF);
        verifyNoMoreInteractions(mTelephonyManager);
        // Enabling updates from second device should not trigger the same subscription
        mHeadsetPhoneState.listenForPhoneState(device2, PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        verifyNoMoreInteractions(mTelephonyManager);
        // Disabling updates from first device should not cancel subscription
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_NONE);
        verifyNoMoreInteractions(mTelephonyManager);
        // Disabling updates from second device should cancel subscription
        mHeadsetPhoneState.listenForPhoneState(device2, PhoneStateListener.LISTEN_NONE);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_NONE));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_NORMAL);
        verifyNoMoreInteractions(mTelephonyManager);
    }

    /**
     * Verity that two device each partially subscribe to an indicator result in subscription to
     * both indicators. Also unsubscribing from one indicator from one device will not cause
     * unrelated indicator from other device from being unsubscribed.
     */
    @Test
    public void testListenForPhoneState_MultiDevice_PartialUpPartialDown() {
        BluetoothDevice device1 = TestUtils.getTestDevice(mAdapter, 1);
        BluetoothDevice device2 = TestUtils.getTestDevice(mAdapter, 2);
        // Partially enabling updates from first device should trigger partial subscription
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_SERVICE_STATE);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE));
        verifyNoMoreInteractions(mTelephonyManager);
        // Partially enabling updates from second device should trigger partial subscription
        mHeadsetPhoneState.listenForPhoneState(device2, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_NONE));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_NORMAL);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SERVICE_STATE
                | PhoneStateListener.LISTEN_SIGNAL_STRENGTHS));
        verify(mTelephonyManager).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_IGNORE_SCREEN_OFF);
        verifyNoMoreInteractions(mTelephonyManager);
        // Partially disabling updates from first device should not cancel all subscription
        mHeadsetPhoneState.listenForPhoneState(device1, PhoneStateListener.LISTEN_NONE);
        verify(mTelephonyManager, times(2)).listen(any(), eq(PhoneStateListener.LISTEN_NONE));
        verify(mTelephonyManager, times(2)).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_NORMAL);
        verify(mTelephonyManager).listen(any(), eq(PhoneStateListener.LISTEN_SIGNAL_STRENGTHS));
        verify(mTelephonyManager, times(2)).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_IGNORE_SCREEN_OFF);
        verifyNoMoreInteractions(mTelephonyManager);
        // Partially disabling updates from second device should cancel subscription
        mHeadsetPhoneState.listenForPhoneState(device2, PhoneStateListener.LISTEN_NONE);
        verify(mTelephonyManager, times(3)).listen(any(), eq(PhoneStateListener.LISTEN_NONE));
        verify(mTelephonyManager, times(3)).setRadioIndicationUpdateMode(
                TelephonyManager.INDICATION_FILTER_SIGNAL_STRENGTH,
                TelephonyManager.INDICATION_UPDATE_MODE_NORMAL);
        verifyNoMoreInteractions(mTelephonyManager);
    }
}
