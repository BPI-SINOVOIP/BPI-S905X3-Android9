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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Allows for running tearDown on preparers that are included in a config as an object.
 *
 * <p>When a preparer is included as an object in a config rather than an {@link ITargetPreparer},
 * it's tearDown will not be called. Referencing it from this preparer will make sure that it's
 * tearDown will be called.
 */
public class TearDownPassThroughPreparer extends BaseTargetPreparer
        implements IConfigurationReceiver, ITargetCleaner {
    private IConfiguration mConfiguration;

    @Option(name = "preparer", description = "names of preparers to tearDown")
    private Collection<String> mPreparers = new ArrayList<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfiguration = configuration;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) {
        //setUp is taken care of elsewhere
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        for (String preparer : mPreparers) {
            Object configObject = mConfiguration.getConfigurationObject(preparer);
            if (configObject instanceof ITargetCleaner) {
                ITargetCleaner cleaner = (ITargetCleaner)configObject;
                cleaner.tearDown(device, buildInfo, e);
            }
            if (configObject instanceof IHostCleaner) {
                IHostCleaner cleaner = (IHostCleaner)configObject;
                cleaner.cleanUp(buildInfo, e);
            }
        }
    }
}
