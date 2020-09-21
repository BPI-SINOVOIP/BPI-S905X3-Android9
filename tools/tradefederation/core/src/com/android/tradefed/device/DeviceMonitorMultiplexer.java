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

import java.util.LinkedList;
import java.util.List;

/**
 * A proxy class to propagate requests to multiple {@link IDeviceMonitor}s.
 */
public class DeviceMonitorMultiplexer implements IDeviceMonitor {

    private final List<IDeviceMonitor> mDeviceMonitors;

    public DeviceMonitorMultiplexer() {
        mDeviceMonitors = new LinkedList<>();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void run() {
        for (IDeviceMonitor monitor : mDeviceMonitors) {
            monitor.run();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void setDeviceLister(DeviceLister lister) {
        for (IDeviceMonitor monitor : mDeviceMonitors) {
            monitor.setDeviceLister(lister);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void notifyDeviceStateChange(String serial, DeviceAllocationState oldState,
            DeviceAllocationState newState) {
        for (IDeviceMonitor monitor : mDeviceMonitors) {
            monitor.notifyDeviceStateChange(serial, oldState, newState);
        }
    }

    public synchronized void addMonitors(List<IDeviceMonitor> globalDeviceMonitors) {
        mDeviceMonitors.addAll(globalDeviceMonitors);
    }

    public synchronized void addMonitor(IDeviceMonitor globalDeviceMonitor) {
        mDeviceMonitors.add(globalDeviceMonitor);
    }

    public synchronized void removeMonitor(IDeviceMonitor mon) {
        mDeviceMonitors.remove(mon);
    }

    @Override
    public synchronized void stop() {
        for (IDeviceMonitor monitor : mDeviceMonitors) {
            monitor.stop();
        }
    }
}
