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

package android.taskswitching.control.cts;

import android.content.Intent;
import android.net.Uri;
import android.os.RemoteCallback;

import com.android.compatibility.common.util.CtsAndroidTestCase;
import com.android.compatibility.common.util.DeviceReportLog;
import com.android.compatibility.common.util.MeasureRun;
import com.android.compatibility.common.util.MeasureTime;
import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.compatibility.common.util.Stat;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Device test which actually launches two apps sequentially and
 * measure time for switching.
 * Completion of launch is notified via broadcast.
 */
public class TaskSwitchingDeviceTest extends CtsAndroidTestCase {
    private static final String REPORT_LOG_NAME = "CtsUiHostTestCases";
    private static final long TASK_SWITCHING_WAIT_TIME = 5;

    private final Semaphore mSemaphore = new Semaphore(0);

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        startActivitiesABSequentially();
    }

    public void testMeasureTaskSwitching() throws Exception {
        final int NUMBER_REPEAT = 10;
        final int SWITCHING_PER_ONE_TRY = 10;

        double[] results = MeasureTime.measure(NUMBER_REPEAT, new MeasureRun() {

            @Override
            public void run(int i) throws Exception {
                for (int j = 0; j < SWITCHING_PER_ONE_TRY; j++) {
                    startActivitiesABSequentially();
                }
            }
        });
        String streamName = "test_measure_task_switching";
        DeviceReportLog report = new DeviceReportLog(REPORT_LOG_NAME, streamName);
        report.addValues("task_switching_time", results, ResultType.LOWER_BETTER, ResultUnit.MS);
        Stat.StatResult stat = Stat.getStat(results);
        report.setSummary("task_switching_time_average", stat.mAverage, ResultType.LOWER_BETTER,
                ResultUnit.MS);
        report.submit(getInstrumentation());
    }

    private void startActivitiesABSequentially()
            throws InterruptedException, TimeoutException, ExecutionException {
        startActivityAndWait('a');
        startActivityAndWait('b');
    }

    private void startActivityAndWait(char activityLetter)
            throws InterruptedException, TimeoutException, ExecutionException {
        getContext().startActivity(new Intent(Intent.ACTION_VIEW)
                .setData(Uri.parse("https://foo.com/app" + activityLetter))
                .addCategory(Intent.CATEGORY_DEFAULT)
                .addCategory(Intent.CATEGORY_BROWSABLE)
                .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .putExtra("callback", new RemoteCallback(b -> mSemaphore.release())));
        mSemaphore.tryAcquire(TASK_SWITCHING_WAIT_TIME, TimeUnit.SECONDS);
    }
}
