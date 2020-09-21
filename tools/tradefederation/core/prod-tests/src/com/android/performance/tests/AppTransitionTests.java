/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.performance.tests;

import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.loganalysis.item.LatencyItem;
import com.android.loganalysis.item.TransitionDelayItem;
import com.android.loganalysis.parser.EventsLogParser;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.IFileEntry;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.LogcatReceiver;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.SimpleStats;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.ZipUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Test that drives the transition delays during different user behavior like
 * cold launch from launcher, hot launch from recent etc.This class invokes
 * the instrumentation test apk that does the transition and captures the events logs
 * during the transition and parse them and report in the dashboard.
 */
public class AppTransitionTests implements IRemoteTest, IDeviceTest {

    private static final String PACKAGE_NAME = "com.android.apptransition.tests";
    private static final String CLASS_NAME = "com.android.apptransition.tests.AppTransitionTests";
    private static final String TEST_COLD_LAUNCH = "testColdLaunchFromLauncher";
    private static final String TEST_HOT_LAUNCH = "testHotLaunchFromLauncher";
    private static final String TEST_APP_TO_HOME = "testAppToHome";
    private static final String TEST_APP_TO_RECENT = "testAppToRecents";
    private static final String TEST_LATENCY = "testLatency";
    private static final String TEST_HOT_LAUNCH_FROM_RECENTS = "testHotLaunchFromRecents";
    private static final String DROP_CACHE_SCRIPT_FILE = "dropCache";
    private static final String SCRIPT_EXTENSION = ".sh";
    private static final String DROP_CACHE_CMD = "echo 3 > /proc/sys/vm/drop_caches";
    private static final String DEVICE_TEMPORARY_DIR_PATH = "/data/local/tmp/";
    private static final String REMOVE_CMD = "rm -rf %s/%s";
    private static final String EVENTS_CMD = "logcat -v threadtime -b events";
    private static final String EVENTS_CLEAR_CMD = "logcat -v threadtime -b events -c";
    private static final String EVENTS_LOG = "events_log";
    private static final long EVENTS_LOGCAT_SIZE = 80 * 1024 * 1024;

    @Option(name = "cold-apps", description = "Apps used for cold app launch"
            + " transition delay testing from launcher screen.")
    private String mColdLaunchApps = null;

    @Option(name = "hot-apps", description = "Apps used for hot app launch"
            + " transition delay testing from launcher screen.")
    private String mHotLaunchApps = null;

    @Option(name = "pre-launch-apps", description = "Apps used for populating the"
            + " recents apps list before starting app_to_recents or hot_app_from_recents"
            + " testing.")
    private String mPreLaunchApps = null;

    @Option(name = "apps-to-recents", description = "Apps used for app to recents"
            + " transition delay testing.")
    private String mAppToRecents = null;

    @Option(name = "hot-apps-from-recents", description = "Apps used for hot"
            + " launch app from recents list transition delay testing.")
    private String mRecentsToApp = null;

    @Option(name = "launch-iteration", description = "Iterations for launching each app to"
            + "test the transition delay.")
    private int mLaunchIteration = 10;

    @Option(name = "trace-directory", description = "Directory to store the trace files"
            + "while testing ther app transition delay.")
    private String mTraceDirectory = null;

    @Option(name = "runner", description = "The instrumentation test runner class name to use.")
    private String mRunnerName = "android.support.test.runner.AndroidJUnitRunner";

    @Option(name = "run-arg",
            description = "Additional test specific arguments to provide.")
    private Map<String, String> mArgMap = new LinkedHashMap<String, String>();

    @Option(name = "launcher-activity", description = "Home activity name")
    private String mLauncherActivity = ".NexusLauncherActivity";

    @Option(name = "class",
            description = "test class to run, may be repeated; multiple classess will be run"
                    + " in the same order as provided in command line")
    private List<String> mClasses = new ArrayList<String>();

    @Option(name = "package",
            description = "The manifest package name of the UI test package")
    private String mPackage = "com.android.apptransition.tests";

    @Option(name = "latency-iteration", description = "Iterations to be used in the latency tests.")
    private int mLatencyIteration = 10;

    @Option(name = "timeout",
            description = "Aborts the test run if any test takes longer than the specified number "
                    + "of milliseconds. For no timeout, set to 0.", isTimeVal = true)
    private long mTestTimeout = 45 * 60 * 1000;  // default to 45 minutes

    private ITestDevice mDevice = null;
    private IRemoteAndroidTestRunner mRunner = null;
    private CollectingTestListener mLaunchListener = null;
    private LogcatReceiver mLaunchEventsLogs = null;
    private EventsLogParser mEventsLogParser = new EventsLogParser();
    private ITestInvocationListener mListener = null;

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {

        addDropCacheScriptFile();
        mListener = listener;

        if (null != mColdLaunchApps && !mColdLaunchApps.isEmpty()) {
            try {
                mRunner = createRemoteAndroidTestRunner(TEST_COLD_LAUNCH, mColdLaunchApps, null);
                mLaunchListener = new CollectingTestListener();
                mLaunchEventsLogs = new LogcatReceiver(getDevice(), EVENTS_CMD, EVENTS_LOGCAT_SIZE,
                        0);
                startEventsLogs(mLaunchEventsLogs, TEST_COLD_LAUNCH);
                mDevice.runInstrumentationTests(mRunner, mLaunchListener);
                analyzeColdLaunchDelay(parseTransitionDelayInfo());
            } finally {
                stopEventsLogs(mLaunchEventsLogs, TEST_COLD_LAUNCH);
                if (isTraceDirEnabled()) {
                    uploadTraceFiles(listener, TEST_COLD_LAUNCH);
                }
            }
        }

        if (null != mHotLaunchApps && !mHotLaunchApps.isEmpty()) {
            try {
                mRunner = createRemoteAndroidTestRunner(TEST_HOT_LAUNCH, mHotLaunchApps, null);
                mLaunchListener = new CollectingTestListener();
                mLaunchEventsLogs = new LogcatReceiver(getDevice(), EVENTS_CMD, EVENTS_LOGCAT_SIZE,
                        0);
                startEventsLogs(mLaunchEventsLogs, TEST_HOT_LAUNCH);
                mDevice.runInstrumentationTests(mRunner, mLaunchListener);
                analyzeHotLaunchDelay(parseTransitionDelayInfo());
            } finally {
                stopEventsLogs(mLaunchEventsLogs, TEST_HOT_LAUNCH);
                if (isTraceDirEnabled()) {
                    uploadTraceFiles(listener, TEST_HOT_LAUNCH);
                }
            }
        }

        if ((null != mAppToRecents && !mAppToRecents.isEmpty()) &&
                (null != mPreLaunchApps && !mPreLaunchApps.isEmpty())) {
            try {
                mRunner = createRemoteAndroidTestRunner(TEST_APP_TO_RECENT, mAppToRecents,
                        mPreLaunchApps);
                mLaunchListener = new CollectingTestListener();
                mLaunchEventsLogs = new LogcatReceiver(getDevice(), EVENTS_CMD, EVENTS_LOGCAT_SIZE,
                        0);
                startEventsLogs(mLaunchEventsLogs, TEST_APP_TO_RECENT);
                mDevice.runInstrumentationTests(mRunner, mLaunchListener);
                analyzeAppToRecentsDelay(parseTransitionDelayInfo());
            } finally {
                stopEventsLogs(mLaunchEventsLogs, TEST_APP_TO_RECENT);
                if (isTraceDirEnabled()) {
                    uploadTraceFiles(listener, TEST_APP_TO_RECENT);
                }
            }
        }

        if ((null != mRecentsToApp && !mRecentsToApp.isEmpty()) &&
                (null != mPreLaunchApps && !mPreLaunchApps.isEmpty())) {
            try {
                mRunner = createRemoteAndroidTestRunner(TEST_HOT_LAUNCH_FROM_RECENTS,
                        mRecentsToApp,
                        mPreLaunchApps);
                mLaunchListener = new CollectingTestListener();
                mLaunchEventsLogs = new LogcatReceiver(getDevice(), EVENTS_CMD, EVENTS_LOGCAT_SIZE,
                        0);
                startEventsLogs(mLaunchEventsLogs, TEST_HOT_LAUNCH_FROM_RECENTS);
                mDevice.runInstrumentationTests(mRunner, mLaunchListener);
                analyzeRecentsToAppDelay(parseTransitionDelayInfo());
            } finally {
                stopEventsLogs(mLaunchEventsLogs, TEST_HOT_LAUNCH_FROM_RECENTS);
                if (isTraceDirEnabled()) {
                    uploadTraceFiles(listener, TEST_HOT_LAUNCH_FROM_RECENTS);
                }
            }
        }

        if (!mClasses.isEmpty()) {
            try {
                mRunner = createTestRunner();
                mLaunchListener = new CollectingTestListener();
                mLaunchEventsLogs = new LogcatReceiver(getDevice(), EVENTS_CMD, EVENTS_LOGCAT_SIZE,
                        0);
                startEventsLogs(mLaunchEventsLogs, TEST_LATENCY);
                mDevice.runInstrumentationTests(mRunner, mLaunchListener);
                analyzeLatencyInfo(parseLatencyInfo());
            } finally {
                stopEventsLogs(mLaunchEventsLogs, TEST_LATENCY);
                if (isTraceDirEnabled()) {
                    uploadTraceFiles(listener, TEST_LATENCY);
                }
            }
        }

    }

    /**
     * Push drop cache script file to test device used for clearing the cache between the app
     * launches.
     *
     * @throws DeviceNotAvailableException
     */
    private void addDropCacheScriptFile() throws DeviceNotAvailableException {
        File scriptFile = null;
        try {
            scriptFile = FileUtil.createTempFile(DROP_CACHE_SCRIPT_FILE, SCRIPT_EXTENSION);
            FileUtil.writeToFile(DROP_CACHE_CMD, scriptFile);
            getDevice().pushFile(scriptFile, String.format("%s%s.sh",
                    DEVICE_TEMPORARY_DIR_PATH, DROP_CACHE_SCRIPT_FILE));
        } catch (IOException ioe) {
            CLog.e("Unable to create the Script file");
            CLog.e(ioe);
        }
        getDevice().executeShellCommand(String.format("chmod 755 %s%s.sh",
                DEVICE_TEMPORARY_DIR_PATH, DROP_CACHE_SCRIPT_FILE));
        scriptFile.delete();
    }

    /**
     * Method to create the runner with given list of arguments
     *
     * @return the {@link IRemoteAndroidTestRunner} to use.
     * @throws DeviceNotAvailableException
     */
    IRemoteAndroidTestRunner createRemoteAndroidTestRunner(String testName, String launchApps,
            String preLaunchApps) throws DeviceNotAvailableException {
        RemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                PACKAGE_NAME, mRunnerName, mDevice.getIDevice());
        runner.setMethodName(CLASS_NAME, testName);
        runner.addInstrumentationArg("launch_apps", launchApps);
        runner.setMaxTimeout(mTestTimeout, TimeUnit.MILLISECONDS);
        if (null != preLaunchApps && !preLaunchApps.isEmpty()) {
            runner.addInstrumentationArg("pre_launch_apps", preLaunchApps);
        }
        runner.addInstrumentationArg("launch_iteration", Integer.toString(mLaunchIteration));
        for (Map.Entry<String, String> entry : getTestRunArgMap().entrySet()) {
            runner.addInstrumentationArg(entry.getKey(), entry.getValue());
        }
        if (isTraceDirEnabled()) {
            mDevice.executeShellCommand(String.format("rm -rf %s/%s", mTraceDirectory, testName));
            runner.addInstrumentationArg("trace_directory", mTraceDirectory);
        }
        return runner;
    }

    /**
     * Method to create the runner with given runner name, package and list of classes.
     * @return the {@link IRemoteAndroidTestRunner} to use.
     * @throws DeviceNotAvailableException
     */
    IRemoteAndroidTestRunner createTestRunner() throws DeviceNotAvailableException {
        IRemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(mPackage, mRunnerName,
                getDevice().getIDevice());
        if (!mClasses.isEmpty()) {
            runner.setClassNames(mClasses.toArray(new String[] {}));
        }
        runner.addInstrumentationArg("iteration_count", Integer.toString(mLatencyIteration));
        for (Map.Entry<String, String> entry : getTestRunArgMap().entrySet()) {
            runner.addInstrumentationArg(entry.getKey(), entry.getValue());
        }
        if (isTraceDirEnabled()) {
            mDevice.executeShellCommand(String.format("rm -rf %s/%s", mTraceDirectory,
                    TEST_LATENCY));
            runner.addInstrumentationArg("trace_directory",
                    String.format("%s/%s", mTraceDirectory, TEST_LATENCY));
        }
        return runner;
    }


    /**
     * Start the events logcat
     *
     * @param logReceiver
     * @param testName
     * @throws DeviceNotAvailableException
     */
    private void startEventsLogs(LogcatReceiver logReceiver, String testName)
            throws DeviceNotAvailableException {
        getDevice().clearLogcat();
        getDevice().executeShellCommand(EVENTS_CLEAR_CMD);
        logReceiver.start();
    }

    /**
     * Stop the events logcat and upload the data to sponge
     *
     * @param logReceiver
     */
    private void stopEventsLogs(LogcatReceiver logReceiver,String launchDesc) {
        try (InputStreamSource logcatData = logReceiver.getLogcatData()) {
            mListener.testLog(
                    String.format("%s-%s", EVENTS_LOG, launchDesc), LogDataType.TEXT, logcatData);
        } finally {
            logReceiver.stop();
        }
    }

    /**
     * Pull the trace files if exist under destDirectory and log it.
     *
     * @param listener test result listener
     * @param srcDirectory source directory in the device where the files are copied to the local
     *     tmp directory
     * @param subFolderName to store the files corresponding to the test
     * @throws DeviceNotAvailableException
     * @throws IOException
     */
    private void logTraceFiles(
            ITestInvocationListener listener, String srcDirectory, String subFolderName)
            throws DeviceNotAvailableException, IOException {
        File tmpDestDir = null;
        FileInputStreamSource streamSource = null;
        File zipFile = null;
        try {
            tmpDestDir = FileUtil.createTempDir(subFolderName);
            IFileEntry srcDir = mDevice.getFileEntry(String.format("%s/%s",
                    srcDirectory, subFolderName));
            // Files are retrieved from source directory in device
            if (srcDir != null) {
                for (IFileEntry file : srcDir.getChildren(false)) {
                    File pulledFile = new File(tmpDestDir, file.getName());
                    if (!mDevice.pullFile(file.getFullPath(), pulledFile)) {
                        throw new IOException(
                                "Not able to pull the file from test device");
                    }
                }
                zipFile = ZipUtil.createZip(tmpDestDir);
                streamSource = new FileInputStreamSource(zipFile);
                listener.testLog(tmpDestDir.getName(), LogDataType.ZIP, streamSource);
            }
        } finally {
            FileUtil.recursiveDelete(tmpDestDir);
            StreamUtil.cancel(streamSource);
            FileUtil.deleteFile(zipFile);
        }
    }

    /**
     * To upload the trace files stored in the traceDirectory in device to sponge.
     *
     * @param listener
     * @param subFolderName
     * @throws DeviceNotAvailableException
     */
    private void uploadTraceFiles(ITestInvocationListener listener, String subFolderName)
            throws DeviceNotAvailableException {
        try {
            logTraceFiles(listener, mTraceDirectory, subFolderName);
        } catch (IOException ioe) {
            CLog.e("Problem in uploading the log files.");
            CLog.e(ioe);
        }
        mDevice.executeShellCommand(String.format(REMOVE_CMD, mTraceDirectory,
                subFolderName));
    }

    /**
     * Returns false if the traceDirectory is not set.
     */
    private boolean isTraceDirEnabled() {
        return (null != mTraceDirectory && !mTraceDirectory.isEmpty());
    }

    /**
     * To parse the transition delay info from the events log.
     */
    private List<TransitionDelayItem> parseTransitionDelayInfo() {
        List<TransitionDelayItem> transitionDelayItems = null;
        try (InputStreamSource logcatData = mLaunchEventsLogs.getLogcatData();
                InputStream logcatStream = logcatData.createInputStream();
                InputStreamReader streamReader = new InputStreamReader(logcatStream);
                BufferedReader reader = new BufferedReader(streamReader)) {
            transitionDelayItems = mEventsLogParser.parseTransitionDelayInfo(reader);
        } catch (IOException e) {
            CLog.e("Problem in parsing the transition delay items from events log");
            CLog.e(e);
        }
        return transitionDelayItems;
    }

    /**
     * To parse the latency info from the events log.
     */
    private List<LatencyItem> parseLatencyInfo() {
        List<LatencyItem> latencyItems = null;
        try (InputStreamSource logcatData = mLaunchEventsLogs.getLogcatData();
                InputStream logcatStream = logcatData.createInputStream();
                InputStreamReader streamReader = new InputStreamReader(logcatStream);
                BufferedReader reader = new BufferedReader(streamReader)) {
            latencyItems = mEventsLogParser.parseLatencyInfo(reader);
        } catch (IOException e) {
            CLog.e("Problem in parsing the latency items from events log");
            CLog.e(e);
        }
        return latencyItems;
    }

    /**
     * Analyze and report the cold launch transition delay from launcher screen.
     *
     * @param transitionDelayItems
     */
    private void analyzeColdLaunchDelay(List<TransitionDelayItem> transitionDelayItems) {
        Map<String, String> cmpNameAppMap = reverseAppCmpInfoMap(getAppComponentInfoMap());
        Map<String, List<Long>> appKeyTransitionDelayMap = new HashMap<>();
        // Handle launcher to cold app launch transition
        for (TransitionDelayItem delayItem : transitionDelayItems) {
            if (cmpNameAppMap.containsKey(delayItem.getComponentName())) {
                String appName = cmpNameAppMap.get(delayItem.getComponentName());
                if (delayItem.getStartingWindowDelay() != null) {
                    if (appKeyTransitionDelayMap.containsKey(appName)) {
                        appKeyTransitionDelayMap.get(appName).add(
                                delayItem.getStartingWindowDelay());
                    } else {
                        List<Long> delayTimeList = new ArrayList<Long>();
                        delayTimeList.add(delayItem.getStartingWindowDelay());
                        appKeyTransitionDelayMap.put(appName, delayTimeList);
                    }
                }
            }
        }
        removeAdditionalLaunchInfo(appKeyTransitionDelayMap);
        computeAndUploadResults(TEST_COLD_LAUNCH, appKeyTransitionDelayMap);
    }

    /**
     * Analyze and report the hot launch transition delay from launcher and app
     * to home transition delay. Keep track of launcher to app transition delay
     * which immediately followed by app to home transition. Skip the
     * intitial cold launch on the apps.
     *
     * @param transitionDelayItems
     */
    private void analyzeHotLaunchDelay(List<TransitionDelayItem> transitionDelayItems) {
        Map<String, String> cmpNameAppMap = reverseAppCmpInfoMap(getAppComponentInfoMap());
        Map<String, List<Long>> appKeyTransitionDelayMap = new HashMap<>();
        Map<String, List<Long>> appToHomeKeyTransitionDelayMap = new HashMap<>();
        String prevAppName = null;
        for (TransitionDelayItem delayItem : transitionDelayItems) {
            // Handle app to home transition
            if (null != prevAppName) {
                if (delayItem.getComponentName().contains(mLauncherActivity)) {
                    if (appToHomeKeyTransitionDelayMap.containsKey(prevAppName)) {
                        appToHomeKeyTransitionDelayMap
                                .get(prevAppName)
                                .add(delayItem.getWindowDrawnDelay());
                    } else {
                        List<Long> delayTimeList = new ArrayList<Long>();
                        delayTimeList.add(delayItem.getWindowDrawnDelay());
                        appToHomeKeyTransitionDelayMap.put(prevAppName, delayTimeList);
                    }
                    prevAppName = null;
                }
                continue;
            }
            // Handle launcher to hot app launch transition
            if (cmpNameAppMap.containsKey(delayItem.getComponentName())) {
                // Not to consider the first cold launch for the app.
                if (delayItem.getStartingWindowDelay() != null) {
                    continue;
                }
                String appName = cmpNameAppMap.get(delayItem.getComponentName());
                if (appKeyTransitionDelayMap.containsKey(appName)) {
                    appKeyTransitionDelayMap.get(appName).add(delayItem.getTransitionDelay());
                } else {
                    List<Long> delayTimeList = new ArrayList<Long>();
                    delayTimeList.add(delayItem.getTransitionDelay());
                    appKeyTransitionDelayMap.put(appName, delayTimeList);
                }
                prevAppName = appName;
            }
        }
        // Remove the first hot launch info through intents
        removeAdditionalLaunchInfo(appKeyTransitionDelayMap);
        computeAndUploadResults(TEST_HOT_LAUNCH, appKeyTransitionDelayMap);
        removeAdditionalLaunchInfo(appToHomeKeyTransitionDelayMap);
        computeAndUploadResults(TEST_APP_TO_HOME, appToHomeKeyTransitionDelayMap);
    }

    /**
     * Analyze and report app to recents transition delay info.
     *
     * @param transitionDelayItems
     */
    private void analyzeAppToRecentsDelay(List<TransitionDelayItem> transitionDelayItems) {
        Map<String, String> cmpNameAppMap = reverseAppCmpInfoMap(getAppComponentInfoMap());
        Map<String, List<Long>> appKeyTransitionDelayMap = new HashMap<>();
        String prevAppName = null;
        for (TransitionDelayItem delayItem : transitionDelayItems) {
            if (delayItem.getComponentName().contains(mLauncherActivity)) {
                if (appKeyTransitionDelayMap.containsKey(prevAppName)) {
                    appKeyTransitionDelayMap.get(prevAppName).add(delayItem.getWindowDrawnDelay());
                } else {
                    if (null != prevAppName) {
                        List<Long> delayTimeList = new ArrayList<Long>();
                        delayTimeList.add(delayItem.getWindowDrawnDelay());
                        appKeyTransitionDelayMap.put(prevAppName, delayTimeList);
                    }
                }
                prevAppName = null;
                continue;
            }

            if (cmpNameAppMap.containsKey(delayItem.getComponentName())) {
                prevAppName = cmpNameAppMap.get(delayItem.getComponentName());
            }
        }
        //Removing the first cold launch to recents transition delay.
        removeAdditionalLaunchInfo(appKeyTransitionDelayMap);
        computeAndUploadResults(TEST_APP_TO_RECENT, appKeyTransitionDelayMap);
    }

    /**
     * Analyze and report recents to hot app launch delay info. Skip the intitial cold
     * launch transition delay on the apps. Also the first launch cannot always be the
     * cold launch because the apps could be part of preapps list. The transition delay is
     * tracked based on recents to apps transition delay items.
     *
     * @param transitionDelayItems
     */
    private void analyzeRecentsToAppDelay(List<TransitionDelayItem> transitionDelayItems) {
        Map<String, String> cmpNameAppMap = reverseAppCmpInfoMap(getAppComponentInfoMap());
        Map<String, List<Long>> appKeyTransitionDelayMap = new HashMap<>();
        boolean isRecentsBefore = false;
        for (TransitionDelayItem delayItem : transitionDelayItems) {
            if (delayItem.getComponentName().contains(mLauncherActivity)) {
                isRecentsBefore = true;
                continue;
            }
            if (isRecentsBefore && cmpNameAppMap.containsKey(delayItem.getComponentName())) {
                if (delayItem.getStartingWindowDelay() != null) {
                    continue;
                }
                String appName = cmpNameAppMap.get(delayItem.getComponentName());
                if (appKeyTransitionDelayMap.containsKey(appName)) {
                    appKeyTransitionDelayMap.get(appName).add(delayItem.getTransitionDelay());
                } else {
                    List<Long> delayTimeList = new ArrayList<Long>();
                    delayTimeList.add(delayItem.getTransitionDelay());
                    appKeyTransitionDelayMap.put(appName, delayTimeList);
                }
            }
            isRecentsBefore = false;
        }
        removeAdditionalLaunchInfo(appKeyTransitionDelayMap);
        computeAndUploadResults(TEST_HOT_LAUNCH_FROM_RECENTS, appKeyTransitionDelayMap);
    }


    /**
     * Analyze and report different latency delay items captured from the events log
     * file while running the LatencyTests
     * @param latencyItemsList
     */
    private void analyzeLatencyInfo(List<LatencyItem> latencyItemsList) {
        Map<String, List<Long>> actionDelayListMap = new HashMap<>();
        for (LatencyItem delayItem : latencyItemsList) {
            if (actionDelayListMap.containsKey(Integer.toString(delayItem.getActionId()))) {
                actionDelayListMap.get(Integer.toString(delayItem.getActionId())).add(
                        delayItem.getDelay());
            } else {
                List<Long> delayList = new ArrayList<Long>();
                delayList.add(delayItem.getDelay());
                actionDelayListMap.put(Integer.toString(delayItem.getActionId()), delayList);
            }
        }
        computeAndUploadResults(TEST_LATENCY, actionDelayListMap);
    }


    /**
     * To remove the additional launch info at the beginning of the test.
     *
     * @param appKeyTransitionDelayMap
     */
    private void removeAdditionalLaunchInfo(Map<String, List<Long>> appKeyTransitionDelayMap) {
        for (List<Long> delayList : appKeyTransitionDelayMap.values()) {
            while (delayList.size() > mLaunchIteration) {
                delayList.remove(0);
            }
        }
    }

    /**
     * To compute the min,max,avg,median and std_dev on the transition delay for each app
     * and upload the metrics
     *
     * @param reportingKey
     * @param appKeyTransitionDelayMap
     */
    private void computeAndUploadResults(String reportingKey,
            Map<String, List<Long>> appKeyTransitionDelayMap) {
        CLog.i(String.format("Testing : %s", reportingKey));
        Map<String, String> activityMetrics = new HashMap<String, String>();
        for (String appNameKey : appKeyTransitionDelayMap.keySet()) {
            List<Long> delayList = appKeyTransitionDelayMap.get(appNameKey);
            StringBuffer delayListStr = new StringBuffer();
            for (Long delayItem : delayList) {
                delayListStr.append(delayItem);
                delayListStr.append(",");
            }
            CLog.i("%s : %s", appNameKey, delayListStr);
            SimpleStats stats = new SimpleStats();
            for (Long delay : delayList) {
                stats.add(Double.parseDouble(delay.toString()));
            }
            activityMetrics.put(appNameKey + "_min", stats.min().toString());
            activityMetrics.put(appNameKey + "_max", stats.max().toString());
            activityMetrics.put(appNameKey + "_avg", stats.mean().toString());
            activityMetrics.put(appNameKey + "_median", stats.median().toString());
            activityMetrics.put(appNameKey + "_std_dev", stats.stdev().toString());
        }
        mListener.testRunStarted(reportingKey, 0);
        mListener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(activityMetrics));
    }

    /**
     * Retrieve the map of appname,componenetname from the results.
     */
    private Map<String,String> getAppComponentInfoMap() {
        Collection<TestResult> testResultsCollection = mLaunchListener
                .getCurrentRunResults().getTestResults().values();
        List<TestResult> testResults = new ArrayList<>(
                testResultsCollection);
        return testResults.get(0).getMetrics();
    }

    /**
     * Reverse and returnthe given appName,componentName info map to componenetName,appName info
     * map.
     */
    private Map<String, String> reverseAppCmpInfoMap(Map<String, String> appNameCmpNameMap) {
        Map<String, String> cmpNameAppNameMap = new HashMap<String, String>();
        for (Map.Entry<String, String> entry : appNameCmpNameMap.entrySet()) {
            cmpNameAppNameMap.put(entry.getValue(), entry.getKey());
        }
        return cmpNameAppNameMap;
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * @return the arguments map to pass to the test runner.
     */
    public Map<String, String> getTestRunArgMap() {
        return mArgMap;
    }

    /**
     * @param runArgMap the arguments to pass to the test runner.
     */
    public void setTestRunArgMap(Map<String, String> runArgMap) {
        mArgMap = runArgMap;
    }

}

