/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.settings.notification;

import static com.google.common.truth.Truth.assertThat;
import static junit.framework.Assert.assertEquals;

import android.app.NotificationManager;
import android.app.NotificationManager.Policy;
import android.content.Context;
import android.provider.SearchIndexableResource;

import com.android.settings.R;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

import java.util.List;

@RunWith(SettingsRobolectricTestRunner.class)
public class ZenModeSettingsTest {

    private ZenModeSettings.SummaryBuilder mBuilder;
    private Context mContext;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application.getApplicationContext();
        mBuilder = new ZenModeSettings.SummaryBuilder(mContext);
    }

    @Test
    public void testBlockedEffectsSummary_none() {
        Policy policy = new Policy(0, 0, 0, 0);
        assertEquals(mContext.getString(R.string.zen_mode_restrict_notifications_summary_muted),
                mBuilder.getBlockedEffectsSummary(policy));
    }

    @Test
    public void testBlockedEffectsSummary_some() {
        Policy policy = new Policy(0, 0, 0, NotificationManager.Policy.SUPPRESSED_EFFECT_PEEK);
        assertEquals(mContext.getString(R.string.zen_mode_restrict_notifications_summary_custom),
                mBuilder.getBlockedEffectsSummary(policy));
    }

    @Test
    public void testBlockedEffectsSummary_all() {
        Policy policy = new Policy(0, 0, 0, 511);
        assertEquals(mContext.getString(R.string.zen_mode_restrict_notifications_summary_hidden),
                mBuilder.getBlockedEffectsSummary(policy));
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_none() {
        Policy policy = new Policy(0, 0, 0, 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy)).isEqualTo("None");
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_single() {
        Policy policy = new Policy(
                Policy.PRIORITY_CATEGORY_ALARMS | Policy.PRIORITY_CATEGORY_EVENTS, 0 , 0 , 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy)).isEqualTo("Events");
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_someMsgs() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_MESSAGES, 0,
                Policy.PRIORITY_SENDERS_CONTACTS , 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy)).isEqualTo("Some messages");

        policy = new Policy(Policy.PRIORITY_CATEGORY_MESSAGES, 0,
                Policy.PRIORITY_SENDERS_STARRED , 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy)).isEqualTo("Some messages");
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_msgs() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_MESSAGES, 0, 0, 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy)).isEqualTo("Messages");
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_someMsgsAndOther() {
        Policy policy = new Policy(
                Policy.PRIORITY_CATEGORY_MESSAGES | Policy.PRIORITY_CATEGORY_REMINDERS,
                0, Policy.PRIORITY_SENDERS_CONTACTS , 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy))
                .isEqualTo("Some messages and reminders");
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_someMsgsAndAllOthers() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_EVENTS
                | Policy.PRIORITY_CATEGORY_MESSAGES | Policy.PRIORITY_CATEGORY_REMINDERS,
                0, Policy.PRIORITY_SENDERS_CONTACTS , 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy))
                .isEqualTo("Some messages, events, and reminders");
    }

    @Test
    public void testGetMsgEventReminderSettingSummary_noMsgsAndOther() {
        Policy policy = new Policy(
                Policy.PRIORITY_CATEGORY_EVENTS | Policy.PRIORITY_CATEGORY_REMINDERS,
                0,0, 0);
        assertThat(mBuilder.getMsgEventReminderSettingSummary(policy))
                .isEqualTo("Events and reminders");
    }

    @Test
    public void testGetCallsSettingSummary_none() {
        Policy policy = new Policy(0, 0, 0, 0);
        assertThat(mBuilder.getCallsSettingSummary(policy)).isEqualTo("None");
    }

    @Test
    public void testGetCallsSettingSummary_contacts() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_ALARMS | Policy.PRIORITY_CATEGORY_CALLS,
                Policy.PRIORITY_SENDERS_CONTACTS, 0, 0);
        assertThat(mBuilder.getCallsSettingSummary(policy)).isEqualTo("From contacts");
    }

    @Test
    public void testGetCallsSettingSummary_repeatCallers() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_REPEAT_CALLERS, 0, 0, 0);
        assertThat(mBuilder.getCallsSettingSummary(policy)).isEqualTo("From repeat callers");
    }

    @Test
    public void testGetCallsSettingSummary_starredRepeatCallers() {
        Policy policy = new Policy(
                Policy.PRIORITY_CATEGORY_REPEAT_CALLERS | Policy.PRIORITY_CATEGORY_CALLS,
                Policy.PRIORITY_SENDERS_STARRED, 0, 0);
        assertThat(mBuilder.getCallsSettingSummary(policy))
                .isEqualTo("From starred contacts and repeat callers");
    }

    @Test
    public void testGetSoundSettingSummary_allOff() {
        Policy policy = new Policy(0, 0, 0, 0);
        assertThat(mBuilder.getSoundSettingSummary(policy)).isEqualTo("Muted");
    }

    @Test
    public void testGetSoundSettingSummary_allOn() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_ALARMS | Policy.PRIORITY_CATEGORY_SYSTEM
                | Policy.PRIORITY_CATEGORY_MEDIA, 0, 0, 0);
        assertThat(mBuilder.getSoundSettingSummary(policy))
                .isEqualTo("Muted, but allow alarms, media, and touch sounds");
    }

    @Test
    public void testGetSoundSettingSummary_allOffButOne() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_MEDIA, 0, 0, 0);
        assertThat(mBuilder.getSoundSettingSummary(policy)).isEqualTo("Muted, but allow media");
    }

    @Test
    public void testGetSoundSettingSummary_allOffButTwo() {
        Policy policy = new Policy(Policy.PRIORITY_CATEGORY_SYSTEM
                | Policy.PRIORITY_CATEGORY_MEDIA, 0, 0, 0);
        assertThat(mBuilder.getSoundSettingSummary(policy))
                .isEqualTo("Muted, but allow media and touch sounds");
    }

    @Test
    public void searchProvider_shouldIndexDefaultXml() {
        final List<SearchIndexableResource> sir = ZenModeSettings.SEARCH_INDEX_DATA_PROVIDER
                .getXmlResourcesToIndex(mContext, true /* enabled */);

        assertThat(sir).hasSize(1);
        assertThat(sir.get(0).xmlResId).isEqualTo(R.xml.zen_mode_settings);
    }
}
