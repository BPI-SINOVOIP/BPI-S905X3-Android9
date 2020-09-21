/*
 * Copyright (C) 2017 The Android Open Source Project
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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit Tests for {@link DeviceStorageFiller}. */
@RunWith(JUnit4.class)
public class DeviceStorageFillerTest {
    private DeviceStorageFiller mDeviceStorageFiller;
    private ITestDevice mMockDevice;
    private IBuildInfo mMockBuildInfo;

    @Before
    public void setUp() {
        mDeviceStorageFiller = new DeviceStorageFiller();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = EasyMock.createMock(IBuildInfo.class);
    }

    @Test
    public void testSetUpWriteFile() throws Exception {
        OptionSetter optionSetter = new OptionSetter(mDeviceStorageFiller);
        optionSetter.setOptionValue("free-bytes", "24");
        optionSetter.setOptionValue("partition", "/p");
        optionSetter.setOptionValue("file-name", "f");

        EasyMock.expect(mMockDevice.getPartitionFreeSpace("/p")).andReturn(1L).once();
        EasyMock.expect(mMockDevice.executeShellCommand("fallocate -l 1000 /p/f"))
                .andReturn(null)
                .once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mDeviceStorageFiller.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockDevice);
    }

    @Test
    public void testSetUpSkip() throws Exception {
        OptionSetter optionSetter = new OptionSetter(mDeviceStorageFiller);
        optionSetter.setOptionValue("free-bytes", "2000");
        optionSetter.setOptionValue("partition", "/p");
        optionSetter.setOptionValue("file-name", "f");
        EasyMock.expect(mMockDevice.getPartitionFreeSpace("/p")).andReturn(1L).once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mDeviceStorageFiller.setUp(mMockDevice, mMockBuildInfo);
        EasyMock.verify(mMockBuildInfo, mMockDevice);
    }

    @Test
    public void testTearDown() throws Exception {
        OptionSetter optionSetter = new OptionSetter(mDeviceStorageFiller);
        optionSetter.setOptionValue("free-bytes", "24");
        optionSetter.setOptionValue("partition", "/p");
        optionSetter.setOptionValue("file-name", "f");
        EasyMock.expect(mMockDevice.executeShellCommand("rm -f /p/f")).andReturn(null).once();
        EasyMock.replay(mMockDevice, mMockBuildInfo);

        mDeviceStorageFiller.tearDown(mMockDevice, mMockBuildInfo, null);
        EasyMock.verify(mMockBuildInfo, mMockDevice);
    }
}
