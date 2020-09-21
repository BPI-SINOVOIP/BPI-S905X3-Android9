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
import com.android.tradefed.device.DeviceUtilStatsMonitor.UtilizationDesc;
import com.android.tradefed.device.IDeviceMonitor.DeviceLister;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.util.ArrayList;
import java.util.List;

/**
 * Simple unit tests for {@link DeviceUtilStatsMonitor}
 */
public class DeviceUtilStatsMonitorTest extends TestCase {

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

    public void testEmpty() {
        EasyMock.replay(mMockDeviceManager);
        assertEquals(0, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);
    }

    /**
     * Test case where device has been available but never allocated
     */
    public void testOnlyAvailable() {
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList(
                DeviceAllocationState.Available));
        EasyMock.replay(mMockDeviceManager);

        mDeviceUtilMonitor.getSamplingTask().run();
        UtilizationDesc desc = mDeviceUtilMonitor.getUtilizationStats();
        assertEquals(0, desc.mTotalUtil);
        assertEquals(1, desc.mDeviceUtil.size());
        assertEquals(0L, (long)desc.mDeviceUtil.get("serial0"));
    }

    /**
     * Test case where device has been allocated but never available
     */
    public void testOnlyAllocated() {
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList(
                DeviceAllocationState.Allocated));
        EasyMock.replay(mMockDeviceManager);

        mDeviceUtilMonitor.getSamplingTask().run();
        UtilizationDesc desc = mDeviceUtilMonitor.getUtilizationStats();
        assertEquals(100L, desc.mTotalUtil);
        assertEquals(1, desc.mDeviceUtil.size());
        assertEquals(100L, (long)desc.mDeviceUtil.get("serial0"));
    }

    /**
     * Test case where samples exceed max
     */
    public void testExceededSamples() {
        mDeviceUtilMonitor.setMaxSamples(2);
        // first return allocated, then return samples with device missing
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList(
                DeviceAllocationState.Allocated));
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList(DeviceAllocationState.Available));
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList(DeviceAllocationState.Available));
        EasyMock.replay(mMockDeviceManager);

        mDeviceUtilMonitor.getSamplingTask().run();
        // only 1 sample - allocated
        assertEquals(100L, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);
        mDeviceUtilMonitor.getSamplingTask().run();
        // 1 out of 2
        assertEquals(50L, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);
        mDeviceUtilMonitor.getSamplingTask().run();
        // 0 out of 2
        assertEquals(0L, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);
    }

    /**
     * Test case where device disappears. Ensure util numbers are calculated until > max samples
     * have been collected with it missing
     */
    public void testMissingDevice() {
        mDeviceUtilMonitor.setMaxSamples(2);
        // first return allocated, then return samples with device missing
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList(
                DeviceAllocationState.Allocated));
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList());
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList());
        EasyMock.expect(mMockDeviceManager.listAllDevices()).andReturn(buildDeviceList());
        EasyMock.replay(mMockDeviceManager);

        // only 1 sample - allocated
        mDeviceUtilMonitor.getSamplingTask().run();
        assertEquals(100L, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);

        // 1 out of 2
        mDeviceUtilMonitor.getSamplingTask().run();
        assertEquals(50L, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);

        // 0 out of 2
        mDeviceUtilMonitor.getSamplingTask().run();
        assertEquals(0L, mDeviceUtilMonitor.getUtilizationStats().mTotalUtil);

        // now removed
        mDeviceUtilMonitor.getSamplingTask().run();
        assertEquals(0L, mDeviceUtilMonitor.getUtilizationStats().mDeviceUtil.size());

    }

    private List<DeviceDescriptor> buildDeviceList(DeviceAllocationState... states) {
        List<DeviceDescriptor> deviceList = new ArrayList<>(states.length);
        for (int i =0; i < states.length; i++) {
            DeviceDescriptor device = createDeviceDesc("serial" + i, states[i]);
            deviceList.add(device);
        }
        return deviceList;
    }

    /**
     * Helper method to create a {@link DeviceDescriptor} using only serial and state.
     */
    private DeviceDescriptor createDeviceDesc(String serial, DeviceAllocationState state) {
        return new DeviceDescriptor(serial, false, state, null, null, null, null, null);
    }
}
