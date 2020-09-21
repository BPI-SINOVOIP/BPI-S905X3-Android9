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

package android.filesystem.cts;

import static android.support.test.InstrumentationRegistry.getContext;
import static android.support.test.InstrumentationRegistry.getInstrumentation;

import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import com.android.compatibility.common.util.DeviceReportLog;
import com.android.compatibility.common.util.SystemUtil;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

@RunWith(AndroidJUnit4.class)
public class AlmostFullTest {
    private static final String DIR_INITIAL_FILL = "INITIAL_FILL";
    private static final String DIR_SEQ_UPDATE = "SEQ_UPDATE";
    private static final String DIR_RANDOM_WR = "RANDOM_WR";
    private static final String DIR_RANDOM_RD = "RANDOM_RD";
    private static final String TAG = "AlmostFullTest";
    private static final String REPORT_LOG_NAME = "CtsFileSystemTestCases";

    private static final long FREE_SPACE_FINAL = 1000L * 1024 * 1024L;

    // test runner creates multiple instances at the begging.
    // use that to fill disk only once.
    // set as final to initialize it only once
    private static final AtomicInteger mRefCounter = new AtomicInteger(0);
    private static final AtomicBoolean mDiskFilled = new AtomicBoolean(false);

    public AlmostFullTest() {
        int currentCounter = mRefCounter.incrementAndGet();
        Log.i(TAG, "++currentCounter: " + currentCounter);
    }

    @Before
    public void setUp() throws Exception {
        if (mDiskFilled.compareAndSet(false, true)) {
            Log.i(TAG, "Filling disk");
            // initial fill done in two stage as disk can be filled by other
            // components
            long freeDisk = SystemUtil.getFreeDiskSize(getContext());
            long diskToFill = freeDisk - FREE_SPACE_FINAL;
            if (diskToFill >= 0) {
                Log.i(TAG, "free disk " + freeDisk + ", to fill " + diskToFill);
            } else {
                Log.i(TAG, "free disk " + freeDisk + " too small, needs " + FREE_SPACE_FINAL);
                return;
            }
            // Ensure MAX_SIZE_TO_FILL is an integral multiple of FileUtil.BUFFER_SIZE to avoid
            // rounding errors caused by FileUtil.createNewFilledFile. See b/63535343.
            final long MAX_FILE_SIZE_TO_FILL = FileUtil.BUFFER_SIZE * 100L;
            long filled = 0;
            while (filled < diskToFill) {
                long toFill = diskToFill - filled;
                if (toFill > MAX_FILE_SIZE_TO_FILL) {
                    toFill = MAX_FILE_SIZE_TO_FILL;
                }
                Log.i(TAG, "Generating file " + toFill);
                FileUtil.createNewFilledFile(getContext(),
                        DIR_INITIAL_FILL, toFill);
                filled += toFill;
            }
        }
        Log.i(TAG, "free disk " + SystemUtil.getFreeDiskSize(getContext()));
    }

    @After
    public void tearDown() throws Exception {
        Log.i(TAG, "tearDown free disk " + SystemUtil.getFreeDiskSize(getContext()));
        int currentCounter = mRefCounter.decrementAndGet();
        Log.i(TAG, "--currentCounter: " + currentCounter);
        if (currentCounter == 0) {
            FileUtil.removeFileOrDir(getContext(), DIR_INITIAL_FILL);
        }
        FileUtil.removeFileOrDir(getContext(), DIR_SEQ_UPDATE);
        FileUtil.removeFileOrDir(getContext(), DIR_RANDOM_WR);
        FileUtil.removeFileOrDir(getContext(), DIR_RANDOM_RD);
        Log.i(TAG, "tearDown free disk " + SystemUtil.getFreeDiskSize(getContext()));
    }

    @Test
    public void testSequentialUpdate() throws Exception {
        // now about freeSpaceToLeave should be left
        // and try updating exceeding the free space size
        final long FILE_SIZE = 400L * 1024L * 1024L;
        long freeDisk = SystemUtil.getFreeDiskSize(getContext());
        Log.i(TAG, "Now free space is " + freeDisk);
        if (freeDisk < FILE_SIZE) {
            Log.w(TAG, "too little space: " + freeDisk);
            return;
        }
        final int BUFFER_SIZE = 10 * 1024 * 1024;
        final int NUMBER_REPETITION = 10;
        String streamName = "test_sequential_update";
        FileUtil.doSequentialUpdateTest(getContext(), DIR_SEQ_UPDATE, FILE_SIZE, BUFFER_SIZE,
                NUMBER_REPETITION, REPORT_LOG_NAME, streamName);
    }

    // TODO: file size too small and caching will give wrong better result.
    // needs to flush cache by reading big files per each read.
    @Test
    public void testRandomRead() throws Exception {
        final int BUFFER_SIZE = 4 * 1024;
        final long fileSize = 400L * 1024L * 1024L;
        long freeDisk = SystemUtil.getFreeDiskSize(getContext());
        if (freeDisk < fileSize) {
            Log.w(TAG, "too little space: " + freeDisk);
            return;
        }
        String streamName = "test_random_read";
        DeviceReportLog report = new DeviceReportLog(REPORT_LOG_NAME, streamName);
        FileUtil.doRandomReadTest(getContext(), DIR_RANDOM_RD, report, fileSize, BUFFER_SIZE);
        report.submit(getInstrumentation());
    }

    @Test
    public void testRandomUpdate() throws Exception {
        final int BUFFER_SIZE = 4 * 1024;
        final long fileSize = 256L * 1024L * 1024L;
        long freeDisk = SystemUtil.getFreeDiskSize(getContext());
        if (freeDisk < fileSize) {
            Log.w(TAG, "too little space: " + freeDisk);
            return;
        }
        String streamName = "test_random_update";
        DeviceReportLog report = new DeviceReportLog(REPORT_LOG_NAME, streamName);
        FileUtil.doRandomWriteTest(getContext(), DIR_RANDOM_WR, report, fileSize, BUFFER_SIZE);
        report.submit(getInstrumentation());
    }
}
