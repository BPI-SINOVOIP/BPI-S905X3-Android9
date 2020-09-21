/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import junit.framework.TestCase;

import org.easymock.EasyMock;

/**
 * Unit tests for {@link DeviceBuildDescriptor}.
 */
public class DeviceBuildDescriptorTest extends TestCase {

    public void testDeviceBuildDescriptor() throws DeviceNotAvailableException {
        BuildInfo b = new BuildInfo();
        ITestDevice d = EasyMock.createNiceMock(ITestDevice.class);
        EasyMock.expect(d.getProperty("ro.product.name")).andReturn("yakju");
        EasyMock.expect(d.getProperty("ro.build.type")).andReturn("userdebug");
        EasyMock.expect(d.getProperty("ro.product.brand")).andReturn("google");
        EasyMock.expect(d.getProperty("ro.product.model")).andReturn("Galaxy Nexus");
        EasyMock.expect(d.getProperty("ro.build.version.release")).andReturn("4.2");
        EasyMock.replay(d);
        DeviceBuildDescriptor.injectDeviceAttributes(d, b);
        DeviceBuildDescriptor db = new DeviceBuildDescriptor(b);
        assertEquals("yakju-userdebug", db.getDeviceBuildFlavor());
        assertEquals("Google Galaxy Nexus 4.2", db.getDeviceUserDescription());
    }
}
