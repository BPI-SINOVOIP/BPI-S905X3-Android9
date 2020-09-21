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

package com.android.tradefed.device.metric;

import com.android.annotations.VisibleForTesting;
import com.android.loganalysis.item.CompactMemInfoItem;
import com.android.loganalysis.parser.CompactMemInfoParser;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.FileUtil;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A {@link ScheduledDeviceMetricCollector} to collect memory dumps of processes at regular
 * intervals.
 */
public class DumpHeapCollector extends ScheduledDeviceMetricCollector {
    private static final String DUMPHEAP_OUTPUT = "/data/local/tmp";
    private static final String SUFFIX = "trigger";

    @Option(
        name = "dumpheap-thresholds",
        description =
                "Threshold map for taking process dumpheaps. "
                        + "The key should be the process name and its corresponding value is the "
                        + "maximum acceptable heap size for that process."
                        + "Note that to get heap dump for native and managed processes set their "
                        + "threshold to 0."
    )
    protected Map<String, Long> mDumpheapThresholds = new HashMap<String, Long>();

    @Override
    public void collect(ITestDevice device, DeviceMetricData runData) throws InterruptedException {
        CLog.i("Running dumpheap collection...");
        List<File> dumpFiles = new ArrayList<>();
        try {
            for (String process : mDumpheapThresholds.keySet()) {
                String output =
                        device.executeShellCommand(
                                String.format("dumpsys meminfo -c | grep %s", process));

                dumpFiles = takeDumpheap(device, output, process, mDumpheapThresholds.get(process));

                dumpFiles.forEach(dumpheap -> saveDumpheap(dumpheap));
            }
        } catch (DeviceNotAvailableException e) {
            CLog.e(e);
        } finally {
            dumpFiles.forEach(dumpFile -> FileUtil.deleteFile(dumpFile));
        }
    }

    /**
     * Collects heap dump for each requested process if the PSS is greater than a threshold.
     *
     * @param device
     * @param output of the meminfo command.
     * @param process for which we need the heap dump.
     * @param threshold which is the maximum tolerable PSS.
     * @return the list of {@link File}s in the host containing the report. Empty list if something
     *     failed.
     * @throws DeviceNotAvailableException
     */
    @VisibleForTesting
    List<File> takeDumpheap(ITestDevice device, String output, String process, Long threshold)
            throws DeviceNotAvailableException {
        List<File> dumpFiles = new ArrayList<>();
        if (output.isEmpty()) {
            CLog.i("Skipping %s -- no process found.", process);
            return dumpFiles;
        }

        CompactMemInfoItem item =
                new CompactMemInfoParser().parse(Arrays.asList(output.split("\n")));

        for (Integer pid : item.getPids()) {
            if (item.getName(pid).equals(process) && item.getPss(pid) > threshold) {
                File dump = device.dumpHeap(process, getDevicePath(process));
                dumpFiles.add(dump);
            }
        }
        return dumpFiles;
    }

    /**
     * Returns the path on the device to put the dump.
     *
     * @param process for which dump is being requested.
     * @return a write-able path in device.
     */
    private String getDevicePath(String process) {
        return String.format(
                "%s/%s_%s_%s.hprof", DUMPHEAP_OUTPUT, process, SUFFIX, getFileSuffix());
    }

    @VisibleForTesting
    void saveDumpheap(File dumpheap) {
        if (dumpheap == null) {
            CLog.e("Failed to take dumpheap.");
            return;
        }
        try (FileInputStreamSource stream = new FileInputStreamSource(dumpheap)) {
            getInvocationListener()
                    .testLog(FileUtil.getBaseName(dumpheap.getName()), LogDataType.HPROF, stream);
        }
    }
}
