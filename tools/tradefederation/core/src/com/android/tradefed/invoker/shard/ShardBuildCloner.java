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
package com.android.tradefed.invoker.shard;

import com.android.tradefed.build.ExistingBuildProvider;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;

/**
 * Helper class that handles cloning a build info from the command line. Shard will get the build
 * info directly by using {@link ExistingBuildProvider} instead of downloading anything.
 */
public class ShardBuildCloner {

    /**
     * Helper to set the Sharded configuration build provider to the {@link ExistingBuildProvider}.
     *
     * @param fromConfig Original configuration
     * @param toConfig cloned configuration recreated from the command line.
     * @param context invocation context
     */
    public static void cloneBuildInfos(
            IConfiguration fromConfig, IConfiguration toConfig, IInvocationContext context) {
        for (String deviceName : context.getDeviceConfigNames()) {
            IBuildInfo toBuild = context.getBuildInfo(deviceName).clone();
            try {
                toConfig.getDeviceConfigByName(deviceName)
                        .addSpecificConfig(
                                new ExistingBuildProvider(
                                        toBuild,
                                        fromConfig
                                                .getDeviceConfigByName(deviceName)
                                                .getBuildProvider()));
            } catch (ConfigurationException e) {
                // Should never happen, no action taken
                CLog.e(e);
            }
        }
    }
}
