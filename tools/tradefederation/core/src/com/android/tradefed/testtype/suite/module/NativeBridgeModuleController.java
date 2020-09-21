/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tradefed.testtype.suite.module;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.StubDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.AbiUtils;

/**
 * A module controller to check if a device support native bridge. If it does support it and we
 * detect a native bridge situation the module will be skipped.
 */
public class NativeBridgeModuleController extends BaseModuleController {

    public static final String NATIVE_BRIDGE_PROP = "ro.dalvik.vm.native.bridge";

    @Override
    public RunStrategy shouldRun(IInvocationContext context) {
        for (ITestDevice device : context.getDevices()) {
            if (device.getIDevice() instanceof StubDevice) {
                continue;
            }
            try {
                String bridge = device.getProperty(NATIVE_BRIDGE_PROP);
                if ("0".equals(bridge.trim())) {
                    continue;
                }
                IAbi moduleAbi = getModuleAbi();
                String moduleArch = AbiUtils.getBaseArchForAbi(moduleAbi.getName());

                String primaryAbiString = AbiFormatter.getDefaultAbi(device, "");
                String deviceArch = AbiUtils.getBaseArchForAbi(primaryAbiString.trim());
                if (deviceArch.equals(moduleArch)) {
                    continue;
                }
                // If the current architecture of the module does not match the primary architecture
                // of the device then we are in a native bridge situation and should skip the module
                CLog.d(
                        "Skipping module %s to avoid running the tests against the native bridge.",
                        getModuleName());
                return RunStrategy.FULL_MODULE_BYPASS;
            } catch (DeviceNotAvailableException e) {
                CLog.e("Couldn't check native bridge on %s", device.getSerialNumber());
                CLog.e(e);
            }
        }
        return RunStrategy.RUN;
    }
}
