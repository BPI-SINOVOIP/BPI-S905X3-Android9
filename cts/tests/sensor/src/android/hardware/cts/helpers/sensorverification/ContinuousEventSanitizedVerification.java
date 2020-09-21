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

package android.hardware.cts.helpers.sensorverification;

import android.hardware.Sensor;
import android.hardware.cts.helpers.SensorStats;
import android.hardware.cts.helpers.TestSensorEnvironment;
import android.hardware.cts.helpers.TestSensorEvent;
import android.os.SystemClock;
import android.util.Log;
import junit.framework.Assert;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 * A {@link ISensorVerification} which verifies that continuous events are sanitized
 * when the UID is idle. In this case no events should be fired. We expect to receive
 * events and stop receiving since some events may have been cached already.
 */
public class ContinuousEventSanitizedVerification extends AbstractSensorVerification {
    public static final String PASSED_KEY = "continuous_event_sanitization_passed";

    // Number of indices to print in assert message before truncating
    private static final int TRUNCATE_MESSAGE_LENGTH = 3;

    private final List<TestSensorEvent> mNonSanitizedEvents = new LinkedList<>();

    private final long mVerificationStartTimeNano;

    /**
     * Get the default {@link ContinuousEventSanitizedVerification} for a sensor.
     *
     * @param environment the test environment
     * @param verificationDelayNano After what delay to expect no events (nanoseconds).
     * @return the verification or null if the verification does not apply to the sensor.
     */
    public static ContinuousEventSanitizedVerification getDefault(
            TestSensorEnvironment environment, long verificationDelayNano) {
        final int reportingMode = environment.getSensor().getReportingMode();
        if (reportingMode == Sensor.REPORTING_MODE_CONTINUOUS) {
            return new ContinuousEventSanitizedVerification(
                    SystemClock.elapsedRealtimeNanos() + verificationDelayNano);
        }
        return null;
    }

    private ContinuousEventSanitizedVerification(long verificationDelayNano) {
        mVerificationStartTimeNano = verificationDelayNano;
    }

    /**
     * Verify that the events are sanitized. Add {@value #PASSED_KEY} and
     * {@value SensorStats#EVENT_NOT_SANITIZED_KEY} keys to {@link SensorStats}.
     *
     * @throws AssertionError if the verification failed.
     */
    @Override
    public void verify(TestSensorEnvironment environment, SensorStats stats) {
        final int count = mNonSanitizedEvents.size();
        if (count > 0) {
            stats.addValue(PASSED_KEY, false);
            stats.addValue(SensorStats.EVENT_NOT_SANITIZED_KEY, mNonSanitizedEvents);

            final StringBuilder sb = new StringBuilder();
            sb.append(mNonSanitizedEvents).append(" non-sanitized events: ");

            for (int i = 0; i < Math.min(count, TRUNCATE_MESSAGE_LENGTH); i++) {
                final TestSensorEvent event = mNonSanitizedEvents.get(i);
                sb.append(String.format("sensor:%s", event.sensor.getName()));
                sb.append(String.format("accuracy:%d", event.accuracy));
                sb.append(String.format("values:%s", Arrays.toString(event.values)));

                if (count > TRUNCATE_MESSAGE_LENGTH) {
                    sb.append(count - TRUNCATE_MESSAGE_LENGTH).append(" more");
                } else {
                    // Delete the trailing "; "
                    sb.delete(sb.length() - 2, sb.length());
                    break;
                }
            }

            Assert.fail(sb.toString());
        } else {
            stats.addValue(PASSED_KEY, true);
        }
    }

    @Override
    public ContinuousEventSanitizedVerification clone() {
        return new ContinuousEventSanitizedVerification(mVerificationStartTimeNano);
    }

    @Override
    protected void addSensorEventInternal(TestSensorEvent event) {
        Log.i("OPALA", "event time: " + event.timestamp + " verif start:" + mVerificationStartTimeNano);
        if (event.receivedTimestamp > mVerificationStartTimeNano) {
            mNonSanitizedEvents.add(event);
        }
    }
}