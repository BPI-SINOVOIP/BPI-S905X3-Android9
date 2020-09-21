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

import junit.framework.TestCase;

import org.easymock.EasyMock;
import org.easymock.IMocksControl;

import java.io.File;

public class SystemUpdaterDeviceFlasherTest extends TestCase {

    private static final String A_BUILD_ID = "1";

    private static final String TEST_STRING = "foo";

    private SystemUpdaterDeviceFlasher mFlasher;

    private IMocksControl mControl;

    private IDeviceBuildInfo mMockDeviceBuild;

    private ITestDevice mMockDevice;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        ITestsZipInstaller mockZipInstaller = EasyMock.createMock(ITestsZipInstaller.class);
        mFlasher = new SystemUpdaterDeviceFlasher();
        mFlasher.setTestsZipInstaller(mockZipInstaller);
        mControl = EasyMock.createStrictControl();
        mControl.checkOrder(false);
        mMockDevice = mControl.createMock(ITestDevice.class);
        mMockDeviceBuild = mControl.createMock(IDeviceBuildInfo.class);
        EasyMock.expect(mMockDevice.getSerialNumber()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockDevice.getProductType()).andStubReturn(TEST_STRING);
        EasyMock.expect(mMockDevice.getDeviceDescriptor()).andStubReturn(null);
    }

    public void testFlash() throws DeviceNotAvailableException, TargetSetupError {
        yieldDifferentBuilds(true);
        File fakeImage = new File("fakeImageFile");
        mControl.checkOrder(true);
        EasyMock.expect(mMockDeviceBuild.getOtaPackageFile()).andReturn(fakeImage);
        EasyMock.expect(mMockDevice.pushFile(fakeImage, "/cache/update.zip")).andReturn(true);
        String commandsRegex = "echo +--update_package +> +/cache/recovery/command +&& *"
                + "echo +/cache/update.zip +>> +/cache/recovery/command";
        EasyMock.expect(mMockDevice.executeShellCommand(EasyMock.matches(commandsRegex))).andReturn(
                "foo");
        mMockDevice.rebootIntoRecovery();
        mMockDevice.waitForDeviceAvailable();
        mMockDevice.reboot();

        mControl.replay();
        mFlasher.flash(mMockDevice, mMockDeviceBuild);
        mControl.verify();
    }

    public void testFlash_noOta() throws DeviceNotAvailableException {
        yieldDifferentBuilds(true);
        EasyMock.expect(mMockDeviceBuild.getOtaPackageFile()).andReturn(null);

        mControl.replay();
        try {
            mFlasher.flash(mMockDevice, mMockDeviceBuild);
            fail("didn't throw expected exception when OTA is missing: "
                    + TargetSetupError.class.getSimpleName());
        } catch (TargetSetupError e) {
            assertTrue(true);
        } finally {
            mControl.verify();
        }
    }

    public void testFlashSameBuild() throws DeviceNotAvailableException, TargetSetupError {
        yieldDifferentBuilds(false);
        mControl.replay();
        mFlasher.flash(mMockDevice, mMockDeviceBuild);
        mControl.verify();
    }

    private void yieldDifferentBuilds(boolean different) throws DeviceNotAvailableException {
        EasyMock.expect(mMockDevice.getBuildId()).andReturn(A_BUILD_ID).anyTimes();
        EasyMock.expect(mMockDeviceBuild.getDeviceBuildId()).andReturn(
                (different ? A_BUILD_ID + 1 : A_BUILD_ID)).anyTimes();
    }
}
