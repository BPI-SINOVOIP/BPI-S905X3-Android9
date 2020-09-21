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
package com.android.tradefed.build;

import static org.junit.Assert.*;

import com.android.ddmlib.IDevice;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link BootstrapBuildProvider}. */
@RunWith(JUnit4.class)
public class BootstrapBuildProviderTest {
    private BootstrapBuildProvider mProvider;
    private ITestDevice mMockDevice;

    @Before
    public void setUp() {
        mProvider = new BootstrapBuildProvider();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
    }

    @Test
    public void testGetBuild() throws Exception {
        EasyMock.expect(mMockDevice.getBuildId()).andReturn("5");
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(EasyMock.createMock(IDevice.class));
        EasyMock.expect(mMockDevice.waitForDeviceShell(EasyMock.anyLong())).andReturn(true);
        EasyMock.expect(mMockDevice.getProperty(EasyMock.anyObject())).andStubReturn("property");
        EasyMock.expect(mMockDevice.getProductVariant()).andStubReturn("variant");
        EasyMock.expect(mMockDevice.getBuildFlavor()).andStubReturn("flavor");
        EasyMock.expect(mMockDevice.getBuildAlias()).andStubReturn("alias");
        EasyMock.replay(mMockDevice);
        IBuildInfo res = mProvider.getBuild(mMockDevice);
        assertNotNull(res);
        try {
            assertTrue(res instanceof IDeviceBuildInfo);
            // Ensure tests dir is never null
            assertTrue(((IDeviceBuildInfo) res).getTestsDir() != null);
            EasyMock.verify(mMockDevice);
        } finally {
            mProvider.cleanUp(res);
        }
    }

    /**
     * Test that when using the provider with a StubDevice information that cannot be queried are
     * stubbed.
     */
    @Test
    public void testGetBuild_stubDevice() throws Exception {
        EasyMock.expect(mMockDevice.getBuildId()).andReturn("5");
        EasyMock.expect(mMockDevice.getIDevice()).andReturn(new StubDevice("serial"));
        EasyMock.expect(mMockDevice.getBuildFlavor()).andStubReturn("flavor");
        EasyMock.expect(mMockDevice.getBuildAlias()).andStubReturn("alias");
        EasyMock.replay(mMockDevice);
        IBuildInfo res = mProvider.getBuild(mMockDevice);
        assertNotNull(res);
        try {
            assertTrue(res instanceof IDeviceBuildInfo);
            // Ensure tests dir is never null
            assertTrue(((IDeviceBuildInfo) res).getTestsDir() != null);
            assertEquals("stub", res.getBuildBranch());
            EasyMock.verify(mMockDevice);
        } finally {
            mProvider.cleanUp(res);
        }
    }
}
