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

import com.android.ddmlib.NullOutputReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.concurrent.TimeUnit;

/**
 * A {@link IMetricCollector} that collects traces during the test using trace-cmd, and logs them to
 * the invocation.
 *
 * <p>This trace collector allows for USB disconnection during the test (as in power testing).
 *
 * <p>The system default tool, atrace, is used in tandem with this collector to set the
 * android-specific sysfs flags.
 *
 * <p>A trace-cmd (https://git.kernel.org/pub/scm/linux/kernel/git/rostedt/trace-cmd.git) binary
 * compatible with Android must be specified.
 *
 * <p>This will upload the trace.dat format (see man 5 trace-cmd.dat) produced by trace-cmd.
 */
@OptionClass(alias = "trace-cmd")
public class TraceCmdCollector extends AtraceCollector {
    /* Params for use of trace-cmd binary
     * The upstream tool, trace-cmd (https://git.kernel.org/pub/scm/linux/kernel/git/rostedt/trace-cmd.git)
     * will scrape the kernel buffers to disk at a configurable interval.
     * This is advantageous because:
     * 1) a packed format is produced, using less disk usage.
     * 2) no conversion of packed data to human-readable text occurs, resulting in less runtime
     *    processing overhead.
     * 3) an arbitrarily large file can be produced.
     * 4) wakeups can be scheduled at a large enough interval to minimize side-effects.
     *
     * Since its not a default system tool, a trace-cmd binary compatible with the target
     * device must be pushed to the device, and specified with --trace-cmd-binary
     */
    @Option(
        name = "trace-cmd-binary",
        description = "The path on the device of the trace-cmd binary to use"
    )
    private String mTraceCmdBinary = null;

    /* 'trace-cmd record' will be issued with these args.
     * the valid arguments are described in 'man 1 trace-cmd-record' (https://linux.die.net/man/1/trace-cmd-record)
     * The default is configured to wake up and spill to disk a 10s interval (-s), and use the boot clock.
     */
    @Option(
        name = "trace-cmd-recording-args",
        description = "The flags to pass to 'trace-cmd record'"
    )
    private String mTraceCmdRecordArgs =
            "-e sched:sched_waking "
                    + "-e sched:sched_wakeup -e sched:sched_wakeup_new "
                    + "-e sched:sched_switch -e power:cpu_idle "
                    + "-e power:cpu_frequency -e power:cpu_frequency_limits "
                    + "-e power:suspend_resume -e power:clock_set_rate "
                    + "-e power:clock_enable -e power:clock_disable "
                    + "-b 12000 -C boot -s 10000000 ";

    @Override
    protected LogDataType getLogType() {
        return LogDataType.KERNEL_TRACE;
    }

    @Override
    protected void startTracing(ITestDevice device) {
        if (mTraceCmdBinary == null) {
            CLog.w("--trace-cmd-binary was not set, skipping trace metric collection");
            return;
        }
        /* trace-cmd will periodically scrape the kernel buffer to disk at a configurable rate.
         * No-raw-buffer -> human-readable-text will be performed by the kernel either.
         * The events set by atrace should still stay enabled.
         */
        super.startTracing(device);

        StringBuilder traceCmd = new StringBuilder(100);
        traceCmd.append("nohup ");
        traceCmd.append(mTraceCmdBinary);
        traceCmd.append(" record -o ");
        traceCmd.append(fullLogPath());
        traceCmd.append(" ");
        traceCmd.append(mTraceCmdRecordArgs);
        traceCmd.append(" > /dev/null 2>&1 &");
        CLog.i("Issuing trace-cmd: %s ", traceCmd.toString());
        CollectingOutputReceiver c = new CollectingOutputReceiver();
        try {
            device.executeShellCommand("chmod +x " + mTraceCmdBinary, c, 1, TimeUnit.SECONDS, 1);
            device.executeShellCommand(traceCmd.toString(), c, 1, TimeUnit.SECONDS, 1);
        } catch (DeviceNotAvailableException e) {
            CLog.e("Error starting trace-cmd:");
            CLog.e(e);
        }
    }

    @Override
    protected void stopTracing(ITestDevice device) {
        if (mTraceCmdBinary == null) {
            CLog.w("trace-cmd was not set, skipping attempt to stop trace collection");
            return;
        }

        try {
            // sigterm the trace process and monitor for the process's demise.
            // Failure to wait can result in a partial log being pulled.
            // Since it was started with nohup in a separate shell, trace-cmd is not a child
            // of this shell, and 'wait' won't work.
            CLog.i("Collecting trace-cmd log from device: " + device.getSerialNumber());
            device.executeShellCommand(
                    "for PID in $(pidof trace-cmd); "
                            + "do while kill -s sigint $PID; do sleep 0.3; done; done;",
                    new NullOutputReceiver(),
                    60,
                    TimeUnit.SECONDS,
                    1);

        } catch (DeviceNotAvailableException e) {
            CLog.e("Error stopping atrace");
            CLog.e(e);
        }
    }
}
