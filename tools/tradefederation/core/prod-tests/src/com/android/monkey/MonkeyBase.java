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

package com.android.monkey;

import com.android.ddmlib.CollectingOutputReceiver;
import com.android.ddmlib.IShellOutputReceiver;
import com.android.loganalysis.item.AnrItem;
import com.android.loganalysis.item.BugreportItem;
import com.android.loganalysis.item.MiscKernelLogItem;
import com.android.loganalysis.item.MonkeyLogItem;
import com.android.loganalysis.parser.BugreportParser;
import com.android.loganalysis.parser.KernelLogParser;
import com.android.loganalysis.parser.MonkeyLogParser;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.DeviceFileReporter;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRetriableTest;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.Bugreport;
import com.android.tradefed.util.CircularAtraceUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.TimeUnit;

/**
 * Runner for stress tests which use the monkey command.
 */
public class MonkeyBase implements IDeviceTest, IRemoteTest, IRetriableTest {

    public static final String MONKEY_LOG_NAME = "monkey_log";
    public static final String BUGREPORT_NAME = "bugreport";

    /**
     * Allow a 15 second buffer between the monkey run time and the delta uptime.
     */
    public static final long UPTIME_BUFFER = 15 * 1000;

    private static final String DEVICE_WHITELIST_PATH = "/data/local/tmp/monkey_whitelist.txt";

    /**
     * am command template to launch app intent with same action, category and task flags as if user
     * started it from the app's launcher icon
     */
    private static final String LAUNCH_APP_CMD = "am start -W -n '%s' " +
            "-a android.intent.action.MAIN -c android.intent.category.LAUNCHER -f 0x10200000";

    private static final String NULL_UPTIME = "0.00";

    /**
     * Helper to run a monkey command with an absolute timeout.
     * <p>
     * This is used so that the command can be stopped after a set timeout, since the timeout that
     * {@link ITestDevice#executeShellCommand(String, IShellOutputReceiver, long, TimeUnit, int)}
     * takes applies to the time between output, not the overall time of the command.
     * </p>
     */
    private class CommandHelper {
        private DeviceNotAvailableException mException = null;
        private String mOutput = null;

        public void runCommand(final ITestDevice device, final String command, long timeout)
                throws DeviceNotAvailableException {
            final CollectingOutputReceiver receiver = new CollectingOutputReceiver();
            Thread t = new Thread() {
                @Override
                public void run() {
                    try {
                        device.executeShellCommand(command, receiver);
                    } catch (DeviceNotAvailableException e) {
                        mException = e;
                    }
                }
            };

            t.start();

            try {
                t.join(timeout);
            } catch (InterruptedException e) {
                // Ignore and log.  The thread should terminate once receiver.cancel() is called.
                CLog.e("Thread was interrupted while running %s", command);
            }

            mOutput = receiver.getOutput();
            receiver.cancel();

            if (mException != null) {
                throw mException;
            }
        }

        public String getOutput() {
            return mOutput;
        }
    }

    @Option(name = "package", description = "Package name to send events to.  May be repeated.")
    private Collection<String> mPackages = new LinkedList<>();

    @Option(name = "exclude-package", description = "Substring of package names to exclude from " +
            "the package list. May be repeated.", importance = Importance.IF_UNSET)
    private Collection<String> mExcludePackages = new HashSet<>();

    @Option(name = "category", description = "App Category. May be repeated.")
    private Collection<String> mCategories = new LinkedList<>();

    @Option(name = "option", description = "Option to pass to monkey command. May be repeated.")
    private Collection<String> mOptions = new LinkedList<>();

    @Option(name = "launch-extras-int", description = "Launch int extras. May be repeated. " +
            "Format: --launch-extras-i key value. Note: this will be applied to all components.")
    private Map<String, Integer> mIntegerExtras = new HashMap<>();

    @Option(name = "launch-extras-str", description = "Launch string extras. May be repeated. " +
            "Format: --launch-extras-s key value. Note: this will be applied to all components.")
    private Map<String, String> mStringExtras = new HashMap<>();

    @Option(name = "target-count", description = "Target number of events to send.",
            importance = Importance.ALWAYS)
    private int mTargetCount = 125000;

    @Option(name = "random-seed", description = "Random seed to use for the monkey.")
    private Long mRandomSeed = null;

    @Option(name = "throttle", description = "How much time to wait between sending successive " +
            "events, in msecs.  Default is 0ms.")
    private int mThrottle = 0;

    @Option(name = "ignore-crashes", description = "Monkey should keep going after encountering " +
            "an app crash")
    private boolean mIgnoreCrashes = false;

    @Option(name = "ignore-timeout", description = "Monkey should keep going after encountering " +
            "an app timeout (ANR)")
    private boolean mIgnoreTimeouts = false;

    @Option(name = "reboot-device", description = "Reboot device before running monkey. Defaults " +
            "to true.")
    private boolean mRebootDevice = true;

    @Option(name = "idle-time", description = "How long to sleep before running monkey, in secs")
    private int mIdleTimeSecs = 5 * 60;

    @Option(name = "monkey-arg", description = "Extra parameters to pass onto monkey. Key/value " +
            "pairs should be passed as key:value. May be repeated.")
    private Collection<String> mMonkeyArgs = new LinkedList<>();

    @Option(name = "use-pkg-whitelist-file", description = "Whether to use the monkey " +
            "--pkg-whitelist-file option to work around cmdline length limits")
    private boolean mUseWhitelistFile = false;

    @Option(name = "monkey-timeout", description = "How long to wait for the monkey to " +
            "complete, in minutes. Default is 4 hours.")
    private int mMonkeyTimeout = 4 * 60;

    @Option(name = "warmup-component", description = "Component name of app to launch for " +
            "\"warming up\" before monkey test, will be used in an intent together with standard " +
            "flags and parameters as launched from Launcher. May be repeated")
    private List<String> mLaunchComponents = new ArrayList<>();

    @Option(name = "retry-on-failure", description = "Retry the test on failure")
    private boolean mRetryOnFailure = false;

    // FIXME: Remove this once traces.txt is no longer needed.
    @Option(name = "upload-file-pattern", description = "File glob of on-device files to upload " +
            "if found. Takes two arguments: the glob, and the file type " +
            "(text/xml/zip/gzip/png/unknown).  May be repeated.")
    private Map<String, LogDataType> mUploadFilePatterns = new LinkedHashMap<>();

    @Option(name = "screenshot", description = "Take a device screenshot on monkey completion")
    private boolean mScreenshot = false;

    @Option(name = "ignore-security-exceptions",
            description = "Ignore SecurityExceptions while injecting events")
    private boolean mIgnoreSecurityExceptions = true;

    @Option(name = "collect-atrace",
            description = "Enable a continuous circular buffer to collect atrace information")
    private boolean mAtraceEnabled = false;

    // options for generating ANR report via post processing script
    @Option(name = "generate-anr-report", description = "Generate ANR report via post-processing")
    private boolean mGenerateAnrReport = false;

    @Option(name = "anr-report-script", description = "Path to the script for monkey ANR "
            + "report generation.")
    private String mAnrReportScriptPath = null;

    @Option(name = "anr-report-storage-backend-base-path", description = "Base path to the storage "
            + "backend used for saving the reports")
    private String mAnrReportBasePath = null;

    @Option(name = "anr-report-storage-backend-url-prefix", description = "URL prefix for the "
            + "storage backend that would enable web acess to the stored reports.")
    private String mAnrReportUrlPrefix = null;

    @Option(name = "anr-report-storage-path", description = "Sub path under the base storage "
            + "location for generated monkey ANR reports.")
    private String mAnrReportPath = null;

    private ITestDevice mTestDevice = null;
    private MonkeyLogItem mMonkeyLog = null;
    private BugreportItem mBugreport = null;
    private AnrReportGenerator mAnrGen = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(getDevice());

        TestDescription id = new TestDescription(getClass().getCanonicalName(), "monkey");
        long startTime = System.currentTimeMillis();

        listener.testRunStarted(getClass().getCanonicalName(), 1);
        listener.testStarted(id);

        try {
            runMonkey(listener);
        } finally {
            listener.testEnded(id, new HashMap<String, Metric>());
            listener.testRunEnded(
                    System.currentTimeMillis() - startTime, new HashMap<String, Metric>());
        }
    }

    /**
     * Returns the command that should be used to launch the app,
     */
    private String getAppCmdWithExtras() {
        String extras = "";
        for (Map.Entry<String, String> sEntry : mStringExtras.entrySet()) {
            extras += String.format(" -e %s %s", sEntry.getKey(), sEntry.getValue());
        }
        for (Map.Entry<String, Integer> sEntry : mIntegerExtras.entrySet()) {
            extras += String.format(" --ei %s %d", sEntry.getKey(), sEntry.getValue());
        }
        return LAUNCH_APP_CMD + extras;
    }

    /**
     * Run the monkey one time and return a {@link MonkeyLogItem} for the run.
     */
    protected void runMonkey(ITestInvocationListener listener) throws DeviceNotAvailableException {
        ITestDevice device = getDevice();
        if (mRebootDevice) {
            CLog.v("Rebooting device prior to running Monkey");
            device.reboot();
        } else {
            CLog.v("Pre-run reboot disabled; skipping...");
        }

        if (mIdleTimeSecs > 0) {
            CLog.i("Sleeping for %d seconds to allow device to settle...", mIdleTimeSecs);
            getRunUtil().sleep(mIdleTimeSecs * 1000);
            CLog.i("Done sleeping.");
        }

        // launch the list of apps that needs warm-up
        for (String componentName : mLaunchComponents) {
            getDevice().executeShellCommand(String.format(getAppCmdWithExtras(), componentName));
            // give it some more time to settle down
            getRunUtil().sleep(5000);
        }

        if (mUseWhitelistFile) {
            // Use \r\n for new lines on the device.
            String whitelist = ArrayUtil.join("\r\n", setSubtract(mPackages, mExcludePackages));
            device.pushString(whitelist.toString(), DEVICE_WHITELIST_PATH);
        }

        // Generate the monkey command to run, given the options
        String command = buildMonkeyCommand();
        CLog.i("About to run monkey with at %d minute timeout: %s", mMonkeyTimeout, command);

        StringBuilder outputBuilder = new StringBuilder();
        CommandHelper commandHelper = new CommandHelper();

        long start = System.currentTimeMillis();
        long duration = 0;
        Date dateAfter = null;
        String uptimeAfter = NULL_UPTIME;
        FileInputStreamSource atraceStream = null;

        // Generate the monkey log prefix, which includes the device uptime
        outputBuilder.append(String.format("# %s - device uptime = %s: Monkey command used " +
                "for this test:\nadb shell %s\n\n", new Date().toString(), getUptime(), command));

        // Start atrace before running the monkey command, but after reboot
        if (mAtraceEnabled) {
            CircularAtraceUtil.startTrace(getDevice(), null, 10);
        }

        if (mGenerateAnrReport) {
            mAnrGen = new AnrReportGenerator(mAnrReportScriptPath, mAnrReportBasePath,
                    mAnrReportUrlPrefix, mAnrReportPath, mTestDevice.getBuildId(),
                    mTestDevice.getBuildFlavor(), mTestDevice.getSerialNumber());
        }

        try {
            onMonkeyStart();
            commandHelper.runCommand(mTestDevice, command, getMonkeyTimeoutMs());
        } finally {
            // Wait for device to recover if it's not online.  If it hasn't recovered, ignore.
            try {
                mTestDevice.waitForDeviceOnline(2 * 60 * 1000);
                duration = System.currentTimeMillis() - start;
                dateAfter = new Date();
                uptimeAfter = getUptime();
                onMonkeyFinish();
                takeScreenshot(listener, "screenshot");

                if (mAtraceEnabled) {
                    atraceStream = CircularAtraceUtil.endTrace(getDevice());
                }

                mBugreport = takeBugreport(listener, BUGREPORT_NAME);
                // FIXME: Remove this once traces.txt is no longer needed.
                takeTraces(listener);
            } finally {
                // @@@ DO NOT add anything that requires device interaction into this block     @@@
                // @@@ logging that no longer requires device interaction MUST be in this block @@@
                outputBuilder.append(commandHelper.getOutput());
                if (dateAfter == null) {
                    dateAfter = new Date();
                }

                // Generate the monkey log suffix, which includes the device uptime.
                outputBuilder.append(String.format("\n# %s - device uptime = %s: Monkey command "
                        + "ran for: %d:%02d (mm:ss)\n", dateAfter.toString(), uptimeAfter,
                        duration / 1000 / 60, duration / 1000 % 60));
                mMonkeyLog = createMonkeyLog(listener, MONKEY_LOG_NAME, outputBuilder.toString());

                boolean isAnr = mMonkeyLog.getCrash() instanceof AnrItem;
                if (mAtraceEnabled && isAnr) {
                    // This was identified as an ANR; post atrace data
                    listener.testLog("circular-atrace", LogDataType.TEXT, atraceStream);
                }
                if (mAnrGen != null) {
                    if (isAnr) {
                        if (!mAnrGen.genereateAnrReport(listener)) {
                            CLog.w("Failed to post-process ANR.");
                        } else {
                            CLog.i("Successfully post-processed ANR.");
                        }
                        mAnrGen.cleanTempFiles();
                    } else {
                        CLog.d("ANR post-processing enabled but no ANR detected.");
                    }
                }
                StreamUtil.cancel(atraceStream);
            }
        }

        // Extra logs for what was found
        if (mBugreport != null && mBugreport.getLastKmsg() != null) {
            List<MiscKernelLogItem> kernelErrors = mBugreport.getLastKmsg().getMiscEvents(
                    KernelLogParser.KERNEL_ERROR);
            List<MiscKernelLogItem> kernelResets = mBugreport.getLastKmsg().getMiscEvents(
                    KernelLogParser.KERNEL_ERROR);
            CLog.d("Found %d kernel errors and %d kernel resets in last kmsg",
                    kernelErrors.size(), kernelResets.size());
            for (int i = 0; i < kernelErrors.size(); i++) {
                String stack = kernelErrors.get(i).getStack();
                if (stack != null) {
                    CLog.d("Kernel Error #%d: %s", i + 1, stack.split("\n")[0].trim());
                }
            }
            for (int i = 0; i < kernelResets.size(); i++) {
                String stack = kernelResets.get(i).getStack();
                if (stack != null) {
                    CLog.d("Kernel Reset #%d: %s", i + 1, stack.split("\n")[0].trim());
                }
            }
        }

        checkResults();
    }

    /**
     * A hook to allow subclasses to perform actions just before the monkey starts.
     */
    protected void onMonkeyStart() {
       // empty
    }

    /**
     * A hook to allow sublaccess to perform actions just after the monkey finished.
     */
    protected void onMonkeyFinish() {
        // empty
    }

    /**
     * If enabled, capture a screenshot and send it to a listener.
     * @throws DeviceNotAvailableException
     */
    protected void takeScreenshot(ITestInvocationListener listener, String screenshotName)
            throws DeviceNotAvailableException {
        if (mScreenshot) {
            try (InputStreamSource screenshot = mTestDevice.getScreenshot("JPEG")) {
                listener.testLog(screenshotName, LogDataType.JPEG, screenshot);
            }
        }
    }

    /**
     * Capture a bugreport and send it to a listener.
     */
    protected BugreportItem takeBugreport(ITestInvocationListener listener, String bugreportName) {
        Bugreport bugreport = mTestDevice.takeBugreport();
        if (bugreport == null) {
            CLog.e("Could not take bugreport");
            return null;
        }
        bugreport.log(bugreportName, listener);
        File main = null;
        InputStreamSource is = null;
        try {
            main = bugreport.getMainFile();
            if (main == null) {
                CLog.e("Bugreport has no main file");
                return null;
            }
            if (mAnrGen != null) {
                is = new FileInputStreamSource(main);
                mAnrGen.setBugReportInfo(is);
            }
            return new BugreportParser().parse(new BufferedReader(new FileReader(main)));
        } catch (IOException e) {
            CLog.e("Could not process bugreport");
            CLog.e(e);
            return null;
        } finally {
            StreamUtil.close(bugreport);
            StreamUtil.cancel(is);
            FileUtil.deleteFile(main);
        }
    }

    protected void takeTraces(ITestInvocationListener listener) {
        DeviceFileReporter dfr = new DeviceFileReporter(mTestDevice, listener);
        dfr.addPatterns(mUploadFilePatterns);
        try {
            dfr.run();
        } catch (DeviceNotAvailableException e) {
            // Log but don't throw
            CLog.e("Device %s became unresponsive while pulling files",
                    mTestDevice.getSerialNumber());
        }
    }

    /**
     * Create the monkey log, parse it, and send it to a listener.
     */
    protected MonkeyLogItem createMonkeyLog(ITestInvocationListener listener, String monkeyLogName,
            String log) {
        try (InputStreamSource source = new ByteArrayInputStreamSource(log.getBytes())) {
            if (mAnrGen != null) {
                mAnrGen.setMonkeyLogInfo(source);
            }
            listener.testLog(monkeyLogName, LogDataType.MONKEY_LOG, source);
            return new MonkeyLogParser().parse(new BufferedReader(new InputStreamReader(
                    source.createInputStream())));
        } catch (IOException e) {
            CLog.e("Could not process monkey log.");
            CLog.e(e);
            return null;
        }
    }

    /**
     * A helper method to build a monkey command given the specified arguments.
     * <p>
     * Actual output argument order is:
     * {@code monkey [-p PACKAGE]... [-c CATEGORY]... [--OPTION]... -s SEED -v -v -v COUNT}
     * </p>
     *
     * @return a {@link String} containing the command with the arguments assembled in the proper
     *         order.
     */
    protected String buildMonkeyCommand() {
        List<String> cmdList = new LinkedList<>();
        cmdList.add("monkey");

        if (!mUseWhitelistFile) {
            for (String pkg : setSubtract(mPackages, mExcludePackages)) {
                cmdList.add("-p");
                cmdList.add(pkg);
            }
        }

        for (String cat : mCategories) {
            cmdList.add("-c");
            cmdList.add(cat);
        }

        if (mIgnoreSecurityExceptions) {
            cmdList.add("--ignore-security-exceptions");
        }

        if (mThrottle >= 1) {
            cmdList.add("--throttle");
            cmdList.add(Integer.toString(mThrottle));
        }
        if (mIgnoreCrashes) {
            cmdList.add("--ignore-crashes");
        }
        if (mIgnoreTimeouts) {
            cmdList.add("--ignore-timeouts");
        }

        if (mUseWhitelistFile) {
            cmdList.add("--pkg-whitelist-file");
            cmdList.add(DEVICE_WHITELIST_PATH);
        }

        for (String arg : mMonkeyArgs) {
            String[] args = arg.split(":");
            cmdList.add(String.format("--%s", args[0]));
            if (args.length > 1) {
                cmdList.add(args[1]);
            }
        }

        cmdList.addAll(mOptions);

        cmdList.add("-s");
        if (mRandomSeed == null) {
            // Pick a number that is random, but in a small enough range that some seeds are likely
            // to be repeated
            cmdList.add(Long.toString(new Random().nextInt(1000)));
        } else {
            cmdList.add(Long.toString(mRandomSeed));
        }

        // verbose
        cmdList.add("-v");
        cmdList.add("-v");
        cmdList.add("-v");
        cmdList.add(Integer.toString(mTargetCount));

        return ArrayUtil.join(" ", cmdList);
    }

    /**
     * Get a {@link String} containing the number seconds since the device was booted.
     * <p>
     * {@code NULL_UPTIME} is returned if the device becomes unresponsive. Used in the monkey log
     * prefix and suffix.
     * </p>
     */
    protected String getUptime() {
        try {
            // make two attempts to get valid uptime
            for (int i = 0; i < 2; i++) {
                // uptime will typically have a format like "5278.73 1866.80".  Use the first one
                // (which is wall-time)
                String uptime = mTestDevice.executeShellCommand("cat /proc/uptime").split(" ")[0];
                try {
                    Float.parseFloat(uptime);
                    // if this parsed, its a valid uptime
                    return uptime;
                } catch (NumberFormatException e) {
                    CLog.w("failed to get valid uptime from %s. Received: '%s'",
                            mTestDevice.getSerialNumber(), uptime);
                }
            }
        } catch (DeviceNotAvailableException e) {
            CLog.e("Device %s became unresponsive while getting the uptime.",
                    mTestDevice.getSerialNumber());
        }
        return NULL_UPTIME;
    }

    /**
     * Perform set subtraction between two {@link Collection} objects.
     * <p>
     * The return value will consist of all of the elements of {@code keep}, excluding the elements
     * that are also in {@code exclude}. Exposed for unit testing.
     * </p>
     *
     * @param keep the minuend in the subtraction
     * @param exclude the subtrahend
     * @return the collection of elements in {@code keep} that are not also in {@code exclude}. If
     * {@code keep} is an ordered {@link Collection}, the remaining elements in the return value
     * will remain in their original order.
     */
    static Collection<String> setSubtract(Collection<String> keep, Collection<String> exclude) {
        if (exclude.isEmpty()) {
            return keep;
        }

        Collection<String> output = new ArrayList<>(keep);
        output.removeAll(exclude);
        return output;
    }

    /**
     * Get {@link IRunUtil} to use. Exposed for unit testing.
     */
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mTestDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mTestDevice;
    }

    /**
     * {@inheritDoc}
     *
     * @return {@code false} if retry-on-failure is not set, if the monkey ran to completion,
     * crashed in an understood way, or if there were no packages to run, {@code true} otherwise.
     */
    @Override
    public boolean isRetriable() {
        return mRetryOnFailure;
    }

    /**
     * Check the results and return if valid or throw an assertion error if not valid.
     */
    private void checkResults() {
        Assert.assertNotNull("Monkey log is null", mMonkeyLog);
        Assert.assertNotNull("Bugreport is null", mBugreport);
        Assert.assertNotNull("Bugreport is empty", mBugreport.getTime());

        // If there are no activities, retrying the test won't matter.
        if (mMonkeyLog.getNoActivities()) {
            return;
        }

        Assert.assertNotNull("Start uptime is missing", mMonkeyLog.getStartUptimeDuration());
        Assert.assertNotNull("Stop uptime is missing", mMonkeyLog.getStopUptimeDuration());
        Assert.assertNotNull("Total duration is missing", mMonkeyLog.getTotalDuration());

        long startUptime = mMonkeyLog.getStartUptimeDuration();
        long stopUptime = mMonkeyLog.getStopUptimeDuration();
        long totalDuration = mMonkeyLog.getTotalDuration();

        Assert.assertTrue("Uptime failure",
                stopUptime - startUptime > totalDuration - UPTIME_BUFFER);

        // False count
        Assert.assertFalse("False count", mMonkeyLog.getIsFinished() &&
                mMonkeyLog.getTargetCount() - mMonkeyLog.getIntermediateCount() > 100);

        // Monkey finished or crashed, so don't fail
        if (mMonkeyLog.getIsFinished() || mMonkeyLog.getFinalCount() != null) {
            return;
        }

        // Missing count
        Assert.fail("Missing count");
    }

    /**
     * Get the monkey timeout in milliseconds
     */
    protected long getMonkeyTimeoutMs() {
        return mMonkeyTimeout * 60 * 1000;
    }
}
