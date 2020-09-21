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
package android.device.collectors;

import android.app.Instrumentation;
import android.os.Bundle;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

import static org.junit.Assert.assertEquals;

/**
 * Android Unit tests for {@link ScheduledRunMetricListener}.
 */
@RunWith(AndroidJUnit4.class)
public class ScheduledRunMetricListenerTest {

    private static final String TEST_RUN_KEY = "periodic_key";
    private static final String TEST_RUN_VALUE = "periodic_value";

    private ScheduledRunMetricListener mListener;

    @Before
    public void setUp() {
        Instrumentation mockInstrumentation = Mockito.mock(Instrumentation.class);
        Bundle b = new Bundle();
        b.putString(ScheduledRunMetricListener.INTERVAL_ARG_KEY, "100");
        mListener = new ScheduledRunMetricListener(b) {
            private int counter = 0;
            @Override
            public void collect(DataRecord runData, Description desc) throws InterruptedException {
                runData.addStringMetric(TEST_RUN_KEY + counter, TEST_RUN_VALUE + counter);
                counter++;
            }
        };
        mListener.setInstrumentation(mockInstrumentation);
    }

    @Test
    public void testPeriodicRun() throws Exception {
        Description runDescription = Description.createSuiteDescription("run");
        mListener.testRunStarted(runDescription);
        Thread.sleep(500L);
        mListener.testRunFinished(new Result());
        // AJUR runner is then gonna call instrumentationRunFinished
        Bundle resultBundle = new Bundle();
        mListener.instrumentationRunFinished(System.out, resultBundle, new Result());

        // Check that final bundle contains some of the periodic run results
        Assert.assertTrue(resultBundle.size() > 4);
        assertEquals(TEST_RUN_VALUE + "0", resultBundle.getString(TEST_RUN_KEY + "0"));
        assertEquals(TEST_RUN_VALUE + "1", resultBundle.getString(TEST_RUN_KEY + "1"));
        assertEquals(TEST_RUN_VALUE + "2", resultBundle.getString(TEST_RUN_KEY + "2"));
    }
}
