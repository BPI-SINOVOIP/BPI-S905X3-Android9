/*
 * Copyright (C) 2011 The Android Open Source Project
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


package com.android.tradefed.targetprep;

import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.Collection;

/**
 * Installs tests from a tests zip file (as outputted by the build system) on
 * a device.
 */
public interface ITestsZipInstaller {

    /**
     * Pushes the contents of the tests.zip file onto the device's data partition.
     *
     * @param device the {@link ITestDevice} to flash, assumed to be in adb mode.
     * @param deviceBuild the {@link IDeviceBuildInfo} that contains the tests zip to flash
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError
     */
    public void pushTestsZipOntoData(ITestDevice device, IDeviceBuildInfo deviceBuild)
            throws DeviceNotAvailableException, TargetSetupError;

    /**
     * Sets the list of paths under {@code /data} to avoid clearing.
     *
     * @param skipList the list of directories to skip.
     * <p />
     * Note that the granularity of the skip list is direct children of {@code /data}.
     *
     * @see #deleteData
     */
    public void setDataWipeSkipList(Collection<String> skipList);

    /**
     * Sets the list of paths under {@code /data} to avoid clearing.
     *
     * @param skipList the list of directories to skip.
     * <p />
     * Note that the granularity of the skip list is direct children of {@code /data}.
     *
     * @see #deleteData
     */
    public void setDataWipeSkipList(String... skipList);

    /**
     * Removes all of the files/directories from {@code /data} on the specified device, with the
     * exception of those excluded by the skip list.
     * <p/>
     * Implementation will stop the runtime on device. It is highly recommended to reboot the device
     * upon completion of this method.
     *
     * @param device The {@link ITestDevice} to act on
     * @see #setDataWipeSkipList
     */
    public void deleteData(ITestDevice device) throws DeviceNotAvailableException, TargetSetupError;

}
