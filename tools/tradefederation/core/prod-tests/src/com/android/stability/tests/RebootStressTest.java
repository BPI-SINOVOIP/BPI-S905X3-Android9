/*
 * Copyright (C) 2012 The Android Open Source Project
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
package com.android.stability.tests;

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * A test that reboots device many times, and reports successful iteration count.
 */
public class RebootStressTest implements IRemoteTest, IDeviceTest, IShardableTest {

    private static final String[] LAST_KMSG_PATHS = {
        "/sys/fs/pstore/console-ramoops-0",
        "/sys/fs/pstore/console-ramoops",
        "/proc/last_kmsg",
    };
    // max number of ms to allowed for the post-boot waitForDeviceAvailable check
    private static final long DEVICE_AVAIL_TIME = 3 * 1000;

    @Option(name = "iterations", description = "number of reboot iterations to perform")
    private int mIterations = 1;

    @Option(name = "shards", description = "Optional number of shards to split test into. " +
            "Iterations will be split evenly among shards.", importance = Importance.IF_UNSET)
    private Integer mShards = null;

    @Option(name = "run-name", description =
            "The test run name used to report metrics.")
    private String mRunName = "reboot-stress";

    @Option(name = "post-boot-wait-time", description =
            "Total number of seconds to wait between reboot attempts")
    private int mWaitTime = 10;

    @Option(name = "post-boot-poll-time", description =
            "Number of seconds to wait between device health checks")
    private int mPollSleepTime = 1;

    @Option(name = "wipe-data", description =
            "Flag to set if userdata should be wiped between reboots")
    private boolean mWipeUserData = false;

    @Option(name = "wipe-data-cmd", description =
            "the fastboot command(s) to use to wipe data and reboot device.")
    private Collection<String> mFastbootWipeCmds = new ArrayList<String>();

    @Option(name = "boot-into-bootloader", description =
            "Flag to set if reboot should enter bootloader")
    private boolean mBootIntoBootloader = true;

    private ITestDevice mDevice;
    private String mLastKmsg;
    private String mDmesg;

    /**
     * Set the run name
     */
    void setRunName(String runName) {
        mRunName = runName;
    }

    /**
     * Return the number of iterations.
     * <p/>
     * Exposed for unit testing
     */
    public int getIterations() {
        return mIterations;
    }

    /**
     * Set the iterations
     */
    void setIterations(int iterations) {
        mIterations = iterations;
    }

    /**
     * Set the number of shards
     */
    void setShards(int shards) {
        mShards = shards;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    void setWaitTime(int waitTimeSec) {
        mWaitTime = waitTimeSec;
    }

    void setPollTime(int pollTime) {
        mPollSleepTime = pollTime;

    }

    void setWipeData(boolean wipeData) {
        mWipeUserData = wipeData;
    }

    void setFastbootWipeDataCmds(Collection<String> cmds) {
        mFastbootWipeCmds.addAll(cmds);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<IRemoteTest> split() {
        if (mShards == null || mShards <= 1) {
            return null;
        }
        Collection<IRemoteTest> shards = new ArrayList<IRemoteTest>(mShards);
        int remainingIterations = mIterations;
        for (int i = mShards; i > 0; i--) {
            RebootStressTest testShard = new RebootStressTest();
            // device will be set by test invoker
            testShard.setRunName(mRunName);
            testShard.setWaitTime(mWaitTime);
            testShard.setWipeData(mWipeUserData);
            testShard.setFastbootWipeDataCmds(mFastbootWipeCmds);
            testShard.setPollTime(mPollSleepTime);
            // attempt to divide iterations evenly among shards with no remainder
            int iterationsForShard = Math.round(remainingIterations/i);
            if (iterationsForShard > 0) {
                testShard.setIterations(iterationsForShard);
                remainingIterations -= iterationsForShard;
                shards.add(testShard);
            }
        }
        return shards;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(getDevice());

        listener.testRunStarted(mRunName, 0);
        long startTime = System.currentTimeMillis();
        int actualIterations = 0;
        try {
            for (actualIterations = 0; actualIterations < mIterations; actualIterations++) {
                CLog.i("Reboot attempt %d of %d", actualIterations+1, mIterations);
                if (mBootIntoBootloader){
                    getDevice().rebootIntoBootloader();
                    if (mWipeUserData) {
                        executeFastbootWipeCommand();
                    } else {
                        getDevice().executeFastbootCommand("reboot");
                    }
                } else {
                    getDevice().reboot();
                }
                getDevice().waitForDeviceAvailable();
                doWaitAndCheck();
                CLog.logAndDisplay(LogLevel.INFO, "Device %s completed %d of %d iterations",
                        getDevice().getSerialNumber(), actualIterations+1, mIterations);
            }
        } finally {
            Map<String, String> metrics = new HashMap<String, String>(1);
            long durationMs = System.currentTimeMillis() - startTime;
            metrics.put("iterations", Integer.toString(actualIterations));
            metrics.put("shards", "1");
            listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
            if (mLastKmsg != null) {
                listener.testLog(String.format("last_kmsg_%s", getDevice().getSerialNumber()),
                        LogDataType.KERNEL_LOG,
                        new ByteArrayInputStreamSource(mLastKmsg.getBytes()));
            }
            if (mDmesg != null) {
                listener.testLog(String.format("dmesg_%s", getDevice().getSerialNumber()),
                        LogDataType.KERNEL_LOG, new ByteArrayInputStreamSource(mDmesg.getBytes()));
            }
            CLog.logAndDisplay(LogLevel.INFO, "Device %s completed %d of %d iterations",
                    getDevice().getSerialNumber(), actualIterations, mIterations);
        }
    }

    private void executeFastbootWipeCommand() throws DeviceNotAvailableException {
        if (mFastbootWipeCmds.isEmpty()) {
            // default to use fastboot -w reboot
            mFastbootWipeCmds.add("-w reboot");
        }
        for (String fastbootCmd : mFastbootWipeCmds) {
            CLog.i("Running '%s'", fastbootCmd);
            CommandResult result = getDevice().executeFastbootCommand(fastbootCmd.split(" "));
            Assert.assertEquals(result.getStderr(), CommandStatus.SUCCESS, result.getStatus());
        }
    }

    /**
     * Perform wait between reboots. Perform periodic checks on device to ensure is still available.
     *
     * @throws DeviceNotAvailableException
     */
    private void doWaitAndCheck() throws DeviceNotAvailableException {
        long waitTimeMs = mWaitTime * 1000;
        long elapsedTime = 0;

        while (elapsedTime < waitTimeMs) {
            long startTime = System.currentTimeMillis();
            // ensure device is still up
            getDevice().waitForDeviceAvailable(DEVICE_AVAIL_TIME);
            checkForUserDataFailure();
            RunUtil.getDefault().sleep(mPollSleepTime * 1000);
            elapsedTime += System.currentTimeMillis() - startTime;
        }
    }

    /**
     * Check logs for userdata formatting/mounting issues.
     *
     * @throws DeviceNotAvailableException
     */
    private void checkForUserDataFailure() throws DeviceNotAvailableException {
        for (String path : LAST_KMSG_PATHS) {
            if (getDevice().doesFileExist(path)) {
                mLastKmsg = getDevice().executeShellCommand("cat " + path);
                Assert.assertFalse(String.format("Last kmsg log showed a kernel panic on %s",
                        getDevice().getSerialNumber()), mLastKmsg.contains("Kernel panic"));
                break;
            }
        }
        mDmesg = getDevice().executeShellCommand("dmesg");
        Assert.assertFalse(String.format("Read only mount of userdata detected on %s",
                getDevice().getSerialNumber()),
                mDmesg.contains("Remounting filesystem read-only"));
    }
}
