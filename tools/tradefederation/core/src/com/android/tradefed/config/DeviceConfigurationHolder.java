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
package com.android.tradefed.config;

import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.build.StubBuildProvider;
import com.android.tradefed.device.DeviceSelectionOptions;
import com.android.tradefed.device.IDeviceRecovery;
import com.android.tradefed.device.IDeviceSelection;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.device.WaitDeviceRecovery;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.ITargetPreparer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A concrete {@link IDeviceConfiguration} implementation that stores the loaded device
 * configuration objects in its attributes.
 */
public class DeviceConfigurationHolder implements IDeviceConfiguration {
    private final String mDeviceName;
    private final boolean mIsFake;

    private IBuildProvider mBuildProvider = new StubBuildProvider();
    private List<ITargetPreparer> mListTargetPreparer = new ArrayList<ITargetPreparer>();
    private IDeviceRecovery mDeviceRecovery = new WaitDeviceRecovery();
    private IDeviceSelection mDeviceSelection = new DeviceSelectionOptions();
    private TestDeviceOptions mTestDeviceOption = new TestDeviceOptions();

    private Map<Object, Integer> mFreqMap = new HashMap<>();

    public DeviceConfigurationHolder() {
        this("", false);
    }

    public DeviceConfigurationHolder(String deviceName) {
        this(deviceName, false);
    }

    public DeviceConfigurationHolder(String deviceName, boolean isFake) {
        mDeviceName = deviceName;
        mIsFake = isFake;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getDeviceName() {
        return mDeviceName;
    }

    /** {@inheritDoc} */
    @Override
    public boolean isFake() {
        return mIsFake;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addSpecificConfig(Object config) throws ConfigurationException {
        if (config instanceof IBuildProvider) {
            mBuildProvider = (IBuildProvider) config;
        } else if (config instanceof ITargetPreparer){
            if (isFake()) {
                throw new ConfigurationException(
                        "cannot specify a target_preparer for a isFake=true device.");
            }
            mListTargetPreparer.add((ITargetPreparer) config);
        } else if (config instanceof IDeviceRecovery) {
            mDeviceRecovery = (IDeviceRecovery) config;
        } else if (config instanceof IDeviceSelection) {
            mDeviceSelection = (IDeviceSelection) config;
        } else if (config instanceof TestDeviceOptions) {
            mTestDeviceOption = (TestDeviceOptions) config;
        } else {
            throw new ConfigurationException(String.format("Cannot add %s class "
                    + "to a device specific definition", config.getClass()));
        }
    }

    /** {@inheritDoc} */
    @Override
    public void addFrequency(Object config, Integer frequency) {
        mFreqMap.put(config, frequency);
    }

    /** {@inheritDoc} */
    @Override
    public Integer getFrequency(Object config) {
        return mFreqMap.get(config);
    }

    /** {@inheritDoc} */
    @Override
    public List<Object> getAllObjects() {
        List<Object> allObject = new ArrayList<Object>();
        allObject.add(mBuildProvider);
        allObject.addAll(mListTargetPreparer);
        allObject.add(mDeviceRecovery);
        allObject.add(mDeviceSelection);
        allObject.add(mTestDeviceOption);
        return allObject;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildProvider getBuildProvider() {
        return mBuildProvider;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<ITargetPreparer> getTargetPreparers() {
        return mListTargetPreparer;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDeviceRecovery getDeviceRecovery() {
        return mDeviceRecovery;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDeviceSelection getDeviceRequirements() {
        return mDeviceSelection;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestDeviceOptions getDeviceOptions() {
        return mTestDeviceOption;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDeviceConfiguration clone() {
        IDeviceConfiguration newDeviceConfig = new DeviceConfigurationHolder(getDeviceName());
        for (Object obj : getAllObjects()) {
            try {
                newDeviceConfig.addSpecificConfig(obj);
            } catch (ConfigurationException e) {
                // Shoud never happen.
                CLog.e(e);
            }
        }
        return newDeviceConfig;
    }
}
