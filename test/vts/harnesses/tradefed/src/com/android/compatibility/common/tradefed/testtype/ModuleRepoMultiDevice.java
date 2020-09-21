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

package com.android.compatibility.common.tradefed.testtype;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Retrieves Compatibility multi-device test module definitions from the repository.
 */
public class ModuleRepoMultiDevice
        extends ModuleRepo implements IMultiDeviceTest, IInvocationContextReceiver {
    private Map<ITestDevice, IBuildInfo> mDeviceInfos = null;
    private IInvocationContext mInvocationContext = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceInfos = deviceInfos;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mInvocationContext = invocationContext;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void addModuleDef(String name, IAbi abi, IRemoteTest test, String[] configPaths)
            throws ConfigurationException {
        // Invokes parser to process the test module config file
        IConfiguration config = getConfigFactory().createConfigurationFromArgs(configPaths);

        List<ITargetPreparer> preparers = new ArrayList<ITargetPreparer>();

        if (mDeviceInfos.size() <= 1) {
            preparers = config.getTargetPreparers();
        }

        addModuleDef(new ModuleDefMultiDevice(name, abi, test, preparers,
                config.getMultiTargetPreparers(), config.getConfigurationDescription()));
    }
}
