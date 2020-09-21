/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ByteArrayInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import org.junit.Assert;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A harness that test video playback and reports result.
 */
public class VideoMultimeterTest implements IDeviceTest, IRemoteTest {

    static final String RUN_KEY = "video_multimeter";

    @Option(name = "multimeter-util-path", description = "path for multimeter control util",
            importance = Importance.ALWAYS)
    String mMeterUtilPath = "/tmp/util.sh";

    @Option(name = "start-video-cmd", description = "adb shell command to start video playback; " +
        "use '%s' as placeholder for media source filename", importance = Importance.ALWAYS)
    String mCmdStartVideo = "am instrument -w -r -e media-file"
            + " \"%s\" -e class com.android.mediaframeworktest.stress.MediaPlayerStressTest"
            + " com.android.mediaframeworktest/.MediaPlayerStressTestRunner";

    @Option(name = "stop-video-cmd", description = "adb shell command to stop video playback",
            importance = Importance.ALWAYS)
    String mCmdStopVideo = "am force-stop com.android.mediaframeworktest";

    @Option(name="video-spec", description=
            "Comma deliminated information for test video files with the following format: " +
            "video_filename, reporting_key_prefix, fps, duration(in sec) " +
            "May be repeated for test with multiple files.")
    private Collection<String> mVideoSpecs = new ArrayList<>();

    @Option(name="wait-time-between-runs", description=
        "wait time between two test video measurements, in millisecond")
    private long mWaitTimeBetweenRuns = 3 * 60 * 1000;

    @Option(name="calibration-video", description=
        "filename of calibration video")
    private String mCaliVideoDevicePath = "video_cali.mp4";

    @Option(
        name = "debug-without-hardware",
        description = "Use option to debug test without having specialized hardware",
        importance = Importance.NEVER,
        mandatory = false
    )
    protected boolean mDebugWithoutHardware = false;

    static final String ROTATE_LANDSCAPE = "content insert --uri content://settings/system"
            + " --bind name:s:user_rotation --bind value:i:1";

    // Max number of trailing frames to trim
    static final int TRAILING_FRAMES_MAX = 3;
    // Min threshold for duration of trailing frames
    static final long FRAME_DURATION_THRESHOLD_US = 500 * 1000; // 0.5s

    static final String CMD_GET_FRAMERATE_STATE = "GETF";
    static final String CMD_START_CALIBRATION = "STAC";
    static final String CMD_SET_CALIBRATION_VALS = "SETCAL";
    static final String CMD_STOP_CALIBRATION = "STOC";
    static final String CMD_START_MEASUREMENT = "STAM";
    static final String CMD_STOP_MEASUREMENT = "STOM";
    static final String CMD_GET_NUM_FRAMES = "GETN";
    static final String CMD_GET_ALL_DATA = "GETD";

    static final long DEVICE_SYNC_TIME_MS = 30 * 1000;
    static final long CALIBRATION_TIMEOUT_MS = 30 * 1000;
    static final long COMMAND_TIMEOUT_MS = 5 * 1000;
    static final long GETDATA_TIMEOUT_MS = 10 * 60 * 1000;

    // Regex for: "OK (time); (frame duration); (marker color); (total dropped frames)"
    static final String VIDEO_FRAME_DATA_PATTERN = "OK\\s+\\d+;\\s*(-?\\d+);\\s*[a-z]+;\\s*(\\d+)";

    // Regex for: "OK (time); (frame duration); (marker color); (total dropped frames); (lipsync)"
    // results in: $1 == ts, $2 == lipsync
    static final String LIPSYNC_DATA_PATTERN =
            "OK\\s+(\\d+);\\s*\\d+;\\s*[a-z]+;\\s*\\d+;\\s*(-?\\d+)";
    //          ts        dur     color      missed       latency
    static final int LIPSYNC_SIGNAL = 2000000; // every 2 seconds
    static final int LIPSYNC_SIGNAL_MIN = 1500000; // must be at least 1.5 seconds after prev

    ITestDevice mDevice;

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

    private void rotateScreen() throws DeviceNotAvailableException {
        // rotate to landscape mode, except for manta
        if (!getDevice().getProductType().contains("manta")) {
            getDevice().executeShellCommand(ROTATE_LANDSCAPE);
        }
    }

    protected boolean setupTestEnv() throws DeviceNotAvailableException {
        return setupTestEnv(null);
    }

    protected void startPlayback(final String videoPath) {
        new Thread() {
            @Override
            public void run() {
                try {
                    CollectingOutputReceiver receiver = new CollectingOutputReceiver();
                    getDevice().executeShellCommand(String.format(
                            mCmdStartVideo, videoPath),
                            receiver, 1L, TimeUnit.SECONDS, 0);
                } catch (DeviceNotAvailableException e) {
                    CLog.e(e.getMessage());
                }
            }
        }.start();
    }

    /**
     * Perform calibration process for video multimeter
     *
     * @return boolean whether calibration succeeds
     * @throws DeviceNotAvailableException
     */
    protected boolean doCalibration() throws DeviceNotAvailableException {
        // play calibration video
        startPlayback(mCaliVideoDevicePath);
        getRunUtil().sleep(3 * 1000);
        rotateScreen();
        getRunUtil().sleep(1 * 1000);
        CommandResult cr = getRunUtil().runTimedCmd(
                COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_START_CALIBRATION);
        CLog.i("Starting calibration: " + cr.getStdout());
        // check whether multimeter is calibrated
        boolean isCalibrated = false;
        long calibrationStartTime = System.currentTimeMillis();
        while (!isCalibrated &&
                System.currentTimeMillis() - calibrationStartTime <= CALIBRATION_TIMEOUT_MS) {
            getRunUtil().sleep(1 * 1000);
            cr = getRunUtil().runTimedCmd(2 * 1000, mMeterUtilPath, CMD_GET_FRAMERATE_STATE);
            if (cr.getStdout().contains("calib0")) {
                isCalibrated = true;
            }
        }
        if (!isCalibrated) {
            // stop calibration if time out
            cr = getRunUtil().runTimedCmd(
                        COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_STOP_CALIBRATION);
            CLog.e("Calibration timed out.");
        } else {
            CLog.i("Calibration succeeds.");
        }
        getDevice().executeShellCommand(mCmdStopVideo);
        return isCalibrated;
    }

    protected boolean setupTestEnv(String caliValues) throws DeviceNotAvailableException {
        getRunUtil().sleep(DEVICE_SYNC_TIME_MS);
        CommandResult cr = getRunUtil().runTimedCmd(
                COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_STOP_MEASUREMENT);

        getDevice().setDate(new Date());
        CLog.i("syncing device time to host time");
        getRunUtil().sleep(3 * 1000);

        // TODO: need a better way to clear old data
        // start and stop to clear old data
        cr = getRunUtil().runTimedCmd(COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_START_MEASUREMENT);
        getRunUtil().sleep(3 * 1000);
        cr = getRunUtil().runTimedCmd(COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_STOP_MEASUREMENT);
        getRunUtil().sleep(3 * 1000);
        CLog.i("Stopping measurement: " + cr.getStdout());

        if (caliValues == null) {
            return doCalibration();
        } else {
            CLog.i("Setting calibration values: " + caliValues);
            final String calibrationValues = CMD_SET_CALIBRATION_VALS + " " + caliValues;
            cr = getRunUtil().runTimedCmd(COMMAND_TIMEOUT_MS, mMeterUtilPath, calibrationValues);
            final String response = mDebugWithoutHardware ? "OK" : cr.getStdout();
            if (response != null && response.startsWith("OK")) {
                CLog.i("Calibration values are set to: " + caliValues);
                return true;
            } else {
                CLog.e("Failed to set calibration values: " + cr.getStdout());
                return false;
            }
        }
    }

    private void doMeasurement(final String testVideoPath, long durationSecond)
            throws DeviceNotAvailableException {
        CommandResult cr;
        getDevice().clearErrorDialogs();
        getRunUtil().sleep(mWaitTimeBetweenRuns);

        // play test video
        startPlayback(testVideoPath);

        getRunUtil().sleep(3 * 1000);

        rotateScreen();
        getRunUtil().sleep(1 * 1000);
        cr = getRunUtil().runTimedCmd(COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_START_MEASUREMENT);
        CLog.i("Starting measurement: " + cr.getStdout());

        // end measurement
        getRunUtil().sleep(durationSecond * 1000);

        cr = getRunUtil().runTimedCmd(COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_STOP_MEASUREMENT);
        CLog.i("Stopping measurement: " + cr.getStdout());
        if (cr == null || !cr.getStdout().contains("OK")) {
            cr = getRunUtil().runTimedCmd(
                    COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_STOP_MEASUREMENT);
            CLog.i("Retry - Stopping measurement: " + cr.getStdout());
        }

        getDevice().executeShellCommand(mCmdStopVideo);
        getDevice().clearErrorDialogs();
    }

    private Map<String, String> getResult(ITestInvocationListener listener,
            Map<String, String> metrics, String keyprefix, float fps, boolean lipsync) {
        CommandResult cr;

        // get number of results
        getRunUtil().sleep(5 * 1000);
        cr = getRunUtil().runTimedCmd(COMMAND_TIMEOUT_MS, mMeterUtilPath, CMD_GET_NUM_FRAMES);
        String frameNum = cr.getStdout();

        CLog.i("== Video Multimeter Result '%s' ==", keyprefix);
        CLog.i("Number of results: " + frameNum);

        String nrOfDataPointsStr = extractNumberOfCollectedDataPoints(frameNum);
        metrics.put(keyprefix + "frame_captured", nrOfDataPointsStr);

        long nrOfDataPoints = Long.parseLong(nrOfDataPointsStr);

        Assert.assertTrue("Multimeter did not collect any data for " + keyprefix,
                          nrOfDataPoints > 0);

        CLog.i("Captured frames: " + nrOfDataPointsStr);

        // get all results from multimeter and write to output file
        cr = getRunUtil().runTimedCmd(GETDATA_TIMEOUT_MS, mMeterUtilPath, CMD_GET_ALL_DATA);
        String allData = cr.getStdout();
        listener.testLog(
                keyprefix, LogDataType.TEXT, new ByteArrayInputStreamSource(allData.getBytes()));

        // parse results
        return parseResult(metrics, nrOfDataPoints, allData, keyprefix, fps, lipsync);
    }

    private String extractNumberOfCollectedDataPoints(String numFrames) {
        // Create pattern that matches string like "OK 14132" capturing the
        // number of data points.
        Pattern p = Pattern.compile("OK\\s+(\\d+)$");
        Matcher m = p.matcher(numFrames.trim());

        String frameCapturedStr = "0";
        if (m.matches()) {
            frameCapturedStr = m.group(1);
        }

        return frameCapturedStr;
    }

    protected void runMultimeterTest(ITestInvocationListener listener,
            Map<String,String> metrics) throws DeviceNotAvailableException {
        for (String videoSpec : mVideoSpecs) {
            String[] videoInfo = videoSpec.split(",");
            String filename = videoInfo[0].trim();
            String keyPrefix = videoInfo[1].trim();
            float fps = Float.parseFloat(videoInfo[2].trim());
            long duration = Long.parseLong(videoInfo[3].trim());
            doMeasurement(filename, duration);
            metrics = getResult(listener, metrics, keyPrefix, fps, true);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        TestDescription testId = new TestDescription(getClass()
                .getCanonicalName(), RUN_KEY);

        listener.testRunStarted(RUN_KEY, 0);
        listener.testStarted(testId);

        long testStartTime = System.currentTimeMillis();
        Map<String, String> metrics = new HashMap<>();

        if (setupTestEnv()) {
            runMultimeterTest(listener, metrics);
        }

        long durationMs = System.currentTimeMillis() - testStartTime;
        listener.testEnded(testId, TfMetricProtoUtil.upgradeConvert(metrics));
        listener.testRunEnded(durationMs, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    /**
     * Parse Multimeter result.
     *
     * @param result
     * @return a {@link HashMap} that contains metrics keys and results
     */
    private Map<String, String> parseResult(Map<String, String> metrics,
            long frameCaptured, String result, String keyprefix, float fps,
            boolean lipsync) {
        final int MISSING_FRAME_CEILING = 5; //5+ frames missing count the same
        final double[] MISSING_FRAME_WEIGHT = {0.0, 1.0, 2.5, 5.0, 6.25, 8.0};

        // Get total captured frames and calculate smoothness and freezing score
        // format: "OK (time); (frame duration); (marker color); (total dropped frames)"
        Pattern p = Pattern.compile(VIDEO_FRAME_DATA_PATTERN);
        Matcher m = null;
        String[] lines = result.split(System.getProperty("line.separator"));
        String totalDropFrame = "-1";
        String lastDropFrame = "0";
        long frameCount = 0;
        long consecutiveDropFrame = 0;
        double freezingPenalty = 0.0;
        long frameDuration = 0;
        double offByOne = 0;
        double offByMultiple = 0;
        double expectedFrameDurationInUs = 1000000.0 / fps;
        for (int i = 0; i < lines.length; i++) {
            m = p.matcher(lines[i].trim());
            if (m.matches()) {
                frameDuration = Long.parseLong(m.group(1));
                // frameDuration = -1 indicates dropped frame
                if (frameDuration > 0) {
                    frameCount++;
                }
                totalDropFrame = m.group(2);
                // trim the last few data points if needed
                if (frameCount >= frameCaptured - TRAILING_FRAMES_MAX - 1 &&
                        frameDuration > FRAME_DURATION_THRESHOLD_US) {
                    metrics.put(keyprefix + "frame_captured", String.valueOf(frameCount));
                    break;
                }
                if (lastDropFrame.equals(totalDropFrame)) {
                    if (consecutiveDropFrame > 0) {
                      freezingPenalty += MISSING_FRAME_WEIGHT[(int) (Math.min(consecutiveDropFrame,
                              MISSING_FRAME_CEILING))] * consecutiveDropFrame;
                      consecutiveDropFrame = 0;
                    }
                } else {
                    consecutiveDropFrame++;
                }
                lastDropFrame = totalDropFrame;

                if (frameDuration < expectedFrameDurationInUs * 0.5) {
                    offByOne++;
                } else if (frameDuration > expectedFrameDurationInUs * 1.5) {
                    if (frameDuration < expectedFrameDurationInUs * 2.5) {
                        offByOne++;
                    } else {
                        offByMultiple++;
                    }
                }
            }
        }
        if (totalDropFrame.equals("-1")) {
            // no matching result found
            CLog.w("No result found for " + keyprefix);
            return metrics;
        } else {
            metrics.put(keyprefix + "frame_drop", totalDropFrame);
            CLog.i("Dropped frames: " + totalDropFrame);
        }
        double smoothnessScore = 100.0 - (offByOne / frameCaptured) * 100.0 -
                (offByMultiple / frameCaptured) * 300.0;
        metrics.put(keyprefix + "smoothness", String.valueOf(smoothnessScore));
        CLog.i("Off by one frame: " + offByOne);
        CLog.i("Off by multiple frames: " + offByMultiple);
        CLog.i("Smoothness score: " + smoothnessScore);

        double freezingScore = 100.0 - 100.0 * freezingPenalty / frameCaptured;
        metrics.put(keyprefix + "freezing", String.valueOf(freezingScore));
        CLog.i("Freezing score: " + freezingScore);

        // parse lipsync results (the audio and video synchronization offset)
        // format: "OK (time); (frame duration); (marker color); (total dropped frames); (lipsync)"
        if (lipsync) {
            ArrayList<Integer> lipsyncVals = new ArrayList<>();
            StringBuilder lipsyncValsStr = new StringBuilder("[");
            long lipsyncSum = 0;
            int lipSyncLastTime = -1;

            Pattern pLip = Pattern.compile(LIPSYNC_DATA_PATTERN);
            for (int i = 0; i < lines.length; i++) {
                m = pLip.matcher(lines[i].trim());
                if (m.matches()) {
                    int lipSyncTime = Integer.parseInt(m.group(1));
                    int lipSyncVal = Integer.parseInt(m.group(2));
                    if (lipSyncLastTime != -1) {
                        if ((lipSyncTime - lipSyncLastTime) < LIPSYNC_SIGNAL_MIN) {
                            continue; // ignore the early/spurious one
                        }
                    }
                    lipSyncLastTime = lipSyncTime;

                    lipsyncVals.add(lipSyncVal);
                    lipsyncValsStr.append(lipSyncVal);
                    lipsyncValsStr.append(", ");
                    lipsyncSum += lipSyncVal;
                }
            }
            if (lipsyncVals.size() > 0) {
                lipsyncValsStr.append("]");
                CLog.i("Lipsync values: " + lipsyncValsStr);
                Collections.sort(lipsyncVals);
                int lipsyncCount = lipsyncVals.size();
                int minLipsync = lipsyncVals.get(0);
                int maxLipsync = lipsyncVals.get(lipsyncCount - 1);
                metrics.put(keyprefix + "lipsync_count", String.valueOf(lipsyncCount));
                CLog.i("Lipsync Count: " + lipsyncCount);
                metrics.put(keyprefix + "lipsync_min", String.valueOf(lipsyncVals.get(0)));
                CLog.i("Lipsync Min: " + minLipsync);
                metrics.put(keyprefix + "lipsync_max", String.valueOf(maxLipsync));
                CLog.i("Lipsync Max: " + maxLipsync);
                double meanLipSync = (double) lipsyncSum / lipsyncCount;
                metrics.put(keyprefix + "lipsync_mean", String.valueOf(meanLipSync));
                CLog.i("Lipsync Mean: " + meanLipSync);
            } else {
                CLog.w("Lipsync value not found in result.");
            }
        }
        CLog.i("== End ==", keyprefix);
        return metrics;
    }

    protected IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
