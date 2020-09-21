/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.ddmlib.EmulatorConsole;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.ISdkBuildInfo;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import com.google.common.annotations.VisibleForTesting;

import org.junit.Assert;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** A {@link ITargetPreparer} that will create an avd and launch an emulator */
public class SdkAvdPreparer extends BaseTargetPreparer implements IHostCleaner {


    @Option(name = "sdk-target", description = "the name of SDK target to launch. " +
            "If unspecified, will use first target found")
    private String mTargetName = null;

    @Option(name = "boot-time", description =
        "the maximum time in minutes to wait for emulator to boot.")
    private long mMaxBootTime = 5;

    @Option(name = "window", description = "launch emulator with a graphical window display.")
    private boolean mWindow = false;

    @Option(name = "launch-attempts", description = "max number of attempts to launch emulator")
    private int mLaunchAttempts = 1;

    @Option(name = "sdcard-size", description = "capacity of the SD card")
    private String mSdcardSize = "10M";

    @Option(name = "tag", description = "The sys-img tag to use for the AVD.")
    private String mAvdTag = null;

    @Option(name = "skin", description = "AVD skin")
    private String mAvdSkin = null;

    @Option(name = "gpu", description = "launch emulator with GPU on")
    private boolean mGpu = false;

    @Option(name = "force-kvm", description = "require kvm for emulator launch")
    private boolean mForceKvm = false;

    @Option(name = "avd-timeout", description = "the maximum time in seconds to wait for avd " +
            "creation")
    private int mAvdTimeoutSeconds = 30;

    @Option(name = "emulator-device-type", description = "emulator device type to launch." +
            "If unspecified, will launch generic version")
    private String mDevice = null;

    @Option(name = "display", description = "which display to launch the emulator in. " +
            "If unspecified, display will not be set. Display values should start with :" +
            " for example for display 1 use ':1'.")
    private String mDisplay = null;

    @Option(name = "abi", description = "abi to select for the avd")
    private String mAbi = null;

    @Option(name = "emulator-system-image",
            description = "system image will be loaded into emulator.")
    private String mEmulatorSystemImage = null;

    @Option(name = "emulator-ramdisk-image",
            description = "ramdisk image will be loaded into emulator.")
    private String mEmulatorRamdiskImage = null;

    @Option(name = "prop", description = "pass key-value pairs of system props")
    private Map<String,String> mProps = new HashMap<String, String>();

    @Option(name = "hw-options", description = "pass key-value pairs of avd hardware options")
    private Map<String,String> mHwOptions = new HashMap<String, String>();

    @Option(name = "emulator-binary", description = "location of the emulator binary")
    private String mEmulatorBinary = null;

    @Option(name = "emulator-arg",
            description = "Additional argument to launch the emulator with. Can be repeated.")
    private Collection<String> mEmulatorArgs = new ArrayList<String>();

    @Option(name = "verbose", description = "Use verbose for emulator output")
    private boolean mVerbose = false;

    private final IRunUtil mRunUtil;
    private IDeviceManager mDeviceManager;
    private ITestDevice mTestDevice;

    private File mSdkHome = null;

    /**
     * Creates a {@link SdkAvdPreparer}.
     */
    public SdkAvdPreparer() {
        this(new RunUtil(), null);
    }

    /**
     * Alternate constructor for injecting dependencies.
     *
     * @param runUtil
     */
    SdkAvdPreparer(IRunUtil runUtil, IDeviceManager deviceManager) {
        mRunUtil = runUtil;
        mDeviceManager = deviceManager;
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            DeviceNotAvailableException, BuildError {
        Assert.assertTrue("Provided build is not a ISdkBuildInfo",
                buildInfo instanceof ISdkBuildInfo);
        mTestDevice = device;
        ISdkBuildInfo sdkBuildInfo = (ISdkBuildInfo)buildInfo;
        launchEmulatorForAvd(sdkBuildInfo, device, createAvd(sdkBuildInfo));
    }

    /**
     * Finds SDK target based on the {@link ISdkBuildInfo}, creates AVD for
     * this target and returns its name.
     *
     * @param sdkBuildInfo the {@link ISdkBuildInfo}
     * @return the created AVD name
     * @throws TargetSetupError if could not get targets
     * @throws BuildError if failed to create the AVD
     */
    public String createAvd(ISdkBuildInfo sdkBuildInfo)
          throws TargetSetupError, BuildError {
        String[] targets = getSdkTargets(sdkBuildInfo);
        setAndroidSdkHome();
        String target = findTargetToLaunch(targets);
        return createAvdForTarget(sdkBuildInfo, target);
    }

    /**
     * Launch an emulator for given avd, and wait for it to become available.
     * Will launch the emulator on the port specified in the allocated {@link ITestDevice}
     *
     * @param sdkBuild the {@link ISdkBuildInfo}
     * @param device the placeholder {@link ITestDevice} representing allocated emulator device
     * @param avd the avd to launch
     * @throws DeviceNotAvailableException
     * @throws TargetSetupError if could not get targets
     * @throws BuildError if emulator fails to boot
     */
    public void launchEmulatorForAvd(ISdkBuildInfo sdkBuild, ITestDevice device, String avd)
            throws DeviceNotAvailableException, TargetSetupError, BuildError {
        if (!device.getDeviceState().equals(TestDeviceState.NOT_AVAILABLE)) {
            CLog.w("Emulator %s is already running, killing", device.getSerialNumber());
            getDeviceManager().killEmulator(device);
        } else if (!device.getIDevice().isEmulator()) {
            throw new TargetSetupError("Invalid stub device, it is not of type emulator",
                    device.getDeviceDescriptor());
        }

        mRunUtil.setEnvVariable("ANDROID_SDK_ROOT", sdkBuild.getSdkDir().getAbsolutePath());

        String emulatorBinary =
            mEmulatorBinary == null ? sdkBuild.getEmulatorToolPath() : mEmulatorBinary;
        List<String> emulatorArgs = ArrayUtil.list(emulatorBinary, "-avd", avd);

        if (mDisplay != null) {
            emulatorArgs.add(0, "DISPLAY=" + mDisplay);
        }
        // Ensure the emulator will launch on the same port as the allocated emulator device
        Integer port = EmulatorConsole.getEmulatorPort(device.getSerialNumber());
        if (port == null) {
            // Serial number is not in expected format <type>-<consolePort> as defined by ddmlib
            throw new TargetSetupError(String.format(
                    "Failed to determine emulator port for %s", device.getSerialNumber()),
                    device.getDeviceDescriptor());
        }
        emulatorArgs.add("-port");
        emulatorArgs.add(port.toString());

        if (!mWindow) {
            emulatorArgs.add("-no-window");
            emulatorArgs.add("-no-audio");
        }

        if (mGpu) {
            emulatorArgs.add("-gpu");
            emulatorArgs.add("on");
        }

        if (mVerbose) {
            emulatorArgs.add("-verbose");
        }

        for (Map.Entry<String, String> propEntry : mProps.entrySet()) {
            emulatorArgs.add("-prop");
            emulatorArgs.add(String.format("%s=%s", propEntry.getKey(), propEntry.getValue()));
        }
        for (String arg : mEmulatorArgs) {
            String[] tokens = arg.split(" ");
            if (tokens.length == 1 && tokens[0].startsWith("-")) {
                emulatorArgs.add(tokens[0]);
            } else if (tokens.length == 2) {
                if (!tokens[0].startsWith("-")) {
                    throw new TargetSetupError(String.format("The emulator arg '%s' is invalid.",
                            arg), device.getDeviceDescriptor());
                }
                emulatorArgs.add(tokens[0]);
                emulatorArgs.add(tokens[1]);
            } else {
                throw new TargetSetupError(String.format(
                        "The emulator arg '%s' is invalid.", arg), device.getDeviceDescriptor());
            }
        }

        setCommandList(emulatorArgs, "-system", mEmulatorSystemImage);
        setCommandList(emulatorArgs, "-ramdisk", mEmulatorRamdiskImage);

        // qemu must be the last parameter, it assumes params that follow it are it's own
        if(mForceKvm) {
            emulatorArgs.add("-qemu");
            emulatorArgs.add("-enable-kvm");
        }

        launchEmulator(device, avd, emulatorArgs);
        if (!avd.equals(getAvdNameFromEmulator(device))) {
            // not good. Either emulator isn't reporting its avd name properly, or somehow
            // the wrong emulator launched. Treat as a BuildError
            throw new BuildError(String.format(
                    "Emulator booted with incorrect avd name '%s'. Expected: '%s'.",
                    device.getIDevice().getAvdName(), avd), device.getDeviceDescriptor());
        }
    }

    String getAvdNameFromEmulator(ITestDevice device) {
        String avdName = device.getIDevice().getAvdName();
        if (avdName == null) {
            CLog.w("IDevice#getAvdName is null");
            // avdName is set asynchronously on startup, which explains why it might be null
            // query directly as work around
            EmulatorConsole console = EmulatorConsole.getConsole(device.getIDevice());
            if (console != null) {
                avdName = console.getAvdName();
            }
        }
        return avdName;
    }

    /**
     * Sets programmatically whether the gpu should be on or off.
     *
     * @param gpu
     */
    public void setGpu(boolean gpu) {
        mGpu = gpu;
    }

    public void setForceKvm(boolean forceKvm) {
        mForceKvm = forceKvm;
    }

    /**
     * Gets the list of sdk targets from the given sdk.
     *
     * @param sdkBuild
     * @return a list of defined targets
     * @throws TargetSetupError if could not get targets
     */
    private String[] getSdkTargets(ISdkBuildInfo sdkBuild) throws TargetSetupError {
        // Need to set the ANDROID_SWT environment variable needed by android tool.
        mRunUtil.setEnvVariable("ANDROID_SWT", getSWTDirPath(sdkBuild));
        CommandResult result = mRunUtil.runTimedCmd(getAvdTimeoutMS(),
                sdkBuild.getAndroidToolPath(), "list", "targets", "--compact");
        if (!result.getStatus().equals(CommandStatus.SUCCESS)) {
            throw new TargetSetupError(String.format(
                    "Unable to get list of SDK targets using %s. Result %s. stdout: %s, err: %s",
                    sdkBuild.getAndroidToolPath(), result.getStatus(), result.getStdout(),
                    result.getStderr()), mTestDevice.getDeviceDescriptor());
        }
        String[] targets = result.getStdout().split("\n");
        if (result.getStdout().trim().isEmpty() || targets.length == 0) {
            throw new TargetSetupError(String.format("No targets found in SDK %s.",
                    sdkBuild.getSdkDir().getAbsolutePath()), mTestDevice.getDeviceDescriptor());
        }
        return targets;
    }

    private String getSWTDirPath(ISdkBuildInfo sdkBuild) {
        return FileUtil.getPath(sdkBuild.getSdkDir().getAbsolutePath(), "tools", "lib");
    }

    /**
     * Sets the ANDROID_SDK_HOME environment variable. The SDK home directory is used as the
     * location for SDK file storage of AVD definition files, etc.
     */
    private void setAndroidSdkHome() throws TargetSetupError {
        try {
            // if necessary, create a dir to group the tmp sdk homes
            File tmpParent = createParentSdkHome();
            // create a temp dir inside the grouping folder
            mSdkHome = FileUtil.createTempDir("SDK_home", tmpParent);
            // store avds etc in tmp location, and clean up on teardown
            mRunUtil.setEnvVariable("ANDROID_SDK_HOME", mSdkHome.getAbsolutePath());
        } catch (IOException e) {
            throw new TargetSetupError("Failed to create sdk home",
                    mTestDevice.getDeviceDescriptor());
        }
    }

    /**
     * Create the parent directory where SDK_home will be stored.
     */
    @VisibleForTesting
    File createParentSdkHome() throws IOException {
        return FileUtil.createNamedTempDir("SDK_homes");
    }

    /**
     * Find the SDK target to use.
     * <p/>IOException
     * Will use the 'sdk-target' option if specified, otherwise will return last target in target
     * list.
     *
     * @param targets the list of targets in SDK
     * @return the SDK target name
     * @throws TargetSetupError if specified 'sdk-target' cannot be found
     */
    private String findTargetToLaunch(String[] targets) throws TargetSetupError {
        if (mTargetName != null) {
            for (String foundTarget : targets) {
                if (foundTarget.equals(mTargetName)) {
                    return mTargetName;
                }
            }
            throw new TargetSetupError(String.format("Could not find target %s in sdk",
                    mTargetName), mTestDevice.getDeviceDescriptor());
        }
        // just return last target
        return targets[targets.length - 1];
    }

    /**
     * Create an AVD for given SDK target.
     *
     * @param sdkBuild the {@link ISdkBuildInfo}
     * @param target the SDK target name
     * @return the created AVD name
     * @throws BuildError if failed to create the AVD
     *
     */
    private String createAvdForTarget(ISdkBuildInfo sdkBuild, String target)
            throws BuildError, TargetSetupError {
        // answer 'no' when prompted for creating a custom hardware profile
        final String cmdInput = "no\r\n";
        final String targetName = createAvdName(target);
        final String successPattern = String.format("Created AVD '%s'", targetName);
        CLog.d("Creating avd for target %s with name %s", target, targetName);

        List<String> avdCommand = ArrayUtil.list(sdkBuild.getAndroidToolPath(), "create", "avd");

        setCommandList(avdCommand, "--abi", mAbi);
        setCommandList(avdCommand, "--device", mDevice);
        setCommandList(avdCommand, "--sdcard", mSdcardSize);
        setCommandList(avdCommand, "--target", target);
        setCommandList(avdCommand, "--name", targetName);
        setCommandList(avdCommand, "--tag", mAvdTag);
        setCommandList(avdCommand, "--skin", mAvdSkin);
        avdCommand.add("--force");

        CommandResult result = mRunUtil.runTimedCmdWithInput(getAvdTimeoutMS(),
              cmdInput, avdCommand);
        if (!result.getStatus().equals(CommandStatus.SUCCESS) || result.getStdout() == null ||
                !result.getStdout().contains(successPattern)) {
            // stdout usually doesn't contain useful data, so don't want to add it to the
            // exception message. However, log it here as a debug log so the info is captured
            // in log
            CLog.d("AVD creation failed. status: '%s' stdout: '%s'", result.getStatus(),
                    result.getStdout());
            // treat as BuildError
            throw new BuildError(String.format(
                    "Unable to create avd for target '%s'. stderr: '%s'", target,
                    result.getStderr()), mTestDevice.getDeviceDescriptor());
        }

        // Further customise hardware options after AVD was created
        if (!mHwOptions.isEmpty()) {
            addHardwareOptions();
        }

        return targetName;
    }

    // Create a valid AVD name, by removing invalid characters from target name.
    private String createAvdName(String target) {
        if (target == null)  {
            return null;
        }
        return target.replaceAll("[^a-zA-Z0-9\\.\\-]", "");
    }

    // Overwrite or add AVD hardware options by appending them to the config file used by the AVD
    private void addHardwareOptions() throws TargetSetupError {
        if (mHwOptions.isEmpty()) {
            CLog.d("No hardware options to add");
            return;
        }

        // config.ini file contains all the hardware options loaded on the AVD
        final String configFileName = "config.ini";
        File configFile = FileUtil.findFile(mSdkHome, configFileName);
        if (configFile == null) {
            // Shouldn't happened if AVD was created successfully
            throw new RuntimeException("Failed to find " + configFileName);
        }

        for (Map.Entry<String, String> hwOption : mHwOptions.entrySet()) {
            // if the config file contain the same option more then once, the last one will take
            // precedence. Also, all unsupported hardware options will be ignores.
            String cmd = "echo " + hwOption.getKey() + "=" + hwOption.getValue() + " >> "
                    + configFile.getAbsolutePath();
            CommandResult result = mRunUtil.runTimedCmd(getAvdTimeoutMS(), "sh", "-c", cmd);
            if (!result.getStatus().equals(CommandStatus.SUCCESS)) {
                CLog.d("Failed to add AVD hardware option '%s' stdout: '%s'", result.getStatus(),
                        result.getStdout());
                // treat as TargetSetupError
                throw new TargetSetupError(String.format(
                        "Unable to add hardware option to AVD. stderr: '%s'", result.getStderr()),
                        mTestDevice.getDeviceDescriptor());
            }
        }
    }


    /**
     * Launch emulator, performing multiple attempts if necessary as specified.
     *
     * @param device
     * @param avd
     * @param emulatorArgs
     * @throws BuildError
     */
    void launchEmulator(ITestDevice device, String avd, List<String> emulatorArgs)
            throws BuildError {
        for (int i = 1; i <= mLaunchAttempts; i++) {
            try {
                getDeviceManager().launchEmulator(device, mMaxBootTime * 60 * 1000, mRunUtil,
                        emulatorArgs);
                // hack alert! adb to emulator communication on first boot is notoriously flaky
                // b/4644136
                // send it a few adb commands to ensure the communication channel is stable
                CLog.d("Testing adb to %s communication", device.getSerialNumber());
                for (int j = 0; j < 3; j++) {
                    device.executeShellCommand("pm list instrumentation");
                    mRunUtil.sleep(2 * 1000);
                }

                // hurray - launched!
                return;
            } catch (DeviceNotAvailableException e) {
                CLog.w("Emulator for avd '%s' failed to launch on attempt %d of %d. Cause: %s",
                        avd, i, mLaunchAttempts, e);
            }
            try {
                // ensure process has been killed
                getDeviceManager().killEmulator(device);
            } catch (DeviceNotAvailableException e) {
                // ignore
            }
        }
        throw new DeviceFailedToBootError(
                String.format("Emulator for avd '%s' failed to boot.", avd),
                device.getDeviceDescriptor());
    }

    /**
     * Sets the number of launch attempts to perform.
     *
     * @param launchAttempts
     */
    void setLaunchAttempts(int launchAttempts) {
        mLaunchAttempts = launchAttempts;
    }

    @Override
    public void cleanUp(IBuildInfo buildInfo, Throwable e) {
        if (mSdkHome != null) {
            CLog.i("Removing tmp sdk home dir %s", mSdkHome.getAbsolutePath());
            FileUtil.recursiveDelete(mSdkHome);
            mSdkHome = null;
        }
    }

    private IDeviceManager getDeviceManager() {
        if (mDeviceManager == null) {
            mDeviceManager = GlobalConfiguration.getDeviceManagerInstance();
        }
        return mDeviceManager;
    }

    private int getAvdTimeoutMS() {
        return mAvdTimeoutSeconds * 1000;
    }

    private void setCommandList(List<String> commands, String option, String value) {
        if (value != null) {
            commands.add(option);
            commands.add(value);
        }
    }
}
