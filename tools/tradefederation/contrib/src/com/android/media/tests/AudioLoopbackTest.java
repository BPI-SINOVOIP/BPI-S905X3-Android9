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
package com.android.media.tests;

import static java.nio.file.StandardCopyOption.REPLACE_EXISTING;

import com.android.ddmlib.NullOutputReceiver;
import com.android.media.tests.AudioLoopbackImageAnalyzer.Result;
import com.android.media.tests.AudioLoopbackTestHelper.LogFileType;
import com.android.media.tests.AudioLoopbackTestHelper.ResultData;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Runs Audio Latency and Audio Glitch test and reports result.
 *
 * <p>Strategy for Audio Latency Stress test: RUN test 1000 times. In each iteration, collect result
 * files from device, parse and collect data in a ResultData object that also keeps track of
 * location to test files for a particular iteration.
 *
 * <p>ANALYZE test results to produce statistics for 1. Latency and Confidence (Min, Max, Mean,
 * Median) 2. Create CSV file with test run data 3. Print bad test data to host log file 4. Get
 * number of test runs with valid data to send to dashboard 5. Produce histogram in host log file;
 * count number of test results that fall into 1 ms wide buckets.
 *
 * <p>UPLOAD test results + log files from “bad” runs; i.e. runs that is missing some or all result
 * data.
 */
public class AudioLoopbackTest implements IDeviceTest, IRemoteTest {

    //===================================================================
    // TEST OPTIONS
    //===================================================================
    @Option(name = "run-key", description = "Run key for the test")
    private String mRunKey = "AudioLoopback";

    @Option(name = "sampling-freq", description = "Sampling Frequency for Loopback app")
    private String mSamplingFreq = "48000";

    @Option(name = "mic-source", description = "Mic Source for Loopback app")
    private String mMicSource = "3";

    @Option(name = "audio-thread", description = "Audio Thread for Loopback app")
    private String mAudioThread = "1";

    @Option(
        name = "audio-level",
        description =
                "Audio Level for Loopback app. A device specific"
                        + "param which makes waveform in loopback test hit 60% to 80% range"
    )
    private String mAudioLevel = "-1";

    @Option(name = "test-type", description = "Test type to be executed")
    private String mTestType = TESTTYPE_LATENCY_STR;

    @Option(name = "buffer-test-duration", description = "Buffer test duration in seconds")
    private String mBufferTestDuration = "10";

    @Option(name = "key-prefix", description = "Key Prefix for reporting")
    private String mKeyPrefix = "48000_Mic3_";

    @Option(name = "iterations", description = "Number of test iterations")
    private int mIterations = 1;

    @Option(name = "baseline_latency", description = "")
    private float mBaselineLatency = 0f;

    //===================================================================
    // CLASS VARIABLES
    //===================================================================
    private static final Map<String, String> METRICS_KEY_MAP = createMetricsKeyMap();
    private Map<LogFileType, LogFileData> mFileDataKeyMap;
    private ITestDevice mDevice;
    private TestRunHelper mTestRunHelper;
    private AudioLoopbackTestHelper mLoopbackTestHelper;

    //===================================================================
    // CONSTANTS
    //===================================================================
    private static final String TESTTYPE_LATENCY_STR = "222";
    private static final String TESTTYPE_GLITCH_STR = "223";
    private static final long TIMEOUT_MS = 5 * 60 * 1000; // 5 min
    private static final long DEVICE_SYNC_MS = 5 * 60 * 1000; // 5 min
    private static final long POLLING_INTERVAL_MS = 5 * 1000;
    private static final int MAX_ATTEMPTS = 3;
    private static final int MAX_NR_OF_LOG_UPLOADS = 100;

    private static final int LATENCY_ITERATIONS_LOWER_BOUND = 1;
    private static final int LATENCY_ITERATIONS_UPPER_BOUND = 10000;
    private static final int GLITCH_ITERATIONS_LOWER_BOUND = 1;
    private static final int GLITCH_ITERATIONS_UPPER_BOUND = 1;

    private static final String DEVICE_TEMP_DIR_PATH = "/sdcard/";
    private static final String FMT_OUTPUT_PREFIX = "output_%1$d_" + System.currentTimeMillis();
    private static final String FMT_DEVICE_FILENAME = FMT_OUTPUT_PREFIX + "%2$s";
    private static final String FMT_DEVICE_PATH = DEVICE_TEMP_DIR_PATH + FMT_DEVICE_FILENAME;

    private static final String AM_CMD =
            "am start -n org.drrickorang.loopback/.LoopbackActivity"
                    + " --ei SF %s --es FileName %s --ei MicSource %s --ei AudioThread %s"
                    + " --ei AudioLevel %s --ei TestType %s --ei BufferTestDuration %s";

    private static final String ERR_PARAMETER_OUT_OF_BOUNDS =
            "Test parameter '%1$s' is out of bounds. Lower limit = %2$d, upper limit = %3$d";

    private static final String KEY_RESULT_LATENCY_MS = "latency_ms";
    private static final String KEY_RESULT_LATENCY_CONFIDENCE = "latency_confidence";
    private static final String KEY_RESULT_RECORDER_BENCHMARK = "recorder_benchmark";
    private static final String KEY_RESULT_RECORDER_OUTLIER = "recorder_outliers";
    private static final String KEY_RESULT_PLAYER_BENCHMARK = "player_benchmark";
    private static final String KEY_RESULT_PLAYER_OUTLIER = "player_outliers";
    private static final String KEY_RESULT_NUMBER_OF_GLITCHES = "number_of_glitches";
    private static final String KEY_RESULT_RECORDER_BUFFER_CALLBACK = "late_recorder_callbacks";
    private static final String KEY_RESULT_PLAYER_BUFFER_CALLBACK = "late_player_callbacks";
    private static final String KEY_RESULT_GLITCHES_PER_HOUR = "glitches_per_hour";
    private static final String KEY_RESULT_TEST_STATUS = "test_status";
    private static final String KEY_RESULT_AUDIO_LEVEL = "audio_level";
    private static final String KEY_RESULT_RMS = "rms";
    private static final String KEY_RESULT_RMS_AVERAGE = "rms_average";
    private static final String KEY_RESULT_SAMPLING_FREQUENCY_CONFIDENCE = "sampling_frequency";
    private static final String KEY_RESULT_PERIOD_CONFIDENCE = "period_confidence";
    private static final String KEY_RESULT_SAMPLING_BLOCK_SIZE = "block_size";

    private static final String REDUCED_GLITCHES_TEST_DURATION = "600"; // 10 min

    private static final LogFileType[] LATENCY_TEST_LOGS = {
        LogFileType.RESULT,
        LogFileType.GRAPH,
        LogFileType.WAVE,
        LogFileType.PLAYER_BUFFER,
        LogFileType.PLAYER_BUFFER_HISTOGRAM,
        LogFileType.PLAYER_BUFFER_PERIOD_TIMES,
        LogFileType.RECORDER_BUFFER,
        LogFileType.RECORDER_BUFFER_HISTOGRAM,
        LogFileType.RECORDER_BUFFER_PERIOD_TIMES,
        LogFileType.LOGCAT
    };

    private static final LogFileType[] GLITCH_TEST_LOGS = {
        LogFileType.RESULT,
        LogFileType.GRAPH,
        LogFileType.WAVE,
        LogFileType.PLAYER_BUFFER,
        LogFileType.PLAYER_BUFFER_HISTOGRAM,
        LogFileType.PLAYER_BUFFER_PERIOD_TIMES,
        LogFileType.RECORDER_BUFFER,
        LogFileType.RECORDER_BUFFER_HISTOGRAM,
        LogFileType.RECORDER_BUFFER_PERIOD_TIMES,
        LogFileType.GLITCHES_MILLIS,
        LogFileType.HEAT_MAP,
        LogFileType.LOGCAT
    };

    /**
     * The Audio Latency and Audio Glitch test deals with many various types of log files. To be
     * able to generate log files in a generic manner, this map is provided to get access to log
     * file properties like log name prefix, log name file extension and log type (leveraging
     * tradefed class LogDataType, used when uploading log).
     */
    private final synchronized Map<LogFileType, LogFileData> getLogFileDataKeyMap() {
        if (mFileDataKeyMap != null) {
            return mFileDataKeyMap;
        }

        final Map<LogFileType, LogFileData> result = new HashMap<LogFileType, LogFileData>();

        // Populate dictionary with info about various types of logfiles
        LogFileData l = new LogFileData(".txt", "result", LogDataType.TEXT);
        result.put(LogFileType.RESULT, l);

        l = new LogFileData(".png", "graph", LogDataType.PNG);
        result.put(LogFileType.GRAPH, l);

        l = new LogFileData(".wav", "wave", LogDataType.UNKNOWN);
        result.put(LogFileType.WAVE, l);

        l = new LogFileData("_playerBufferPeriod.txt", "player_buffer", LogDataType.TEXT);
        result.put(LogFileType.PLAYER_BUFFER, l);

        l = new LogFileData("_playerBufferPeriod.png", "player_buffer_histogram", LogDataType.PNG);
        result.put(LogFileType.PLAYER_BUFFER_HISTOGRAM, l);

        String fileExtension = "_playerBufferPeriodTimes.txt";
        String uploadName = "player_buffer_period_times";
        l = new LogFileData(fileExtension, uploadName, LogDataType.TEXT);
        result.put(LogFileType.PLAYER_BUFFER_PERIOD_TIMES, l);

        l = new LogFileData("_recorderBufferPeriod.txt", "recorder_buffer", LogDataType.TEXT);
        result.put(LogFileType.RECORDER_BUFFER, l);

        fileExtension = "_recorderBufferPeriod.png";
        uploadName = "recorder_buffer_histogram";
        l = new LogFileData(fileExtension, uploadName, LogDataType.PNG);
        result.put(LogFileType.RECORDER_BUFFER_HISTOGRAM, l);

        fileExtension = "_recorderBufferPeriodTimes.txt";
        uploadName = "recorder_buffer_period_times";
        l = new LogFileData(fileExtension, uploadName, LogDataType.TEXT);
        result.put(LogFileType.RECORDER_BUFFER_PERIOD_TIMES, l);

        l = new LogFileData("_glitchMillis.txt", "glitches_millis", LogDataType.TEXT);
        result.put(LogFileType.GLITCHES_MILLIS, l);


        l = new LogFileData("_heatMap.png", "heat_map", LogDataType.PNG);
        result.put(LogFileType.HEAT_MAP, l);

        l = new LogFileData(".txt", "logcat", LogDataType.TEXT);
        result.put(LogFileType.LOGCAT, l);

        mFileDataKeyMap = Collections.unmodifiableMap(result);
        return mFileDataKeyMap;
    }

    private static final Map<String, String> createMetricsKeyMap() {
        final Map<String, String> result = new HashMap<String, String>();

        result.put("LatencyMs", KEY_RESULT_LATENCY_MS);
        result.put("LatencyConfidence", KEY_RESULT_LATENCY_CONFIDENCE);
        result.put("SF", KEY_RESULT_SAMPLING_FREQUENCY_CONFIDENCE);
        result.put("Recorder Benchmark", KEY_RESULT_RECORDER_BENCHMARK);
        result.put("Recorder Number of Outliers", KEY_RESULT_RECORDER_OUTLIER);
        result.put("Player Benchmark", KEY_RESULT_PLAYER_BENCHMARK);
        result.put("Player Number of Outliers", KEY_RESULT_PLAYER_OUTLIER);
        result.put("Total Number of Glitches", KEY_RESULT_NUMBER_OF_GLITCHES);
        result.put("kth% Late Recorder Buffer Callbacks", KEY_RESULT_RECORDER_BUFFER_CALLBACK);
        result.put("kth% Late Player Buffer Callbacks", KEY_RESULT_PLAYER_BUFFER_CALLBACK);
        result.put("Glitches Per Hour", KEY_RESULT_GLITCHES_PER_HOUR);
        result.put("Test Status", KEY_RESULT_TEST_STATUS);
        result.put("AudioLevel", KEY_RESULT_AUDIO_LEVEL);
        result.put("RMS", KEY_RESULT_RMS);
        result.put("Average", KEY_RESULT_RMS_AVERAGE);
        result.put("PeriodConfidence", KEY_RESULT_PERIOD_CONFIDENCE);
        result.put("BS", KEY_RESULT_SAMPLING_BLOCK_SIZE);

        return Collections.unmodifiableMap(result);
    }

    //===================================================================
    // ENUMS
    //===================================================================
    public enum TestType {
        GLITCH,
        LATENCY,
        LATENCY_STRESS,
        NONE
    }

    //===================================================================
    // INNER CLASSES
    //===================================================================
    public final class LogFileData {
        private String fileExtension;
        private String filePrefix;
        private LogDataType logDataType;

        private LogFileData(String fileExtension, String filePrefix, LogDataType logDataType) {
            this.fileExtension = fileExtension;
            this.filePrefix = filePrefix;
            this.logDataType = logDataType;
        }
    }

    //===================================================================
    // FUNCTIONS
    //===================================================================

    /** {@inheritDoc} */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /** {@inheritDoc} */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * Test Entry Point
     *
     * <p>{@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {

        initializeTest(listener);

        mTestRunHelper.startTest(1);

        Map<String, String> metrics = null;
        try {
            if (!verifyTestParameters()) {
                return;
            }

            // Stop logcat logging so we can record one logcat log per iteration
            getDevice().stopLogcat();

            switch (getTestType()) {
                case GLITCH:
                    runGlitchesTest(mTestRunHelper, mLoopbackTestHelper);
                    break;
                case LATENCY:
                case LATENCY_STRESS:
                    // Run test iterations
                    runLatencyTest(mLoopbackTestHelper, mIterations);
                    break;
                default:
                    break;
            }

            mLoopbackTestHelper.processTestData();
            metrics = uploadLogsReturnMetrics(listener);
            CLog.i("Uploading metrics values:\n" + Arrays.toString(metrics.entrySet().toArray()));
            mTestRunHelper.endTest(metrics);
        } catch (TestFailureException e) {
            CLog.i("TestRunHelper.reportFailure triggered");
        } finally {
            CLog.i("Test ended - cleanup");
            deleteAllTempFiles();
            getDevice().startLogcat();
        }
    }

    private void runLatencyTest(AudioLoopbackTestHelper loopbackTestHelper, int iterations)
            throws DeviceNotAvailableException, TestFailureException {
        for (int i = 0; i < iterations; i++) {
            CLog.i("---- Iteration " + i + " of " + (iterations - 1) + " -----");

            final ResultData d = new ResultData();
            d.setIteration(i);
            Map<String, String> resultsDictionary = null;
            resultsDictionary = runTest(d, getSingleTestTimeoutValue());
            loopbackTestHelper.addTestData(d, resultsDictionary, true);
        }
    }

    /**
     * Glitches test, strategy:
     * <p>
     *
     * <ul>
     *   <li>1. Calibrate Audio level
     *   <li>2. Run Audio Latency test until seeing good waveform
     *   <li>3. Run small Glitches test, 5-10 seconds
     *   <li>4. If numbers look good, run long Glitches test, else run reduced Glitches test
     * </ul>
     *
     * @param testRunHelper
     * @param loopbackTestHelper
     * @throws DeviceNotAvailableException
     * @throws TestFailureException
     */
    private void runGlitchesTest(TestRunHelper testRunHelper,
            AudioLoopbackTestHelper loopbackTestHelper)
                    throws DeviceNotAvailableException, TestFailureException {
        final int MAX_RETRIES = 3;
        int nrOfSuccessfulTests;
        int counter = 0;
        AudioLoopbackTestHelper tempTestHelper = null;
        boolean runningReducedGlitchesTest = false;

        // Step 1: Calibrate Audio level
        // Step 2: Run Audio Latency test until seeing good waveform
        final int LOOPBACK_ITERATIONS = 4;
        final String originalTestType = mTestType;
        final String originalBufferTestDuration = mBufferTestDuration;
        mTestType = TESTTYPE_LATENCY_STR;
        do {
            nrOfSuccessfulTests = 0;
            tempTestHelper = new AudioLoopbackTestHelper(LOOPBACK_ITERATIONS);
            runLatencyTest(tempTestHelper, LOOPBACK_ITERATIONS);
            nrOfSuccessfulTests = tempTestHelper.processTestData();
            counter++;
        } while (nrOfSuccessfulTests <= 0 && counter <= MAX_RETRIES);

        if (nrOfSuccessfulTests <= 0) {
            testRunHelper.reportFailure("Glitch Setup failed: Latency test");
        }

        // Retrieve audio level from successful test
        int audioLevel = -1;
        List<ResultData> results = tempTestHelper.getAllTestData();
        for (ResultData rd : results) {
            // Check if test passed
            if (rd.getImageAnalyzerResult() == Result.PASS && rd.getConfidence() == 1.0) {
                audioLevel = rd.getAudioLevel();
                break;
            }
        }

        if (audioLevel < 6) {
            testRunHelper.reportFailure("Glitch Setup failed: Audio level not valid");
        }

        CLog.i("Audio Glitch: Audio level is " + audioLevel);

        // Step 3: Run small Glitches test, 5-10 seconds
        mTestType = originalTestType;
        mBufferTestDuration = "10";
        mAudioLevel = Integer.toString(audioLevel);

        counter = 0;
        int glitches = -1;
        do {
            tempTestHelper = new AudioLoopbackTestHelper(1);
            runLatencyTest(tempTestHelper, 1);
            Map<String, String> resultsDictionary =
                    tempTestHelper.getResultDictionaryForIteration(0);
            final String nrOfGlitches =
                    resultsDictionary.get(getMetricsKey(KEY_RESULT_NUMBER_OF_GLITCHES));
            glitches = Integer.parseInt(nrOfGlitches);
            CLog.i("10 s glitch test produced " + glitches + " glitches");
            counter++;
        } while (glitches > 10 || glitches < 0 && counter <= MAX_RETRIES);

        // Step 4: If numbers look good, run long Glitches test
        if (glitches > 10 || glitches < 0) {
            // Reduce test time and set some values to 0 once test completes
            runningReducedGlitchesTest = true;
            mBufferTestDuration = REDUCED_GLITCHES_TEST_DURATION;
        } else {
            mBufferTestDuration = originalBufferTestDuration;
        }

        final ResultData d = new ResultData();
        d.setIteration(0);
        Map<String, String> resultsDictionary = null;
        resultsDictionary = runTest(d, getSingleTestTimeoutValue());
        if (runningReducedGlitchesTest) {
            // Special treatment, we want to upload values, but also indicate that pre-test
            // conditions failed. We will set the glitches count and zero out the rest.
            String[] testValuesToChangeArray = new String[] {
                KEY_RESULT_RECORDER_BENCHMARK,
                KEY_RESULT_RECORDER_OUTLIER,
                KEY_RESULT_PLAYER_BENCHMARK,
                KEY_RESULT_PLAYER_OUTLIER,
                KEY_RESULT_RECORDER_BUFFER_CALLBACK,
                KEY_RESULT_PLAYER_BUFFER_CALLBACK
            };

            for (String key : testValuesToChangeArray) {
                final String metricsKey = getMetricsKey(key);
                if (resultsDictionary.containsKey(metricsKey)) {
                    resultsDictionary.put(metricsKey, "0");
                }
            }
        }

        loopbackTestHelper.addTestData(d, resultsDictionary, false);
    }

    private void initializeTest(ITestInvocationListener listener)
            throws UnsupportedOperationException, DeviceNotAvailableException {

        mFileDataKeyMap = getLogFileDataKeyMap();
        TestDescription testId = new TestDescription(getClass().getCanonicalName(), mRunKey);

        // Allocate helpers
        mTestRunHelper = new TestRunHelper(listener, testId);
        mLoopbackTestHelper = new AudioLoopbackTestHelper(mIterations);

        getDevice().disableKeyguard();
        getDevice().waitForDeviceAvailable(DEVICE_SYNC_MS);

        getDevice().setDate(new Date());
        CLog.i("syncing device time to host time");
    }

    private Map<String, String> runTest(ResultData data, final long timeout)
            throws DeviceNotAvailableException, TestFailureException {

        // start measurement and wait for result file
        final NullOutputReceiver receiver = new NullOutputReceiver();

        final String loopbackCmd = getTestCommand(data.getIteration());
        CLog.i("Loopback cmd: " + loopbackCmd);

        // Clear logcat
        // Seems like getDevice().clearLogcat(); doesn't do anything?
        // Do it through ADB
        getDevice().executeAdbCommand("logcat", "-c");
        final long deviceTestStartTime = getDevice().getDeviceDate();

        getDevice()
                .executeShellCommand(
                        loopbackCmd, receiver, TIMEOUT_MS, TimeUnit.MILLISECONDS, MAX_ATTEMPTS);

        final long loopbackStartTime = System.currentTimeMillis();
        File loopbackReport = null;

        data.setDeviceTestStartTime(deviceTestStartTime);

        // Try to retrieve result file from device.
        final String resultFilename = getDeviceFilename(LogFileType.RESULT, data.getIteration());
        do {
            RunUtil.getDefault().sleep(POLLING_INTERVAL_MS);
            if (getDevice().doesFileExist(resultFilename)) {
                // Store device log files in tmp directory on Host and add to ResultData object
                storeDeviceFilesOnHost(data);
                final String reportFilename = data.getLogFile(LogFileType.RESULT);
                if (reportFilename != null && !reportFilename.isEmpty()) {
                    loopbackReport = new File(reportFilename);
                    if (loopbackReport.length() > 0) {
                        break;
                    }
                }
            }

            data.setIsTimedOut(System.currentTimeMillis() - loopbackStartTime >= timeout);
        } while (!data.hasLogFile(LogFileType.RESULT) && !data.isTimedOut());

        // Grab logcat for iteration
        try (final InputStreamSource lc = getDevice().getLogcatSince(deviceTestStartTime)) {
            saveLogcatForIteration(data, lc, data.getIteration());
        }

        // Check if test timed out. If so, don't fail the test, but return to upper logic.
        // We accept certain number of individual test timeouts.
        if (data.isTimedOut()) {
            // No device result files retrieved, so no need to parse
            return null;
        }

        // parse result
        Map<String, String> loopbackResult = null;

        try {
            loopbackResult =
                    AudioLoopbackTestHelper.parseKeyValuePairFromFile(
                            loopbackReport, METRICS_KEY_MAP, mKeyPrefix, "=", "%s: %s");
            populateResultData(loopbackResult, data);

            // Trust but verify, so get Audio Level from ADB and compare to value from app
            final int adbAudioLevel =
                    AudioLevelUtility.extractDeviceHeadsetLevelFromAdbShell(getDevice());
            if (adbAudioLevel > -1 && data.getAudioLevel() != adbAudioLevel) {
                final String errMsg =
                        String.format(
                                "App Audio Level (%1$d) differs from ADB level (%2$d)",
                                data.getAudioLevel(), adbAudioLevel);
                mTestRunHelper.reportFailure(errMsg);
            }
        } catch (final IOException ioe) {
            CLog.e(ioe);
            mTestRunHelper.reportFailure("I/O error while parsing Loopback result.");
        } catch (final NumberFormatException ne) {
            CLog.e(ne);
            mTestRunHelper.reportFailure("Number format error parsing Loopback result.");
        }

        return loopbackResult;
    }

    private String getMetricsKey(final String key) {
        return mKeyPrefix + key;
    }
    private final long getSingleTestTimeoutValue() {
        return Long.parseLong(mBufferTestDuration) * 1000 + TIMEOUT_MS;
    }

    private Map<String, String> uploadLogsReturnMetrics(ITestInvocationListener listener)
            throws DeviceNotAvailableException, TestFailureException {

        // "resultDictionary" is used to post results to dashboards like BlackBox
        // "results" contains test logs to be uploaded; i.e. to Sponge

        List<ResultData> results = null;
        Map<String, String> resultDictionary = new HashMap<String, String>();

        switch (getTestType()) {
            case GLITCH:
                resultDictionary = mLoopbackTestHelper.getResultDictionaryForIteration(0);
                // Upload all test files to be backward compatible with old test
                results = mLoopbackTestHelper.getAllTestData();
                break;
            case LATENCY:
                {
                    final int nrOfValidResults = mLoopbackTestHelper.processTestData();
                    if (nrOfValidResults == 0) {
                        mTestRunHelper.reportFailure("No good data was collected");
                    } else {
                        // use dictionary collected from single test run
                        resultDictionary = mLoopbackTestHelper.getResultDictionaryForIteration(0);
                    }

                    // Upload all test files to be backward compatible with old test
                    results = mLoopbackTestHelper.getAllTestData();
                }
                break;
            case LATENCY_STRESS:
                {
                    final int nrOfValidResults = mLoopbackTestHelper.processTestData();
                    if (nrOfValidResults == 0) {
                        mTestRunHelper.reportFailure("No good data was collected");
                    } else {
                        mLoopbackTestHelper.populateStressTestMetrics(resultDictionary, mKeyPrefix);
                    }

                    results = mLoopbackTestHelper.getWorstResults(MAX_NR_OF_LOG_UPLOADS);

                    // Save all test data in a spreadsheet style csv file for post test analysis
                    try {
                        saveResultsAsCSVFile(listener);
                    } catch (final IOException e) {
                        CLog.e(e);
                    }
                }
                break;
            default:
                break;
        }

        // Upload relevant logs
        for (final ResultData d : results) {
            final LogFileType[] logFileTypes = getLogFileTypesForCurrentTest();
            for (final LogFileType logType : logFileTypes) {
                uploadLog(listener, logType, d);
            }
        }

        return resultDictionary;
    }

    private TestType getTestType() {
        if (mTestType.equals(TESTTYPE_GLITCH_STR)) {
            if (GLITCH_ITERATIONS_LOWER_BOUND <= mIterations
                    && mIterations <= GLITCH_ITERATIONS_UPPER_BOUND) {
                return TestType.GLITCH;
            }
        }

        if (mTestType.equals(TESTTYPE_LATENCY_STR)) {
            if (mIterations == 1) {
                return TestType.LATENCY;
            }

            if (LATENCY_ITERATIONS_LOWER_BOUND <= mIterations
                    && mIterations <= LATENCY_ITERATIONS_UPPER_BOUND) {
                return TestType.LATENCY_STRESS;
            }
        }

        return TestType.NONE;
    }

    private boolean verifyTestParameters() throws TestFailureException {
        if (getTestType() != TestType.NONE) {
            return true;
        }

        if (mTestType.equals(TESTTYPE_GLITCH_STR)
                && (mIterations < GLITCH_ITERATIONS_LOWER_BOUND
                        || mIterations > GLITCH_ITERATIONS_UPPER_BOUND)) {
            final String error =
                    String.format(
                            ERR_PARAMETER_OUT_OF_BOUNDS,
                            "iterations",
                            GLITCH_ITERATIONS_LOWER_BOUND,
                            GLITCH_ITERATIONS_UPPER_BOUND);
            mTestRunHelper.reportFailure(error);
            return false;
        }

        if (mTestType.equals(TESTTYPE_LATENCY_STR)
                && (mIterations < LATENCY_ITERATIONS_LOWER_BOUND
                        || mIterations > LATENCY_ITERATIONS_UPPER_BOUND)) {
            final String error =
                    String.format(
                            ERR_PARAMETER_OUT_OF_BOUNDS,
                            "iterations",
                            LATENCY_ITERATIONS_LOWER_BOUND,
                            LATENCY_ITERATIONS_UPPER_BOUND);
            mTestRunHelper.reportFailure(error);
            return false;
        }

        return true;
    }

    private void populateResultData(final Map<String, String> results, ResultData data) {
        if (results == null || results.isEmpty()) {
            return;
        }

        String key = getMetricsKey(KEY_RESULT_LATENCY_MS);
        if (results.containsKey(key)) {
            data.setLatency(Float.parseFloat(results.get(key)));
        }

        key = getMetricsKey(KEY_RESULT_LATENCY_CONFIDENCE);
        if (results.containsKey(key)) {
            data.setConfidence(Float.parseFloat(results.get(key)));
        }

        key = getMetricsKey(KEY_RESULT_AUDIO_LEVEL);
        if (results.containsKey(key)) {
            data.setAudioLevel(Integer.parseInt(results.get(key)));
        }

        key = getMetricsKey(KEY_RESULT_RMS);
        if (results.containsKey(key)) {
            data.setRMS(Float.parseFloat(results.get(key)));
        }

        key = getMetricsKey(KEY_RESULT_RMS_AVERAGE);
        if (results.containsKey(key)) {
            data.setRMSAverage(Float.parseFloat(results.get(key)));
        }

        key = getMetricsKey(KEY_RESULT_PERIOD_CONFIDENCE);
        if (results.containsKey(key)) {
            data.setPeriodConfidence(Float.parseFloat(results.get(key)));
        }

        key = getMetricsKey(KEY_RESULT_SAMPLING_BLOCK_SIZE);
        if (results.containsKey(key)) {
            data.setBlockSize(Integer.parseInt(results.get(key)));
        }
    }

    private void storeDeviceFilesOnHost(ResultData data) throws DeviceNotAvailableException {
        final int iteration = data.getIteration();
        for (final LogFileType log : getLogFileTypesForCurrentTest()) {
            if (getDevice().doesFileExist(getDeviceFilename(log, iteration))) {
                final String deviceFileName = getDeviceFilename(log, iteration);
                final File logFile = getDevice().pullFile(deviceFileName);
                data.setLogFile(log, logFile.getAbsolutePath());
                CLog.i("Delete file from device: " + deviceFileName);
                deleteFileFromDevice(deviceFileName);
            }
        }
    }

    private void deleteAllTempFiles() {
        for (final ResultData d : mLoopbackTestHelper.getAllTestData()) {
            final LogFileType[] logFileTypes = getLogFileTypesForCurrentTest();
            for (final LogFileType logType : logFileTypes) {
                final String logFilename = d.getLogFile(logType);
                if (logFilename == null || logFilename.isEmpty()) {
                    CLog.e("Logfile not found for LogFileType=" + logType.name());
                } else {
                    FileUtil.deleteFile(new File(logFilename));
                }
            }
        }
    }

    private void deleteFileFromDevice(String deviceFileName) throws DeviceNotAvailableException {
        getDevice().executeShellCommand("rm -f " + deviceFileName);
    }

    private final LogFileType[] getLogFileTypesForCurrentTest() {
        switch (getTestType()) {
            case GLITCH:
                return GLITCH_TEST_LOGS;
            case LATENCY:
            case LATENCY_STRESS:
                return LATENCY_TEST_LOGS;
            default:
                return null;
        }
    }

    private String getKeyPrefixForIteration(int iteration) {
        if (mIterations == 1) {
            // If only one run, skip the iteration number
            return mKeyPrefix;
        }
        return mKeyPrefix + iteration + "_";
    }

    private String getDeviceFilename(LogFileType key, int iteration) {
        final Map<LogFileType, LogFileData> map = getLogFileDataKeyMap();
        if (map.containsKey(key)) {
            final LogFileData data = map.get(key);
            return String.format(FMT_DEVICE_PATH, iteration, data.fileExtension);
        }
        return null;
    }

    private void uploadLog(ITestInvocationListener listener, LogFileType key, ResultData data) {
        final Map<LogFileType, LogFileData> map = getLogFileDataKeyMap();
        if (!map.containsKey(key)) {
            return;
        }

        final LogFileData logInfo = map.get(key);
        final String prefix = getKeyPrefixForIteration(data.getIteration()) + logInfo.filePrefix;
        final LogDataType logDataType = logInfo.logDataType;
        final String logFilename = data.getLogFile(key);
        if (logFilename == null || logFilename.isEmpty()) {
            CLog.e("Logfile not found for LogFileType=" + key.name());
        } else {
            File logFile = new File(logFilename);
            try (InputStreamSource iss = new FileInputStreamSource(logFile)) {
                listener.testLog(prefix, logDataType, iss);
            }
        }
    }

    private void saveLogcatForIteration(ResultData data, InputStreamSource logcat, int iteration) {
        if (logcat == null) {
            CLog.i("Logcat could not be saved for iteration " + iteration);
            return;
        }

        //create a temp file
        File temp;
        try {
            temp = FileUtil.createTempFile("logcat_" + iteration + "_", ".txt");
            data.setLogFile(LogFileType.LOGCAT, temp.getAbsolutePath());

            // Copy logcat data into temp file
            Files.copy(logcat.createInputStream(), temp.toPath(), REPLACE_EXISTING);
            logcat.close();
        } catch (final IOException e) {
            CLog.i("Error when saving logcat for iteration=" + iteration);
            CLog.e(e);
        }
    }

    private void saveResultsAsCSVFile(ITestInvocationListener listener)
            throws DeviceNotAvailableException, IOException {
        final File csvTmpFile = File.createTempFile("audio_test_data", "csv");
        mLoopbackTestHelper.writeAllResultsToCSVFile(csvTmpFile, getDevice());
        try (InputStreamSource iss = new FileInputStreamSource(csvTmpFile)) {
            listener.testLog("audio_test_data", LogDataType.JACOCO_CSV, iss);
        }
        // cleanup
        csvTmpFile.delete();
    }

    private String getTestCommand(int currentIteration) {
        return String.format(
                AM_CMD,
                mSamplingFreq,
                String.format(FMT_OUTPUT_PREFIX, currentIteration),
                mMicSource,
                mAudioThread,
                mAudioLevel,
                mTestType,
                mBufferTestDuration);
    }
}
