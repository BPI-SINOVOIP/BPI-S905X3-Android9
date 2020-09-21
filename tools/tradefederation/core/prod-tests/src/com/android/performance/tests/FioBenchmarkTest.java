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

package com.android.performance.tests;

import com.android.ddmlib.IDevice;
import com.android.ddmlib.MultiLineReceiver;
import com.android.ddmlib.NullOutputReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.proto.TfMetricProtoUtil;

import junit.framework.TestCase;

import org.junit.Assert;

import java.io.File;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Runs the FIO benchmarks.
 * <p>
 * This test pushes the FIO executable to the device and runs several benchmarks.  Running each
 * benchmark consists of creating a config file, creating one or more data files, clearing the disk
 * cache and then running FIO. The test runs a variety of different configurations including a
 * simple benchmark with a single thread, a storage benchmark with 4 threads, a media server
 * emulator, and a media scanner emulator.
 * </p>
 */
public class FioBenchmarkTest implements IDeviceTest, IRemoteTest {
    // TODO: Refactor this to only pick out fields we care about.
    private static final String[] FIO_V0_RESULT_FIELDS = {
        "jobname", "groupid", "error",
        // Read stats
        "read-kb-io", "read-bandwidth", "read-runtime",
        "read-slat-min", "read-slat-max", "read-slat-mean", "read-slat-stddev",
        "read-clat-min", "read-clat-max", "read-clat-mean", "read-clat-stddev",
        "read-bandwidth-min", "read-bandwidth-max", "read-bandwidth-percent", "read-bandwidth-mean",
        "read-bandwidth-stddev",
        // Write stats
        "write-kb-io", "write-bandwidth", "write-runtime",
        "write-slat-min", "write-slat-max", "write-slat-mean", "write-slat-stddev",
        "write-clat-min", "write-clat-max", "write-clat-mean", "write-clat-stddev",
        "write-bandwidth-min", "write-bandwidth-max", "write-bandwidth-percent",
        "write-bandwidth-mean", "write-bandwidth-stddev",
        // CPU stats
        "cpu-user", "cpu-system", "cpu-context-switches", "cpu-major-page-faults",
        "cpu-minor-page-faults",
        // IO depth stats
        "io-depth-1", "io-depth-2", "io-depth-4", "io-depth-8", "io-depth-16", "io-depth-32",
        "io-depth-64",
        // IO lat stats
        "io-lat-2-ms", "io-lat-4-ms", "io-lat-10-ms", "io-lat-20-ms", "io-lat-50-ms",
        "io-lat-100-ms", "io-lat-250-ms", "io-lat-500-ms", "io-lat-750-ms", "io-lat-1000-ms",
        "io-lat-2000-ms"
    };
    private static final String[] FIO_V3_RESULT_FIELDS = {
        "terse-version", "fio-version", "jobname", "groupid", "error",
        // Read stats
        "read-kb-io", "read-bandwidth", "read-iops", "read-runtime",
        "read-slat-min", "read-slat-max", "read-slat-mean", "read-slat-stddev",
        "read-clat-min", "read-clat-max", "read-clat-mean", "read-clat-stddev",
        null, null, null, null, null, null, null, null, null, null, null, null, null, null, null,
        null, null, null, null, null,
        "read-lat-min", "read-lat-max", "read-lat-mean", "read-lat-stddev",
        "read-bandwidth-min", "read-bandwidth-max", "read-bandwidth-percent", "read-bandwidth-mean",
        "read-bandwidth-stddev",
        // Write stats
        "write-kb-io", "write-bandwidth", "write-iops", "write-runtime",
        "write-slat-min", "write-slat-max", "write-slat-mean", "write-slat-stddev",
        "write-clat-min", "write-clat-max", "write-clat-mean", "write-clat-stddev",
        null, null, null, null, null, null, null, null, null, null, null, null, null, null, null,
        null, null, null, null, null,
        "write-lat-min", "write-lat-max", "write-lat-mean", "write-lat-stddev",
        "write-bandwidth-min", "write-bandwidth-max", "write-bandwidth-percent",
        "write-bandwidth-mean", "write-bandwidth-stddev",
        // CPU stats
        "cpu-user", "cpu-system", "cpu-context-switches", "cpu-major-page-faults",
        "cpu-minor-page-faults",
        // IO depth stats
        "io-depth-1", "io-depth-2", "io-depth-4", "io-depth-8", "io-depth-16", "io-depth-32",
        "io-depth-64",
        // IO lat stats
        "io-lat-2-us", "io-lat-4-us", "io-lat-10-us", "io-lat-20-us", "io-lat-50-us",
        "io-lat-100-us", "io-lat-250-us", "io-lat-500-us", "io-lat-750-us", "io-lat-1000-us",
        "io-lat-2-ms", "io-lat-4-ms", "io-lat-10-ms", "io-lat-20-ms", "io-lat-50-ms",
        "io-lat-100-ms", "io-lat-250-ms", "io-lat-500-ms", "io-lat-750-ms", "io-lat-1000-ms",
        "io-lat-2000-ms", "io-lat-greater"
    };

    private List<TestInfo> mTestCases = null;

    /**
     * Holds info about a job. The job translates into a job in the FIO config file. Contains the
     * job name and a map from keys to values.
     */
    private static class JobInfo {
        public String mJobName = null;
        public Map<String, String> mParameters = new HashMap<>();

        /**
         * Gets the job as a string.
         *
         * @return a string of the job formatted for the config file.
         */
        public String createJob() {
            if (mJobName == null) {
                return "";
            }
            StringBuilder sb = new StringBuilder();
            sb.append(String.format("[%s]\n", mJobName));
            for (Entry<String, String> parameter : mParameters.entrySet()) {
                if (parameter.getValue() == null) {
                    sb.append(String.format("%s\n", parameter.getKey()));
                } else {
                    sb.append(String.format("%s=%s\n", parameter.getKey(), parameter.getValue()));
                }
            }
            return sb.toString();
        }
    }

    /**
     * Holds info about a file used in the benchmark. Because of limitations in FIO on Android,
     * the file needs to be created before the tests are run. Contains the file name and size in kB.
     */
    private static class TestFileInfo {
        public String mFileName = null;
        public int mSize = -1;
    }

    /**
     * Holds info about the perf metric that are cared about for a given job.
     */
    private static class PerfMetricInfo {
        public enum ResultType {
            STRING, INT, FLOAT, PERCENT;

            String value(String input) {
                switch(this) {
                    case STRING:
                    case INT:
                    case FLOAT:
                        return input;
                    case PERCENT:
                        if (input.length() < 2 || !input.endsWith("%")) {
                            return null;
                        }
                        try {
                            return String.format("%f", Double.parseDouble(
                                    input.substring(0, input.length() - 1)) / 100);
                        } catch (NumberFormatException e) {
                            return null;
                        }
                    default: return null;
                }
            }
        }

        public String mJobName = null;
        public String mFieldName = null;
        public String mPostKey = null;
        public ResultType mType = ResultType.STRING;
    }

    /**
     * Holds the info associated with a test.
     * <p>
     * Contains the test name, key, a list of {@link JobInfo}, a set of {@link TestFileInfo},
     * and a set of {@link PerfMetricInfo}.
     * </p>
     */
    private static class TestInfo {
        public String mTestName = null;
        public String mKey = null;
        public List<JobInfo> mJobs = new LinkedList<>();
        public Set<TestFileInfo> mTestFiles = new HashSet<>();
        public Set<PerfMetricInfo> mPerfMetrics = new HashSet<>();

        /**
         * Gets the config file.
         *
         * @return a string containing the contents of the config file needed to run the benchmark.
         */
        private String createConfig() {
            StringBuilder sb = new StringBuilder();
            for (JobInfo job : mJobs) {
                sb.append(String.format("%s\n", job.createJob()));
            }
            return sb.toString();
        }
    }

    /**
     * Parses the output of the FIO and allows the values to be looked up by job name and property.
     */
    private static class FioParser extends MultiLineReceiver {
        public Map<String, Map<String, String>> mResults = new HashMap<>();

        /**
         * Gets the result for a job and property, or null if the job or the property do not exist.
         *
         * @param job the name of the job.
         * @param property the name of the property. See {@code FIO_RESULT_FIELDS}.
         * @return the fio results for the job and property or null if it does not exist.
         */
        public String getResult(String job, String property) {
            if (!mResults.containsKey(job)) {
                return null;
            }
            return mResults.get(job).get(property);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void processNewLines(String[] lines) {
            for (String line : lines) {
                CLog.d(line);
                String[] fields = line.split(";");
                if (fields.length < FIO_V0_RESULT_FIELDS.length) {
                    continue;
                }
                if (fields.length < FIO_V3_RESULT_FIELDS.length) {
                    Map<String, String> r = new HashMap<>();
                    for (int i = 0; i < FIO_V0_RESULT_FIELDS.length; i++) {
                        r.put(FIO_V0_RESULT_FIELDS[i], fields[i]);
                    }
                    mResults.put(fields[0], r); // Job name is index 0
                } else if ("3".equals(fields[0])) {
                    Map<String, String> r = new HashMap<>();
                    for (int i = 0; i < FIO_V3_RESULT_FIELDS.length; i++) {
                        r.put(FIO_V3_RESULT_FIELDS[i], fields[i]);
                    }
                    mResults.put(fields[2], r); // Job name is index 2
                } else {
                    Assert.fail("Unknown fio terse output version");
                }
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean isCancelled() {
            return false;
        }
    }

    ITestDevice mTestDevice = null;

    private String mFioDir = null;
    private String mFioBin = null;
    private String mFioConfig = null;

    @Option(name="fio-location", description="The path to the precompiled FIO executable. If "
            + "unset, try to use fio from the system image.")
    private String mFioLocation = null;

    @Option(name="tmp-dir", description="The directory used for interal benchmarks.")
    private String mTmpDir = "/data/tmp/fio";

    @Option(name="internal-test-dir", description="The directory used for interal benchmarks.")
    private String mInternalTestDir = "/data/fio/data";

    @Option(name="media-test-dir", description="The directory used for media benchmarks.")
    private String mMediaTestDir = "${EXTERNAL_STORAGE}/fio";

    @Option(name="external-test-dir", description="The directory used for external benchmarks.")
    private String mExternalTestDir = "${EXTERNAL_STORAGE}/fio";

    @Option(name="collect-yaffs-logs", description="Collect yaffs logs before and after tests")
    private Boolean mCollectYaffsLogs = false;

    @Option(name="run-simple-internal-test", description="Run the simple internal benchmark.")
    private Boolean mRunSimpleInternalTest = true;

    @Option(name="simple-internal-file-size",
            description="The file size of the simple internal benchmark in MB.")
    private int mSimpleInternalFileSize = 256;

    @Option(name="run-simple-external-test", description="Run the simple external benchmark.")
    private Boolean mRunSimpleExternalTest = false;

    @Option(name="simple-external-file-size",
            description="The file size of the simple external benchmark in MB.")
    private int mSimpleExternalFileSize = 256;

    @Option(name="run-storage-internal-test", description="Run the storage internal benchmark.")
    private Boolean mRunStorageInternalTest = true;

    @Option(name="storage-internal-file-size",
            description="The file size of the storage internal benchmark in MB.")
    private int mStorageInternalFileSize = 256;

    @Option(name="storage-internal-job-count",
            description="The number of jobs for the storage internal benchmark.")
    private int mStorageInternalJobCount = 4;

    @Option(name="run-storage-external-test", description="Run the storage external benchmark.")
    private Boolean mRunStorageExternalTest = false;

    @Option(name="storage-external-file-size",
            description="The file size of the storage external benchmark in MB.")
    private int mStorageExternalFileSize = 256;

    @Option(name="storage-external-job-count",
            description="The number of jobs for the storage external benchmark.")
    private int mStorageExternalJobCount = 4;

    @Option(name="run-media-server-test", description="Run the media server benchmark.")
    private Boolean mRunMediaServerTest = false;

    @Option(name="media-server-duration",
            description="The duration of the media server benchmark in secs.")
    private long mMediaServerDuration = 30;

    @Option(name="media-server-media-file-size",
            description="The media file size of the media server benchmark in MB.")
    private int mMediaServerMediaFileSize = 256;

    @Option(name="media-server-worker-file-size",
            description="The worker file size of the media server benchmark in MB.")
    private int mMediaServerWorkerFileSize = 256;

    @Option(name="media-server-worker-job-count",
            description="The number of worker jobs for the media server benchmark.")
    private int mMediaServerWorkerJobCount = 4;

    @Option(name="run-media-scanner-test", description="Run the media scanner benchmark.")
    private Boolean mRunMediaScannerTest = false;

    @Option(name="media-scanner-media-file-size",
            description="The media file size of the media scanner benchmark in kB.")
    private int mMediaScannerMediaFileSize = 8;

    @Option(name="media-scanner-media-file-count",
            description="The number of media files to scan.")
    private int mMediaScannerMediaFileCount = 256;

    @Option(name="media-scanner-worker-file-size",
            description="The worker file size of the media scanner benchmark in MB.")
    private int mMediaScannerWorkerFileSize = 256;

    @Option(name="media-scanner-worker-job-count",
            description="The number of worker jobs for the media server benchmark.")
    private int mMediaScannerWorkerJobCount = 4;

    @Option(name="key-suffix",
            description="The suffix to add to the reporting key in order to override the default")
    private String mKeySuffix = null;

    /**
     * Sets up all the benchmarks.
     */
    private void setupTests() {
        if (mTestCases != null) {
            // assume already set up
            return;
        }

        mTestCases = new LinkedList<>();

        if (mRunSimpleInternalTest) {
            addSimpleTest("read", "sync", true);
            addSimpleTest("write", "sync", true);
            addSimpleTest("randread", "sync", true);
            addSimpleTest("randwrite", "sync", true);
            addSimpleTest("randread", "mmap", true);
            addSimpleTest("randwrite", "mmap", true);
        }

        if (mRunSimpleExternalTest) {
            addSimpleTest("read", "sync", false);
            addSimpleTest("write", "sync", false);
            addSimpleTest("randread", "sync", false);
            addSimpleTest("randwrite", "sync", false);
            addSimpleTest("randread", "mmap", false);
            addSimpleTest("randwrite", "mmap", false);
        }

        if (mRunStorageInternalTest) {
            addStorageTest("read", "sync", true);
            addStorageTest("write", "sync", true);
            addStorageTest("randread", "sync", true);
            addStorageTest("randwrite", "sync", true);
            addStorageTest("randread", "mmap", true);
            addStorageTest("randwrite", "mmap", true);
        }

        if (mRunStorageExternalTest) {
            addStorageTest("read", "sync", false);
            addStorageTest("write", "sync", false);
            addStorageTest("randread", "sync", false);
            addStorageTest("randwrite", "sync", false);
            addStorageTest("randread", "mmap", false);
            addStorageTest("randwrite", "mmap", false);
        }

        if (mRunMediaServerTest) {
            addMediaServerTest("read");
            addMediaServerTest("write");
        }

        if (mRunMediaScannerTest) {
            addMediaScannerTest();
        }
    }

    /**
     * Sets up the simple FIO benchmark.
     * <p>
     * The test consists of a single process reading or writing to a file.
     * </p>
     *
     * @param rw the type of IO pattern. One of {@code read}, {@code write}, {@code randread}, or
     * {@code randwrite}.
     * @param ioengine defines how the job issues I/O. Such as {@code sync}, {@code vsync},
     * {@code mmap}, or {@code cpuio} and others.
     * @param internal whether the test should be run on the internal (/data) partition or the
     * external partition.
     */
    private void addSimpleTest(String rw, String ioengine, boolean internal) {
        String fileName = "testfile";
        String jobName = "job";

        String directory;
        int fileSize;

        TestInfo t = new TestInfo();
        if (internal) {
            t.mTestName = String.format("SimpleBenchmark-int-%s-%s", ioengine, rw);
            t.mKey = "fio_simple_int_benchmark";
            directory = mInternalTestDir;
            fileSize = mSimpleInternalFileSize;
        } else {
            t.mTestName = String.format("SimpleBenchmark-ext-%s-%s", ioengine, rw);
            t.mKey = "fio_simple_ext_benchmark";
            directory = mExternalTestDir;
            fileSize = mSimpleExternalFileSize;
        }

        TestFileInfo f = new TestFileInfo();
        f.mFileName = new File(directory, fileName).getAbsolutePath();
        f.mSize = fileSize * 1024;  // fileSize is in MB but we want it in kB.
        t.mTestFiles.add(f);

        JobInfo j = new JobInfo();
        j.mJobName = jobName;
        j.mParameters.put("directory", directory);
        j.mParameters.put("filename", fileName);
        j.mParameters.put("fsync", "1024");
        j.mParameters.put("ioengine", ioengine);
        j.mParameters.put("rw", rw);
        j.mParameters.put("size", String.format("%dM",fileSize));
        t.mJobs.add(j);

        PerfMetricInfo m = new PerfMetricInfo();
        m.mJobName = jobName;
        if ("sync".equals(ioengine)) {
            m.mPostKey = String.format("%s_bandwidth", rw);
        } else {
            m.mPostKey = String.format("%s_%s_bandwidth", rw, ioengine);
        }
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        if (rw.endsWith("read")) {
            m.mFieldName = "read-bandwidth-mean";
        } else if (rw.endsWith("write")) {
            m.mFieldName = "write-bandwidth-mean";
        }
        t.mPerfMetrics.add(m);

        mTestCases.add(t);
    }

    /**
     * Sets up the storage FIO benchmark.
     * <p>
     * The test consists of several processes reading or writing to a file.
     * </p>
     *
     * @param rw the type of IO pattern. One of {@code read}, {@code write}, {@code randread}, or
     * {@code randwrite}.
     * @param ioengine defines how the job issues I/O. Such as {@code sync}, {@code vsync},
     * {@code mmap}, or {@code cpuio} and others.
     * @param internal whether the test should be run on the internal (/data) partition or the
     * external partition.
     */
    private void addStorageTest(String rw, String ioengine, boolean internal) {
        String fileName = "testfile";
        String jobName = "workers";

        String directory;
        int fileSize;
        int jobCount;

        TestInfo t = new TestInfo();
        if (internal) {
            t.mTestName = String.format("StorageBenchmark-int-%s-%s", ioengine, rw);
            t.mKey = "fio_storage_int_benchmark";
            directory = mInternalTestDir;
            fileSize = mStorageInternalFileSize;
            jobCount = mStorageInternalJobCount;
        } else {
            t.mTestName = String.format("StorageBenchmark-ext-%s-%s", ioengine, rw);
            t.mKey = "fio_storage_ext_benchmark";
            directory = mExternalTestDir;
            fileSize = mStorageExternalFileSize;
            jobCount = mStorageExternalJobCount;
        }

        TestFileInfo f = new TestFileInfo();
        f.mFileName = new File(directory, fileName).getAbsolutePath();
        f.mSize = fileSize * 1024;  // fileSize is in MB but we want it in kB.
        t.mTestFiles.add(f);

        JobInfo j = new JobInfo();
        j.mJobName = jobName;
        j.mParameters.put("directory", directory);
        j.mParameters.put("filename", fileName);
        j.mParameters.put("fsync", "1024");
        j.mParameters.put("group_reporting", null);
        j.mParameters.put("ioengine", ioengine);
        j.mParameters.put("new_group", null);
        j.mParameters.put("numjobs", String.format("%d", jobCount));
        j.mParameters.put("rw", rw);
        j.mParameters.put("size", String.format("%dM",fileSize));
        t.mJobs.add(j);

        PerfMetricInfo m = new PerfMetricInfo();
        m.mJobName = jobName;
        if ("sync".equals(ioengine)) {
            m.mPostKey = String.format("%s_bandwidth", rw);
        } else {
            m.mPostKey = String.format("%s_%s_bandwidth", rw, ioengine);
        }
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        if (rw.endsWith("read")) {
            m.mFieldName = "read-bandwidth-mean";
        } else if (rw.endsWith("write")) {
            m.mFieldName = "write-bandwidth-mean";
        }
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = jobName;
        if ("sync".equals(ioengine)) {
            m.mPostKey = String.format("%s_latency", rw);
        } else {
            m.mPostKey = String.format("%s_%s_latency", rw, ioengine);
        }
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        if (rw.endsWith("read")) {
            m.mFieldName = "read-clat-mean";
        } else if (rw.endsWith("write")) {
            m.mFieldName = "write-clat-mean";
        }
        t.mPerfMetrics.add(m);

        mTestCases.add(t);
    }

    /**
     * Sets up the media server benchmark.
     * <p>
     * The test consists of a single process at a higher priority reading or writing to a file
     * while several other worker processes read and write to a different file.
     * </p>
     *
     * @param rw the type of IO pattern. One of {@code read}, {@code write}
     */
    private void addMediaServerTest(String rw) {
        String mediaJob = "media-server";
        String mediaFile = "mediafile";
        String workerJob = "workers";
        String workerFile = "workerfile";

        TestInfo t = new TestInfo();
        t.mTestName = String.format("MediaServerBenchmark-%s", rw);
        t.mKey = "fio_media_server_benchmark";

        TestFileInfo f = new TestFileInfo();
        f.mFileName = new File(mMediaTestDir, mediaFile).getAbsolutePath();
        f.mSize = mMediaServerMediaFileSize * 1024;  // File size is in MB but we want it in kB.
        t.mTestFiles.add(f);

        f = new TestFileInfo();
        f.mFileName = new File(mMediaTestDir, workerFile).getAbsolutePath();
        f.mSize = mMediaServerWorkerFileSize * 1024;  // File size is in MB but we want it in kB.
        t.mTestFiles.add(f);

        JobInfo j = new JobInfo();
        j.mJobName = "global";
        j.mParameters.put("directory", mMediaTestDir);
        j.mParameters.put("fsync", "1024");
        j.mParameters.put("ioengine", "sync");
        j.mParameters.put("runtime", String.format("%d", mMediaServerDuration));
        j.mParameters.put("time_based", null);
        t.mJobs.add(j);

        j = new JobInfo();
        j.mJobName = mediaJob;
        j.mParameters.put("filename", mediaFile);
        j.mParameters.put("iodepth", "32");
        j.mParameters.put("nice", "-16");
        j.mParameters.put("rate", "6m");
        j.mParameters.put("rw", rw);
        j.mParameters.put("size", String.format("%dM", mMediaServerMediaFileSize));
        t.mJobs.add(j);

        j = new JobInfo();
        j.mJobName = workerJob;
        j.mParameters.put("filename", workerFile);
        j.mParameters.put("group_reporting", null);
        j.mParameters.put("new_group", null);
        j.mParameters.put("nice", "0");
        j.mParameters.put("numjobs", String.format("%d", mMediaServerWorkerJobCount));
        j.mParameters.put("rw", "randrw");
        j.mParameters.put("size", String.format("%dM", mMediaServerWorkerFileSize));
        t.mJobs.add(j);

        PerfMetricInfo m = new PerfMetricInfo();
        m.mJobName = mediaJob;
        m.mPostKey = String.format("%s_media_bandwidth", rw);
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        if (rw.endsWith("read")) {
            m.mFieldName = "read-bandwidth-mean";
        } else if (rw.endsWith("write")) {
            m.mFieldName = "write-bandwidth-mean";
        }
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = mediaJob;
        m.mPostKey = String.format("%s_media_latency", rw);
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        if (rw.endsWith("read")) {
            m.mFieldName = "read-clat-mean";
        } else if (rw.endsWith("write")) {
            m.mFieldName = "write-clat-mean";
        }
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = workerJob;
        m.mPostKey = String.format("%s_workers_read_bandwidth", rw);
        m.mFieldName = "read-bandwidth-mean";
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = workerJob;
        m.mPostKey = String.format("%s_workers_write_bandwidth", rw);
        m.mFieldName = "write-bandwidth-mean";
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        t.mPerfMetrics.add(m);

        mTestCases.add(t);
    }

    /**
     * Sets up the media scanner benchmark.
     * <p>
     * The test consists of a single process reading several small files while several other worker
     * processes read and write to a different file.
     * </p>
     */
    private void addMediaScannerTest() {
        String mediaJob = "media-server";
        String mediaFile = "mediafile.%d";
        String workerJob = "workers";
        String workerFile = "workerfile";

        TestInfo t = new TestInfo();
        t.mTestName = "MediaScannerBenchmark";
        t.mKey = "fio_media_scanner_benchmark";

        TestFileInfo f;
        for (int i = 0; i < mMediaScannerMediaFileCount; i++) {
            f = new TestFileInfo();
            f.mFileName = new File(mMediaTestDir, String.format(mediaFile, i)).getAbsolutePath();
            f.mSize = mMediaScannerMediaFileSize;  // File size is already in kB so do nothing.
            t.mTestFiles.add(f);
        }

        f = new TestFileInfo();
        f.mFileName = new File(mMediaTestDir, workerFile).getAbsolutePath();
        f.mSize = mMediaScannerWorkerFileSize * 1024;  // File size is in MB but we want it in kB.
        t.mTestFiles.add(f);

        JobInfo j = new JobInfo();
        j.mJobName = "global";
        j.mParameters.put("directory", mMediaTestDir);
        j.mParameters.put("fsync", "1024");
        j.mParameters.put("ioengine", "sync");
        t.mJobs.add(j);

        j = new JobInfo();
        j.mJobName = mediaJob;
        StringBuilder fileNames = new StringBuilder();
        fileNames.append(String.format(mediaFile, 0));
        for (int i = 1; i < mMediaScannerMediaFileCount; i++) {
            fileNames.append(String.format(":%s", String.format(mediaFile, i)));
        }
        j.mParameters.put("filename", fileNames.toString());
        j.mParameters.put("exitall", null);
        j.mParameters.put("openfiles", "4");
        j.mParameters.put("rw", "read");
        t.mJobs.add(j);

        j = new JobInfo();
        j.mJobName = workerJob;
        j.mParameters.put("filename", workerFile);
        j.mParameters.put("group_reporting", null);
        j.mParameters.put("new_group", null);
        j.mParameters.put("numjobs", String.format("%d", mMediaScannerWorkerJobCount));
        j.mParameters.put("rw", "randrw");
        j.mParameters.put("size", String.format("%dM", mMediaScannerWorkerFileSize));
        t.mJobs.add(j);

        PerfMetricInfo m = new PerfMetricInfo();
        m.mJobName = mediaJob;
        m.mPostKey = "media_bandwidth";
        m.mFieldName = "read-bandwidth-mean";
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = mediaJob;
        m.mPostKey = "media_latency";
        m.mFieldName = "read-clat-mean";
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = workerJob;
        m.mPostKey = "workers_read_bandwidth";
        m.mFieldName = "read-bandwidth-mean";
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        t.mPerfMetrics.add(m);

        m = new PerfMetricInfo();
        m.mJobName = workerJob;
        m.mPostKey = "workers_write_bandwidth";
        m.mFieldName = "write-bandwidth-mean";
        m.mType = PerfMetricInfo.ResultType.FLOAT;
        t.mPerfMetrics.add(m);

        mTestCases.add(t);
    }

    /**
     * Creates the directories needed to run FIO, pushes the FIO executable to the device, and
     * stops the runtime.
     *
     * @throws DeviceNotAvailableException if the device is not available.
     */
    private void setupDevice() throws DeviceNotAvailableException {
        mTestDevice.executeShellCommand("stop");
        mTestDevice.executeShellCommand(String.format("mkdir -p %s", mFioDir));
        mTestDevice.executeShellCommand(String.format("mkdir -p %s", mTmpDir));
        mTestDevice.executeShellCommand(String.format("mkdir -p %s", mInternalTestDir));
        mTestDevice.executeShellCommand(String.format("mkdir -p %s", mMediaTestDir));
        if (mExternalTestDir != null) {
            mTestDevice.executeShellCommand(String.format("mkdir -p %s", mExternalTestDir));
        }
        if (mFioLocation != null) {
            mTestDevice.pushFile(new File(mFioLocation), mFioBin);
            mTestDevice.executeShellCommand(String.format("chmod 755 %s", mFioBin));
        }
    }

    /**
     * Reverses the actions of {@link #setDevice(ITestDevice)}.
     *
     * @throws DeviceNotAvailableException If the device is not available.
     */
    private void cleanupDevice() throws DeviceNotAvailableException {
        if (mExternalTestDir != null) {
            mTestDevice.executeShellCommand(String.format("rm -r %s", mExternalTestDir));
        }
        mTestDevice.executeShellCommand(String.format("rm -r %s", mMediaTestDir));
        mTestDevice.executeShellCommand(String.format("rm -r %s", mInternalTestDir));
        mTestDevice.executeShellCommand(String.format("rm -r %s", mTmpDir));
        mTestDevice.executeShellCommand(String.format("rm -r %s", mFioDir));
        mTestDevice.executeShellCommand("start");
        mTestDevice.waitForDeviceAvailable();
    }

    /**
     * Runs a single test, including creating the test files, clearing the cache, collecting before
     * and after files, running the benchmark, and reporting the results.
     *
     * @param test the benchmark.
     * @param listener the ITestInvocationListener
     * @throws DeviceNotAvailableException if the device is not available.
     */
    private void runTest(TestInfo test, ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        CLog.i("Running %s benchmark", test.mTestName);
        mTestDevice.executeShellCommand(String.format("rm -r %s/*", mTmpDir));
        mTestDevice.executeShellCommand(String.format("rm -r %s/*", mInternalTestDir));
        mTestDevice.executeShellCommand(String.format("rm -r %s/*", mMediaTestDir));
        if (mExternalTestDir != null) {
            mTestDevice.executeShellCommand(String.format("rm -r %s/*", mExternalTestDir));
        }

        for (TestFileInfo file : test.mTestFiles) {
            CLog.v("Creating file: %s, size: %dkB", file.mFileName, file.mSize);
            String cmd = String.format("dd if=/dev/urandom of=%s bs=1024 count=%d", file.mFileName,
                    file.mSize);
            int timeout = file.mSize * 2 * 1000; // Timeout is 2 seconds per kB.
            mTestDevice.executeShellCommand(cmd, new NullOutputReceiver(),
                    timeout, TimeUnit.MILLISECONDS, 2);
        }

        CLog.i("Creating config");
        CLog.d("Config file:\n%s", test.createConfig());
        mTestDevice.pushString(test.createConfig(), mFioConfig);

        CLog.i("Dropping cache");
        mTestDevice.executeShellCommand("echo 3 > /proc/sys/vm/drop_caches");

        collectLogs(test, listener, "before");

        CLog.i("Running test");
        FioParser output = new FioParser();
        // Run FIO with a timeout of 1 hour.
        mTestDevice.executeShellCommand(String.format("%s --minimal %s", mFioBin, mFioConfig),
                output, 60 * 60 * 1000, TimeUnit.MILLISECONDS, 2);

        collectLogs(test, listener, "after");

        // Report metrics
        Map<String, String> metrics = new HashMap<>();
        String key = mKeySuffix == null ? test.mKey : test.mKey + mKeySuffix;

        listener.testRunStarted(key, 0);
        for (PerfMetricInfo m : test.mPerfMetrics) {
            if (!output.mResults.containsKey(m.mJobName)) {
                CLog.w("Job name %s was not found in the results", m.mJobName);
                continue;
            }

            String value = output.getResult(m.mJobName, m.mFieldName);
            if (value != null) {
                metrics.put(m.mPostKey, m.mType.value(value));
            } else {
                CLog.w("%s was not in results for the job %s", m.mFieldName, m.mJobName);
            }
        }

        CLog.d("About to report metrics to %s: %s", key, metrics);
        listener.testRunEnded(0, TfMetricProtoUtil.upgradeConvert(metrics));
    }

    private void collectLogs(TestInfo testInfo, ITestInvocationListener listener,
            String descriptor) throws DeviceNotAvailableException {
        if (mCollectYaffsLogs && mTestDevice.doesFileExist("/proc/yaffs")) {
            logFile("/proc/yaffs", String.format("%s-yaffs-%s", testInfo.mTestName, descriptor),
                    mTestDevice, listener);
        }
    }

    private void logFile(String remoteFileName, String localFileName, ITestDevice testDevice,
            ITestInvocationListener listener) throws DeviceNotAvailableException {
        File outputFile = null;
        InputStreamSource outputSource = null;
        try {
            outputFile = testDevice.pullFile(remoteFileName);
            if (outputFile != null) {
                CLog.d("Sending %d byte file %s to logosphere!", outputFile.length(), outputFile);
                outputSource = new FileInputStreamSource(outputFile);
                listener.testLog(localFileName, LogDataType.TEXT, outputSource);
            }
        } finally {
            FileUtil.deleteFile(outputFile);
            StreamUtil.cancel(outputSource);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        Assert.assertNotNull(mTestDevice);

        mFioDir = new File(mTestDevice.getMountPoint(IDevice.MNT_DATA), "fio").getAbsolutePath();
        if (mFioLocation != null) {
            mFioBin = new File(mFioDir, "fio").getAbsolutePath();
        } else {
            mFioBin = "fio";
        }
        mFioConfig = new File(mFioDir, "config.fio").getAbsolutePath();

        setupTests();
        setupDevice();

        for (TestInfo test : mTestCases) {
            runTest(test, listener);
        }

        cleanupDevice();
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
     * A meta-test to ensure that the bits of FioBenchmarkTest are working properly.
     */
    public static class MetaTest extends TestCase {

        /**
         * Test that {@link JobInfo#createJob()} properly formats a job.
         */
        public void testCreateJob() {
            JobInfo j = new JobInfo();
            assertEquals("", j.createJob());
            j.mJobName = "job";
            assertEquals("[job]\n", j.createJob());
            j.mParameters.put("param1", null);
            j.mParameters.put("param2", "value");
            String[] lines = j.createJob().split("\n");
            assertEquals(3, lines.length);
            assertEquals("[job]", lines[0]);
            Set<String> params = new HashSet<>(2);
            params.add(lines[1]);
            params.add(lines[2]);
            assertTrue(params.contains("param1"));
            assertTrue(params.contains("param2=value"));
        }

        /**
         * Test that {@link TestInfo#createConfig()} properly formats a config.
         */
        public void testCreateConfig() {
            TestInfo t = new TestInfo();
            JobInfo j = new JobInfo();
            j.mJobName = "job1";
            j.mParameters.put("param1", "value1");
            t.mJobs.add(j);

            j = new JobInfo();
            j.mJobName = "job2";
            j.mParameters.put("param2", "value2");
            t.mJobs.add(j);

            j = new JobInfo();
            j.mJobName = "job3";
            j.mParameters.put("param3", "value3");
            t.mJobs.add(j);

            assertEquals("[job1]\nparam1=value1\n\n" +
                    "[job2]\nparam2=value2\n\n" +
                    "[job3]\nparam3=value3\n\n", t.createConfig());
        }

        /**
         * Test that output lines are parsed correctly by the FioParser, invalid lines are ignored,
         * and that the various fields are accessible with
         * {@link FioParser#getResult(String, String)}.
         */
        public void testFioParser() {
            String[] lines = new String[4];
            // We build the lines up as follows (assuming FIO_RESULTS_FIELDS.length == 58):
            // 0;3;6;...;171
            // 1;4;7;...;172
            // 2;5;8;...;173
            for (int i = 0; i < 3; i++) {
                StringBuilder sb = new StringBuilder();
                sb.append(i);
                for (int j = 1; j < FIO_V0_RESULT_FIELDS.length; j++) {
                    sb.append(";");
                    sb.append(j * 3 + i);
                }
                lines[i] = sb.toString();
            }
            // A line may have an optional description on the end which we don't care about, make
            // sure it still parses.
            lines[2] = lines[2] += ";description";
            // Make sure an invalid output line does not parse.
            lines[3] = "invalid";

            FioParser p = new FioParser();
            p.processNewLines(lines);


            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < FIO_V0_RESULT_FIELDS.length; j++) {
                    assertEquals(String.format("job=%d, field=%s", i, FIO_V0_RESULT_FIELDS[j]),
                            String.format("%d", j * 3 + i),
                            p.getResult(String.format("%d", i), FIO_V0_RESULT_FIELDS[j]));
                }
            }
            assertNull(p.getResult("missing", "jobname"));
            assertNull(p.getResult("invalid", "jobname"));
            assertNull(p.getResult("0", "missing"));
        }

        /**
         * Test that {@link PerfMetricInfo.ResultType#value(String)} correctly transforms strings
         * based on the result type.
         */
        public void testResultTypeValue() {
            assertEquals("test", PerfMetricInfo.ResultType.STRING.value("test"));
            assertEquals("1", PerfMetricInfo.ResultType.INT.value("1"));
            assertEquals("3.14159", PerfMetricInfo.ResultType.FLOAT.value("3.14159"));
            assertEquals(String.format("%f", 0.34567),
                    PerfMetricInfo.ResultType.PERCENT.value("34.567%"));
            assertNull(PerfMetricInfo.ResultType.PERCENT.value(""));
            assertNull(PerfMetricInfo.ResultType.PERCENT.value("34.567"));
            assertNull(PerfMetricInfo.ResultType.PERCENT.value("test%"));
        }
    }
}
