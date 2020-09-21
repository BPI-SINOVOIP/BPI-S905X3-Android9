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

package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

/** Target preparer to fill up storage so some amount of free space is available. */
@OptionClass(alias = "fill-storage")
public class DeviceStorageFiller extends BaseTargetPreparer implements ITargetCleaner {

    @Option(name = "partition", description = "Partition to check storage")
    private String mPartition = "/data";

    @Option(
        name = "file-name",
        description = "Name of file to use to fill up storage, relative to partition"
    )
    private String mFileName = "bigfile";

    @Option(
        name = "free-bytes",
        description = "Number of bytes that should be left free on the device."
    )
    private long mFreeBytesRequested = 100L * 1024 * 1024 * 1024; // 100GB

    private String getFullFileName() {
        return String.format("%s/%s", mPartition, mFileName);
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        long freeSpace = device.getPartitionFreeSpace(mPartition) * 1024;
        if (freeSpace > mFreeBytesRequested) {
            String fileName = getFullFileName();
            long bytesToWrite = freeSpace - mFreeBytesRequested;
            device.executeShellCommand(String.format("fallocate -l %d %s", bytesToWrite, fileName));
            CLog.i("Wrote %d bytes to %s", bytesToWrite, fileName);
        } else {
            CLog.i(
                    "Not enough free space (%d bytes requested free, %d bytes actually free)",
                    mFreeBytesRequested, freeSpace);
        }
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        String fileName = getFullFileName();
        device.executeShellCommand(String.format("rm -f %s", fileName));
    }
}
