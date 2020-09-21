/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.device;

import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.device.IDeviceMonitor.DeviceLister;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.ArrayList;
import java.util.List;

/**
 * Load test for {@link DeviceUtilStatsMonitor} Used to ensure memory used by monitor under heavy
 * load is reasonable
 */
public class DeviceUtilStatsMonitorLoadTest extends TestCase {

    private static final int NUM_DEVICES = 100;

    private IDeviceManager mMockDeviceManager;
    private DeviceUtilStatsMonitor mDeviceUtilMonitor;

    @Override
    public void setUp() {
        mMockDeviceManager = EasyMock.createNiceMock(IDeviceManager.class);
        mDeviceUtilMonitor = new DeviceUtilStatsMonitor() {
            @Override
            IDeviceManager getDeviceManager() {
                return mMockDeviceManager;
            }
        };
        mDeviceUtilMonitor.setDeviceLister(new DeviceLister() {
            @Override
            public List<DeviceDescriptor> listDevices() {
                return mMockDeviceManager.listAllDevices();
            }
        });
        mDeviceUtilMonitor.calculateMaxSamples();
    }

    /**
     * Simulate a heavy load by generating constant allocation events length for
     * all NUM_DEVICES devices.
     * <p/>
     * Intended to be run under a profiler.
     * @throws InterruptedException
     */
    public void testManyRecords() throws InterruptedException {
        List<DeviceDescriptor> deviceList = new ArrayList<>(NUM_DEVICES);
        for (int i =0; i <NUM_DEVICES; i++) {
            DeviceDescriptor device = createDeviceDesc("serial" + i, DeviceAllocationState.Allocated);
            deviceList.add(device);
        }
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andStubReturn(deviceList);
        EasyMock.replay(mMockDeviceManager);

        for (int i = 0; i < mDeviceUtilMonitor.getMaxSamples(); i++) {
            mDeviceUtilMonitor.getSamplingTask().run();
        }
        // This takes ~ 1.9 MB in heap if DeviceUtilStatsMonitor uses a LinkedList<Byte> to
        // store samples
        // takes ~ 65K if CircularByteArray is used
        Thread.sleep(5 * 60 * 1000);
    }

    /**
     * Helper method to create a {@link DeviceDescriptor} using only serial and state.
     */
    private DeviceDescriptor createDeviceDesc(String serial, DeviceAllocationState state) {
        return new DeviceDescriptor(serial, false, state, null, null, null, null, null);
    }

    public static void main(String[] args) {
        //new DeviceUtilStatsMonitorLoadTest().testManyRecords();
    }
}
