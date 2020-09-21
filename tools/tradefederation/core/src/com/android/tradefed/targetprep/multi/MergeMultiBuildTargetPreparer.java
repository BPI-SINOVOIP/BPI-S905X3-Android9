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
package com.android.tradefed.targetprep.multi;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;

import java.util.HashSet;
import java.util.Set;

/**
 * A {@link IMultiTargetPreparer} that allows to pass information from one build to another by
 * naming them and the file key to copy to the other build. If the key already exists in the
 * destination build, then nothing is done.
 */
@OptionClass(alias = "multi-build-merge")
public final class MergeMultiBuildTargetPreparer extends BaseMultiTargetPreparer {

    @Option(
        name = "src-device",
        description =
                "The name of the device that will provide the files. As specified in the "
                        + "<device> tag of the configuration.",
        mandatory = true
    )
    private String mProvider;

    @Option(
        name = "dest-device",
        description =
                "The name of the device that will receive the files. As specified in the "
                        + "<device> tag of the configuration.",
        mandatory = true
    )
    private String mReceiver;

    @Option(name = "key-to-copy", description = "The name of the files that needs to be moved.")
    private Set<String> mKeysToMove = new HashSet<>();

    @Override
    public void setUp(IInvocationContext context)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        // First assert that the two device are found
        IBuildInfo providerInfo = context.getBuildInfo(mProvider);
        if (providerInfo == null) {
            // Use the first device descriptor since if the build is missing we are probably not
            // going to find the device.
            throw new TargetSetupError(
                    String.format("Could not find a build associated with '%s'", mProvider),
                    context.getDevices().get(0).getDeviceDescriptor());
        }
        IBuildInfo receiverInfo = context.getBuildInfo(mReceiver);
        if (receiverInfo == null) {
            // Use the first device descriptor since if the build is missing we are probably not
            // going to find the device.
            throw new TargetSetupError(
                    String.format("Could not find a build associated with '%s'", mReceiver),
                    context.getDevices().get(0).getDeviceDescriptor());
        }

        // Move the requested keys.
        for (String key : mKeysToMove) {
            VersionedFile toBeMoved = providerInfo.getVersionedFile(key);
            if (toBeMoved == null) {
                CLog.w("Key '%s' did not match any files, ignoring.", key);
                continue;
            }
            receiverInfo.setFile(key, toBeMoved.getFile(), toBeMoved.getVersion());
        }
    }
}
