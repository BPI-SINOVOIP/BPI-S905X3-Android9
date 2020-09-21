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

import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.IBluetoothHeadset;
import android.content.Context;
import android.media.AudioManager;
import android.os.ParcelUuid;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ServiceTestRule;
import android.support.test.runner.AndroidJUnit4;

import com.android.bluetooth.R;
import com.android.bluetooth.TestUtils;
import com.android.bluetooth.btservice.AdapterService;

import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Set;

/**
 * Tests for {@link HeadsetService}
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class HeadsetServiceTest {
    private static final int MAX_HEADSET_CONNECTIONS = 5;
    private static final ParcelUuid[] FAKE_HEADSET_UUID = {BluetoothUuid.Handsfree};
    private static final int ASYNC_CALL_TIMEOUT_MILLIS = 250;
    private static final String TEST_PHONE_NUMBER = "1234567890";

    private Context mTargetContext;
    private HeadsetService mHeadsetService;
    private IBluetoothHeadset.Stub mHeadsetServiceBinder;
    private BluetoothAdapter mAdapter;
    private HeadsetNativeInterface mNativeInterface;
    private BluetoothDevice mCurrentDevice;
    private final HashMap<BluetoothDevice, HeadsetStateMachine> mStateMachines = new HashMap<>();

    @Rule public final ServiceTestRule mServiceRule = new ServiceTestRule();

    @Spy private HeadsetObjectsFactory mObjectsFactory = HeadsetObjectsFactory.getInstance();
    @Mock private AdapterService mAdapterService;
    @Mock private HeadsetSystemInterface mSystemInterface;
    @Mock private AudioManager mAudioManager;
    @Mock private HeadsetPhoneState mPhoneState;

    @Before
    public void setUp() throws Exception {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        Assume.assumeTrue("Ignore test when HeadsetService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_hs_hfp));
        MockitoAnnotations.initMocks(this);
        TestUtils.setAdapterService(mAdapterService);
        // We cannot mock HeadsetObjectsFactory.getInstance() with Mockito.
        // Hence we need to use reflection to call a private method to
        // initialize properly the HeadsetObjectsFactory.sInstance field.
        Method method = HeadsetObjectsFactory.class.getDeclaredMethod("setInstanceForTesting",
                HeadsetObjectsFactory.class);
        method.setAccessible(true);
        method.invoke(null, mObjectsFactory);
        doReturn(true).when(mAdapterService).isEnabled();
        doReturn(MAX_HEADSET_CONNECTIONS).when(mAdapterService).getMaxConnectedAudioDevices();
        doReturn(new ParcelUuid[]{BluetoothUuid.Handsfree}).when(mAdapterService)
                .getRemoteUuids(any(BluetoothDevice.class));
        // This line must be called to make sure relevant objects are initialized properly
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        // Mock methods in AdapterService
        doReturn(FAKE_HEADSET_UUID).when(mAdapterService)
                .getRemoteUuids(any(BluetoothDevice.class));
        doReturn(BluetoothDevice.BOND_BONDED).when(mAdapterService)
                .getBondState(any(BluetoothDevice.class));
        doAnswer(invocation -> {
            Set<BluetoothDevice> keys = mStateMachines.keySet();
            return keys.toArray(new BluetoothDevice[keys.size()]);
        }).when(mAdapterService).getBondedDevices();
        // Mock system interface
        doNothing().when(mSystemInterface).init();
        doNothing().when(mSystemInterface).stop();
        when(mSystemInterface.getHeadsetPhoneState()).thenReturn(mPhoneState);
        when(mSystemInterface.getAudioManager()).thenReturn(mAudioManager);
        when(mSystemInterface.isCallIdle()).thenReturn(true);
        // Mock methods in HeadsetNativeInterface
        mNativeInterface = spy(HeadsetNativeInterface.getInstance());
        doNothing().when(mNativeInterface).init(anyInt(), anyBoolean());
        doNothing().when(mNativeInterface).cleanup();
        doReturn(true).when(mNativeInterface).connectHfp(any(BluetoothDevice.class));
        doReturn(true).when(mNativeInterface).disconnectHfp(any(BluetoothDevice.class));
        doReturn(true).when(mNativeInterface).connectAudio(any(BluetoothDevice.class));
        doReturn(true).when(mNativeInterface).disconnectAudio(any(BluetoothDevice.class));
        doReturn(true).when(mNativeInterface).setActiveDevice(any(BluetoothDevice.class));
        doReturn(true).when(mNativeInterface).sendBsir(any(BluetoothDevice.class), anyBoolean());
        // Mock methods in HeadsetObjectsFactory
        doAnswer(invocation -> {
            Assert.assertNotNull(mCurrentDevice);
            final HeadsetStateMachine stateMachine = mock(HeadsetStateMachine.class);
            doReturn(BluetoothProfile.STATE_DISCONNECTED).when(stateMachine).getConnectionState();
            doReturn(BluetoothHeadset.STATE_AUDIO_DISCONNECTED).when(stateMachine).getAudioState();
            mStateMachines.put(mCurrentDevice, stateMachine);
            return stateMachine;
        }).when(mObjectsFactory).makeStateMachine(any(), any(), any(), any(), any(), any());
        doReturn(mSystemInterface).when(mObjectsFactory).makeSystemInterface(any());
        doReturn(mNativeInterface).when(mObjectsFactory).getNativeInterface();
        TestUtils.startService(mServiceRule, HeadsetService.class);
        mHeadsetService = HeadsetService.getHeadsetService();
        Assert.assertNotNull(mHeadsetService);
        verify(mObjectsFactory).makeSystemInterface(mHeadsetService);
        verify(mObjectsFactory).getNativeInterface();
        mHeadsetServiceBinder = (IBluetoothHeadset.Stub) mHeadsetService.initBinder();
        Assert.assertNotNull(mHeadsetServiceBinder);
        mHeadsetServiceBinder.setForceScoAudio(true);
    }

    @After
    public void tearDown() throws Exception {
        if (!mTargetContext.getResources().getBoolean(R.bool.profile_supported_hs_hfp)) {
            return;
        }
        TestUtils.stopService(mServiceRule, HeadsetService.class);
        mHeadsetService = HeadsetService.getHeadsetService();
        Assert.assertNull(mHeadsetService);
        mStateMachines.clear();
        mCurrentDevice = null;
        Method method = HeadsetObjectsFactory.class.getDeclaredMethod("setInstanceForTesting",
                HeadsetObjectsFactory.class);
        method.setAccessible(true);
        method.invoke(null, (HeadsetObjectsFactory) null);
        TestUtils.clearAdapterService(mAdapterService);
    }

    /**
     * Test to verify that HeadsetService can be successfully started
     */
    @Test
    public void testGetHeadsetService() {
        Assert.assertEquals(mHeadsetService, HeadsetService.getHeadsetService());
        // Verify default connection and audio states
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertEquals(BluetoothHeadset.STATE_AUDIO_DISCONNECTED,
                mHeadsetService.getAudioState(mCurrentDevice));
    }

    /**
     *  Test okToAcceptConnection method using various test cases
     */
    @Test
    public void testOkToAcceptConnection() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        int badPriorityValue = 1024;
        int badBondState = 42;
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_NONE,
                BluetoothProfile.PRIORITY_UNDEFINED, false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_NONE,
                BluetoothProfile.PRIORITY_OFF, false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_NONE,
                BluetoothProfile.PRIORITY_ON, false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_NONE,
                BluetoothProfile.PRIORITY_AUTO_CONNECT, false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_NONE, badPriorityValue,
                false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDING,
                BluetoothProfile.PRIORITY_UNDEFINED, true);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDING,
                BluetoothProfile.PRIORITY_OFF, false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDING,
                BluetoothProfile.PRIORITY_ON, true);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDING,
                BluetoothProfile.PRIORITY_AUTO_CONNECT, true);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDING, badPriorityValue,
                false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDED,
                BluetoothProfile.PRIORITY_UNDEFINED, true);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDED,
                BluetoothProfile.PRIORITY_OFF, false);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDED,
                BluetoothProfile.PRIORITY_ON, true);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDED,
                BluetoothProfile.PRIORITY_AUTO_CONNECT, true);
        testOkToAcceptConnectionCase(mCurrentDevice, BluetoothDevice.BOND_BONDED, badPriorityValue,
                false);
        testOkToAcceptConnectionCase(mCurrentDevice, badBondState,
                BluetoothProfile.PRIORITY_UNDEFINED, false);
        testOkToAcceptConnectionCase(mCurrentDevice, badBondState, BluetoothProfile.PRIORITY_OFF,
                false);
        testOkToAcceptConnectionCase(mCurrentDevice, badBondState, BluetoothProfile.PRIORITY_ON,
                false);
        testOkToAcceptConnectionCase(mCurrentDevice, badBondState,
                BluetoothProfile.PRIORITY_AUTO_CONNECT, false);
        testOkToAcceptConnectionCase(mCurrentDevice, badBondState, badPriorityValue, false);
        // Restore prirority to undefined for this test device
        Assert.assertTrue(
                mHeadsetService.setPriority(mCurrentDevice, BluetoothProfile.PRIORITY_UNDEFINED));
    }

    /**
     * Test to verify that {@link HeadsetService#connect(BluetoothDevice)} returns true when the
     * device was not connected and number of connected devices is less than
     * {@link #MAX_HEADSET_CONNECTIONS}
     */
    @Test
    public void testConnectDevice_connectDeviceBelowLimit() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
        verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService, mAdapterService,
                mNativeInterface, mSystemInterface);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                mHeadsetService.getConnectionState(mCurrentDevice));
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertEquals(Collections.singletonList(mCurrentDevice),
                mHeadsetService.getConnectedDevices());
        // 2nd connection attempt will fail
        Assert.assertFalse(mHeadsetService.connect(mCurrentDevice));
        // Verify makeStateMachine is only called once
        verify(mObjectsFactory).makeStateMachine(any(), any(), any(), any(), any(), any());
        // Verify CONNECT is only sent once
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(eq(HeadsetStateMachine.CONNECT),
                any());
    }

    /**
     * Test that {@link HeadsetService#messageFromNative(HeadsetStackEvent)} will send correct
     * message to the underlying state machine
     */
    @Test
    public void testMessageFromNative_deviceConnected() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        // Test connect from native
        HeadsetStackEvent connectedEvent =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED,
                        HeadsetHalConstants.CONNECTION_STATE_CONNECTED, mCurrentDevice);
        mHeadsetService.messageFromNative(connectedEvent);
        verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService, mAdapterService,
                mNativeInterface, mSystemInterface);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.STACK_EVENT,
                connectedEvent);
        when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertEquals(Collections.singletonList(mCurrentDevice),
                mHeadsetService.getConnectedDevices());
        // Test disconnect from native
        HeadsetStackEvent disconnectEvent =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED,
                        HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED, mCurrentDevice);
        mHeadsetService.messageFromNative(disconnectEvent);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.STACK_EVENT,
                disconnectEvent);
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertEquals(Collections.EMPTY_LIST, mHeadsetService.getConnectedDevices());
    }

    /**
     * Stack connection event to {@link HeadsetHalConstants#CONNECTION_STATE_CONNECTING} should
     * create new state machine
     */
    @Test
    public void testMessageFromNative_deviceConnectingUnknown() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        HeadsetStackEvent connectingEvent =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED,
                        HeadsetHalConstants.CONNECTION_STATE_CONNECTING, mCurrentDevice);
        mHeadsetService.messageFromNative(connectingEvent);
        verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService, mAdapterService,
                mNativeInterface, mSystemInterface);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.STACK_EVENT,
                connectingEvent);
    }

    /**
     * Stack connection event to {@link HeadsetHalConstants#CONNECTION_STATE_DISCONNECTED} should
     * crash by throwing {@link IllegalStateException} if the device is unknown
     */
    @Test
    public void testMessageFromNative_deviceDisconnectedUnknown() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        HeadsetStackEvent connectingEvent =
                new HeadsetStackEvent(HeadsetStackEvent.EVENT_TYPE_CONNECTION_STATE_CHANGED,
                        HeadsetHalConstants.CONNECTION_STATE_DISCONNECTED, mCurrentDevice);
        try {
            mHeadsetService.messageFromNative(connectingEvent);
            Assert.fail("Expect an IllegalStateException");
        } catch (IllegalStateException exception) {
            // Do nothing
        }
        verifyNoMoreInteractions(mObjectsFactory);
    }

    /**
     * Test to verify that {@link HeadsetService#connect(BluetoothDevice)} fails after
     * {@link #MAX_HEADSET_CONNECTIONS} connection requests
     */
    @Test
    public void testConnectDevice_connectDeviceAboveLimit() {
        ArrayList<BluetoothDevice> connectedDevices = new ArrayList<>();
        for (int i = 0; i < MAX_HEADSET_CONNECTIONS; ++i) {
            mCurrentDevice = TestUtils.getTestDevice(mAdapter, i);
            Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
            verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                    mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService,
                    mAdapterService, mNativeInterface, mSystemInterface);
            verify(mObjectsFactory, times(i + 1)).makeStateMachine(any(BluetoothDevice.class),
                    eq(mHeadsetService.getStateMachinesThreadLooper()), eq(mHeadsetService),
                    eq(mAdapterService), eq(mNativeInterface), eq(mSystemInterface));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                    mCurrentDevice);
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(eq(HeadsetStateMachine.CONNECT),
                    any(BluetoothDevice.class));
            // Put device to connecting
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTING);
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            // Put device to connected
            connectedDevices.add(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTED);
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                    mHeadsetService.getConnectionState(mCurrentDevice));
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
        }
        // Connect the next device will fail
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, MAX_HEADSET_CONNECTIONS);
        Assert.assertFalse(mHeadsetService.connect(mCurrentDevice));
        // Though connection failed, a new state machine is still lazily created for the device
        verify(mObjectsFactory, times(MAX_HEADSET_CONNECTIONS + 1)).makeStateMachine(
                any(BluetoothDevice.class), eq(mHeadsetService.getStateMachinesThreadLooper()),
                eq(mHeadsetService), eq(mAdapterService), eq(mNativeInterface),
                eq(mSystemInterface));
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertThat(mHeadsetService.getConnectedDevices(),
                Matchers.containsInAnyOrder(connectedDevices.toArray()));
    }

    /**
     * Test to verify that {@link HeadsetService#connectAudio(BluetoothDevice)} return true when
     * the device is connected and audio is not connected and returns false when audio is already
     * connecting
     */
    @Test
    public void testConnectAudio_withOneDevice() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
        verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService, mAdapterService,
                mNativeInterface, mSystemInterface);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_CONNECTED);
        when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                SystemClock.uptimeMillis());
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertEquals(Collections.singletonList(mCurrentDevice),
                mHeadsetService.getConnectedDevices());
        mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.STATE_CONNECTED);
        // Test connect audio - set the device first as the active device
        Assert.assertTrue(mHeadsetService.setActiveDevice(mCurrentDevice));
        Assert.assertTrue(mHeadsetService.connectAudio(mCurrentDevice));
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT_AUDIO,
                mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getAudioState()).thenReturn(
                BluetoothHeadset.STATE_AUDIO_CONNECTING);
        // 2nd connection attempt for the same device will succeed as well
        Assert.assertTrue(mHeadsetService.connectAudio(mCurrentDevice));
        // Verify CONNECT_AUDIO is only sent once
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                eq(HeadsetStateMachine.CONNECT_AUDIO), any());
        // Test disconnect audio
        Assert.assertTrue(mHeadsetService.disconnectAudio(mCurrentDevice));
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.DISCONNECT_AUDIO,
                mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getAudioState()).thenReturn(
                BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
        // Further disconnection requests will fail
        Assert.assertFalse(mHeadsetService.disconnectAudio(mCurrentDevice));
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                eq(HeadsetStateMachine.DISCONNECT_AUDIO), any(BluetoothDevice.class));
    }

    /**
     * Test to verify that HFP audio connection can be initiated when multiple devices are connected
     * and can be canceled or disconnected as well
     */
    @Test
    public void testConnectAudio_withMultipleDevices() {
        ArrayList<BluetoothDevice> connectedDevices = new ArrayList<>();
        for (int i = 0; i < MAX_HEADSET_CONNECTIONS; ++i) {
            mCurrentDevice = TestUtils.getTestDevice(mAdapter, i);
            Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
            verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                    mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService,
                    mAdapterService, mNativeInterface, mSystemInterface);
            verify(mObjectsFactory, times(i + 1)).makeStateMachine(any(BluetoothDevice.class),
                    eq(mHeadsetService.getStateMachinesThreadLooper()), eq(mHeadsetService),
                    eq(mAdapterService), eq(mNativeInterface), eq(mSystemInterface));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                    mCurrentDevice);
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(eq(HeadsetStateMachine.CONNECT),
                    any(BluetoothDevice.class));
            // Put device to connecting
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTING);
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.STATE_CONNECTING);
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            // Put device to connected
            connectedDevices.add(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTED);
            when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                    SystemClock.uptimeMillis());
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                    mHeadsetService.getConnectionState(mCurrentDevice));
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_CONNECTING, BluetoothProfile.STATE_CONNECTED);
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            // Try to connect audio
            // Should fail
            Assert.assertFalse(mHeadsetService.connectAudio(mCurrentDevice));
            // Should succeed after setActiveDevice()
            Assert.assertTrue(mHeadsetService.setActiveDevice(mCurrentDevice));
            Assert.assertTrue(mHeadsetService.connectAudio(mCurrentDevice));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                    HeadsetStateMachine.CONNECT_AUDIO, mCurrentDevice);
            // Put device to audio connecting state
            when(mStateMachines.get(mCurrentDevice).getAudioState()).thenReturn(
                    BluetoothHeadset.STATE_AUDIO_CONNECTING);
            // 2nd connection attempt will also succeed
            Assert.assertTrue(mHeadsetService.connectAudio(mCurrentDevice));
            // Verify CONNECT_AUDIO is only sent once
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                    eq(HeadsetStateMachine.CONNECT_AUDIO), any());
            // Put device to audio connected state
            when(mStateMachines.get(mCurrentDevice).getAudioState()).thenReturn(
                    BluetoothHeadset.STATE_AUDIO_CONNECTED);
            // Disconnect audio
            Assert.assertTrue(mHeadsetService.disconnectAudio(mCurrentDevice));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                    HeadsetStateMachine.DISCONNECT_AUDIO, mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getAudioState()).thenReturn(
                    BluetoothHeadset.STATE_AUDIO_DISCONNECTED);
            // Further disconnection requests will fail
            Assert.assertFalse(mHeadsetService.disconnectAudio(mCurrentDevice));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                    eq(HeadsetStateMachine.DISCONNECT_AUDIO), any(BluetoothDevice.class));
        }
    }

    /**
     * Verify that only one device can be in audio connecting or audio connected state, further
     * attempt to call {@link HeadsetService#connectAudio(BluetoothDevice)} should fail by returning
     * false
     */
    @Test
    public void testConnectAudio_connectTwoAudioChannelsShouldFail() {
        ArrayList<BluetoothDevice> connectedDevices = new ArrayList<>();
        for (int i = 0; i < MAX_HEADSET_CONNECTIONS; ++i) {
            mCurrentDevice = TestUtils.getTestDevice(mAdapter, i);
            Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
            verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                    mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService,
                    mAdapterService, mNativeInterface, mSystemInterface);
            verify(mObjectsFactory, times(i + 1)).makeStateMachine(any(BluetoothDevice.class),
                    eq(mHeadsetService.getStateMachinesThreadLooper()), eq(mHeadsetService),
                    eq(mAdapterService), eq(mNativeInterface), eq(mSystemInterface));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                    mCurrentDevice);
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(eq(HeadsetStateMachine.CONNECT),
                    any(BluetoothDevice.class));
            // Put device to connecting
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTING);
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.STATE_CONNECTING);
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            // Put device to connected
            connectedDevices.add(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTED);
            when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                    SystemClock.uptimeMillis());
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_CONNECTING, BluetoothProfile.STATE_CONNECTED);
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                    mHeadsetService.getConnectionState(mCurrentDevice));
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
        }
        if (MAX_HEADSET_CONNECTIONS >= 2) {
            // Try to connect audio
            BluetoothDevice firstDevice = connectedDevices.get(0);
            BluetoothDevice secondDevice = connectedDevices.get(1);
            // Set the first device as the active device
            Assert.assertTrue(mHeadsetService.setActiveDevice(firstDevice));
            Assert.assertTrue(mHeadsetService.connectAudio(firstDevice));
            verify(mStateMachines.get(firstDevice)).sendMessage(HeadsetStateMachine.CONNECT_AUDIO,
                    firstDevice);
            // Put device to audio connecting state
            when(mStateMachines.get(firstDevice).getAudioState()).thenReturn(
                    BluetoothHeadset.STATE_AUDIO_CONNECTING);
            // 2nd connection attempt will succeed for the same device
            Assert.assertTrue(mHeadsetService.connectAudio(firstDevice));
            // Connect to 2nd device will fail
            Assert.assertFalse(mHeadsetService.connectAudio(secondDevice));
            verify(mStateMachines.get(secondDevice), never()).sendMessage(
                    HeadsetStateMachine.CONNECT_AUDIO, secondDevice);
            // Put device to audio connected state
            when(mStateMachines.get(firstDevice).getAudioState()).thenReturn(
                    BluetoothHeadset.STATE_AUDIO_CONNECTED);
            // Connect to 2nd device will fail
            Assert.assertFalse(mHeadsetService.connectAudio(secondDevice));
            verify(mStateMachines.get(secondDevice), never()).sendMessage(
                    HeadsetStateMachine.CONNECT_AUDIO, secondDevice);
        }
    }

    /**
     * Verify that {@link HeadsetService#connectAudio()} will connect to first connected/connecting
     * device
     */
    @Test
    public void testConnectAudio_firstConnectedAudioDevice() {
        ArrayList<BluetoothDevice> connectedDevices = new ArrayList<>();
        doAnswer(invocation -> {
            BluetoothDevice[] devicesArray = new BluetoothDevice[connectedDevices.size()];
            return connectedDevices.toArray(devicesArray);
        }).when(mAdapterService).getBondedDevices();
        for (int i = 0; i < MAX_HEADSET_CONNECTIONS; ++i) {
            mCurrentDevice = TestUtils.getTestDevice(mAdapter, i);
            Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
            verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                    mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService,
                    mAdapterService, mNativeInterface, mSystemInterface);
            verify(mObjectsFactory, times(i + 1)).makeStateMachine(any(BluetoothDevice.class),
                    eq(mHeadsetService.getStateMachinesThreadLooper()), eq(mHeadsetService),
                    eq(mAdapterService), eq(mNativeInterface), eq(mSystemInterface));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                    mCurrentDevice);
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(eq(HeadsetStateMachine.CONNECT),
                    any(BluetoothDevice.class));
            // Put device to connecting
            when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                    SystemClock.uptimeMillis());
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTING);
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.STATE_CONNECTING);
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            // Put device to connected
            connectedDevices.add(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTED);
            when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                    SystemClock.uptimeMillis());
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                    mHeadsetService.getConnectionState(mCurrentDevice));
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_CONNECTING, BluetoothProfile.STATE_CONNECTED);
        }
        // Try to connect audio
        BluetoothDevice firstDevice = connectedDevices.get(0);
        Assert.assertTrue(mHeadsetService.setActiveDevice(firstDevice));
        Assert.assertTrue(mHeadsetService.connectAudio());
        verify(mStateMachines.get(firstDevice)).sendMessage(HeadsetStateMachine.CONNECT_AUDIO,
                firstDevice);
    }

    /**
     * Test to verify that {@link HeadsetService#connectAudio(BluetoothDevice)} fails if device
     * was never connected
     */
    @Test
    public void testConnectAudio_deviceNeverConnected() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        Assert.assertFalse(mHeadsetService.connectAudio(mCurrentDevice));
    }

    /**
     * Test to verify that {@link HeadsetService#connectAudio(BluetoothDevice)} fails if device
     * is disconnected
     */
    @Test
    public void testConnectAudio_deviceDisconnected() {
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        HeadsetCallState headsetCallState =
                new HeadsetCallState(1, 0, HeadsetHalConstants.CALL_STATE_ALERTING,
                        TEST_PHONE_NUMBER, 128);
        Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
        verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService, mAdapterService,
                mNativeInterface, mSystemInterface);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
        // Put device in disconnected state
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertEquals(Collections.EMPTY_LIST, mHeadsetService.getConnectedDevices());
        mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                BluetoothProfile.STATE_CONNECTED, BluetoothProfile.STATE_DISCONNECTED);
        // connectAudio should fail
        Assert.assertFalse(mHeadsetService.connectAudio(mCurrentDevice));
        verify(mStateMachines.get(mCurrentDevice), never()).sendMessage(
                eq(HeadsetStateMachine.CONNECT_AUDIO), any());
    }

    /**
     * Verifies that phone state change will trigger a system-wide saving of call state even when
     * no device is connected
     *
     * @throws RemoteException if binder call fails
     */
    @Test
    public void testPhoneStateChange_noDeviceSaveState() throws RemoteException {
        HeadsetCallState headsetCallState =
                new HeadsetCallState(1, 0, HeadsetHalConstants.CALL_STATE_ALERTING,
                        TEST_PHONE_NUMBER, 128);
        mHeadsetServiceBinder.phoneStateChanged(headsetCallState.mNumActive,
                headsetCallState.mNumHeld, headsetCallState.mCallState, headsetCallState.mNumber,
                headsetCallState.mType);
        HeadsetTestUtils.verifyPhoneStateChangeSetters(mPhoneState, headsetCallState,
                ASYNC_CALL_TIMEOUT_MILLIS);
    }

    /**
     * Verifies that phone state change will trigger a system-wide saving of call state and send
     * state change to connected devices
     *
     * @throws RemoteException if binder call fails
     */
    @Test
    public void testPhoneStateChange_oneDeviceSaveState() throws RemoteException {
        HeadsetCallState headsetCallState =
                new HeadsetCallState(1, 0, HeadsetHalConstants.CALL_STATE_ALERTING,
                        TEST_PHONE_NUMBER, 128);
        mCurrentDevice = TestUtils.getTestDevice(mAdapter, 0);
        final ArrayList<BluetoothDevice> connectedDevices = new ArrayList<>();
        // Connect one device
        Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
        verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService, mAdapterService,
                mNativeInterface, mSystemInterface);
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
        // Put device to connecting
        when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                SystemClock.uptimeMillis());
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_CONNECTING);
        mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.STATE_CONNECTING);
        Assert.assertThat(mHeadsetService.getConnectedDevices(),
                Matchers.containsInAnyOrder(connectedDevices.toArray()));
        // Put device to connected
        connectedDevices.add(mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
        when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                BluetoothProfile.STATE_CONNECTED);
        when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                SystemClock.uptimeMillis());
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                mHeadsetService.getConnectionState(mCurrentDevice));
        Assert.assertThat(mHeadsetService.getConnectedDevices(),
                Matchers.containsInAnyOrder(connectedDevices.toArray()));
        mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                BluetoothProfile.STATE_CONNECTING, BluetoothProfile.STATE_CONNECTED);
        // Change phone state
        mHeadsetServiceBinder.phoneStateChanged(headsetCallState.mNumActive,
                headsetCallState.mNumHeld, headsetCallState.mCallState, headsetCallState.mNumber,
                headsetCallState.mType);
        // Make sure we notify device about this change
        verify(mStateMachines.get(mCurrentDevice)).sendMessage(
                HeadsetStateMachine.CALL_STATE_CHANGED, headsetCallState);
        // Make sure state is updated once in phone state holder
        HeadsetTestUtils.verifyPhoneStateChangeSetters(mPhoneState, headsetCallState,
                ASYNC_CALL_TIMEOUT_MILLIS);
    }

    /**
     * Verifies that phone state change will trigger a system-wide saving of call state and send
     * state change to connected devices
     *
     * @throws RemoteException if binder call fails
     */
    @Test
    public void testPhoneStateChange_multipleDevicesSaveState() throws RemoteException {
        HeadsetCallState headsetCallState =
                new HeadsetCallState(1, 0, HeadsetHalConstants.CALL_STATE_ALERTING,
                        TEST_PHONE_NUMBER, 128);
        final ArrayList<BluetoothDevice> connectedDevices = new ArrayList<>();
        for (int i = 0; i < MAX_HEADSET_CONNECTIONS; ++i) {
            mCurrentDevice = TestUtils.getTestDevice(mAdapter, i);
            Assert.assertTrue(mHeadsetService.connect(mCurrentDevice));
            verify(mObjectsFactory).makeStateMachine(mCurrentDevice,
                    mHeadsetService.getStateMachinesThreadLooper(), mHeadsetService,
                    mAdapterService, mNativeInterface, mSystemInterface);
            verify(mObjectsFactory, times(i + 1)).makeStateMachine(any(BluetoothDevice.class),
                    eq(mHeadsetService.getStateMachinesThreadLooper()), eq(mHeadsetService),
                    eq(mAdapterService), eq(mNativeInterface), eq(mSystemInterface));
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(HeadsetStateMachine.CONNECT,
                    mCurrentDevice);
            verify(mStateMachines.get(mCurrentDevice)).sendMessage(eq(HeadsetStateMachine.CONNECT),
                    any(BluetoothDevice.class));
            // Put device to connecting
            when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                    SystemClock.uptimeMillis());
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTING);
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_DISCONNECTED, BluetoothProfile.STATE_CONNECTING);
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            // Put device to connected
            connectedDevices.add(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getDevice()).thenReturn(mCurrentDevice);
            when(mStateMachines.get(mCurrentDevice).getConnectionState()).thenReturn(
                    BluetoothProfile.STATE_CONNECTED);
            when(mStateMachines.get(mCurrentDevice).getConnectingTimestampMs()).thenReturn(
                    SystemClock.uptimeMillis());
            Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                    mHeadsetService.getConnectionState(mCurrentDevice));
            Assert.assertThat(mHeadsetService.getConnectedDevices(),
                    Matchers.containsInAnyOrder(connectedDevices.toArray()));
            mHeadsetService.onConnectionStateChangedFromStateMachine(mCurrentDevice,
                    BluetoothProfile.STATE_CONNECTING, BluetoothProfile.STATE_CONNECTED);
        }
        // Change phone state
        mHeadsetServiceBinder.phoneStateChanged(headsetCallState.mNumActive,
                headsetCallState.mNumHeld, headsetCallState.mCallState, headsetCallState.mNumber,
                headsetCallState.mType);
        // Make sure we notify devices about this change
        for (BluetoothDevice device : connectedDevices) {
            verify(mStateMachines.get(device)).sendMessage(HeadsetStateMachine.CALL_STATE_CHANGED,
                    headsetCallState);
        }
        // Make sure state is updated once in phone state holder
        HeadsetTestUtils.verifyPhoneStateChangeSetters(mPhoneState, headsetCallState,
                ASYNC_CALL_TIMEOUT_MILLIS);
    }

    /*
     *  Helper function to test okToAcceptConnection() method
     *
     *  @param device test device
     *  @param bondState bond state value, could be invalid
     *  @param priority value, could be invalid, coudl be invalid
     *  @param expected expected result from okToAcceptConnection()
     */
    private void testOkToAcceptConnectionCase(BluetoothDevice device, int bondState, int priority,
            boolean expected) {
        doReturn(bondState).when(mAdapterService).getBondState(device);
        Assert.assertTrue(mHeadsetService.setPriority(device, priority));
        Assert.assertEquals(expected, mHeadsetService.okToAcceptConnection(device));
    }

}
