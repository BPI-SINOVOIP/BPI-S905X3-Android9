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

import com.android.media.tests.AudioLoopbackImageAnalyzer.Result;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.Pair;

import com.google.common.io.Files;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Helper class for AudioLoopbackTest. It keeps runtime data, analytics, */
public class AudioLoopbackTestHelper {

    private StatisticsData mLatencyStats = null;
    private StatisticsData mConfidenceStats = null;
    private ArrayList<ResultData> mAllResults;
    private ArrayList<ResultData> mGoodResults = new ArrayList<ResultData>();
    private ArrayList<ResultData> mBadResults = new ArrayList<ResultData>();
    private ArrayList<Map<String, String>> mResultDictionaries =
            new ArrayList<Map<String, String>>();

    // Controls acceptable tolerance in ms around median latency
    private static final double TOLERANCE = 2.0;

    //===================================================================
    // ENUMS
    //===================================================================
    public enum LogFileType {
        RESULT,
        WAVE,
        GRAPH,
        PLAYER_BUFFER,
        PLAYER_BUFFER_HISTOGRAM,
        PLAYER_BUFFER_PERIOD_TIMES,
        RECORDER_BUFFER,
        RECORDER_BUFFER_HISTOGRAM,
        RECORDER_BUFFER_PERIOD_TIMES,
        GLITCHES_MILLIS,
        HEAT_MAP,
        LOGCAT
    }

    //===================================================================
    // INNER CLASSES
    //===================================================================
    private class StatisticsData {
        double mMin = 0;
        double mMax = 0;
        double mMean = 0;
        double mMedian = 0;

        @Override
        public String toString() {
            return String.format(
                    "min = %1$f, max = %2$f, median=%3$f, mean = %4$f", mMin, mMax, mMedian, mMean);
        }
    }

    /** ResultData is an inner class that holds results and logfile info from each test run */
    public static class ResultData {
        private Float mLatencyMs;
        private Float mLatencyConfidence;
        private Integer mAudioLevel;
        private Integer mIteration;
        private Long mDeviceTestStartTime;
        private boolean mIsTimedOut = false;
        private HashMap<LogFileType, String> mLogs = new HashMap<LogFileType, String>();
        private Result mImageAnalyzerResult = Result.UNKNOWN;
        private String mFailureReason = null;

        // Optional
        private Float mPeriodConfidence = null;
        private Float mRms = null;
        private Float mRmsAverage = null;
        private Integer mBblockSize = null;

        public float getLatency() {
            return mLatencyMs.floatValue();
        }

        public void setLatency(float latencyMs) {
            this.mLatencyMs = Float.valueOf(latencyMs);
        }

        public boolean hasLatency() {
            return mLatencyMs != null;
        }

        public float getConfidence() {
            return mLatencyConfidence.floatValue();
        }

        public void setConfidence(float latencyConfidence) {
            this.mLatencyConfidence = Float.valueOf(latencyConfidence);
        }

        public boolean hasConfidence() {
            return mLatencyConfidence != null;
        }

        public float getPeriodConfidence() {
            return mPeriodConfidence.floatValue();
        }

        public void setPeriodConfidence(float periodConfidence) {
            this.mPeriodConfidence = Float.valueOf(periodConfidence);
        }

        public boolean hasPeriodConfidence() {
            return mPeriodConfidence != null;
        }

        public float getRMS() {
            return mRms.floatValue();
        }

        public void setRMS(float rms) {
            this.mRms = Float.valueOf(rms);
        }

        public boolean hasRMS() {
            return mRms != null;
        }

        public float getRMSAverage() {
            return mRmsAverage.floatValue();
        }

        public void setRMSAverage(float rmsAverage) {
            this.mRmsAverage = Float.valueOf(rmsAverage);
        }

        public boolean hasRMSAverage() {
            return mRmsAverage != null;
        }

        public int getAudioLevel() {
            return mAudioLevel.intValue();
        }

        public void setAudioLevel(int audioLevel) {
            this.mAudioLevel = Integer.valueOf(audioLevel);
        }

        public boolean hasAudioLevel() {
            return mAudioLevel != null;
        }

        public int getBlockSize() {
            return mBblockSize.intValue();
        }

        public void setBlockSize(int blockSize) {
            this.mBblockSize = Integer.valueOf(blockSize);
        }

        public boolean hasBlockSize() {
            return mBblockSize != null;
        }

        public int getIteration() {
            return mIteration.intValue();
        }

        public void setIteration(int iteration) {
            this.mIteration = Integer.valueOf(iteration);
        }

        public long getDeviceTestStartTime() {
            return mDeviceTestStartTime.longValue();
        }

        public void setDeviceTestStartTime(long deviceTestStartTime) {
            this.mDeviceTestStartTime = Long.valueOf(deviceTestStartTime);
        }

        public Result getImageAnalyzerResult() {
            return mImageAnalyzerResult;
        }

        public void setImageAnalyzerResult(Result imageAnalyzerResult) {
            this.mImageAnalyzerResult = imageAnalyzerResult;
        }

        public String getFailureReason() {
            return mFailureReason;
        }

        public void setFailureReason(String failureReason) {
            this.mFailureReason = failureReason;
        }

        public boolean isTimedOut() {
            return mIsTimedOut;
        }

        public void setIsTimedOut(boolean isTimedOut) {
            this.mIsTimedOut = isTimedOut;
        }

        public String getLogFile(LogFileType log) {
            return mLogs.get(log);
        }

        public void setLogFile(LogFileType log, String filename) {
            CLog.i("setLogFile: type=" + log.name() + ", filename=" + filename);
            if (!mLogs.containsKey(log) && filename != null && !filename.isEmpty()) {
                mLogs.put(log, filename);
            }
        }

        public boolean hasBadResults() {
            return hasTimedOut()
                    || hasNoTestResults()
                    || !hasLatency()
                    || !hasConfidence()
                    || mImageAnalyzerResult == Result.FAIL;
        }

        public boolean hasTimedOut() {
            return mIsTimedOut;
        }

        public boolean hasLogFile(LogFileType log) {
            return mLogs.containsKey(log);
        }

        public boolean hasNoTestResults() {
            return !hasConfidence() && !hasLatency();
        }

        public static Comparator<ResultData> latencyComparator =
                new Comparator<ResultData>() {
                    @Override
                    public int compare(ResultData o1, ResultData o2) {
                        return o1.mLatencyMs.compareTo(o2.mLatencyMs);
                    }
                };

        public static Comparator<ResultData> confidenceComparator =
                new Comparator<ResultData>() {
                    @Override
                    public int compare(ResultData o1, ResultData o2) {
                        return o1.mLatencyConfidence.compareTo(o2.mLatencyConfidence);
                    }
                };

        public static Comparator<ResultData> iteratorComparator =
                new Comparator<ResultData>() {
                    @Override
                    public int compare(ResultData o1, ResultData o2) {
                        return Integer.compare(o1.mIteration, o2.mIteration);
                    }
                };

        @Override
        public String toString() {
            final String NL = "\n";
            final StringBuilder sb = new StringBuilder(512);
            sb.append("{").append(NL);
            sb.append("{\nlatencyMs=").append(mLatencyMs).append(NL);
            sb.append("latencyConfidence=").append(mLatencyConfidence).append(NL);
            sb.append("isTimedOut=").append(mIsTimedOut).append(NL);
            sb.append("iteration=").append(mIteration).append(NL);
            sb.append("logs=").append(Arrays.toString(mLogs.values().toArray())).append(NL);
            sb.append("audioLevel=").append(mAudioLevel).append(NL);
            sb.append("deviceTestStartTime=").append(mDeviceTestStartTime).append(NL);
            sb.append("rms=").append(mRms).append(NL);
            sb.append("rmsAverage=").append(mRmsAverage).append(NL);
            sb.append("}").append(NL);
            return sb.toString();
        }
    }

    public AudioLoopbackTestHelper(int iterations) {
        mAllResults = new ArrayList<ResultData>(iterations);
    }

    public void addTestData(ResultData data,
            Map<String,
            String> resultDictionary,
            boolean useImageAnalyzer) {
        mResultDictionaries.add(data.getIteration(), resultDictionary);
        mAllResults.add(data);

        if (useImageAnalyzer && data.hasLogFile(LogFileType.GRAPH)) {
            // Analyze captured screenshot to see if wave form is within reason
            final String screenshot = data.getLogFile(LogFileType.GRAPH);
            final Pair<Result, String> result = AudioLoopbackImageAnalyzer.analyzeImage(screenshot);
            data.setImageAnalyzerResult(result.first);
            data.setFailureReason(result.second);
        }
    }

    public final List<ResultData> getAllTestData() {
        return mAllResults;
    }

    public Map<String, String> getResultDictionaryForIteration(int i) {
        return mResultDictionaries.get(i);
    }

    /**
     * Returns a list of the worst test result objects, up to maxNrOfWorstResults
     *
     * <p>
     *
     * <ol>
     *   <li> Tests in the bad results list are added first
     *   <li> If still space, add test results based on low confidence and then tests that are
     *       outside tolerance boundaries
     * </ol>
     *
     * @param maxNrOfWorstResults
     * @return list of worst test result objects
     */
    public List<ResultData> getWorstResults(int maxNrOfWorstResults) {
        int counter = 0;
        final ArrayList<ResultData> worstResults = new ArrayList<ResultData>(maxNrOfWorstResults);

        for (final ResultData data : mBadResults) {
            if (counter < maxNrOfWorstResults) {
                worstResults.add(data);
                counter++;
            }
        }

        for (final ResultData data : mGoodResults) {
            if (counter < maxNrOfWorstResults) {
                boolean failed = false;
                if (data.getConfidence() < 1.0f) {
                    data.setFailureReason("Low confidence");
                    failed = true;
                } else if (data.getLatency() < (mLatencyStats.mMedian - TOLERANCE)
                        || data.getLatency() > (mLatencyStats.mMedian + TOLERANCE)) {
                    data.setFailureReason("Latency not within tolerance from median");
                    failed = true;
                }

                if (failed) {
                    worstResults.add(data);
                    counter++;
                }
            }
        }

        return worstResults;
    }

    public static Map<String, String> parseKeyValuePairFromFile(
            File result,
            final Map<String, String> dictionary,
            final String resultKeyPrefix,
            final String splitOn,
            final String keyValueFormat)
            throws IOException {

        final Map<String, String> resultMap = new HashMap<String, String>();
        final BufferedReader br = Files.newReader(result, StandardCharsets.UTF_8);

        try {
            String line = br.readLine();
            while (line != null) {
                line = line.trim().replaceAll(" +", " ");
                final String[] tokens = line.split(splitOn);
                if (tokens.length >= 2) {
                    final String key = tokens[0].trim();
                    final String value = tokens[1].trim();
                    if (dictionary.containsKey(key)) {
                        CLog.i(String.format(keyValueFormat, key, value));
                        resultMap.put(resultKeyPrefix + dictionary.get(key), value);
                    }
                }
                line = br.readLine();
            }
        } finally {
            br.close();
        }
        return resultMap;
    }

    public int processTestData() {

        // Collect statistics about the test run
        int nrOfValidResults = 0;
        double sumLatency = 0;
        double sumConfidence = 0;

        final int totalNrOfTests = mAllResults.size();
        mLatencyStats = new StatisticsData();
        mConfidenceStats = new StatisticsData();
        mBadResults = new ArrayList<ResultData>();
        mGoodResults = new ArrayList<ResultData>(totalNrOfTests);

        // Copy all results into Good results list
        mGoodResults.addAll(mAllResults);

        for (final ResultData data : mAllResults) {
            if (data.hasBadResults()) {
                mBadResults.add(data);
                continue;
            }
            // Get mean values
            sumLatency += data.getLatency();
            sumConfidence += data.getConfidence();
        }

        if (!mBadResults.isEmpty()) {
            analyzeBadResults(mBadResults, mAllResults.size());
        }

        // Remove bad runs from result array
        mGoodResults.removeAll(mBadResults);

        // Fail test immediately if we don't have ANY good results
        if (mGoodResults.isEmpty()) {
            return 0;
        }

        nrOfValidResults = mGoodResults.size();

        // ---- LATENCY: Get Median, Min and Max values ----
        Collections.sort(mGoodResults, ResultData.latencyComparator);

        mLatencyStats.mMin = mGoodResults.get(0).mLatencyMs;
        mLatencyStats.mMax = mGoodResults.get(nrOfValidResults - 1).mLatencyMs;
        mLatencyStats.mMean = sumLatency / nrOfValidResults;
        // Is array even or odd numbered
        if (nrOfValidResults % 2 == 0) {
            final int middle = nrOfValidResults / 2;
            final float middleLeft = mGoodResults.get(middle - 1).mLatencyMs;
            final float middleRight = mGoodResults.get(middle).mLatencyMs;
            mLatencyStats.mMedian = (middleLeft + middleRight) / 2.0f;
        } else {
            // It's and odd numbered array, just grab the middle value
            mLatencyStats.mMedian = mGoodResults.get(nrOfValidResults / 2).mLatencyMs;
        }

        // ---- CONFIDENCE: Get Median, Min and Max values ----
        Collections.sort(mGoodResults, ResultData.confidenceComparator);

        mConfidenceStats.mMin = mGoodResults.get(0).mLatencyConfidence;
        mConfidenceStats.mMax = mGoodResults.get(nrOfValidResults - 1).mLatencyConfidence;
        mConfidenceStats.mMean = sumConfidence / nrOfValidResults;
        // Is array even or odd numbered
        if (nrOfValidResults % 2 == 0) {
            final int middle = nrOfValidResults / 2;
            final float middleLeft = mGoodResults.get(middle - 1).mLatencyConfidence;
            final float middleRight = mGoodResults.get(middle).mLatencyConfidence;
            mConfidenceStats.mMedian = (middleLeft + middleRight) / 2.0f;
        } else {
            // It's and odd numbered array, just grab the middle value
            mConfidenceStats.mMedian = mGoodResults.get(nrOfValidResults / 2).mLatencyConfidence;
        }

        for (final ResultData data : mGoodResults) {
            // Check if within Latency Tolerance
            if (data.getConfidence() < 1.0f) {
                data.setFailureReason("Low confidence");
            } else if (data.getLatency() < (mLatencyStats.mMedian - TOLERANCE)
                    || data.getLatency() > (mLatencyStats.mMedian + TOLERANCE)) {
                data.setFailureReason("Latency not within tolerance from median");
            }
        }

        // Create histogram
        // Strategy: Create buckets based on whole ints, like 16 ms, 17 ms, 18 ms etc. Count how
        // many tests fall into each bucket. Just cast the float to an int, no rounding up/down
        // required.
        final int[] histogram = new int[(int) mLatencyStats.mMax + 1];
        for (final ResultData rd : mGoodResults) {
            // Increase value in bucket
            histogram[(int) (rd.mLatencyMs.floatValue())]++;
        }

        CLog.i("========== VALID RESULTS ============================================");
        CLog.i(String.format("Valid tests: %1$d of %2$d", nrOfValidResults, totalNrOfTests));
        CLog.i("Latency: " + mLatencyStats.toString());
        CLog.i("Confidence: " + mConfidenceStats.toString());
        CLog.i("========== HISTOGRAM ================================================");
        for (int i = 0; i < histogram.length; i++) {
            if (histogram[i] > 0) {
                CLog.i(String.format("%1$01d ms => %2$d", i, histogram[i]));
            }
        }

        // VERIFY the good results by running image analysis on the
        // screenshot of the incoming audio waveform

        return nrOfValidResults;
    }

    public void writeAllResultsToCSVFile(File csvFile, ITestDevice device)
            throws DeviceNotAvailableException, FileNotFoundException,
                    UnsupportedEncodingException {

        final String deviceType = device.getProperty("ro.build.product");
        final String buildId = device.getBuildAlias();
        final String serialNumber = device.getSerialNumber();

        // Sort data on iteration
        Collections.sort(mAllResults, ResultData.iteratorComparator);

        final StringBuilder sb = new StringBuilder(256);
        final PrintWriter writer = new PrintWriter(csvFile, StandardCharsets.UTF_8.name());
        final String SEPARATOR = ",";

        // Write column labels
        writer.println(
                "Device Time,Device Type,Build Id,Serial Number,Iteration,Latency,"
                        + "Confidence,Period Confidence,Block Size,Audio Level,RMS,RMS Average,"
                        + "Image Analysis,Failure Reason");
        for (final ResultData data : mAllResults) {
            final Instant instant = Instant.ofEpochSecond(data.mDeviceTestStartTime);

            sb.append(instant).append(SEPARATOR);
            sb.append(deviceType).append(SEPARATOR);
            sb.append(buildId).append(SEPARATOR);
            sb.append(serialNumber).append(SEPARATOR);
            sb.append(data.getIteration()).append(SEPARATOR);
            sb.append(data.hasLatency() ? data.getLatency() : "").append(SEPARATOR);
            sb.append(data.hasConfidence() ? data.getConfidence() : "").append(SEPARATOR);
            sb.append(data.hasPeriodConfidence() ? data.getPeriodConfidence() : "").append(SEPARATOR);
            sb.append(data.hasBlockSize() ? data.getBlockSize() : "").append(SEPARATOR);
            sb.append(data.hasAudioLevel() ? data.getAudioLevel() : "").append(SEPARATOR);
            sb.append(data.hasRMS() ? data.getRMS() : "").append(SEPARATOR);
            sb.append(data.hasRMSAverage() ? data.getRMSAverage() : "").append(SEPARATOR);
            sb.append(data.getImageAnalyzerResult().name()).append(SEPARATOR);
            sb.append(data.getFailureReason());

            writer.println(sb.toString());

            sb.setLength(0);
        }
        writer.close();
    }

    private void analyzeBadResults(ArrayList<ResultData> badResults, int totalNrOfTests) {
        int testNoData = 0;
        int testTimeoutCounts = 0;
        int testResultsNotFoundCounts = 0;
        int testWithoutLatencyResultCount = 0;
        int testWithoutConfidenceResultCount = 0;

        for (final ResultData data : badResults) {
            if (data.hasTimedOut()) {
                testTimeoutCounts++;
                testNoData++;
                continue;
            }

            if (data.hasNoTestResults()) {
                testResultsNotFoundCounts++;
                testNoData++;
                continue;
            }

            if (!data.hasLatency()) {
                testWithoutLatencyResultCount++;
                testNoData++;
                continue;
            }

            if (!data.hasConfidence()) {
                testWithoutConfidenceResultCount++;
                testNoData++;
                continue;
            }
        }

        CLog.i("========== BAD RESULTS ============================================");
        CLog.i(String.format("No Data: %1$d of %2$d", testNoData, totalNrOfTests));
        CLog.i(String.format("Timed out: %1$d of %2$d", testTimeoutCounts, totalNrOfTests));
        CLog.i(
                String.format(
                        "No results: %1$d of %2$d", testResultsNotFoundCounts, totalNrOfTests));
        CLog.i(
                String.format(
                        "No Latency results: %1$d of %2$d",
                        testWithoutLatencyResultCount, totalNrOfTests));
        CLog.i(
                String.format(
                        "No Confidence results: %1$d of %2$d",
                        testWithoutConfidenceResultCount, totalNrOfTests));
    }

    /** Generates metrics dictionary for stress test */
    public void populateStressTestMetrics(
            Map<String, String> metrics, final String resultKeyPrefix) {
        metrics.put(resultKeyPrefix + "total_nr_of_tests", Integer.toString(mAllResults.size()));
        metrics.put(resultKeyPrefix + "nr_of_good_tests", Integer.toString(mGoodResults.size()));
        metrics.put(resultKeyPrefix + "latency_max", Double.toString(mLatencyStats.mMax));
        metrics.put(resultKeyPrefix + "latency_min", Double.toString(mLatencyStats.mMin));
        metrics.put(resultKeyPrefix + "latency_mean", Double.toString(mLatencyStats.mMean));
        metrics.put(resultKeyPrefix + "latency_median", Double.toString(mLatencyStats.mMedian));
        metrics.put(resultKeyPrefix + "confidence_max", Double.toString(mConfidenceStats.mMax));
        metrics.put(resultKeyPrefix + "confidence_min", Double.toString(mConfidenceStats.mMin));
        metrics.put(resultKeyPrefix + "confidence_mean", Double.toString(mConfidenceStats.mMean));
        metrics.put(
                resultKeyPrefix + "confidence_median", Double.toString(mConfidenceStats.mMedian));
    }
}
