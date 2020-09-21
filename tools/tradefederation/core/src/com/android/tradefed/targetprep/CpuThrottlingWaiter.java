/*
 * Copyright (C) 2015 The Android Open Source Project
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
import com.android.tradefed.util.RunUtil;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * An {@link ITargetPreparer} that waits until max frequency on all cores are restored to highest
 * level available
 */
@OptionClass(alias = "cpu-throttle-waiter")
public class CpuThrottlingWaiter extends BaseTargetPreparer {

    @Option(name = "poll-interval",
            description = "Interval in seconds, to poll for core frequencies; defaults to 5s")
    private long mPollIntervalSecs = 5;

    @Option(name = "max-wait", description = "Max wait time in seconds, for all cores to be"
            + " free of throttling; defaults to 240s")
    private long mMaxWaitSecs = 240;

    @Option(name = "post-idle-wait", description = "Additional time to wait in seconds, after cores"
            + " are no longer subject to throttling; defaults to 120s")
    private long mPostIdleWaitSecs = 120;

    @Option(name = "abort-on-timeout", description = "If test should be aborted if cores are still"
            + " subject to throttling after timeout has reached; defaults to false")
    private boolean mAbortOnTimeout = false;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        // first figure out number of CPU cores available and their corresponding max frequencies
        // map: path/to/core : max frequency
        Map<String, String> cpuMaxFreqs = getCpuMaxFreqs(device);
        if (cpuMaxFreqs.isEmpty()) {
            CLog.i("Unable to determine cores available, falling back to max wait time");
            RunUtil.getDefault().sleep(mMaxWaitSecs * 1000);
            return;
        }
        // poll CPU frequencies
        long start = System.currentTimeMillis();
        long maxWaitMs = mMaxWaitSecs * 1000;
        long intervalMs = mPollIntervalSecs * 1000;
        while (true) {
            boolean ready = true;
            for (Entry<String, String> e : cpuMaxFreqs.entrySet()) {
                // check current frequency of each CPU
                String freq = device.executeShellCommand(
                        String.format("cat %s/cpuinfo_max_freq", e.getKey())).trim();
                if (!e.getValue().equals(freq)) {
                    // not ready
                    CLog.d("CPU %s not ready: %s/%s", e.getKey(), freq, e.getValue());
                    ready = false;
                    break; // for loop
                }
            }
            if (ready) {
                break; // while loop
            }
            if ((System.currentTimeMillis() - start) > maxWaitMs) {
                CLog.w("cores still throttled after %ds", maxWaitMs);
                String result = device.executeShellCommand(
                        "cat /sys/devices/system/cpu/*/cpufreq/cpuinfo_max_freq");
                CLog.w("Current CPU frequencies:\n%s", result);
                if (mAbortOnTimeout) {
                    throw new TargetSetupError("cores are still throttled after wait timeout",
                            device.getDeviceDescriptor());
                }
                break; // while loop
            }
            RunUtil.getDefault().sleep(intervalMs);
        }
        // extra idle time so that in case of thermal related throttling, allow heat to dissipate
        RunUtil.getDefault().sleep(mPostIdleWaitSecs * 1000);
        CLog.i("Done waiting, total time elapsed: %ds",
                (System.currentTimeMillis() - start) / 1000);
    }

    /**
     * Reads info under /sys/devices/system/cpu to determine cores available, and max frequencies
     * possible for each core
     * @param device device under test
     * @return a {@link Map} with paths to sysfs cpuinfo as key, and corresponding max frequency as
     *         value
     * @throws DeviceNotAvailableException
     */
    protected Map<String, String> getCpuMaxFreqs(ITestDevice device)
            throws DeviceNotAvailableException {
        Map<String, String> ret = new HashMap<>();
        String result = device.executeShellCommand("ls -1 -d /sys/devices/system/cpu/cpu*/cpufreq");
        String[] lines = result.split("\r?\n");
        List<String> cpuPaths = new ArrayList<>();
        for (String line : lines) {
            cpuPaths.add(line.trim());
        }
        for (String cpu : cpuPaths) {
            result = device.executeShellCommand(
                    String.format("cat %s/scaling_available_frequencies", cpu)).trim();
            // return should be something like:
            // 300000 422400 652800 729600 883200 960000 1036800 1190400 1267200 1497600 1574400
            String[] freqs = result.split("\\s+");
            String maxFreq = freqs[freqs.length - 1]; // highest available frequency is last
            // check if the string is a number
            if (!maxFreq.matches("^\\d+$")) {
                CLog.w("Unable to determine max frequency available for CPU: %s", cpu);
            } else {
                CLog.d("CPU: %s  MaxFreq: %s", cpu, maxFreq);
                ret.put(cpu, maxFreq);
            }
        }
        return ret;
    }
}
