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
package com.android.tradefed;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.sl4a.Sl4aClient;
import com.android.tradefed.util.sl4a.Sl4aEventDispatcher.EventSl4aObject;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Assert;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Bluetooth discovery test using Sl4A scripting layer to query some device APIs.
 */
public class Sl4aBluetoothDiscovery implements IRemoteTest, IMultiDeviceTest {

    private ITestDevice mDut;
    private ITestDevice mDiscoverer;
    private Map<ITestDevice, IBuildInfo> mDevicesInfos;

    private static final String BLUETOOTH_NAME = "TEST_NAME";

    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDevicesInfos = deviceInfos;
        List<ITestDevice> listDevices = new ArrayList<>(mDevicesInfos.keySet());
        mDut = listDevices.get(0);
        mDiscoverer = listDevices.get(1);
    }

    /**
     * Setup the devices state before doing the actual test.
     *
     * @param clientDut sl4a client of the device under test (DUT)
     * @param clientDiscoverer sl4a client of the device doing the discovery of the DUT
     * @throws IOException
     */
    public void setup(Sl4aClient clientDut, Sl4aClient clientDiscoverer) throws IOException {
        clientDut.rpcCall("bluetoothToggleState", false);
        clientDut.getEventDispatcher().clearAllEvents();
        clientDut.rpcCall("bluetoothToggleState", true);

        clientDiscoverer.rpcCall("bluetoothToggleState", false);
        clientDiscoverer.getEventDispatcher().clearAllEvents();
        clientDiscoverer.rpcCall("bluetoothToggleState", true);

        EventSl4aObject response = clientDut.getEventDispatcher()
                .popEvent("BluetoothStateChangedOn", 20000);
        Assert.assertNotNull(response);
        response = clientDiscoverer.getEventDispatcher()
                .popEvent("BluetoothStateChangedOn", 20000);
        Assert.assertNotNull(response);

        Object rep = clientDut.rpcCall("bluetoothCheckState");
        Assert.assertEquals(true, rep);

        clientDut.rpcCall("bluetoothSetLocalName", BLUETOOTH_NAME);
        rep = clientDut.rpcCall("bluetoothGetLocalName");
        Assert.assertEquals(BLUETOOTH_NAME, rep);
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // We provide null path for the apk to assume it's already installed.
        Sl4aClient dutClient = Sl4aClient.startSL4A(mDut, null);
        Sl4aClient discovererClient = Sl4aClient.startSL4A(mDiscoverer, null);

        TestDescription testId =
                new TestDescription(this.getClass().getCanonicalName(), "bluetooth_discovery");

        long startTime = System.currentTimeMillis();
        listener.testRunStarted("sl4a_bluetooth", 1);
        listener.testStarted(testId);

        try {
            setup(dutClient, discovererClient);
            dutClient.rpcCall("bluetoothMakeDiscoverable");
            Object rep = dutClient.rpcCall("bluetoothGetScanMode");
            // 3 signifies CONNECTABLE and DISCOVERABLE
            Assert.assertEquals(3, rep);

            discovererClient.getEventDispatcher().clearAllEvents();
            discovererClient.rpcCall("bluetoothStartDiscovery");
            discovererClient.getEventDispatcher()
                    .popEvent("BluetoothDiscoveryFinished", 60000);
            Object listDiscovered = discovererClient.rpcCall("bluetoothGetDiscoveredDevices");
            JSONArray response = (JSONArray) listDiscovered;
            boolean found = false;
            for (int i = 0; i < response.length(); i++) {
                JSONObject j = response.getJSONObject(i);
                if (j.has("name") && BLUETOOTH_NAME.equals(j.getString("name"))) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                listener.testFailed(testId, "Did not find the bluetooth from DUT.");
            }
        } catch (IOException | JSONException e) {
            CLog.e(e);
            listener.testFailed(testId, "Exception " + e);
        } finally {
            listener.testEnded(testId, new HashMap<String, Metric>());
            listener.testRunEnded(
                    System.currentTimeMillis() - startTime, new HashMap<String, Metric>());
            dutClient.close();
            discovererClient.close();
        }
    }
}
