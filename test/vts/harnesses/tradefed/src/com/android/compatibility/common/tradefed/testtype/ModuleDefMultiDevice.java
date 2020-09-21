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
import com.android.tradefed.config.ConfigurationDescriptor;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/**
 * Container for Compatibility test module info.
 */
public class ModuleDefMultiDevice extends ModuleDef implements IMultiDeviceTest {
    private Map<ITestDevice, IBuildInfo> mDeviceInfos = null;
    private List<IMultiTargetPreparer> mMultiPreparers = new ArrayList<>();

    public ModuleDefMultiDevice(String name, IAbi abi, IRemoteTest test,
            List<ITargetPreparer> preparersSingleDevice, List<IMultiTargetPreparer> preparers,
            ConfigurationDescriptor configurationDescriptor) {
        super(name, abi, test, preparersSingleDevice, configurationDescriptor);

        boolean hasAbiReceiver = false;
        for (IMultiTargetPreparer preparer : preparers) {
            if (preparer instanceof IAbiReceiver) {
                hasAbiReceiver = true;
                break;
            }
        }
        for (ITargetPreparer preparer : preparersSingleDevice) {
            if (preparer instanceof IAbiReceiver) {
                hasAbiReceiver = true;
                break;
            }
        }

        mMultiPreparers = preparers;

        // Required interfaces:
        super.checkRequiredInterfaces(hasAbiReceiver);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceInfos = deviceInfos;
    }

    /**
     * Getter method for mDeviceInfos.
     */
    public Map<ITestDevice, IBuildInfo> getDeviceInfos() {
        return mDeviceInfos;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void prepareTestClass() {
        super.prepareTestClass();

        IRemoteTest test = getTest();
        if (test instanceof IMultiDeviceTest) {
            ((IMultiDeviceTest) test).setDeviceInfos(mDeviceInfos);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void runPreparerSetups() throws DeviceNotAvailableException {
        super.runPreparerSetups();

        for (IMultiTargetPreparer preparer : mMultiPreparers) {
            String preparerName = preparer.getClass().getCanonicalName();
            if (!getPreparerWhitelist().isEmpty()
                    && !getPreparerWhitelist().contains(preparerName)) {
                CLog.w("Skipping Preparer: %s since it is not in the whitelist %s", preparerName,
                        getPreparerWhitelist());
                return;
            }

            CLog.d("MultiDevicePreparer: %s", preparer.getClass().getSimpleName());

            if (preparer instanceof IAbiReceiver) {
                ((IAbiReceiver) preparer).setAbi(getAbi());
            }

            try {
                preparer.setUp(getInvocationContext());
            } catch (BuildError e) {
                // This should only happen for flashing new build
                CLog.e("Unexpected BuildError from multi-device preparer: %s",
                        preparer.getClass().getCanonicalName());
                throw new RuntimeException(e);
            } catch (TargetSetupError e) {
                // log preparer class then rethrow & let caller handle
                CLog.e("TargetSetupError in multi-device preparer: %s",
                        preparer.getClass().getCanonicalName());
                throw new RuntimeException(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        super.run(listener);

        // Tear down
        for (int i = mMultiPreparers.size() - 1; i >= 0; i--) {
            IMultiTargetPreparer cleaner = mMultiPreparers.get(i);
            CLog.d("MultiDeviceCleaner: %s", cleaner.getClass().getSimpleName());
            cleaner.tearDown(getInvocationContext(), null);
        }
    }
}
