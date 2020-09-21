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
import com.android.tradefed.device.IDeviceRecovery;
import com.android.tradefed.device.IDeviceSelection;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.targetprep.ITargetPreparer;

import java.util.List;

/**
 * Device Configuration Holder Interface.
 * Use to represent an object that can hold the information for the configuration of a device.
 */
public interface IDeviceConfiguration {

    /** Returns The Name of the device specified in the field "name" of the configuration. */
    public String getDeviceName();

    /** Returns whether the container is for a Device Under Test or not. */
    public boolean isFake();

    /**
     * Return The list of all the configuration objects held the instance of
     * {@link IDeviceConfiguration}
     */
    public List<Object> getAllObjects();

    /**
     * Pass one of the allowed objects that the Configuration Holder can keep track of.
     * <p>
     * Complete list of allowed objects are: {@link IBuildProvider}, {@link ITargetPreparer},
     * {@link IDeviceRecovery}, {@link IDeviceSelection}, {@link TestDeviceOptions}
     *
     * @param config object from a type above.
     * @throws ConfigurationException in case the object passed doesn't match the allowed types.
     */
    public void addSpecificConfig(Object config) throws ConfigurationException;

    /**
     * Keep track of the frequency of the object so we can properly inject option against it.
     *
     * @param config the object we are tracking the frequency.
     * @param frequency frequency associated with the object.
     */
    public void addFrequency(Object config, Integer frequency);

    /** Returns the frequency of the object. */
    public Integer getFrequency(Object config);

    /** Return {@link IBuildProvider} that the device configuration holder has reference to. */
    public IBuildProvider getBuildProvider();

    /**
     * Return a list of {@link ITargetPreparer} that the device configuration holder has.
     */
    public List<ITargetPreparer> getTargetPreparers();

    /**
     * Return {@link IDeviceRecovery} that the device configuration holder has.
     */
    public IDeviceRecovery getDeviceRecovery();

    /**
     * Return {@link TestDeviceOptions} that the device configuration holder has.
     */
    public TestDeviceOptions getDeviceOptions();

    /**
     * Return {@link IDeviceSelection} that the device configuration holder has.
     */
    public IDeviceSelection getDeviceRequirements();

    /**
     * Return a shallow copy of this {@link IDeviceConfiguration} object.
     */
    public IDeviceConfiguration clone();
}
