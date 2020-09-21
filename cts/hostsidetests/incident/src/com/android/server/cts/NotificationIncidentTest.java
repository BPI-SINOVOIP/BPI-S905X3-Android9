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

package com.android.server.cts;

import android.app.PolicyProto;
import android.service.notification.ConditionProto;
import android.service.notification.ManagedServicesProto;
import android.service.notification.NotificationRecordProto;
import android.service.notification.NotificationServiceDumpProto;
import android.service.notification.NotificationRecordProto.State;
import android.service.notification.RankingHelperProto;
import android.service.notification.RankingHelperProto.RecordProto;
import android.service.notification.ZenMode;
import android.service.notification.ZenModeProto;
import android.service.notification.ZenRuleProto;

import java.util.List;

/**
 * Test to check that the notification service properly outputs its dump state.
 *
 * make -j32 CtsIncidentHostTestCases
 * cts-tradefed run singleCommand cts-dev -d --module CtsIncidentHostTestCases
 */
public class NotificationIncidentTest extends ProtoDumpTestCase {
    // Constants from android.app.NotificationManager
    private static final int IMPORTANCE_UNSPECIFIED = -1000;
    private static final int IMPORTANCE_NONE = 0;
    private static final int IMPORTANCE_MAX = 5;
    private static final int VISIBILITY_NO_OVERRIDE = -1000;
    // Constants from android.app.Notification
    private static final int PRIORITY_MIN = -2;
    private static final int PRIORITY_MAX = 2;
    private static final int VISIBILITY_SECRET = -1;
    private static final int VISIBILITY_PUBLIC = 1;
    // These constants are those in PackageManager.
    public static final String FEATURE_WATCH = "android.hardware.type.watch";

    /**
     * Tests that at least one notification is posted, and verify its properties are plausible.
     */
    public void testNotificationRecords() throws Exception {
        final NotificationServiceDumpProto dump = getDump(NotificationServiceDumpProto.parser(),
                "dumpsys notification --proto");

        assertTrue(dump.getRecordsCount() > 0);
        boolean found = false;
        for (NotificationRecordProto record : dump.getRecordsList()) {
            if (record.getKey().contains("android")) {
                found = true;
                assertTrue(record.getImportance() > IMPORTANCE_NONE);

                // Ensure these fields exist, at least
                record.getFlags();
                record.getChannelId();
                record.getSound();
                record.getAudioAttributes();
                record.getCanVibrate();
                record.getCanShowLight();
                record.getGroupKey();
            }
            assertTrue(
                NotificationRecordProto.State.getDescriptor()
                        .getValues()
                        .contains(record.getState().getValueDescriptor()));
        }

        assertTrue(found);
    }

    /** Test valid values from the RankingHelper. */
    public void testRankingConfig() throws Exception {
        final NotificationServiceDumpProto dump = getDump(NotificationServiceDumpProto.parser(),
                "dumpsys notification --proto");

        verifyRankingHelperProto(dump.getRankingConfig(), PRIVACY_NONE);
    }

    private static void verifyRankingHelperProto(RankingHelperProto rhProto, final int filterLevel) throws Exception {
        for (RecordProto rp : rhProto.getRecordsList()) {
            verifyRecordProto(rp);
        }
        for (RecordProto rp : rhProto.getRecordsRestoredWithoutUidList()) {
            verifyRecordProto(rp);
        }
    }

    private static void verifyRecordProto(RecordProto rp) throws Exception {
        assertTrue(!rp.getPackage().isEmpty());
        assertTrue(rp.getUid() == -10000 || rp.getUid() >= 0);
        assertTrue("Record importance is an invalid value: " + rp.getImportance(),
                rp.getImportance() == IMPORTANCE_UNSPECIFIED ||
                (rp.getImportance() >= IMPORTANCE_NONE && rp.getImportance() <= IMPORTANCE_MAX));
        assertTrue(rp.getPriority() >= PRIORITY_MIN && rp.getPriority() <= PRIORITY_MAX);
        assertTrue("Record visibility is an invalid value: " + rp.getVisibility(),
                rp.getVisibility() == VISIBILITY_NO_OVERRIDE ||
                (rp.getVisibility() >= VISIBILITY_SECRET &&
                 rp.getVisibility() <= VISIBILITY_PUBLIC));
    }

    // Tests default state: zen mode is a valid/expected value
    public void testZenMode() throws Exception {
        final NotificationServiceDumpProto dump = getDump(NotificationServiceDumpProto.parser(),
                "dumpsys notification --proto");

        verifyZenModeProto(dump.getZen(), PRIVACY_NONE);
    }

    private static void verifyZenModeProto(ZenModeProto zenProto, final int filterLevel) throws Exception {
        assertTrue("Unexpected ZenMode value",
                ZenMode.getDescriptor().getValues().contains(zenProto.getZenMode().getValueDescriptor()));

        List<ZenRuleProto> zenRules = zenProto.getEnabledActiveConditionsList();
        for (int i = 0; i < zenRules.size(); ++i) {
            ZenRuleProto zr = zenRules.get(i);
            ConditionProto cp = zr.getCondition();
            if (filterLevel == PRIVACY_AUTO) {
                assertTrue(zr.getId().isEmpty());
                assertTrue(zr.getName().isEmpty());
                assertTrue(zr.getConditionId().isEmpty());

                assertTrue(cp.getId().isEmpty());
                assertTrue(cp.getSummary().isEmpty());
                assertTrue(cp.getLine1().isEmpty());
                assertTrue(cp.getLine2().isEmpty());
            } else if (i > 0) {
                // There will be at most one manual rule, the rest will be automatic. The fields
                // tested here are required for automatic rules.
                assertFalse(zr.getId().isEmpty());
                assertFalse(zr.getName().isEmpty());
                assertTrue(zr.getCreationTimeMs() > 0);
                assertFalse(zr.getConditionId().isEmpty());
            }

            assertTrue(ConditionProto.State.getDescriptor().getValues()
                    .contains(cp.getState().getValueDescriptor()));
        }

        PolicyProto policy = zenProto.getPolicy();
        for (PolicyProto.Category c : policy.getPriorityCategoriesList()) {
            assertTrue(PolicyProto.Category.getDescriptor().getValues()
                    .contains(c.getValueDescriptor()));
        }
        assertTrue(PolicyProto.Sender.getDescriptor().getValues()
                .contains(policy.getPriorityCallSender().getValueDescriptor()));
        assertTrue(PolicyProto.Sender.getDescriptor().getValues()
                .contains(policy.getPriorityMessageSender().getValueDescriptor()));
        for (PolicyProto.SuppressedVisualEffect sve : policy.getSuppressedVisualEffectsList()) {
            assertTrue(PolicyProto.SuppressedVisualEffect.getDescriptor().getValues()
                    .contains(sve.getValueDescriptor()));
        }
    }

    static void verifyNotificationServiceDumpProto(NotificationServiceDumpProto dump, final int filterLevel) throws Exception {
        for (NotificationRecordProto nr : dump.getRecordsList()) {
            verifyNotificationRecordProto(nr, filterLevel);
        }
        verifyZenModeProto(dump.getZen(), filterLevel);
        verifyManagedServicesProto(dump.getNotificationListeners(), filterLevel);
        verifyManagedServicesProto(dump.getNotificationAssistants(), filterLevel);
        verifyManagedServicesProto(dump.getConditionProviders(), filterLevel);
        verifyRankingHelperProto(dump.getRankingConfig(), filterLevel);
    }

    private static void verifyManagedServicesProto(ManagedServicesProto ms, final int filterLevel) throws Exception {
        for (ManagedServicesProto.ServiceProto sp : ms.getApprovedList()) {
            for (String n : sp.getNameList()) {
                assertFalse(n.isEmpty());
            }
            assertTrue(sp.getUserId() >= 0);
        }
    }

    private static void verifyNotificationRecordProto(NotificationRecordProto record, final int filterLevel) throws Exception {
        // Ensure these fields exist, at least
        record.getFlags();
        record.getChannelId();
        record.getSound();
        record.getAudioAttributes();
        record.getCanVibrate();
        record.getCanShowLight();
        record.getGroupKey();

        if (filterLevel == PRIVACY_AUTO) {
            assertTrue(record.getChannelId().isEmpty());
            assertTrue(record.getSound().isEmpty());
            assertTrue(record.getGroupKey().isEmpty());
        }

        assertTrue(NotificationRecordProto.State.getDescriptor().getValues()
                .contains(record.getState().getValueDescriptor()));
    }
}
