/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.cts.statsd.alarm;

import android.cts.statsd.atom.AtomTestCase;

import com.android.internal.os.StatsdConfigProto;
import com.android.internal.os.StatsdConfigProto.Alarm;
import com.android.internal.os.StatsdConfigProto.IncidentdDetails;
import com.android.internal.os.StatsdConfigProto.StatsdConfig;
import com.android.internal.os.StatsdConfigProto.Subscription;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.List;

/**
 * Statsd Anomaly Detection tests.
 */
public class AlarmTests extends AtomTestCase {

    private static final String TAG = "Statsd.AnomalyDetectionTests";

    private static final boolean INCIDENTD_TESTS_ENABLED = false;

    // Config constants
    private static final int ALARM_ID = 11;
    private static final int SUBSCRIPTION_ID_INCIDENTD = 41;
    private static final int INCIDENTD_SECTION = -1;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (!INCIDENTD_TESTS_ENABLED) {
            CLog.w(TAG, TAG + " alarm tests are disabled by a flag. Change flag to true to run");
        }
    }

    public void testAlarm() throws Exception {
        if (statsdDisabled()) {
            return;
        }
        StatsdConfig.Builder config = getBaseConfig();
        turnScreenOn();
        uploadConfig(config);

        String markTime = getCurrentLogcatDate();
        Thread.sleep(9_000);

        if (INCIDENTD_TESTS_ENABLED) assertTrue("No incident", didIncidentdFireSince(markTime));
    }


    private final StatsdConfig.Builder getBaseConfig() throws Exception {
        return StatsdConfig.newBuilder().setId(CONFIG_ID)
                .addAlarm(Alarm.newBuilder()
                        .setId(ALARM_ID)
                        .setOffsetMillis(2)
                        .setPeriodMillis(5_000) // every 5 seconds.
                )
                .addSubscription(Subscription.newBuilder()
                        .setId(SUBSCRIPTION_ID_INCIDENTD)
                        .setRuleType(Subscription.RuleType.ALARM)
                        .setRuleId(ALARM_ID)
                        .setIncidentdDetails(IncidentdDetails.newBuilder()
                                .addSection(INCIDENTD_SECTION))
                )
                .addAllowedLogSource("AID_SYSTEM");
    }
}
