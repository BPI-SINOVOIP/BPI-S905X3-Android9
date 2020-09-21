/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IRemoteTest;

/**
 * Empty implementation use by {@link ConfigurationFactoryTest#testCreateConfigurationFromArgs_includeConfig()}.
 */
public class StubOptionTest implements IRemoteTest {

    @Option(name = "option", description = "desc")
    public String mOption = "default";

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // ignore
    }
}
