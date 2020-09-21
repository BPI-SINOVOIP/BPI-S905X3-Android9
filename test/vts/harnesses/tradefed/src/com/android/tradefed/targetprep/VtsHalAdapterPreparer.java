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

package com.android.tradefed.targetprep;

import com.android.compatibility.common.tradefed.build.VtsCompatibilityInvocationHelper;
import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.util.CmdUtil;
import com.android.tradefed.util.FileUtil;

import org.junit.Assert;

import java.io.File;
import java.io.IOException;
import java.util.NoSuchElementException;
import java.util.Vector;
import java.util.function.Predicate;

/**
 * Starts and stops a HAL (Hardware Abstraction Layer) adapter.
 */
@OptionClass(alias = "vts-hal-adapter-preparer")
public class VtsHalAdapterPreparer
        implements ITargetPreparer, ITargetCleaner, IMultiTargetPreparer, IAbiReceiver {
    static final int THREAD_COUNT_DEFAULT = 1;

    static final String HAL_INTERFACE_SEP = "::";
    static final String HAL_INSTANCE_SEP = "/";
    // Relative path to vts native tests directory.
    static final String VTS_NATIVE_TEST_DIR = "DATA/nativetest%s/";
    // Path of native tests directory on target device.
    static final String TARGET_NATIVE_TEST_DIR = "/data/nativetest%s/";
    // Sysprop to stop HIDL adapaters. Currently, there's one global flag for all adapters.
    static final String ADAPTER_SYSPROP = "test.hidl.adapters.deactivated";
    // The wrapper script to start an adapter binary in the background.
    static final String SCRIPT_PATH = "/data/local/tmp/vts_adapter.sh";
    // Command to list the registered instance for the given hal@version.
    static final String LIST_HAL_CMD =
            "lshal -ti --neat | grep -E '^hwbinder' | awk '{print $2}' | grep %s";

    @Option(name = "adapter-binary-name",
            description = "Adapter binary file name (typically under /data/nativetest*/)")
    private String mAdapterBinaryName = null;

    @Option(name = "hal-package-name", description = "Target hal to adapter")
    private String mPackageName = null;

    @Option(name = "thread-count", description = "HAL adapter's thread count")
    private int mThreadCount = THREAD_COUNT_DEFAULT;

    // Application Binary Interface (ABI) info of the current test run.
    private IAbi mAbi = null;

    // CmdUtil help to verify the cmd results.
    private CmdUtil mCmdUtil = null;
    // Predicates to stop retrying cmd.
    private Predicate<String> mCheckEmpty = (String str) -> {
        return str.isEmpty();
    };
    private Predicate<String> mCheckNonEmpty = (String str) -> {
        return !str.isEmpty();
    };
    private Vector<String> mCommands = new Vector<String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo)
            throws TargetSetupError, BuildError, DeviceNotAvailableException, RuntimeException {
        String bitness =
                (mAbi != null) ? ((mAbi.getBitness() == "32") ? "" : mAbi.getBitness()) : "";
        try {
            pushAdapter(device, bitness);
        } catch (IOException | NoSuchElementException e) {
            CLog.e("Could not push adapter: " + e.toString());
            throw new TargetSetupError("Could not push adapter.");
        }

        mCmdUtil = mCmdUtil != null ? mCmdUtil : new CmdUtil();
        mCmdUtil.setSystemProperty(device, ADAPTER_SYSPROP, "false");

        String out = device.executeShellCommand(String.format(LIST_HAL_CMD, mPackageName));
        for (String line : out.split("\n")) {
            if (!line.isEmpty()) {
                if (!line.contains(HAL_INTERFACE_SEP)) {
                    throw new RuntimeException("HAL instance with wrong format.");
                }
                String interfaceInstance = line.split(HAL_INTERFACE_SEP, 2)[1];
                if (!interfaceInstance.contains(HAL_INSTANCE_SEP)) {
                    throw new RuntimeException("HAL instance with wrong format.");
                }
                String interfaceName = interfaceInstance.split(HAL_INSTANCE_SEP, 2)[0];
                String instanceName = interfaceInstance.split(HAL_INSTANCE_SEP, 2)[1];
                // starts adapter
                String command = String.format("chmod a+x %s", SCRIPT_PATH);
                mCommands.add(command);
                command = String.format("%s /data/nativetest%s/%s %s %s %d", SCRIPT_PATH, bitness,
                        mAdapterBinaryName, interfaceName, instanceName, mThreadCount);
                CLog.i("Trying to adapter for %s",
                        mPackageName + "::" + interfaceName + "/" + instanceName);
                mCommands.add(command);
            }
        }
        if (mCommands.isEmpty()) {
            CLog.w("The specific HAL service is not running.");
            return;
        }
        if (!mCmdUtil.retry(
                    device, mCommands, String.format(LIST_HAL_CMD, mPackageName), mCheckEmpty)) {
            throw new TargetSetupError("HAL adapter setup failed.");
        }

        mCmdUtil.restartFramework(device);
        if (!mCmdUtil.waitCmdResultWithDelay(
                    device, "service list | grep IPackageManager", mCheckNonEmpty)) {
            throw new TargetSetupError("Failed to start package service");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(IInvocationContext context)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        setUp(context.getDevices().get(0), context.getBuildInfos().get(0));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (!mCommands.isEmpty()) {
            // stops adapter(s)
            String command = String.format("setprop %s %s", ADAPTER_SYSPROP, "true");
            mCmdUtil = mCmdUtil != null ? mCmdUtil : new CmdUtil();
            Assert.assertTrue("HAL restore failed.",
                    mCmdUtil.retry(device, command, String.format(LIST_HAL_CMD, mPackageName),
                            mCheckNonEmpty, mCommands.size() + mCmdUtil.MAX_RETRY_COUNT));

            // TODO: cleanup the pushed adapter files.
            mCmdUtil.restartFramework(device);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(IInvocationContext context, Throwable e)
            throws DeviceNotAvailableException {
        tearDown(context.getDevices().get(0), context.getBuildInfos().get(0), e);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    /**
     * Push the required adapter binary to device.
     *
     * @param device device object.
     * @param bitness ABI bitness.
     * @throws DeviceNotAvailableException.
     * @throws IOException.
     * @throws NoSuchElementException.
     */
    private void pushAdapter(ITestDevice device, String bitness)
            throws DeviceNotAvailableException, IOException, NoSuchElementException {
        VtsCompatibilityInvocationHelper invocationHelper = createVtsHelper();
        File adapterDir = new File(
                invocationHelper.getTestsDir(), String.format(VTS_NATIVE_TEST_DIR, bitness));
        File adapter = FileUtil.findFile(adapterDir, mAdapterBinaryName);
        if (adapter != null) {
            CLog.i("Pushing %s", mAdapterBinaryName);
            device.pushFile(
                    adapter, String.format(TARGET_NATIVE_TEST_DIR, bitness) + mAdapterBinaryName);
        } else {
            throw new NoSuchElementException("Could not find adapter: " + mAdapterBinaryName);
        }
    }

    /**
     * Create and return a {@link VtsCompatibilityInvocationHelper} to use during the preparer.
     */
    @VisibleForTesting
    VtsCompatibilityInvocationHelper createVtsHelper() {
        return new VtsCompatibilityInvocationHelper();
    }

    @VisibleForTesting
    void setCmdUtil(CmdUtil cmdUtil) {
        mCmdUtil = cmdUtil;
    }

    @VisibleForTesting
    void addCommand(String command) {
        mCommands.add(command);
    }
}
