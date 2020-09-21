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
package com.android.tradefed.result;

import static org.junit.Assert.*;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Test for {@link DeviceUnavailEmailResultReporter}.
 */
@RunWith(JUnit4.class)
public class DeviceUnavailEmailResultReporterTest {

    private DeviceUnavailEmailResultReporter mDnaeEmailReporter;

    @Before
    public void setUp() {
        mDnaeEmailReporter = new DeviceUnavailEmailResultReporter() {
            @Override
            String getHostname() {
                return "FAKE_HOST.com";
            }
        };
    }

    @Test
    public void testGenerateEmailSubject() {
        IInvocationContext context = new InvocationContext();
        context.addAllocatedDevice("fakeDevice", EasyMock.createMock(ITestDevice.class));
        context.addDeviceBuildInfo("fakeDevice", new BuildInfo("888", "mybuild"));
        mDnaeEmailReporter.invocationStarted(context);
        assertEquals("Device unavailable BuildInfo{bid=888, target=mybuild} hostname FAKE_HOST.com",
                mDnaeEmailReporter.generateEmailSubject());
    }
}
