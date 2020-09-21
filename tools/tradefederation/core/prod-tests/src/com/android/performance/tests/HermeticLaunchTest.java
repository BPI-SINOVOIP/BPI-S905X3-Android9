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

package com.android.performance.tests;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.testrunner.IRemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
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
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * To test the app launch performance for the list of activities present in the given target package
 * or the custom list of activities present in the target package.Activities are launched number of
 * times present in the launch count. Launch time is analyzed from the logcat data and more detailed
 * timing(section names) is analyzed from the atrace files captured when launching each activity.
 */
public class HermeticLaunchTest implements IRemoteTest, IDeviceTest {

    private static enum AtraceSectionOptions {
        LAYOUT("layout"),
        DRAW("draw"),
        BINDAPPLICATION("bindApplication"),
        ACTIVITYSTART("activityStart"),
        ONCREATE("onCreate");

        private final String name;

        private AtraceSectionOptions(String s) {
            this.name = s;
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private static final String TOTALLAUNCHTIME = "totalLaunchTime";
    private static final String LOGCAT_CMD = "logcat -v threadtime ActivityManager:* *:s";
    private static final String LAUNCH_PREFIX="^\\d*-\\d*\\s*\\d*:\\d*:\\d*.\\d*\\s*\\d*\\s*"
            + "\\d*\\s*I ActivityManager: Displayed\\s*";
    private static final String LAUNCH_SUFFIX=":\\s*\\+(?<launchtime>.[a-zA-Z\\d]*)\\s*"
            + "(?<totallaunch>.*)\\s*$";
    private static final Pattern LAUNCH_ENTRY=Pattern.compile("^\\d*-\\d*\\s*\\d*:\\d*:\\d*."
            + "\\d*\\s*\\d*\\s*\\d*\\s*I ActivityManager: Displayed\\s*(?<launchinfo>.*)\\s*$");
    private static final Pattern TRACE_ENTRY1 = Pattern.compile(
            "^[^-]*-(?<tid>\\d+)\\s+\\[\\d+\\]\\s+\\S{4}\\s+" +
            "(?<secs>\\d+)\\.(?<usecs>\\d+):\\s+(?<function>.*)\\s*$");
    private static final Pattern TRACE_ENTRY2 = Pattern.compile(
            "^[^-]*-(?<tid>\\d+)\\s*\\(\\s*\\d*-*\\)\\s*\\[\\d+\\]\\s+\\S{4}\\s+" +
            "(?<secs>\\d+)\\.(?<usecs>\\d+):\\s+(?<function>.*)\\s*$");
    private static final Pattern ATRACE_BEGIN = Pattern
            .compile("tracing_mark_write: B\\|(?<pid>\\d+)\\|(?<name>.+)");
    // Matches new and old format of END time stamp.
    // rformanceLaunc-6315  ( 6315) [007] ...1   182.622217: tracing_mark_write: E|6315
    // rformanceLaunc-6315  ( 6315) [007] ...1   182.622217: tracing_mark_write: E
    private static final Pattern ATRACE_END = Pattern.compile(
            "tracing_mark_write: E\\|*(?<procid>\\d*)");
    private static final Pattern ATRACE_COUNTER = Pattern
            .compile("tracing_mark_write: C\\|(?<pid>\\d+)\\|(?<name>[^|]+)\\|(?<value>\\d+)");
    private static final Pattern ATRACE_HEADER_ENTRIES = Pattern
            .compile("# entries-in-buffer/entries-written:\\s+(?<buffered>\\d+)/"
                    + "(?<written>\\d+)\\s+#P:\\d+\\s*");
    private static final int LOGCAT_SIZE = 20971520; //20 mb
    private static final long SEC_TO_MILLI = 1000;
    private static final long MILLI_TO_MICRO = 1000;

    @Option(name = "runner", description = "The instrumentation test runner class name to use.")
    private String mRunnerName = "android.support.test.runner.AndroidJUnitRunner";

    @Option(name = "package", shortName = 'p',
            description = "The manifest package name of the Android test application to run.",
            importance = Importance.IF_UNSET)
    private String mPackageName = "com.android.performanceapp.tests";

    @Option(name = "target-package", description = "package which contains all the "
            + "activities to launch")
    private String mtargetPackage = null;

    @Option(name = "activity-names", description = "Fully qualified activity "
            + "names separated by comma"
            + "If not set then all the activities will be included for launching")
    private String mactivityNames = "";

    @Option(name = "launch-count", description = "number of time to launch the each activity")
    private int mlaunchCount = 10;

    @Option(name = "save-atrace", description = "Upload the atrace file in permanent storage")
    private boolean mSaveAtrace = false;

    @Option(name = "atrace-section", description = "Section to be parsed from atrace file. "
            + "This option can be repeated")
    private Set<AtraceSectionOptions> mSectionOptionSet = new HashSet<>();

    private ITestDevice mDevice = null;
    private IRemoteAndroidTestRunner mRunner;
    private LogcatReceiver mLogcat;
    private Set<String> mSectionSet = new HashSet<>();
    private Map<String, String> mActivityTraceFileMap;
    private Map<String, Map<String, String>> mActivityTimeResultMap = new HashMap<>();
    private Map<String,String> activityErrMsg=new HashMap<>();

    @Override
    public void run(ITestInvocationListener listener)
            throws DeviceNotAvailableException {

        mLogcat = new LogcatReceiver(getDevice(), LOGCAT_CMD, LOGCAT_SIZE, 0);
        mLogcat.start();
        try {
            if (mSectionOptionSet.isEmpty()) {
                // Default sections
                mSectionOptionSet.add(AtraceSectionOptions.LAYOUT);
                mSectionOptionSet.add(AtraceSectionOptions.DRAW);
                mSectionOptionSet.add(AtraceSectionOptions.BINDAPPLICATION);
                mSectionOptionSet.add(AtraceSectionOptions.ACTIVITYSTART);
                mSectionOptionSet.add(AtraceSectionOptions.ONCREATE);
            } else if (mSectionOptionSet.contains(AtraceSectionOptions.LAYOUT)) {
                // If layout is added, draw should also be included
                mSectionOptionSet.add(AtraceSectionOptions.DRAW);
            }

            for (AtraceSectionOptions sectionOption : mSectionOptionSet) {
                mSectionSet.add(sectionOption.toString());
            }

            //Remove if there is already existing atrace_logs folder
            mDevice.executeShellCommand("rm -rf ${EXTERNAL_STORAGE}/atrace_logs");

            mRunner = createRemoteAndroidTestRunner(mPackageName, mRunnerName,
                        mDevice.getIDevice());
            CollectingTestListener collectingListener = new CollectingTestListener();
            mDevice.runInstrumentationTests(mRunner, collectingListener);

            Collection<TestResult> testResultsCollection = collectingListener
                        .getCurrentRunResults().getTestResults().values();
            List<TestResult> testResults = new ArrayList<>(
                        testResultsCollection);
            /*
             * Expected Metrics : Map of <activity name>=<comma separated list of atrace file names in
             * external storage of the device>
             */
            mActivityTraceFileMap = testResults.get(0).getMetrics();
            Assert.assertTrue("Unable to get the path to the trace files stored in the device",
                    (mActivityTraceFileMap != null && !mActivityTraceFileMap.isEmpty()));

            // Analyze the logcat data to get total launch time
            analyzeLogCatData(mActivityTraceFileMap.keySet());
        } finally {
            // Stop the logcat
            mLogcat.stop();
        }

        // Analyze the atrace data to get bindApplication,activityStart etc..
        analyzeAtraceData(listener);

        // Report the metrics to dashboard
        reportMetrics(listener);
    }

    /**
     * Report run metrics by creating an empty test run to stick them in.
     * @param listener The {@link ITestInvocationListener} of test results
     */
    private void reportMetrics(ITestInvocationListener listener) {
        for (String activityName : mActivityTimeResultMap.keySet()) {
            // Get the activity name alone from pkgname.activityname
            String[] activityNameSplit = activityName.split("\\.");
            if (!activityErrMsg.containsKey(activityName)) {
                Map<String, String> activityMetrics = mActivityTimeResultMap.get(activityName);
                if (activityMetrics != null && !activityMetrics.isEmpty()) {
                    CLog.v("Metrics for the activity : %s", activityName);
                    for (String sectionName : activityMetrics.keySet()) {
                        CLog.v(String.format("Section name : %s - Time taken : %s",
                                sectionName, activityMetrics.get(sectionName)));
                    }
                    listener.testRunStarted(
                            activityNameSplit[activityNameSplit.length - 1].trim(), 0);
                    listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(activityMetrics));
                }
            } else {
                listener.testRunStarted(
                        activityNameSplit[activityNameSplit.length - 1].trim(), 0);
                listener.testRunFailed(activityErrMsg.get(activityName));
            }
        }
    }

    /**
     * Method to create the runner with given list of arguments
     * @return the {@link IRemoteAndroidTestRunner} to use.
     * @throws DeviceNotAvailableException
     */
    IRemoteAndroidTestRunner createRemoteAndroidTestRunner(String packageName,
            String runnerName, IDevice device)
            throws DeviceNotAvailableException {
        RemoteAndroidTestRunner runner = new RemoteAndroidTestRunner(
                packageName, runnerName, device);
        runner.addInstrumentationArg("targetpackage", mtargetPackage);
        runner.addInstrumentationArg("launchcount", mlaunchCount + "");
        if (mactivityNames != null && !mactivityNames.isEmpty()) {
            runner.addInstrumentationArg("activitylist", mactivityNames);
        }
        if (!mSaveAtrace) {
            runner.addInstrumentationArg("recordtrace", "false");
        }
        return runner;
    }

    /**
     * To analyze the log cat data to get the display time reported by activity manager during the
     * launches
     * activitySet is set of activityNames returned as a part of testMetrics from the device
     */
    public void analyzeLogCatData(Set<String> activitySet) {
        Map<String, List<Integer>> amLaunchTimes = new HashMap<>();

        Map<Pattern, String> activityPatternMap = new HashMap<>();
        Matcher match = null;
        String line;

        /*
         * Sample line format in logcat
         * 06-17 16:55:49.6 60 642 I ActivityManager: Displayed pkg/.activity: +Tms (total +9s9ms)
         */
        for (String activityName : activitySet) {
            int lastIndex = activityName.lastIndexOf(".");
            /*
             * actvitySet has set of activity names in the format packageName.activityName
             * logcat has the format packageName/.activityName --> activityAlias
             */
            String activityAlias = activityName.subSequence(0, lastIndex)
                    + "/" + activityName.subSequence(lastIndex, activityName.length());
            String finalPattern = LAUNCH_PREFIX + activityAlias + LAUNCH_SUFFIX;
            activityPatternMap.put(Pattern.compile(finalPattern),
                    activityName);
        }

        try (InputStreamSource input = mLogcat.getLogcatData();
                BufferedReader br =
                        new BufferedReader(new InputStreamReader(input.createInputStream()))) {
            while ((line = br.readLine()) != null) {
                /*
                 * Launch entry needed otherwise we will end up in comparing all the lines for all
                 * the patterns
                 */
                if ((match = matches(LAUNCH_ENTRY, line)) != null) {
                    for (Pattern pattern : activityPatternMap.keySet()) {
                        if ((match = matches(pattern, line)) != null) {
                            CLog.v("Launch Info : %s", line);
                            int displayTimeInMs = extractLaunchTime(match.group("launchtime"));
                            String activityName = activityPatternMap.get(pattern);
                            if (amLaunchTimes.containsKey(activityName)) {
                                amLaunchTimes.get(activityName).add(displayTimeInMs);
                            } else {
                                List<Integer> launchTimes = new ArrayList<>();
                                launchTimes.add(displayTimeInMs);
                                amLaunchTimes.put(activityName, launchTimes);
                            }
                        }
                    }
                }
            }
        } catch (IOException io) {
            CLog.e(io);
        }

        // Verify logcat data
        for (String activityName : amLaunchTimes.keySet()) {
            Assert.assertEquals("Data lost for launch time for the activity :"
                    + activityName, amLaunchTimes.get(activityName).size(), mlaunchCount);
        }

        /*
         * Extract and store the average launch time data reported by activity manager for each
         * activity
         */
        for (String activityName : amLaunchTimes.keySet()) {
            Double totalTime = 0d;
            for (Integer launchTime : amLaunchTimes.get(activityName)) {
                totalTime += launchTime;
            }
            Double averageTime = Double.valueOf(totalTime / amLaunchTimes.get(activityName).size());
            if (mActivityTimeResultMap.containsKey(activityName)) {
                mActivityTimeResultMap.get(activityName).put(TOTALLAUNCHTIME,
                        String.format("%.2f", averageTime));
            } else {
                Map<String, String> launchTime = new HashMap<>();
                launchTime.put(TOTALLAUNCHTIME,
                        String.format("%.2f", averageTime));
                mActivityTimeResultMap.put(activityName, launchTime);
            }
        }
    }

    /**
     * To extract the launch time displayed in given line
     *
     * @param duration
     * @return
     */
    public int extractLaunchTime(String duration) {
        String formattedString = duration.replace("ms", "");
        if (formattedString.contains("s")) {
            String[] splitString = formattedString.split("s");
            int finalTimeInMs = Integer.parseInt(splitString[0]) * 1000;
            finalTimeInMs = finalTimeInMs + Integer.parseInt(splitString[1]);
            return finalTimeInMs;
        } else {
            return Integer.parseInt(formattedString);
        }
    }

    /**
     * To analyze the trace data collected in the device during each activity launch.
     */
    public void analyzeAtraceData(ITestInvocationListener listener) throws DeviceNotAvailableException {
        for (String activityName : mActivityTraceFileMap.keySet()) {
            try {
                // Get the list of associated filenames for given activity
                String filePathAll = mActivityTraceFileMap.get(activityName);
                Assert.assertNotNull(
                        String.format("Unable to find trace file paths for activity : %s",
                                activityName), filePathAll);
                String[] filePaths = filePathAll.split(",");
                Assert.assertEquals(String.format("Unable to find file path for all the launches "
                        + "for the activity :%s", activityName), filePaths.length, mlaunchCount);
                // Pull and parse the info
                List<Map<String, List<SectionPeriod>>> mutipleLaunchTraceInfo =
                        new LinkedList<>();
                for (int count = 0; count < filePaths.length; count++) {
                    File currentAtraceFile = pullAtraceInfoFile(filePaths[count]);
                    String[] splitName = filePaths[count].split("-");
                    // Process id is appended to original file name
                    Map<String, List<SectionPeriod>> singleLaunchTraceInfo = parseAtraceInfoFile(
                            currentAtraceFile,
                            splitName[splitName.length - 1]);
                    // Upload the file if needed
                    if (mSaveAtrace) {
                        try (FileInputStreamSource stream =
                                new FileInputStreamSource(currentAtraceFile)) {
                            listener.testLog(currentAtraceFile.getName(), LogDataType.TEXT, stream);
                        }
                    }
                    // Remove the atrace files
                    FileUtil.deleteFile(currentAtraceFile);
                    mutipleLaunchTraceInfo.add(singleLaunchTraceInfo);
                }

                // Verify and Average out the aTrace Info and store it in result map
                averageAtraceData(activityName, mutipleLaunchTraceInfo);
            } catch (FileNotFoundException foe) {
                CLog.e(foe);
                activityErrMsg.put(activityName,
                        "Unable to find the trace file for the activity launch :" + activityName);
            } catch (IOException ioe) {
                CLog.e(ioe);
                activityErrMsg.put(activityName,
                        "Unable to read the contents of the atrace file for the activity :"
                                + activityName);
            }
        }

    }

    /**
     * To pull the trace file from the device
     * @param aTraceFile
     * @return
     * @throws DeviceNotAvailableException
     */
    public File pullAtraceInfoFile(String aTraceFile)
            throws DeviceNotAvailableException {
        String dir = "${EXTERNAL_STORAGE}/atrace_logs";
        File atraceFileHandler = null;
        atraceFileHandler = getDevice().pullFile(dir + "/" + aTraceFile);
        Assert.assertTrue("Unable to retrieve the atrace files", atraceFileHandler != null);
        return atraceFileHandler;
    }

    /**
     * To parse and find the time taken for the given section names in each launch
     * @param currentAtraceFile
     * @param sectionSet
     * @param processId
     * @return
     * @throws FileNotFoundException,IOException
     */
    public Map<String, List<SectionPeriod>> parseAtraceInfoFile(
            File currentAtraceFile, String processId)
            throws FileNotFoundException, IOException {
        CLog.v("Currently parsing :" + currentAtraceFile.getName());
        String line;
        BufferedReader br = null;
        br = new BufferedReader(new FileReader(currentAtraceFile));
        LinkedList<TraceRecord> processStack = new LinkedList<>();
        Map<String, List<SectionPeriod>> sectionInfo = new HashMap<>();

        while ((line = br.readLine()) != null) {
            // Skip extra lines that aren't part of the trace
            if (line.isEmpty() || line.startsWith("capturing trace...")
                    || line.equals("TRACE:") || line.equals("done")) {
                continue;
            }
            // Header information
            Matcher match = null;
            // Check if any trace entries were lost
            if ((match = matches(ATRACE_HEADER_ENTRIES, line)) != null) {
                int buffered = Integer.parseInt(match.group("buffered"));
                int written = Integer.parseInt(match.group("written"));
                if (written != buffered) {
                    CLog.w(String.format("%d trace entries lost for the file %s",
                            written - buffered, currentAtraceFile.getName()));
                }
            } else if ((match = matches(TRACE_ENTRY1, line)) != null
                    || (match = matches(TRACE_ENTRY2, line)) != null) {
                /*
                 * Two trace entries because trace format differs across devices <...>-tid [yyy]
                 * ...1 zzz.ttt: tracing_mark_write: B|xxxx|tag_name pkg.name ( tid) [yyy] ...1
                 * zzz.tttt: tracing_mark_write: B|xxxx|tag_name
                 */
                long timestamp = SEC_TO_MILLI
                        * Long.parseLong(match.group("secs"))
                        + Long.parseLong(match.group("usecs")) / MILLI_TO_MICRO;
                // Get the function name from the trace entry
                String taskId = match.group("tid");
                String function = match.group("function");
                // Analyze the lines that matches the processid
                if (!taskId.equals(processId)) {
                    continue;
                }
                if ((match = matches(ATRACE_BEGIN, function)) != null) {
                    // Matching pattern looks like tracing_mark_write: B|xxxx|tag_name
                    String sectionName = match.group("name");
                    // Push to the stack
                    processStack.add(new TraceRecord(sectionName, taskId,
                            timestamp));
                } else if ((match = matches(ATRACE_END, function)) != null) {
                    /*
                     * Matching pattern looks like tracing_mark_write: E Pop from the stack when end
                     * reaches
                     */
                    String endProcId = match.group("procid");
                    if (endProcId.isEmpty() || endProcId.equals(processId)) {
                        TraceRecord matchingBegin = processStack.removeLast();
                        if (mSectionSet.contains(matchingBegin.name)) {
                            if (sectionInfo.containsKey(matchingBegin.name)) {
                                SectionPeriod newSecPeriod = new SectionPeriod(
                                        matchingBegin.timestamp, timestamp);
                                CLog.v("Section :%s took :%f msecs ",
                                        matchingBegin.name, newSecPeriod.duration);
                                sectionInfo.get(matchingBegin.name).add(newSecPeriod);
                            } else {
                                List<SectionPeriod> infoList = new LinkedList<>();
                                SectionPeriod newSecPeriod = new SectionPeriod(
                                        matchingBegin.timestamp, timestamp);
                                CLog.v(String.format("Section :%s took :%f msecs ",
                                        matchingBegin.name, newSecPeriod.duration));
                                infoList.add(newSecPeriod);
                                sectionInfo.put(matchingBegin.name, infoList);
                            }
                        }
                    }
                } else if ((match = matches(ATRACE_COUNTER, function)) != null) {
                    // Skip this for now. May want to track these later if needed.
                }
            }

        }
        StreamUtil.close(br);
        return sectionInfo;
    }

    /**
     * To take the average of the multiple launches for each activity
     * @param activityName
     * @param mutipleLaunchTraceInfo
     */
    public void averageAtraceData(String activityName,
            List<Map<String, List<SectionPeriod>>> mutipleLaunchTraceInfo) {
        String verificationResult = verifyAtraceMapInfo(mutipleLaunchTraceInfo);
        if (verificationResult != null) {
            CLog.w(
                    "Not all the section info captured for the activity :%s. Missing: %s. "
                            + "Please go to atrace file to look for detail.",
                    activityName, verificationResult);
        }
        Map<String, Double> launchSum = new HashMap<>();
        for (String sectionName : mSectionSet) {
            launchSum.put(sectionName, 0d);
        }
        for (Map<String, List<SectionPeriod>> singleLaunchInfo : mutipleLaunchTraceInfo) {
            for (String sectionName : singleLaunchInfo.keySet()) {
                for (SectionPeriod secPeriod : singleLaunchInfo
                        .get(sectionName)) {
                    if (sectionName.equals(AtraceSectionOptions.DRAW.toString())) {
                        // Get the first draw time for the launch
                        Double currentSum = launchSum.get(sectionName)
                                + secPeriod.duration;
                        launchSum.put(sectionName, currentSum);
                        break;
                    }
                    //Sum the multiple layout times before the first draw in this launch
                    if (sectionName.equals(AtraceSectionOptions.LAYOUT.toString())) {
                        Double drawStartTime = singleLaunchInfo
                                .get(AtraceSectionOptions.DRAW.toString()).get(0).startTime;
                        if (drawStartTime < secPeriod.startTime) {
                            break;
                        }
                    }
                    Double currentSum = launchSum.get(sectionName) + secPeriod.duration;
                    launchSum.put(sectionName, currentSum);
                }
            }
        }
        // Update the final result map
        for (String sectionName : mSectionSet) {
            Double averageTime = launchSum.get(sectionName)
                    / mutipleLaunchTraceInfo.size();
            mActivityTimeResultMap.get(activityName).put(sectionName,
                    String.format("%.2f", averageTime));
        }
    }

    /**
     * To check if all the section info caught for all the app launches
     *
     * @param multipleLaunchTraceInfo
     * @return String: the missing section name, null if no section info missing.
     */
    public String verifyAtraceMapInfo(
            List<Map<String, List<SectionPeriod>>> multipleLaunchTraceInfo) {
        for (Map<String, List<SectionPeriod>> singleLaunchInfo : multipleLaunchTraceInfo) {
            Set<String> testSet = new HashSet<>(mSectionSet);
            testSet.removeAll(singleLaunchInfo.keySet());
            if (testSet.size() != 0) {
                return testSet.toString();
            }
        }
        return null;
    }


    /**
     * Checks whether {@code line} matches the given {@link Pattern}.
     * @return The resulting {@link Matcher} obtained by matching the {@code line} against
     *         {@code pattern}, or null if the {@code line} does not match.
     */
    private static Matcher matches(Pattern pattern, String line) {
        Matcher ret = pattern.matcher(line);
        return ret.matches() ? ret : null;
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
     * A record to keep track of the section start time,end time and the duration in milliseconds.
     */
    public static class SectionPeriod {

        private double startTime;
        private double endTime;
        private double duration;

        public SectionPeriod(double startTime, double endTime) {
            this.startTime = startTime;
            this.endTime = endTime;
            this.duration = endTime - startTime;
        }

        public double getStartTime() {
            return startTime;
        }

        public void setStartTime(long startTime) {
            this.startTime = startTime;
        }

        public double getEndTime() {
            return endTime;
        }

        public void setEndTime(long endTime) {
            this.endTime = endTime;
        }

        public double getDuration() {
            return duration;
        }

        public void setDuration(long duration) {
            this.duration = duration;
        }
    }

    /**
     * A record of a trace event. Includes the name of the section, and the time that the event
     * occurred (in milliseconds).
     */
    public static class TraceRecord {

        private String name;
        private String processId;
        private double timestamp;

        /**
         * Construct a new {@link TraceRecord} with the given {@code name} and {@code timestamp} .
         */
        public TraceRecord(String name, String processId, long timestamp) {
            this.name = name;
            this.processId = processId;
            this.timestamp = timestamp;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public String getProcessId() {
            return processId;
        }

        public void setProcessId(String processId) {
            this.processId = processId;
        }

        public double getTimestamp() {
            return timestamp;
        }

        public void setTimestamp(long timestamp) {
            this.timestamp = timestamp;
        }
    }

}
