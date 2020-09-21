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

import com.android.ddmlib.IDevice;
import com.android.tradefed.device.IManagedTestDevice.DeviceEventResponse;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ConditionPriorityBlockingQueue.IMatcher;

import java.util.ArrayList;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.locks.ReentrantLock;

import javax.annotation.concurrent.GuardedBy;

/**
 * A thread-safe data structure that holds all devices known to {@link DeviceManager}.
 * <p/>
 * Iteration is also thread-safe, but not consistent. A copy of the list is made at iterator
 * creation time, and that copy is used as the iteration target. If queue is modified during
 * iteration, a {@link ConcurrentModificationException} will not be thrown, but the iterator
 * will also not reflect the modified contents.
 */
class ManagedDeviceList implements Iterable<IManagedTestDevice> {

    /**
     * A {@link IMatcher} for finding a {@link IManagedTestDevice} that can be allocated.
     * Will change the device state to ALLOCATED upon finding a successful match.
     */
    private static class AllocationMatcher implements IMatcher<IManagedTestDevice> {
        private IDeviceSelection mDeviceSelectionMatcher;

        AllocationMatcher(IDeviceSelection options) {
            mDeviceSelectionMatcher = options;
        }

        @Override
        public boolean matches(IManagedTestDevice element) {
            if (mDeviceSelectionMatcher.matches(element.getIDevice())) {
                DeviceEventResponse r = element.handleAllocationEvent(DeviceEvent.ALLOCATE_REQUEST);
                return r.stateChanged && r.allocationState == DeviceAllocationState.Allocated;
            }
            return false;
        }
    }

    private final ReentrantLock mListLock = new ReentrantLock(true);
    @GuardedBy("mListLock")
    private List<IManagedTestDevice> mList = new LinkedList<IManagedTestDevice>();
    private final IManagedTestDeviceFactory mDeviceFactory;

    public ManagedDeviceList(IManagedTestDeviceFactory d) {
        mDeviceFactory = d;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Iterator<IManagedTestDevice> iterator() {
        return getCopy().iterator();
    }

    /**
     * Get a copy of the contents of the queue.
     */
    List<IManagedTestDevice> getCopy() {
        mListLock.lock();
        try {
            List<IManagedTestDevice> l = new ArrayList<IManagedTestDevice>(size());
            l.addAll(mList);
            return l;
        } finally {
            mListLock.unlock();
        }
    }

    /**
     * Return the number of elements in the list
     */
    public int size() {
        mListLock.lock();
        try {
            return mList.size();
        } finally {
            mListLock.unlock();
        }
    }

    /**
     * Finds the device with the given serial
     *
     * @param serialNumber
     * @return the {@link IManagedTestDevice} or <code>null</code> if not found
     */
    public IManagedTestDevice find(final String serialNumber) {
        return find(new IMatcher<IManagedTestDevice>() {
            @Override
            public boolean matches(IManagedTestDevice element) {
                return serialNumber.equals(element.getSerialNumber());
            }
        });
    }

    private boolean isValidDeviceSerial(String serial) {
        return serial.length() > 1 && !serial.contains("?");
    }

    /**
     * Update the {@link TestDevice#getDeviceState()} of devices as appropriate.
     *
     * @param serials the devices currently on fastboot
     */
    public void updateFastbootStates(Set<String> serials) {
        List<IManagedTestDevice> toRemove = new ArrayList<>();
        mListLock.lock();
        try {
            for (IManagedTestDevice d : mList) {
                if (serials.contains(d.getSerialNumber())) {
                    d.setDeviceState(TestDeviceState.FASTBOOT);
                } else if (d.getDeviceState() == TestDeviceState.FASTBOOT) {
                    // device was previously on fastboot, assume its gone now
                    d.setDeviceState(TestDeviceState.NOT_AVAILABLE);
                    CLog.d("Device %s was in fastboot and not found anymore", d.getSerialNumber());
                    toRemove.add(d);
                }
            }
        } finally {
            mListLock.unlock();
        }
        for (IManagedTestDevice d : toRemove) {
            handleDeviceEvent(d, DeviceEvent.DISCONNECTED);
        }
    }

    /**
     * Attempt to allocate a device from the list
     * @param options
     * @return the {@link IManagedTestDevice} that was successfully allocated, null otherwise
     */
    public IManagedTestDevice allocate(IDeviceSelection options) {
        AllocationMatcher m = new AllocationMatcher(options);
        // this method is a variant of find, that attempts to find a device matching options
        // and that can be transitioned to allocated state.
        // if found, the device will be moved to the back of the list to try to even out
        // allocations among devices
        mListLock.lock();
        try {
            Iterator<IManagedTestDevice> iterator = mList.iterator();
            while (iterator.hasNext()) {
                IManagedTestDevice d = iterator.next();
                if (m.matches(d)) {
                    iterator.remove();
                    mList.add(d);
                    return d;
                }
            }
        } finally {
            mListLock.unlock();
        }
        return null;
    }

    private IManagedTestDevice find(IMatcher<IManagedTestDevice> m) {
        mListLock.lock();
        try {
            for (IManagedTestDevice d : mList) {
                if (m.matches(d)) {
                    return d;
                }
            }
        } finally {
            mListLock.unlock();
        }
        return null;
    }

    /**
     * Remove the contents of this list.
     * <p/>
     * Exposed for unit testing
     */
     void clear() {
         mListLock.lock();
         try {
             mList.clear();
         } finally {
             mListLock.unlock();
         }
    }

    /**
     * Find the {@link IManagedTestDevice} corresponding to the {@link IDevice}. If it does not
     * exist, create a new one.
     *
     * @param idevice
     * @return the {@link IManagedTestDevice}.
     */
    public IManagedTestDevice findOrCreate(IDevice idevice) {
        if (!isValidDeviceSerial(idevice.getSerialNumber())) {
            return null;
        }
        mListLock.lock();
        try {
            IManagedTestDevice d = find(idevice.getSerialNumber());
            if (d == null || DeviceAllocationState.Unavailable.equals(d.getAllocationState())) {
                mList.remove(d);
                d = mDeviceFactory.createDevice(idevice);
                mList.add(d);
            }
            return d;
        } finally {
            mListLock.unlock();
        }
    }

    /**
     * Directly add a device to the list. Should not be used outside of unit testing.
     * @param device
     */
    void add(IManagedTestDevice device) {
        mListLock.lock();
        try {
            mList.add(device);
        } finally {
            mListLock.unlock();
        }
    }

    /**
     * Handle a device event for given device. Will remove device from list if state transitions
     * to unknown.
     * <p/>
     * {@link DeviceManager} should always make calls through this method as opposed to calling
     * {@link IManagedTestDevice#handleAllocationEvent(DeviceEvent)} directly, to ensure list stays
     * valid.
     */
    public DeviceEventResponse handleDeviceEvent(IManagedTestDevice d, DeviceEvent event) {
        DeviceEventResponse r = d.handleAllocationEvent(event);
        if (r != null && r.allocationState == DeviceAllocationState.Unknown) {
           remove(d);
        }
        return r;
    }

    private void remove(IManagedTestDevice d) {
        mListLock.lock();
        try {
            mList.remove(d);
        } finally {
            mListLock.unlock();
        }
    }
}
