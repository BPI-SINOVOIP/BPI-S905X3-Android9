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
 * limitations under the License.
 */
package com.android.server.wifi;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Message;
import android.os.Messenger;
import android.os.test.TestLooper;
import android.support.test.filters.SmallTest;

/**
 * Unit tests for {@link com.android.server.wifi.WifiTrafficPoller}.
 */
@SmallTest
public class WifiTrafficPollerTest {
    public static final String TAG = "WifiTrafficPollerTest";

    private TestLooper mLooper;
    private Handler mHandler;
    private WifiTrafficPoller mWifiTrafficPoller;
    private BroadcastReceiver mReceiver;
    private Intent mIntent;
    private Messenger mMessenger;
    private final static String IFNAME = "wlan0";
    private final static long DEFAULT_PACKET_COUNT = 10;
    private final static long TX_PACKET_COUNT = 40;
    private final static long RX_PACKET_COUNT = 50;

    final ArgumentCaptor<Message> mMessageCaptor = ArgumentCaptor.forClass(Message.class);
    final ArgumentCaptor<BroadcastReceiver> mBroadcastReceiverCaptor =
            ArgumentCaptor.forClass(BroadcastReceiver.class);

    @Mock Context mContext;
    @Mock WifiNative mWifiNative;
    @Mock NetworkInfo mNetworkInfo;

    /**
     * Called before each test
     */
    @Before
    public void setUp() throws Exception {
        // Ensure looper exists
        mLooper = new TestLooper();
        mHandler = spy(new Handler(mLooper.getLooper()));
        mMessenger = new Messenger(mHandler);
        MockitoAnnotations.initMocks(this);

        when(mWifiNative.getTxPackets(any(String.class))).thenReturn(DEFAULT_PACKET_COUNT,
                TX_PACKET_COUNT);
        when(mWifiNative.getRxPackets(any(String.class))).thenReturn(DEFAULT_PACKET_COUNT,
                RX_PACKET_COUNT);
        when(mWifiNative.getClientInterfaceName()).thenReturn(IFNAME);

        mWifiTrafficPoller = new WifiTrafficPoller(mContext, mLooper.getLooper(), mWifiNative);
        // Verify the constructor registers broadcast receiver with the collect intent filters.
        verify(mContext).registerReceiver(mBroadcastReceiverCaptor.capture(), argThat(
                intentFilter -> intentFilter.hasAction(WifiManager.NETWORK_STATE_CHANGED_ACTION) &&
                        intentFilter.hasAction(Intent.ACTION_SCREEN_ON) &&
                        intentFilter.hasAction(Intent.ACTION_SCREEN_OFF)));
        mReceiver = mBroadcastReceiverCaptor.getValue();

        // For the fist call, this is required to set the DEFAULT_PACKET_COUNT to mTxPkts and
        // mRxPkts in WifiTrafficPoll Object.
        triggerForUpdatedInformationOfData(Intent.ACTION_SCREEN_ON,
                NetworkInfo.DetailedState.CONNECTED);
    }

    private void registerClient() {
        // Register Client to verify that Tx/RX packet message is properly received.
        mWifiTrafficPoller.addClient(mMessenger);
        mLooper.dispatchAll();
    }

    private void triggerForUpdatedInformationOfData(String actionScreen,
            NetworkInfo.DetailedState networkState) {
        when(mNetworkInfo.getDetailedState()).thenReturn(NetworkInfo.DetailedState.DISCONNECTED);
        mIntent = new Intent(actionScreen);
        mReceiver.onReceive(mContext, mIntent);
        mLooper.dispatchAll();

        when(mNetworkInfo.getDetailedState()).thenReturn(networkState);
        mIntent = new Intent(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        mIntent.putExtra(WifiManager.EXTRA_NETWORK_INFO, mNetworkInfo);
        mReceiver.onReceive(mContext, mIntent);
        mLooper.dispatchAll();
    }

    /**
     * Verify that StartTrafficStatsPolling should not happen in case a network is not connected
     */
    @Test
    public void testNotStartTrafficStatsPollingWithDisconnected() {
        registerClient();
        triggerForUpdatedInformationOfData(Intent.ACTION_SCREEN_ON,
                NetworkInfo.DetailedState.DISCONNECTED);

        // Client should not get any message when the network is disconnected
        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that StartTrafficStatsPolling should happen in case screen is on and rx/tx packets are
     * available.
     */
    @Test
    public void testStartTrafficStatsPollingWithScreenOn() {
        registerClient();
        triggerForUpdatedInformationOfData(Intent.ACTION_SCREEN_ON,
                NetworkInfo.DetailedState.CONNECTED);

        // Client should get the DATA_ACTIVITY_NOTIFICATION
        verify(mHandler).handleMessage(mMessageCaptor.capture());
        assertEquals(WifiManager.DATA_ACTIVITY_NOTIFICATION, mMessageCaptor.getValue().what);
    }

    /**
     * Verify that StartTrafficStatsPolling should not happen in case screen is off.
     */
    @Test
    public void testNotStartTrafficStatsPollingWithScreenOff() {
        registerClient();
        triggerForUpdatedInformationOfData(Intent.ACTION_SCREEN_OFF,
                NetworkInfo.DetailedState.CONNECTED);

        verify(mNetworkInfo, atLeastOnce()).getDetailedState();
        mLooper.dispatchAll();

        // Client should not get any message when the screen is off
        verify(mHandler, never()).handleMessage(any(Message.class));
    }

    /**
     * Verify that remove client message should be handled
     */
    @Test
    public void testRemoveClient() {
        registerClient();
        mWifiTrafficPoller.removeClient(mMessenger);
        mLooper.dispatchAll();

        triggerForUpdatedInformationOfData(Intent.ACTION_SCREEN_ON,
                NetworkInfo.DetailedState.CONNECTED);

        // Client should not get any message after the client is removed.
        verify(mHandler, never()).handleMessage(any(Message.class));
    }
}
