/*
 * Copyright 2017 The Android Open Source Project
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

package com.android.bluetooth.hid;

import static org.mockito.Mockito.*;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHidDevice;
import android.bluetooth.BluetoothHidDeviceAppSdpSettings;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.IBluetoothHidDeviceCallback;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Looper;
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

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class HidDeviceTest {
    private static final int TIMEOUT_MS = 1000;    // 1s
    private static final byte[] SAMPLE_HID_REPORT = new byte[]{0x01, 0x00, 0x02};
    private static final byte SAMPLE_REPORT_ID = 0x00;
    private static final byte SAMPLE_REPORT_TYPE = 0x00;
    private static final byte SAMPLE_REPORT_ERROR = 0x02;
    private static final byte SAMPLE_BUFFER_SIZE = 100;

    private static final int CALLBACK_APP_REGISTERED = 0;
    private static final int CALLBACK_APP_UNREGISTERED = 1;
    private static final int CALLBACK_ON_GET_REPORT = 2;
    private static final int CALLBACK_ON_SET_REPORT = 3;
    private static final int CALLBACK_ON_SET_PROTOCOL = 4;
    private static final int CALLBACK_ON_INTR_DATA = 5;
    private static final int CALLBACK_ON_VIRTUAL_UNPLUG = 6;

    @Mock private AdapterService mAdapterService;
    @Mock private HidDeviceNativeInterface mHidDeviceNativeInterface;

    private BluetoothAdapter mAdapter;
    private BluetoothDevice mTestDevice;
    private HidDeviceService mHidDeviceService;
    private Context mTargetContext;
    private BluetoothHidDeviceAppSdpSettings mSettings;
    private BroadcastReceiver mConnectionStateChangedReceiver;
    private final BlockingQueue<Intent> mConnectionStateChangedQueue = new LinkedBlockingQueue<>();
    private final BlockingQueue<Integer> mCallbackQueue = new LinkedBlockingQueue<>();

    @Rule public final ServiceTestRule mServiceRule = new ServiceTestRule();

    private static void setHidDeviceNativeInterfaceInstance(HidDeviceNativeInterface instance)
            throws Exception {
        Method method = HidDeviceNativeInterface.class.getDeclaredMethod("setInstance",
                HidDeviceNativeInterface.class);
        method.setAccessible(true);
        method.invoke(null, instance);
    }

    @Before
    public void setUp() throws Exception {
        mTargetContext = InstrumentationRegistry.getTargetContext();
        Assume.assumeTrue("Ignore test when HidDeviceService is not enabled",
                mTargetContext.getResources().getBoolean(R.bool.profile_supported_hid_device));
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        Assert.assertNotNull(Looper.myLooper());

        // Set up mocks and test assets
        MockitoAnnotations.initMocks(this);
        TestUtils.setAdapterService(mAdapterService);
        setHidDeviceNativeInterfaceInstance(mHidDeviceNativeInterface);
        // This line must be called to make sure relevant objects are initialized properly
        mAdapter = BluetoothAdapter.getDefaultAdapter();
        // Get a device for testing
        mTestDevice = mAdapter.getRemoteDevice("10:11:12:13:14:15");

        TestUtils.startService(mServiceRule, HidDeviceService.class);
        mHidDeviceService = HidDeviceService.getHidDeviceService();
        Assert.assertNotNull(mHidDeviceService);

        // Force unregister app first
        mHidDeviceService.unregisterApp();

        Field field = HidDeviceService.class.getDeclaredField("mHidDeviceNativeInterface");
        field.setAccessible(true);
        HidDeviceNativeInterface nativeInterface =
                (HidDeviceNativeInterface) field.get(mHidDeviceService);
        Assert.assertEquals(nativeInterface, mHidDeviceNativeInterface);

        // Dummy SDP settings
        mSettings = new BluetoothHidDeviceAppSdpSettings("Unit test", "test", "Android",
                BluetoothHidDevice.SUBCLASS1_COMBO, new byte[]{});

        // Set up the Connection State Changed receiver
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothHidDevice.ACTION_CONNECTION_STATE_CHANGED);
        mConnectionStateChangedReceiver = new ConnectionStateChangedReceiver();
        mTargetContext.registerReceiver(mConnectionStateChangedReceiver, filter);
        reset(mHidDeviceNativeInterface, mAdapterService);
    }

    @After
    public void tearDown() throws Exception {
        if (!mTargetContext.getResources().getBoolean(R.bool.profile_supported_hid_device)) {
            return;
        }
        TestUtils.stopService(mServiceRule, HidDeviceService.class);
        mHidDeviceService = HidDeviceService.getHidDeviceService();
        Assert.assertNull(mHidDeviceService);
        mTargetContext.unregisterReceiver(mConnectionStateChangedReceiver);
        mConnectionStateChangedQueue.clear();
        mCallbackQueue.clear();
        setHidDeviceNativeInterfaceInstance(null);
        TestUtils.clearAdapterService(mAdapterService);
    }

    private class ConnectionStateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!BluetoothHidDevice.ACTION_CONNECTION_STATE_CHANGED.equals(intent.getAction())) {
                return;
            }
            try {
                mConnectionStateChangedQueue.put(intent);
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }
    }

    private Intent waitForIntent(int timeoutMs, BlockingQueue<Intent> queue) {
        try {
            Intent intent = queue.poll(timeoutMs, TimeUnit.MILLISECONDS);
            Assert.assertNotNull(intent);
            return intent;
        } catch (InterruptedException e) {
            Assert.fail("Cannot obtain an Intent from the queue");
        }
        return null;
    }

    private void verifyConnectionStateIntent(int timeoutMs, BluetoothDevice device, int newState,
            int prevState) {
        Intent intent = waitForIntent(timeoutMs, mConnectionStateChangedQueue);
        Assert.assertNotNull(intent);
        Assert.assertEquals(BluetoothHidDevice.ACTION_CONNECTION_STATE_CHANGED, intent.getAction());
        Assert.assertEquals(device, intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE));
        Assert.assertEquals(newState, intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1));
        Assert.assertEquals(prevState,
                intent.getIntExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, -1));
    }

    private void verifyCallback(int timeoutMs, int callbackType, BlockingQueue<Integer> queue) {
        try {
            Integer lastCallback = queue.poll(timeoutMs, TimeUnit.MILLISECONDS);
            Assert.assertNotNull(lastCallback);
            int lastCallbackType = lastCallback;
            Assert.assertEquals(callbackType, lastCallbackType);
        } catch (InterruptedException e) {
            Assert.fail("Cannot obtain a callback from the queue");
        }
    }

    class BluetoothHidDeviceCallbackTestHelper extends IBluetoothHidDeviceCallback.Stub {
        public void onAppStatusChanged(BluetoothDevice device, boolean registered) {
            try {
                if (registered) {
                    mCallbackQueue.put(CALLBACK_APP_REGISTERED);
                } else {
                    mCallbackQueue.put(CALLBACK_APP_UNREGISTERED);
                }
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }

        public void onConnectionStateChanged(BluetoothDevice device, int state) {

        }

        public void onGetReport(BluetoothDevice device, byte type, byte id, int bufferSize) {
            try {
                mCallbackQueue.put(CALLBACK_ON_GET_REPORT);
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }

        public void onSetReport(BluetoothDevice device, byte type, byte id, byte[] data) {
            try {
                mCallbackQueue.put(CALLBACK_ON_SET_REPORT);
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }

        public void onSetProtocol(BluetoothDevice device, byte protocol) {
            try {
                mCallbackQueue.put(CALLBACK_ON_SET_PROTOCOL);
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }

        public void onInterruptData(BluetoothDevice device, byte reportId, byte[] data) {
            try {
                mCallbackQueue.put(CALLBACK_ON_INTR_DATA);
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }

        public void onVirtualCableUnplug(BluetoothDevice device) {
            try {
                mCallbackQueue.put(CALLBACK_ON_VIRTUAL_UNPLUG);
            } catch (InterruptedException e) {
                Assert.fail("Cannot add Intent to the queue");
            }
        }
    }

    /**
     * Test getting HidDeviceService: getHidDeviceService().
     */
    @Test
    public void testGetHidDeviceService() {
        Assert.assertEquals(mHidDeviceService, HidDeviceService.getHidDeviceService());
    }

    /**
     * Test the logic in registerApp and unregisterApp. Should get a callback
     * onApplicationStateChangedFromNative.
     */
    @Test
    public void testRegistration() throws Exception {
        doReturn(true).when(mHidDeviceNativeInterface)
                .registerApp(anyString(), anyString(), anyString(), anyByte(), any(byte[].class),
                        isNull(), isNull());

        verify(mHidDeviceNativeInterface, never()).registerApp(anyString(), anyString(),
                anyString(), anyByte(), any(byte[].class), isNull(), isNull());

        // Register app
        BluetoothHidDeviceCallbackTestHelper helper = new BluetoothHidDeviceCallbackTestHelper();
        Assert.assertTrue(mHidDeviceService.registerApp(mSettings, null, null, helper));

        verify(mHidDeviceNativeInterface).registerApp(anyString(), anyString(), anyString(),
                anyByte(), any(byte[].class), isNull(), isNull());

        // App registered
        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, true);
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_REGISTERED, mCallbackQueue);

        // Unregister app
        doReturn(true).when(mHidDeviceNativeInterface).unregisterApp();
        Assert.assertEquals(true, mHidDeviceService.unregisterApp());

        verify(mHidDeviceNativeInterface).unregisterApp();

        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, false);
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_UNREGISTERED, mCallbackQueue);

    }

    /**
     * Test the logic in sendReport(). This should fail when the app is not registered.
     */
    @Test
    public void testSendReport() throws Exception {
        doReturn(true).when(mHidDeviceNativeInterface).sendReport(anyInt(), any(byte[].class));
        // sendReport() should fail without app registered
        Assert.assertEquals(false,
                mHidDeviceService.sendReport(mTestDevice, SAMPLE_REPORT_ID, SAMPLE_HID_REPORT));

        // Register app
        doReturn(true).when(mHidDeviceNativeInterface)
                .registerApp(anyString(), anyString(), anyString(), anyByte(), any(byte[].class),
                        isNull(), isNull());
        BluetoothHidDeviceCallbackTestHelper helper = new BluetoothHidDeviceCallbackTestHelper();
        Assert.assertTrue(mHidDeviceService.registerApp(mSettings, null, null, helper));

        // App registered
        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, true);

        // Wait for the app registration callback to complete and verify it
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_REGISTERED, mCallbackQueue);

        // sendReport() should work when app is registered
        Assert.assertEquals(true,
                mHidDeviceService.sendReport(mTestDevice, SAMPLE_REPORT_ID, SAMPLE_HID_REPORT));

        verify(mHidDeviceNativeInterface).sendReport(eq((int) SAMPLE_REPORT_ID),
                eq(SAMPLE_HID_REPORT));

        // Unregister app
        doReturn(true).when(mHidDeviceNativeInterface).unregisterApp();
        Assert.assertEquals(true, mHidDeviceService.unregisterApp());
    }

    /**
     * Test the logic in replyReport(). This should fail when the app is not registered.
     */
    @Test
    public void testReplyReport() throws Exception {
        doReturn(true).when(mHidDeviceNativeInterface)
                .replyReport(anyByte(), anyByte(), any(byte[].class));
        // replyReport() should fail without app registered
        Assert.assertEquals(false,
                mHidDeviceService.replyReport(mTestDevice, SAMPLE_REPORT_TYPE, SAMPLE_REPORT_ID,
                        SAMPLE_HID_REPORT));

        // Register app
        doReturn(true).when(mHidDeviceNativeInterface)
                .registerApp(anyString(), anyString(), anyString(), anyByte(), any(byte[].class),
                        isNull(), isNull());
        BluetoothHidDeviceCallbackTestHelper helper = new BluetoothHidDeviceCallbackTestHelper();
        Assert.assertTrue(mHidDeviceService.registerApp(mSettings, null, null, helper));

        // App registered
        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, true);

        // Wait for the app registration callback to complete and verify it
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_REGISTERED, mCallbackQueue);

        // replyReport() should work when app is registered
        Assert.assertEquals(true,
                mHidDeviceService.replyReport(mTestDevice, SAMPLE_REPORT_TYPE, SAMPLE_REPORT_ID,
                        SAMPLE_HID_REPORT));

        verify(mHidDeviceNativeInterface).replyReport(eq(SAMPLE_REPORT_TYPE), eq(SAMPLE_REPORT_ID),
                eq(SAMPLE_HID_REPORT));

        // Unregister app
        doReturn(true).when(mHidDeviceNativeInterface).unregisterApp();
        Assert.assertEquals(true, mHidDeviceService.unregisterApp());
    }

    /**
     * Test the logic in reportError(). This should fail when the app is not registered.
     */
    @Test
    public void testReportError() throws Exception {
        doReturn(true).when(mHidDeviceNativeInterface).reportError(anyByte());
        // reportError() should fail without app registered
        Assert.assertEquals(false, mHidDeviceService.reportError(mTestDevice, SAMPLE_REPORT_ERROR));

        // Register app
        doReturn(true).when(mHidDeviceNativeInterface)
                .registerApp(anyString(), anyString(), anyString(), anyByte(), any(byte[].class),
                        isNull(), isNull());
        BluetoothHidDeviceCallbackTestHelper helper = new BluetoothHidDeviceCallbackTestHelper();
        Assert.assertTrue(mHidDeviceService.registerApp(mSettings, null, null, helper));

        // App registered
        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, true);

        // Wait for the app registration callback to complete and verify it
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_REGISTERED, mCallbackQueue);

        // reportError() should work when app is registered
        Assert.assertEquals(true, mHidDeviceService.reportError(mTestDevice, SAMPLE_REPORT_ERROR));

        verify(mHidDeviceNativeInterface).reportError(eq(SAMPLE_REPORT_ERROR));

        // Unregister app
        doReturn(true).when(mHidDeviceNativeInterface).unregisterApp();
        Assert.assertEquals(true, mHidDeviceService.unregisterApp());
    }

    /**
     * Test that an outgoing connection/disconnection succeeds
     */
    @Test
    public void testOutgoingConnectDisconnectSuccess() {
        doReturn(true).when(mHidDeviceNativeInterface).connect(any(BluetoothDevice.class));
        doReturn(true).when(mHidDeviceNativeInterface).disconnect();

        // Register app
        doReturn(true).when(mHidDeviceNativeInterface)
                .registerApp(anyString(), anyString(), anyString(), anyByte(), any(byte[].class),
                        isNull(), isNull());
        mHidDeviceService.registerApp(mSettings, null, null, null);

        // App registered
        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, true);

        // Send a connect request
        Assert.assertTrue("Connect failed", mHidDeviceService.connect(mTestDevice));

        mHidDeviceService.onConnectStateChangedFromNative(mTestDevice,
                HidDeviceService.HAL_CONN_STATE_CONNECTING);
        // Verify the connection state broadcast
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_CONNECTING,
                BluetoothProfile.STATE_DISCONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTING,
                mHidDeviceService.getConnectionState(mTestDevice));

        mHidDeviceService.onConnectStateChangedFromNative(mTestDevice,
                HidDeviceService.HAL_CONN_STATE_CONNECTED);
        // Verify the connection state broadcast
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_CONNECTED,
                BluetoothProfile.STATE_CONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_CONNECTED,
                mHidDeviceService.getConnectionState(mTestDevice));

        // Verify the list of connected devices
        Assert.assertTrue(mHidDeviceService.getDevicesMatchingConnectionStates(
                new int[]{BluetoothProfile.STATE_CONNECTED}).contains(mTestDevice));

        // Send a disconnect request
        Assert.assertTrue("Disconnect failed", mHidDeviceService.disconnect(mTestDevice));

        mHidDeviceService.onConnectStateChangedFromNative(mTestDevice,
                HidDeviceService.HAL_CONN_STATE_DISCONNECTING);
        // Verify the connection state broadcast
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_DISCONNECTING,
                BluetoothProfile.STATE_CONNECTED);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTING,
                mHidDeviceService.getConnectionState(mTestDevice));

        mHidDeviceService.onConnectStateChangedFromNative(mTestDevice,
                HidDeviceService.HAL_CONN_STATE_DISCONNECTED);
        // Verify the connection state broadcast
        verifyConnectionStateIntent(TIMEOUT_MS, mTestDevice, BluetoothProfile.STATE_DISCONNECTED,
                BluetoothProfile.STATE_DISCONNECTING);
        Assert.assertEquals(BluetoothProfile.STATE_DISCONNECTED,
                mHidDeviceService.getConnectionState(mTestDevice));

        // Verify the list of connected devices
        Assert.assertFalse(mHidDeviceService.getDevicesMatchingConnectionStates(
                new int[]{BluetoothProfile.STATE_CONNECTED}).contains(mTestDevice));

        // Unregister app
        doReturn(true).when(mHidDeviceNativeInterface).unregisterApp();
        Assert.assertEquals(true, mHidDeviceService.unregisterApp());
    }

    /**
     * Test the logic in callback functions from native stack: onGetReport, onSetReport,
     * onSetProtocol, onInterruptData, onVirtualCableUnplug. The HID Device server should send the
     * callback to the user app.
     */
    @Test
    public void testCallbacks() {
        doReturn(true).when(mHidDeviceNativeInterface)
                .registerApp(anyString(), anyString(), anyString(), anyByte(), any(byte[].class),
                        isNull(), isNull());

        verify(mHidDeviceNativeInterface, never()).registerApp(anyString(), anyString(),
                anyString(), anyByte(), any(byte[].class), isNull(), isNull());

        // Register app
        BluetoothHidDeviceCallbackTestHelper helper = new BluetoothHidDeviceCallbackTestHelper();
        Assert.assertTrue(mHidDeviceService.registerApp(mSettings, null, null, helper));

        verify(mHidDeviceNativeInterface).registerApp(anyString(), anyString(), anyString(),
                anyByte(), any(byte[].class), isNull(), isNull());

        // App registered
        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, true);
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_REGISTERED, mCallbackQueue);

        // Received callback: onGetReport
        mHidDeviceService.onGetReportFromNative(SAMPLE_REPORT_TYPE, SAMPLE_REPORT_ID,
                SAMPLE_BUFFER_SIZE);
        verifyCallback(TIMEOUT_MS, CALLBACK_ON_GET_REPORT, mCallbackQueue);

        // Received callback: onSetReport
        mHidDeviceService.onSetReportFromNative(SAMPLE_REPORT_TYPE, SAMPLE_REPORT_ID,
                SAMPLE_HID_REPORT);
        verifyCallback(TIMEOUT_MS, CALLBACK_ON_SET_REPORT, mCallbackQueue);

        // Received callback: onSetProtocol
        mHidDeviceService.onSetProtocolFromNative(BluetoothHidDevice.PROTOCOL_BOOT_MODE);
        verifyCallback(TIMEOUT_MS, CALLBACK_ON_SET_PROTOCOL, mCallbackQueue);

        // Received callback: onInterruptData
        mHidDeviceService.onInterruptDataFromNative(SAMPLE_REPORT_ID, SAMPLE_HID_REPORT);
        verifyCallback(TIMEOUT_MS, CALLBACK_ON_INTR_DATA, mCallbackQueue);

        // Received callback: onVirtualCableUnplug
        mHidDeviceService.onVirtualCableUnplugFromNative();
        verifyCallback(TIMEOUT_MS, CALLBACK_ON_VIRTUAL_UNPLUG, mCallbackQueue);

        // Unregister app
        doReturn(true).when(mHidDeviceNativeInterface).unregisterApp();
        Assert.assertEquals(true, mHidDeviceService.unregisterApp());

        verify(mHidDeviceNativeInterface).unregisterApp();

        mHidDeviceService.onApplicationStateChangedFromNative(mTestDevice, false);
        verifyCallback(TIMEOUT_MS, CALLBACK_APP_UNREGISTERED, mCallbackQueue);
    }
}
