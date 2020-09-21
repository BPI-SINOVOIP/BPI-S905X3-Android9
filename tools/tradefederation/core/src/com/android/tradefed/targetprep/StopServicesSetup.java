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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.ArrayList;
import java.util.Collection;

/** A {@link ITargetPreparer} that stops services on the device. */
public class StopServicesSetup extends BaseTargetPreparer {

    @Option(name = "stop-framework", description = "stop the framework. Default true.")
    private boolean mStopFramework = true;

    @Option(name = "service", description = "the service to stop on the device. Can be repeated.")
    private Collection<String> mServices = new ArrayList<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws DeviceNotAvailableException {
        if (mStopFramework) {
            device.executeShellCommand("stop");
        }

        for (String service : mServices) {
            device.executeShellCommand(String.format("stop %s", service));
        }
    }

    /**
     * Specify whether to stop the framework.
     */
    public void setStopFramework(boolean stopFramework) {
        mStopFramework = stopFramework;
    }

    /**
     * Adds a service to the list of services.
     */
    public void addService(String service) {
        mServices.add(service);
    }
}
