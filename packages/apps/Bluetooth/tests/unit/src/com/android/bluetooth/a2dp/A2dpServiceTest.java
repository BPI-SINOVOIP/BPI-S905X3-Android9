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

package com.android.bluetooth.a2dp;

import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothCodecConfig;
import android.bluetooth.BluetoothCodecStatus;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Looper;
import android.os.ParcelUuid;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ServiceTestRule;
import android.support.test.runner.AndroidJUnit4;

import com.android.bluetooth.R;
import com.android.bluetooth.TestUtils;
import com.android.bluetooth.btservice.AdapterService;

import org.junit.After;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeoutException;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class A2dpServiceTest {
    private static final int MAX_CONNECTED_AUDIO_DEVICES = 5;

    private BluetoothAdapter mAdapter;
    private Context mTargetContext;
    private A2dpService mA2dpService;
    private BluetoothDevice mTestDevice;
    private static final int TIMEOUT_MS = 1000;    // 1s

    private BroadcastReceiver mA2dpIntentReceiver;
    private final BlockingQueue<Intent> mConnectionStateChangedQueue = new LinkedBlockingQueue<>();
    private final BlockingQueue<Intent> mAudioStateChangedQueue = new LinkedBlockingQueue<>();
    private final BlockingQueue<Intent> mCodecConfigChangedQueue = new LinkedBlockingQueue<>();

    @Mock private AdapterService mAdapterService;
    @Mock private A2dpNativeInterface mA2dpNativeInterface;

    @Rule public final ServiceTestRule mServiceRule = new ServiceTestRule();

    @Before
    public void setUp() throws Exception {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        Assume.assumeTrue("Ignore test when A2dpService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_a2dp));
        // Set up mocks and test assets
        MockitoAnnotations.initMocks(this);

        if (Looper.myLooper() == null) {
            Looper.prepare();
        }

        TestUtils.setAdapterService(mAdapterService);
        doReturn(MAX_CONNECTED_AUDIO_DEVICES).when(mAdapterService).getMaxConnectedAudioDevices();
        doReturn(false).when(mAdapterService).isQuietModeEnabled();

        mAdapter = BluetoothAdapter.getDefaultAdapter();

        startService();
        mA2dpService.mA2dpNativeInterface = mA2dpNativeInterface;

        // Override the timeout value to speed up the test
        A2dpStateMachine.sConnectTimeoutMs = TIMEOUT_MS;    // 1s

        // Set up the Connection State Changed receiver
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothA2dp.ACTION_PLAYING_STATE_CHANGED);
        filter.addAction(BluetoothA2dp.ACTION_CODEC_CONFIG_CHANGED);
        mA2dpIntentReceiver = new A2dpIntentReceiver();
        mTargetContext.registerReceiver(mA2dpIntentReceiver, filter);

        // Get a device for testing
        mTestDevice = mAdapter.getRemoteDevice("00:01:02:03:04:05");
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_UNDEFINED);
        doReturn(BluetoothDevice.BOND_BONDED).when(mAdapterService)
                .getBondState(any(BluetoothDevice.class));
        doReturn(new ParcelUuid[]{BluetoothUuid.AudioSink}).when(mAdapterService)
                .getRemoteUuids(any(BluetoothDevice.class));
    }

    @After
    public void tearDown() throws Exception {
        if (!mTargetContext.getResources().getBoolean(R.bool.profile_supported_a2dp)) {
            return;
        }
        stopService();
        mTargetContext.unregisterReceiver(mA2dpIntentReceiver);
        mConnectionStateChangedQueue.clear();
        mAudioStateChangedQueue.clear();
        mCodecConfigChangedQueue.clear();
        TestUtils.clearAdapterService(mAdapterService);
    }

    private void startService() throws TimeoutException {
        TestUtils.startService(mServiceRule, A2dpService.class);
        mA2dpService = A2dpService.getA2dpService();
        Assert.assertNotNull(mA2dpService);
    }

    private void stopService() throws TimeoutException {
        TestUtils.stopService(mServiceRule, A2dpService.class);
        mA2dpService = A2dpService.getA2dpService();
        Assert.assertNull(mA2dpService);
    }

    private class A2dpIntentReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED.equals(intent.getAction())) {
                try {
                    mConnectionStateChangedQueue.put(intent);
                } catch (InterruptedException e) {
                    Assert.fail("Cannot add Intent to the Connection State queue: "
                                + e.getMessage());
                }
            }
            if (BluetoothA2dp.ACTION_PLAYING_STATE_CHANGED.equals(intent.getAction())) {
                try {
                    mAudioStateChangedQueue.put(intent);
                } catch (InterruptedException e) {
                    Assert.fail("Cannot add Intent to the Audio State queue: " + e.getMessage());
                }
            }
            if (BluetoothA2dp.ACTION_CODEC_CONFIG_CHANGED.equals(intent.getAction())) {
                try {
                    mCodecConfigChangedQueue.put(intent);
                } catch (InterruptedException e) {
                    Assert.fail("Cannot add Intent to the Codec Config queue: " + e.getMessage());
                }
            }
        }
    }

    private void verifyConnectionStateIntent(int timeoutMs, BluetoothDevice device,
                                             int newState, int prevState) {
        Intent intent = TestUtils.waitForIntent(timeoutMs, mConnectionStateChangedQueue);
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothA2dp.ACTION_CONNECTION_STATE_CHANGED,
                            intent.getAction());
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        Assert.assertEquals(newState, intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1));
        Assert.assertEquals(prevState, intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE,
                                                          -1));
    }

    private void verifyNoConnectionStateIntent(int timeoutMs) {
        Intent intent = TestUtils.waitForNoIntent(timeoutMs, mConnectionStateChangedQueue);
        Assert.assertNull(intent);
    }

    private void verifyAudioStateIntent(int timeoutMs, BluetoothDevice device,
                                             int newState, int prevState) {
        Intent intent = TestUtils.waitForIntent(timeoutMs, mAudioStateChangedQueue);
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothA2dp.ACTION_PLAYING_STATE_CHANGED, intent.getAction());
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        Assert.assertEquals(newState, intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1));
        Assert.assertEquals(prevState, intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE,
                                                          -1));
    }

    private void verifyNoAudioStateIntent(int timeoutMs) {
        Intent intent = TestUtils.waitForNoIntent(timeoutMs, mAudioStateChangedQueue);
        Assert.assertNull(intent);
    }

    private void verifyCodecConfigIntent(int timeoutMs, BluetoothDevice device,
                                         BluetoothCodecStatus codecStatus) {
        Intent intent = TestUtils.waitForIntent(timeoutMs, mCodecConfigChangedQueue);
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothA2dp.ACTION_CODEC_CONFIG_CHANGED, intent.getAction());
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        Assert.assertEquals(codecStatus,
                            intent.getParcelableExtra(BluetoothCodecStatus.EXTRA_CODEC_STATUS));
    }

    private void verifyNoCodecConfigIntent(int timeoutMs) {
        Intent intent = TestUtils.waitForNoIntent(timeoutMs, mCodecConfigChangedQueue);
        Assert.assertNull(intent);
    }

    /**
     * Test getting A2DP Service: getA2dpService()
     */
    @Test
    public void testGetA2dpService() {
        Assert.assertEquals(mA2dpService, A2dpService.getA2dpService());
    }

    /**
     * Test stop A2DP Service
     */
    @Test
    public void testStopA2dpService() {
        // Prepare: connect and set active device
        doReturn(true).when(mA2dpNativeInterface).setActiveDevice(any(BluetoothDevice.class));
        connectDevice(mTestDevice);
        Assert.assertTrue(mA2dpService.setActiveDevice(mTestDevice));
        verify(mA2dpNativeInterface).setActiveDevice(mTestDevice);
        // A2DP Service is already running: test stop(). Note: must be done on the main thread.
        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                Assert.assertTrue(mA2dpService.stop());
            }
        });
        // Verify that setActiveDevice(null) was called during shutdown
        verify(mA2dpNativeInterface).setActiveDevice(null);
        // Try to restart the service. Note: must be done on the main thread.
        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            public void run() {
                Assert.assertTrue(mA2dpService.start());
            }
        });
    }

    /**
     * Test get/set priority for BluetoothDevice
     */
    @Test
    public void testGetSetPriority() {
        Assert.assertEquals("Initial device priority",
                            BluetoothProfile.PRIORITY_UNDEFINED,
                            mA2dpService.getPriority(mTestDevice));

        Assert.assertTrue(mA2dpService.setPriority(mTestDevice,  BluetoothProfile.PRIORITY_OFF));
        Assert.assertEquals("Setting device priority to PRIORITY_OFF",
                            BluetoothProfile.PRIORITY_OFF,
                            mA2dpService.getPriority(mTestDevice));

        Assert.assertTrue(mA2dpService.setPriority(mTestDevice,  BluetoothProfile.PRIORITY_ON));
        Assert.assertEquals("Setting device priority to PRIORITY_ON",
                            BluetoothProfile.PRIORITY_ON,
                            mA2dpService.getPriority(mTestDevice));

        Assert.assertTrue(mA2dpService.setPriority(mTestDevice,
                                                   BluetoothProfile.PRIORITY_AUTO_CONNECT));
        Assert.assertEquals("Setting device priority to PRIORITY_AUTO_CONNECT",
                            BluetoothProfile.PRIORITY_AUTO_CONNECT,
                            mA2dpService.getPriority(mTestDevice));
    }

    /**
     *  Test okToConnect method using various test cases
     */
    @Test
    public void testOkToConnect() {
        int badPriorityValue = 1024;
        int badBondState = 42;
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_NONE, BluetoothProfile.PRIORITY_UNDEFINED, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_NONE, BluetoothProfile.PRIORITY_OFF, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_NONE, BluetoothProfile.PRIORITY_ON, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_NONE, BluetoothProfile.PRIORITY_AUTO_CONNECT, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_NONE, badPriorityValue, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDING, BluetoothProfile.PRIORITY_UNDEFINED, true);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDING, BluetoothProfile.PRIORITY_OFF, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDING, BluetoothProfile.PRIORITY_ON, true);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDING, BluetoothProfile.PRIORITY_AUTO_CONNECT, true);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDING, badPriorityValue, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDED, BluetoothProfile.PRIORITY_UNDEFINED, true);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDED, BluetoothProfile.PRIORITY_OFF, false);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDED, BluetoothProfile.PRIORITY_ON, true);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDED, BluetoothProfile.PRIORITY_AUTO_CONNECT, true);
        testOkToConnectCase(mTestDevice,
                BluetoothDevice.BOND_BONDED, badPriorityValue, false);
        testOkToConnectCase(mTestDevice,
                badBondState, BluetoothProfile.PRIORITY_UNDEFINED, false);
        testOkToConnectCase(mTestDevice,
                badBondState, BluetoothProfile.PRIORITY_OFF, false);
        testOkToConnectCase(mTestDevice,
                badBondState, BluetoothProfile.PRIORITY_ON, false);
        testOkToConnectCase(mTestDevice,
                badBondState, BluetoothProfile.PRIORITY_AUTO_CONNECT, false);
        testOkToConnectCase(mTestDevice,
                badBondState, badPriorityValue, false);
        // Restore prirority to undefined for this test device
        Assert.assertTrue(mA2dpService.setPriority(
                mTestDevice, BluetoothProfile.PRIORITY_UNDEFINED));
    }


    /**
     * Test that an outgoing connection to device that does not have A2DP Sink UUID is rejected
     */
    @Test
    public void testOutgoingConnectMissingAudioSinkUuid() {
        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // Return AudioSource UUID instead of AudioSink
        doReturn(new ParcelUuid[]{BluetoothUuid.AudioSource}).when(mAdapterService)
                .getRemoteUuids(any(BluetoothDevice.class));

        // Send a connect request
        Assert.assertFalse("Connect expected to fail", mA2dpService.connect(mTestDevice));
    }

    /**
     * Test that an outgoing connection to device with PRIORITY_OFF is rejected
     */
    @Test
    public void testOutgoingConnectPriorityOff() {
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // Set the device priority to PRIORITY_OFF so connect() should fail
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_OFF);

        // Send a connect request
        Assert.assertFalse("Connect expected to fail", mA2dpService.connect(mTestDevice));
    }

    /**
     * Test that an outgoing connection times out
     */
    @Test
    public void testOutgoingConnectTimeout() {
        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // Send a connect request
        Assert.assertTrue("Connect failed", mA2dpService.connect(mTestDevice));

        // Verify the connection state broadcast, and that we are in Connecting state
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_CONNECTING,
                                    BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));

        // Verify the connection state broadcast, and that we are in Disconnected state
        verifyConnectionStateIntent(A2dpStateMachine.sConnectTimeoutMs * 2,
                                    mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                    BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
    }

    /**
     * Test that an outgoing connection/disconnection succeeds
     */
    @Test
    public void testOutgoingConnectDisconnectSuccess() {
        A2dpStackEvent connCompletedEvent;

        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // Send a connect request
        Assert.assertTrue("Connect failed", mA2dpService.connect(mTestDevice));

        // Verify the connection state broadcast, and that we are in Connecting state
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_CONNECTING,
                                    BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));

        // Send a message to trigger connection completed
        connCompletedEvent = new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        connCompletedEvent.device = mTestDevice;
        connCompletedEvent.valueInt = A2dpStackEvent.CONNECTION_STATE_CONNECTED;
        mA2dpService.messageFromNative(connCompletedEvent);

        // Verify the connection state broadcast, and that we are in Connected state
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_CONNECTED,
                                    BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));

        // Verify the list of connected devices
        Assert.assertTrue(mA2dpService.getConnectedDevices().contains(mTestDevice));

        // Send a disconnect request
        Assert.assertTrue("Disconnect failed", mA2dpService.disconnect(mTestDevice));

        // Verify the connection state broadcast, and that we are in Disconnecting state
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_DISCONNECTING,
                                    BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));

        // Send a message to trigger disconnection completed
        connCompletedEvent = new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        connCompletedEvent.device = mTestDevice;
        connCompletedEvent.valueInt = A2dpStackEvent.CONNECTION_STATE_DISCONNECTED;
        mA2dpService.messageFromNative(connCompletedEvent);

        // Verify the connection state broadcast, and that we are in Disconnected state
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                    BluetoothProfile.STATE_DISCONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));

        // Verify the list of connected devices
        Assert.assertFalse(mA2dpService.getConnectedDevices().contains(mTestDevice));
    }

    /**
     * Test that an outgoing connection/disconnection succeeds
     */
    @Test
    public void testMaxConnectDevices() {
        A2dpStackEvent connCompletedEvent;
        BluetoothDevice[] testDevices = new BluetoothDevice[MAX_CONNECTED_AUDIO_DEVICES];
        BluetoothDevice extraTestDevice;

        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // Prepare and connect all test devices
        for (int i = 0; i < MAX_CONNECTED_AUDIO_DEVICES; i++) {
            BluetoothDevice testDevice = TestUtils.getTestDevice(mAdapter, i);
            testDevices[i] = testDevice;
            mA2dpService.setPriority(testDevice, BluetoothProfile.PRIORITY_ON);
            // Send a connect request
            Assert.assertTrue("Connect failed", mA2dpService.connect(testDevice));
            // Verify the connection state broadcast, and that we are in Connecting state
            verifyConnectionStateIntent(TIMEOUT_MS, testDevice, BluetoothProfile.STATE_CONNECTING,
                                        BluetoothProfile.STATE_DISCONNECTED);
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                                mA2dpService.getConnectionState(testDevice));
            // Send a message to trigger connection completed
            connCompletedEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
            connCompletedEvent.device = testDevice;
            connCompletedEvent.valueInt = A2dpStackEvent.CONNECTION_STATE_CONNECTED;
            mA2dpService.messageFromNative(connCompletedEvent);

            // Verify the connection state broadcast, and that we are in Connected state
            verifyConnectionStateIntent(TIMEOUT_MS, testDevice, BluetoothProfile.STATE_CONNECTED,
                                        BluetoothProfile.STATE_CONNECTING);
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                                mA2dpService.getConnectionState(testDevice));
            // Verify the list of connected devices
            Assert.assertTrue(mA2dpService.getConnectedDevices().contains(testDevice));
        }

        // Prepare and connect the extra test device. The connect request should fail
        extraTestDevice = TestUtils.getTestDevice(mAdapter, MAX_CONNECTED_AUDIO_DEVICES);
        mA2dpService.setPriority(extraTestDevice, BluetoothProfile.PRIORITY_ON);
        // Send a connect request
        Assert.assertFalse("Connect expected to fail", mA2dpService.connect(extraTestDevice));
    }

    /**
     * Test that only CONNECTION_STATE_CONNECTED or CONNECTION_STATE_CONNECTING A2DP stack events
     * will create a state machine.
     */
    @Test
    public void testCreateStateMachineStackEvents() {
        A2dpStackEvent stackEvent;

        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // A2DP stack event: CONNECTION_STATE_CONNECTING - state machine should be created
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTING,
                                            BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine should be removed
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                            BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_CONNECTED - state machine should be created
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTED,
                                            BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine should be removed
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                            BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTING - state machine should not be created
        generateUnexpectedConnectionMessageFromNative(mTestDevice,
                                                      BluetoothProfile.STATE_DISCONNECTING,
                                                      BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine should not be created
        generateUnexpectedConnectionMessageFromNative(mTestDevice,
                                                      BluetoothProfile.STATE_DISCONNECTED,
                                                      BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));
    }

    /**
     * Test that EVENT_TYPE_AUDIO_STATE_CHANGED and EVENT_TYPE_CODEC_CONFIG_CHANGED events
     * are processed.
     */
    @Test
    public void testProcessAudioStateChangedCodecConfigChangedEvents() {
        A2dpStackEvent stackEvent;
        BluetoothCodecConfig codecConfigSbc =
                new BluetoothCodecConfig(
                        BluetoothCodecConfig.SOURCE_CODEC_TYPE_SBC,
                        BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT,
                        BluetoothCodecConfig.SAMPLE_RATE_44100,
                        BluetoothCodecConfig.BITS_PER_SAMPLE_16,
                        BluetoothCodecConfig.CHANNEL_MODE_STEREO,
                        0, 0, 0, 0);       // Codec-specific fields
        BluetoothCodecConfig codecConfig = codecConfigSbc;
        BluetoothCodecConfig[] codecsLocalCapabilities = new BluetoothCodecConfig[1];
        BluetoothCodecConfig[] codecsSelectableCapabilities = new BluetoothCodecConfig[1];
        codecsLocalCapabilities[0] = codecConfigSbc;
        codecsSelectableCapabilities[0] = codecConfigSbc;
        BluetoothCodecStatus codecStatus = new BluetoothCodecStatus(codecConfig,
                                                                    codecsLocalCapabilities,
                                                                    codecsSelectableCapabilities);

        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // A2DP stack event: EVENT_TYPE_AUDIO_STATE_CHANGED - state machine should not be created
        generateUnexpectedAudioMessageFromNative(mTestDevice, A2dpStackEvent.AUDIO_STATE_STARTED,
                                                 BluetoothA2dp.STATE_PLAYING,
                                                 BluetoothA2dp.STATE_NOT_PLAYING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: EVENT_TYPE_CODEC_CONFIG_CHANGED - state machine should not be created
        generateUnexpectedCodecMessageFromNative(mTestDevice, codecStatus);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_CONNECTED - state machine should be created
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTED,
                                            BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: EVENT_TYPE_AUDIO_STATE_CHANGED - Intent broadcast should be generated
        // NOTE: The first message (STATE_PLAYING -> STATE_NOT_PLAYING) is generated internally
        // by the state machine when Connected, and needs to be extracted first before generating
        // the actual message from native.
        verifyAudioStateIntent(TIMEOUT_MS, mTestDevice, BluetoothA2dp.STATE_NOT_PLAYING,
                               BluetoothA2dp.STATE_PLAYING);
        generateAudioMessageFromNative(mTestDevice,
                                       A2dpStackEvent.AUDIO_STATE_STARTED,
                                       BluetoothA2dp.STATE_PLAYING,
                                       BluetoothA2dp.STATE_NOT_PLAYING);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: EVENT_TYPE_CODEC_CONFIG_CHANGED - Intent broadcast should be generated
        generateCodecMessageFromNative(mTestDevice, codecStatus);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine should be removed
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                            BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));
    }

    /**
     * Test that a state machine in DISCONNECTED state is removed only after the device is unbond.
     */
    @Test
    public void testDeleteStateMachineUnbondEvents() {
        A2dpStackEvent stackEvent;

        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // A2DP stack event: CONNECTION_STATE_CONNECTING - state machine should be created
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTING,
                                            BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        // Device unbond - state machine is not removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_CONNECTED - state machine is not removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_BONDED);
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTED,
                                            BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        // Device unbond - state machine is not removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTING - state machine is not removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_BONDED);
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTING,
                                            BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        // Device unbond - state machine is not removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine is not removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_BONDED);
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                            BluetoothProfile.STATE_DISCONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));
        // Device unbond - state machine is removed
        mA2dpService.bondStateChanged(mTestDevice, BluetoothDevice.BOND_NONE);
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));
    }

    /**
     * Test that a CONNECTION_STATE_DISCONNECTED A2DP stack event will remove the state machine
     * only if the device is unbond.
     */
    @Test
    public void testDeleteStateMachineDisconnectEvents() {
        A2dpStackEvent stackEvent;

        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(mTestDevice, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(any(BluetoothDevice.class));
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(any(BluetoothDevice.class));

        // A2DP stack event: CONNECTION_STATE_CONNECTING - state machine should be created
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTING,
                                            BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine is not removed
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                            BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_CONNECTING - state machine remains
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_CONNECTING,
                                            BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // Device bond state marked as unbond - state machine is not removed
        doReturn(BluetoothDevice.BOND_NONE).when(mAdapterService)
                .getBondState(any(BluetoothDevice.class));
        Assert.assertTrue(mA2dpService.getDevices().contains(mTestDevice));

        // A2DP stack event: CONNECTION_STATE_DISCONNECTED - state machine is removed
        generateConnectionMessageFromNative(mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                                            BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                            mA2dpService.getConnectionState(mTestDevice));
        Assert.assertFalse(mA2dpService.getDevices().contains(mTestDevice));
    }

    private void connectDevice(BluetoothDevice device) {
        A2dpStackEvent connCompletedEvent;

        List<BluetoothDevice> prevConnectedDevices = mA2dpService.getConnectedDevices();

        // Update the device priority so okToConnect() returns true
        mA2dpService.setPriority(device, BluetoothProfile.PRIORITY_ON);
        doReturn(true).when(mA2dpNativeInterface).connectA2dp(device);
        doReturn(true).when(mA2dpNativeInterface).disconnectA2dp(device);

        // Send a connect request
        Assert.assertTrue("Connect failed", mA2dpService.connect(device));

        // Verify the connection state broadcast, and that we are in Connecting state
        verifyConnectionStateIntent(TIMEOUT_MS, device, BluetoothProfile.STATE_CONNECTING,
                                    BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                            mA2dpService.getConnectionState(device));

        // Send a message to trigger connection completed
        connCompletedEvent = new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        connCompletedEvent.device = device;
        connCompletedEvent.valueInt = A2dpStackEvent.CONNECTION_STATE_CONNECTED;
        mA2dpService.messageFromNative(connCompletedEvent);

        // Verify the connection state broadcast, and that we are in Connected state
        verifyConnectionStateIntent(TIMEOUT_MS, device, BluetoothProfile.STATE_CONNECTED,
                                    BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                            mA2dpService.getConnectionState(device));

        // Verify that the device is in the list of connected devices
        Assert.assertTrue(mA2dpService.getConnectedDevices().contains(device));
        // Verify the list of previously connected devices
        for (BluetoothDevice prevDevice : prevConnectedDevices) {
            Assert.assertTrue(mA2dpService.getConnectedDevices().contains(prevDevice));
        }
    }

    private void generateConnectionMessageFromNative(BluetoothDevice device, int newConnectionState,
                                                     int oldConnectionState) {
        A2dpStackEvent stackEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        stackEvent.device = device;
        stackEvent.valueInt = newConnectionState;
        mA2dpService.messageFromNative(stackEvent);
        // Verify the connection state broadcast
        verifyConnectionStateIntent(TIMEOUT_MS, device, newConnectionState, oldConnectionState);
    }

    private void generateUnexpectedConnectionMessageFromNative(BluetoothDevice device,
                                                               int newConnectionState,
                                                               int oldConnectionState) {
        A2dpStackEvent stackEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED);
        stackEvent.device = device;
        stackEvent.valueInt = newConnectionState;
        mA2dpService.messageFromNative(stackEvent);
        // Verify the connection state broadcast
        verifyNoConnectionStateIntent(TIMEOUT_MS);
    }

    private void generateAudioMessageFromNative(BluetoothDevice device, int audioStackEvent,
                                                int newAudioState, int oldAudioState) {
        A2dpStackEvent stackEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_AUDIO_STATE_CHANGED);
        stackEvent.device = device;
        stackEvent.valueInt = audioStackEvent;
        mA2dpService.messageFromNative(stackEvent);
        // Verify the audio state broadcast
        verifyAudioStateIntent(TIMEOUT_MS, device, newAudioState, oldAudioState);
    }

    private void generateUnexpectedAudioMessageFromNative(BluetoothDevice device,
                                                          int audioStackEvent, int newAudioState,
                                                          int oldAudioState) {
        A2dpStackEvent stackEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_AUDIO_STATE_CHANGED);
        stackEvent.device = device;
        stackEvent.valueInt = audioStackEvent;
        mA2dpService.messageFromNative(stackEvent);
        // Verify the audio state broadcast
        verifyNoAudioStateIntent(TIMEOUT_MS);
    }

    private void generateCodecMessageFromNative(BluetoothDevice device,
                                                BluetoothCodecStatus codecStatus) {
        A2dpStackEvent stackEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CODEC_CONFIG_CHANGED);
        stackEvent.device = device;
        stackEvent.codecStatus = codecStatus;
        mA2dpService.messageFromNative(stackEvent);
        // Verify the codec status broadcast
        verifyCodecConfigIntent(TIMEOUT_MS, device, codecStatus);
    }

    private void generateUnexpectedCodecMessageFromNative(BluetoothDevice device,
                                                          BluetoothCodecStatus codecStatus) {
        A2dpStackEvent stackEvent =
                new A2dpStackEvent(A2dpStackEvent.EVENT_TYPE_CODEC_CONFIG_CHANGED);
        stackEvent.device = device;
        stackEvent.codecStatus = codecStatus;
        mA2dpService.messageFromNative(stackEvent);
        // Verify the codec status broadcast
        verifyNoCodecConfigIntent(TIMEOUT_MS);
    }

    /**
     * Helper function to test okToConnect() method.
     *
     * @param device test device
     * @param bondState bond state value, could be invalid
     * @param priority value, could be invalid, coudl be invalid
     * @param expected expected result from okToConnect()
     */
    private void testOkToConnectCase(BluetoothDevice device, int bondState, int priority,
            boolean expected) {
        doReturn(bondState).when(mAdapterService).getBondState(device);
        Assert.assertTrue(mA2dpService.setPriority(device, priority));

        // Test when the AdapterService is in non-quiet mode: the result should not depend
        // on whether the connection request is outgoing or incoming.
        doReturn(false).when(mAdapterService).isQuietModeEnabled();
        Assert.assertEquals(expected, mA2dpService.okToConnect(device, true));  // Outgoing
        Assert.assertEquals(expected, mA2dpService.okToConnect(device, false)); // Incoming

        // Test when the AdapterService is in quiet mode: the result should always be
        // false when the connection request is incoming.
        doReturn(true).when(mAdapterService).isQuietModeEnabled();
        Assert.assertEquals(expected, mA2dpService.okToConnect(device, true));  // Outgoing
        Assert.assertEquals(false, mA2dpService.okToConnect(device, false)); // Incoming
    }
}
