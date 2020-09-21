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
import com.android.tradefed.testtype.DeviceTestCase;

/**
 * Functional tests for {@link DeviceBuildDescriptor}.
 * <p/>
 * Sanity checks data can be retrieved off device.
 */
public class DeviceBuildDescriptorFuncTest extends DeviceTestCase {

    public void testDeviceBuildDescriptor() throws DeviceNotAvailableException {
        BuildInfo b = new BuildInfo();
        DeviceBuildDescriptor.injectDeviceAttributes(getDevice(), b);
        assertTrue(DeviceBuildDescriptor.describesDeviceBuild(b));
        DeviceBuildDescriptor d2 = new DeviceBuildDescriptor(b);
        assertNotNull(d2.getDeviceBuildId());
        assertNotNull(d2.getDeviceBuildFlavor());
        assertNotNull(d2.getDeviceUserDescription());
    }
}
