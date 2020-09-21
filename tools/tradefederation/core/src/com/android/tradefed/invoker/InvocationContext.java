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
package com.android.tradefed.invoker;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.BuildSerializedVersion;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationDescriptor;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.ITestDevice.RecoveryMode;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.suite.ITestSuite;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.UniqueMultiMap;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Generic implementation of a {@link IInvocationContext}.
 */
public class InvocationContext implements IInvocationContext {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;

    // Transient field are not serialized
    private transient Map<ITestDevice, IBuildInfo> mAllocatedDeviceAndBuildMap;
    /** Map of the configuration device name and the actual {@link ITestDevice} * */
    private transient Map<String, ITestDevice> mNameAndDeviceMap;
    private Map<String, IBuildInfo> mNameAndBuildinfoMap;
    private final UniqueMultiMap<String, String> mInvocationAttributes =
            new UniqueMultiMap<String, String>();
    private Map<IInvocationContext.TimingEvent, Long> mInvocationTimingMetrics;
    /** Invocation test-tag **/
    private String mTestTag;
    /** configuration descriptor */
    private ConfigurationDescriptor mConfigurationDescriptor;
    /** module invocation context (when running as part of a {@link ITestSuite} */
    private IInvocationContext mModuleContext;
    /**
     * List of map the device serials involved in the sharded invocation, empty if not a sharded
     * invocation.
     */
    private Map<Integer, List<String>> mShardSerials;

    private boolean mLocked;

    /**
     * Creates a {@link BuildInfo} using default attribute values.
     */
    public InvocationContext() {
        mInvocationTimingMetrics = new LinkedHashMap<>();
        mAllocatedDeviceAndBuildMap = new LinkedHashMap<ITestDevice, IBuildInfo>();
        // Use LinkedHashMap to ensure key ordering by insertion order
        mNameAndDeviceMap = new LinkedHashMap<String, ITestDevice>();
        mNameAndBuildinfoMap = new LinkedHashMap<String, IBuildInfo>();
        mShardSerials = new LinkedHashMap<Integer, List<String>>();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getNumDevicesAllocated() {
        return mAllocatedDeviceAndBuildMap.size();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllocatedDevice(String devicename, ITestDevice testDevice) {
        mNameAndDeviceMap.put(devicename, testDevice);
        // back fill the information if possible
        if (mNameAndBuildinfoMap.get(devicename) != null) {
            mAllocatedDeviceAndBuildMap.put(testDevice, mNameAndBuildinfoMap.get(devicename));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllocatedDevice(Map<String, ITestDevice> deviceWithName) {
        mNameAndDeviceMap.putAll(deviceWithName);
        // back fill the information if possible
        for (Entry<String, ITestDevice> entry : deviceWithName.entrySet()) {
            if (mNameAndBuildinfoMap.get(entry.getKey()) != null) {
                mAllocatedDeviceAndBuildMap.put(
                        entry.getValue(), mNameAndBuildinfoMap.get(entry.getKey()));
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Map<ITestDevice, IBuildInfo> getDeviceBuildMap() {
        return mAllocatedDeviceAndBuildMap;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<ITestDevice> getDevices() {
        return new ArrayList<ITestDevice>(mNameAndDeviceMap.values());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<IBuildInfo> getBuildInfos() {
        return new ArrayList<IBuildInfo>(mNameAndBuildinfoMap.values());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> getSerials() {
        List<String> listSerials = new ArrayList<String>();
        for (ITestDevice testDevice : mNameAndDeviceMap.values()) {
            listSerials.add(testDevice.getSerialNumber());
        }
        return listSerials;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> getDeviceConfigNames() {
        List<String> listNames = new ArrayList<String>();
        listNames.addAll(mNameAndDeviceMap.keySet());
        return listNames;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice(String deviceName) {
        return mNameAndDeviceMap.get(deviceName);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuildInfo(String deviceName) {
        return mNameAndBuildinfoMap.get(deviceName);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addDeviceBuildInfo(String deviceName, IBuildInfo buildinfo) {
        mNameAndBuildinfoMap.put(deviceName, buildinfo);
        mAllocatedDeviceAndBuildMap.put(getDevice(deviceName), buildinfo);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuildInfo(ITestDevice testDevice) {
        return mAllocatedDeviceAndBuildMap.get(testDevice);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addInvocationAttribute(String attributeName, String attributeValue) {
        if (mLocked) {
            throw new IllegalStateException(
                    "Attempting to add invocation attribute during a test.");
        }
        mInvocationAttributes.put(attributeName, attributeValue);
    }

    /** {@inheritDoc} */
    @Override
    public void addInvocationAttributes(UniqueMultiMap<String, String> attributesMap) {
        if (mLocked) {
            throw new IllegalStateException(
                    "Attempting to add invocation attribute during a test.");
        }
        mInvocationAttributes.putAll(attributesMap);
    }

    /** {@inheritDoc} */
    @Override
    public MultiMap<String, String> getAttributes() {
        // Return a copy of the map to avoid unwanted modifications.
        UniqueMultiMap<String, String> copy = new UniqueMultiMap<>();
        copy.putAll(mInvocationAttributes);
        return copy;
    }


    /** {@inheritDoc} */
    @Override
    public Map<IInvocationContext.TimingEvent, Long> getInvocationTimingMetrics() {
        return mInvocationTimingMetrics;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addInvocationTimingMetric(IInvocationContext.TimingEvent timingEvent,
            Long durationMillis) {
        mInvocationTimingMetrics.put(timingEvent, durationMillis);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDeviceBySerial(String serial) {
        for (ITestDevice testDevice : mNameAndDeviceMap.values()) {
            if (testDevice.getSerialNumber().equals(serial)) {
                return testDevice;
            }
        }
        CLog.d("Device with serial '%s', not found in the metadata", serial);
        return null;
    }

    /** {@inheritDoc} */
    @Override
    public String getDeviceName(ITestDevice device) {
        for (String name : mNameAndDeviceMap.keySet()) {
            if (device.equals(getDevice(name))) {
                return name;
            }
        }
        CLog.d(
                "Device with serial '%s' doesn't match a name in the metadata",
                device.getSerialNumber());
        return null;
    }

    /** {@inheritDoc} */
    @Override
    public String getTestTag() {
        return mTestTag;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestTag(String testTag) {
        mTestTag = testTag;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setRecoveryModeForAllDevices(RecoveryMode mode) {
        for (ITestDevice device : getDevices()) {
            device.setRecoveryMode(mode);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void setConfigurationDescriptor(ConfigurationDescriptor configurationDescriptor) {
        mConfigurationDescriptor = configurationDescriptor;
    }

    /** {@inheritDoc} */
    @Override
    public ConfigurationDescriptor getConfigurationDescriptor() {
        return mConfigurationDescriptor;
    }

    /** {@inheritDoc} */
    @Override
    public void setModuleInvocationContext(IInvocationContext invocationContext) {
        mModuleContext = invocationContext;
    }

    /** {@inheritDoc} */
    @Override
    public IInvocationContext getModuleInvocationContext() {
        return mModuleContext;
    }

    /** Lock the context to prevent more invocation attributes to be added. */
    public void lockAttributes() {
        mLocked = true;
    }

    /** {@inheritDoc} */
    @Override
    public void addSerialsFromShard(Integer index, List<String> serials) {
        if (mLocked) {
            throw new IllegalStateException(
                    "Attempting to add serial from shard attribute during a test.");
        }
        mShardSerials.put(index, serials);
    }

    /** {@inheritDoc} */
    @Override
    public Map<Integer, List<String>> getShardsSerials() {
        return new LinkedHashMap<>(mShardSerials);
    }

    /** Special java method that allows for custom deserialization. */
    private void readObject(ObjectInputStream in) throws IOException, ClassNotFoundException {
        // our "pseudo-constructor"
        in.defaultReadObject();
        // now we are a "live" object again, so let's init the transient field
        mAllocatedDeviceAndBuildMap = new LinkedHashMap<ITestDevice, IBuildInfo>();
        mNameAndDeviceMap = new LinkedHashMap<String, ITestDevice>();
        // For compatibility, when parent TF does not have the invocation timing yet.
        if (mInvocationTimingMetrics == null) {
            mInvocationTimingMetrics = new LinkedHashMap<>();
        }
    }
}
