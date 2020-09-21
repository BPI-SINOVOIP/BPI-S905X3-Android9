/**
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

package android.app.usage.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.app.AppOpsManager;
import android.app.KeyguardManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.usage.EventStats;
import android.app.usage.UsageEvents;
import android.app.usage.UsageEvents.Event;
import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Parcel;
import android.os.SystemClock;
import android.platform.test.annotations.AppModeFull;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.text.format.DateUtils;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseLongArray;
import android.view.KeyEvent;

import com.android.compatibility.common.util.AppStandbyUtils;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Test the UsageStats API. It is difficult to test the entire surface area
 * of the API, as a lot of the testing depends on what data is already present
 * on the device and for how long that data has been aggregating.
 *
 * These tests perform simple checks that each interval is of the correct duration,
 * and that events do appear in the event log.
 *
 * Tests to add that are difficult to add now:
 * - Invoking a device configuration change and then watching for it in the event log.
 * - Changing the system time and verifying that all data has been correctly shifted
 *   along with the new time.
 * - Proper eviction of old data.
 */
@RunWith(AndroidJUnit4.class)
public class UsageStatsTest {
    private static final boolean DEBUG = false;
    private static final String TAG = "UsageStatsTest";

    private static final String APPOPS_SET_SHELL_COMMAND = "appops set {0} " +
            AppOpsManager.OPSTR_GET_USAGE_STATS + " {1}";

    private static final long TIMEOUT = TimeUnit.SECONDS.toMillis(5);
    private static final long MINUTE = TimeUnit.MINUTES.toMillis(1);
    private static final long DAY = TimeUnit.DAYS.toMillis(1);
    private static final long WEEK = 7 * DAY;
    private static final long MONTH = 30 * DAY;
    private static final long YEAR = 365 * DAY;
    private static final long TIME_DIFF_THRESHOLD = 200;
    private static final String CHANNEL_ID = "my_channel";


    private UiDevice mUiDevice;
    private UsageStatsManager mUsageStatsManager;
    private String mTargetPackage;

    @Before
    public void setUp() throws Exception {
        mUiDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mUsageStatsManager = (UsageStatsManager) InstrumentationRegistry.getInstrumentation()
                .getContext().getSystemService(Context.USAGE_STATS_SERVICE);
        mTargetPackage = InstrumentationRegistry.getContext().getPackageName();
        assumeTrue("App Standby not enabled on device", AppStandbyUtils.isAppStandbyEnabled());
        setAppOpsMode("allow");
    }

    private static void assertLessThan(long left, long right) {
        assertTrue("Expected " + left + " to be less than " + right, left < right);
    }

    private static void assertLessThanOrEqual(long left, long right) {
        assertTrue("Expected " + left + " to be less than " + right, left <= right);
    }

    private void setAppOpsMode(String mode) throws Exception {
        final String command = MessageFormat.format(APPOPS_SET_SHELL_COMMAND,
                InstrumentationRegistry.getContext().getPackageName(), mode);
        mUiDevice.executeShellCommand(command);
    }

    private void launchSubActivity(Class<? extends Activity> clazz) {
        final Context context = InstrumentationRegistry.getInstrumentation().getContext();
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setClassName(mTargetPackage, clazz.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
        mUiDevice.wait(Until.hasObject(By.clazz(clazz)), TIMEOUT);
    }

    private void launchSubActivities(Class<? extends Activity>[] activityClasses) {
        for (Class<? extends Activity> clazz : activityClasses) {
            launchSubActivity(clazz);
        }
    }

    @AppModeFull // No usage events access in instant apps
    @Test
    public void testOrderedActivityLaunchSequenceInEventLog() throws Exception {
        @SuppressWarnings("unchecked")
        Class<? extends Activity>[] activitySequence = new Class[] {
                Activities.ActivityOne.class,
                Activities.ActivityTwo.class,
                Activities.ActivityThree.class,
        };

        final long startTime = System.currentTimeMillis() - MINUTE;

        // Launch the series of Activities.
        launchSubActivities(activitySequence);

        final long endTime = System.currentTimeMillis();
        UsageEvents events = mUsageStatsManager.queryEvents(startTime, endTime);

        // Consume all the events.
        ArrayList<UsageEvents.Event> eventList = new ArrayList<>();
        while (events.hasNextEvent()) {
            UsageEvents.Event event = new UsageEvents.Event();
            assertTrue(events.getNextEvent(event));
            eventList.add(event);
        }

        // Find the last Activity's MOVE_TO_FOREGROUND event.
        int end = eventList.size();
        while (end > 0) {
            UsageEvents.Event event = eventList.get(end - 1);
            if (event.getClassName().equals(activitySequence[activitySequence.length - 1].getName())
                    && event.getEventType() == UsageEvents.Event.MOVE_TO_FOREGROUND) {
                break;
            }
            end--;
        }

        // We expect 2 events per Activity launched (foreground + background)
        // except for the last Activity, which was in the foreground when
        // we queried the event log.
        final int start = end - ((activitySequence.length * 2) - 1);
        assertTrue("Not enough events", start >= 0);

        final int activityCount = activitySequence.length;
        for (int i = 0; i < activityCount; i++) {
            final int index = start + (i * 2);

            // Check for foreground event.
            UsageEvents.Event event = eventList.get(index);
            assertEquals(mTargetPackage, event.getPackageName());
            assertEquals(activitySequence[i].getName(), event.getClassName());
            assertEquals(UsageEvents.Event.MOVE_TO_FOREGROUND, event.getEventType());

            // Only check for the background event if this is not the
            // last activity.
            if (i < activityCount - 1) {
                event = eventList.get(index + 1);
                assertEquals(mTargetPackage, event.getPackageName());
                assertEquals(activitySequence[i].getName(), event.getClassName());
                assertEquals(UsageEvents.Event.MOVE_TO_BACKGROUND, event.getEventType());
            }
        }
    }

    @AppModeFull // No usage events access in instant apps
    @Test
    public void testAppLaunchCount() throws Exception {
        long endTime = System.currentTimeMillis();
        long startTime = endTime - DateUtils.DAY_IN_MILLIS;
        Map<String,UsageStats> events = mUsageStatsManager.queryAndAggregateUsageStats(
                startTime, endTime);
        UsageStats stats = events.get(mTargetPackage);
        int startingCount = stats.getAppLaunchCount();
        launchSubActivity(Activities.ActivityOne.class);
        launchSubActivity(Activities.ActivityTwo.class);
        endTime = System.currentTimeMillis();
        events = mUsageStatsManager.queryAndAggregateUsageStats(
                startTime, endTime);
        stats = events.get(mTargetPackage);
        assertEquals(startingCount + 1, stats.getAppLaunchCount());
        mUiDevice.pressHome();
        launchSubActivity(Activities.ActivityOne.class);
        launchSubActivity(Activities.ActivityTwo.class);
        launchSubActivity(Activities.ActivityThree.class);
        endTime = System.currentTimeMillis();
        events = mUsageStatsManager.queryAndAggregateUsageStats(
                startTime, endTime);
        stats = events.get(mTargetPackage);
        assertEquals(startingCount + 2, stats.getAppLaunchCount());
    }

    @AppModeFull // No usage events access in instant apps
    @Test
    public void testStandbyBucketChangeLog() throws Exception {
        final long startTime = System.currentTimeMillis();
        mUiDevice.executeShellCommand("am set-standby-bucket " + mTargetPackage + " rare");

        final long endTime = System.currentTimeMillis();
        UsageEvents events = mUsageStatsManager.queryEvents(startTime - 1_000, endTime + 1_000);

        boolean found = false;
        // Check all the events.
        while (events.hasNextEvent()) {
            UsageEvents.Event event = new UsageEvents.Event();
            assertTrue(events.getNextEvent(event));
            if (event.getEventType() == UsageEvents.Event.STANDBY_BUCKET_CHANGED) {
                found |= event.getAppStandbyBucket() == UsageStatsManager.STANDBY_BUCKET_RARE;
            }
        }

        assertTrue(found);
    }

    @Test
    public void testGetAppStandbyBuckets() throws Exception {
        final boolean origValue = AppStandbyUtils.isAppStandbyEnabledAtRuntime();
        AppStandbyUtils.setAppStandbyEnabledAtRuntime(true);
        try {
            assumeTrue("Skip GetAppStandby test: app standby is disabled.",
                    AppStandbyUtils.isAppStandbyEnabled());

            mUiDevice.executeShellCommand("am set-standby-bucket " + mTargetPackage + " rare");
            Map<String, Integer> bucketMap = mUsageStatsManager.getAppStandbyBuckets();
            assertTrue("No bucket data returned", bucketMap.size() > 0);
            final int bucket = bucketMap.getOrDefault(mTargetPackage, -1);
            assertEquals("Incorrect bucket returned for " + mTargetPackage, bucket,
                    UsageStatsManager.STANDBY_BUCKET_RARE);
        } finally {
            AppStandbyUtils.setAppStandbyEnabledAtRuntime(origValue);
        }
    }

    @Test
    public void testGetAppStandbyBucket() throws Exception {
        // App should be at least active, since it's running instrumentation tests
        assertLessThanOrEqual(UsageStatsManager.STANDBY_BUCKET_ACTIVE,
                mUsageStatsManager.getAppStandbyBucket());
    }

    @Test
    public void testQueryEventsForSelf() throws Exception {
        setAppOpsMode("ignore"); // To ensure permission is not required
        // Time drifts of 2s are expected inside usage stats
        final long start = System.currentTimeMillis() - 2_000;
        mUiDevice.executeShellCommand("am set-standby-bucket " + mTargetPackage + " rare");
        Thread.sleep(100);
        mUiDevice.executeShellCommand("am set-standby-bucket " + mTargetPackage + " working_set");
        Thread.sleep(100);
        final long end = System.currentTimeMillis() + 2_000;
        final UsageEvents events = mUsageStatsManager.queryEventsForSelf(start, end);
        long rareTimeStamp = end + 1; // Initializing as rareTimeStamp > workingTimeStamp
        long workingTimeStamp = start - 1;
        int numEvents = 0;
        while (events.hasNextEvent()) {
            UsageEvents.Event event = new UsageEvents.Event();
            assertTrue(events.getNextEvent(event));
            numEvents++;
            assertEquals("Event for a different package", mTargetPackage, event.getPackageName());
            if (event.getEventType() == Event.STANDBY_BUCKET_CHANGED) {
                if (event.getAppStandbyBucket() == UsageStatsManager.STANDBY_BUCKET_RARE) {
                    rareTimeStamp = event.getTimeStamp();
                }
                else if (event.getAppStandbyBucket() == UsageStatsManager
                        .STANDBY_BUCKET_WORKING_SET) {
                    workingTimeStamp = event.getTimeStamp();
                }
            }
        }
        assertTrue("Only " + numEvents + " events returned", numEvents >= 2);
        assertLessThan(rareTimeStamp, workingTimeStamp);
    }

    /**
     * We can't run this test because we are unable to change the system time.
     * It would be nice to add a shell command or other to allow the shell user
     * to set the time, thereby allowing this test to set the time using the UIAutomator.
     */
    @Ignore
    @Test
    public void ignore_testStatsAreShiftedInTimeWhenSystemTimeChanges() throws Exception {
        launchSubActivity(Activities.ActivityOne.class);
        launchSubActivity(Activities.ActivityThree.class);

        long endTime = System.currentTimeMillis();
        long startTime = endTime - MINUTE;
        Map<String, UsageStats> statsMap = mUsageStatsManager.queryAndAggregateUsageStats(startTime,
                endTime);
        assertFalse(statsMap.isEmpty());
        assertTrue(statsMap.containsKey(mTargetPackage));
        final UsageStats before = statsMap.get(mTargetPackage);

        SystemClock.setCurrentTimeMillis(System.currentTimeMillis() - (DAY / 2));
        try {
            endTime = System.currentTimeMillis();
            startTime = endTime - MINUTE;
            statsMap = mUsageStatsManager.queryAndAggregateUsageStats(startTime, endTime);
            assertFalse(statsMap.isEmpty());
            assertTrue(statsMap.containsKey(mTargetPackage));
            final UsageStats after = statsMap.get(mTargetPackage);
            assertEquals(before.getPackageName(), after.getPackageName());

            long diff = before.getFirstTimeStamp() - after.getFirstTimeStamp();
            assertLessThan(Math.abs(diff - (DAY / 2)), TIME_DIFF_THRESHOLD);

            assertEquals(before.getLastTimeStamp() - before.getFirstTimeStamp(),
                    after.getLastTimeStamp() - after.getFirstTimeStamp());
            assertEquals(before.getLastTimeUsed() - before.getFirstTimeStamp(),
                    after.getLastTimeUsed() - after.getFirstTimeStamp());
            assertEquals(before.getTotalTimeInForeground(), after.getTotalTimeInForeground());
        } finally {
            SystemClock.setCurrentTimeMillis(System.currentTimeMillis() + (DAY / 2));
        }
    }

    @Test
    public void testUsageEventsParceling() throws Exception {
        final long startTime = System.currentTimeMillis() - MINUTE;

        // Ensure some data is in the UsageStats log.
        @SuppressWarnings("unchecked")
        Class<? extends Activity>[] activityClasses = new Class[] {
                Activities.ActivityTwo.class,
                Activities.ActivityOne.class,
                Activities.ActivityThree.class,
        };
        launchSubActivities(activityClasses);

        final long endTime = System.currentTimeMillis();
        UsageEvents events = mUsageStatsManager.queryEvents(startTime, endTime);
        assertTrue(events.getNextEvent(new UsageEvents.Event()));

        Parcel p = Parcel.obtain();
        p.setDataPosition(0);
        events.writeToParcel(p, 0);
        p.setDataPosition(0);

        UsageEvents reparceledEvents = UsageEvents.CREATOR.createFromParcel(p);

        UsageEvents.Event e1 = new UsageEvents.Event();
        UsageEvents.Event e2 = new UsageEvents.Event();
        while (events.hasNextEvent() && reparceledEvents.hasNextEvent()) {
            events.getNextEvent(e1);
            reparceledEvents.getNextEvent(e2);
            assertEquals(e1.getPackageName(), e2.getPackageName());
            assertEquals(e1.getClassName(), e2.getClassName());
            assertEquals(e1.getConfiguration(), e2.getConfiguration());
            assertEquals(e1.getEventType(), e2.getEventType());
            assertEquals(e1.getTimeStamp(), e2.getTimeStamp());
        }

        assertEquals(events.hasNextEvent(), reparceledEvents.hasNextEvent());
    }

    @AppModeFull // No usage events access in instant apps
    @Test
    public void testPackageUsageStatsIntervals() throws Exception {
        final long beforeTime = System.currentTimeMillis();

        // Launch an Activity.
        launchSubActivity(Activities.ActivityFour.class);
        launchSubActivity(Activities.ActivityThree.class);

        final long endTime = System.currentTimeMillis();

        final SparseLongArray intervalLengths = new SparseLongArray();
        intervalLengths.put(UsageStatsManager.INTERVAL_DAILY, DAY);
        intervalLengths.put(UsageStatsManager.INTERVAL_WEEKLY, WEEK);
        intervalLengths.put(UsageStatsManager.INTERVAL_MONTHLY, MONTH);
        intervalLengths.put(UsageStatsManager.INTERVAL_YEARLY, YEAR);

        final int intervalCount = intervalLengths.size();
        for (int i = 0; i < intervalCount; i++) {
            final int intervalType = intervalLengths.keyAt(i);
            final long intervalDuration = intervalLengths.valueAt(i);
            final long startTime = endTime - (2 * intervalDuration);
            final List<UsageStats> statsList = mUsageStatsManager.queryUsageStats(intervalType,
                    startTime, endTime);
            assertFalse(statsList.isEmpty());

            boolean foundPackage = false;
            for (UsageStats stats : statsList) {
                // Verify that each period is a day long.
                assertLessThanOrEqual(stats.getLastTimeStamp() - stats.getFirstTimeStamp(),
                        intervalDuration);
                if (stats.getPackageName().equals(mTargetPackage) &&
                        stats.getLastTimeUsed() >= beforeTime - TIME_DIFF_THRESHOLD) {
                    foundPackage = true;
                }
            }

            assertTrue("Did not find package " + mTargetPackage + " in interval " + intervalType,
                    foundPackage);
        }
    }

    @Test
    public void testNoAccessSilentlyFails() throws Exception {
        final long startTime = System.currentTimeMillis() - MINUTE;

        launchSubActivity(android.app.usage.cts.Activities.ActivityOne.class);
        launchSubActivity(android.app.usage.cts.Activities.ActivityThree.class);

        final long endTime = System.currentTimeMillis();
        List<UsageStats> stats = mUsageStatsManager.queryUsageStats(UsageStatsManager.INTERVAL_BEST,
                startTime, endTime);
        assertFalse(stats.isEmpty());

        // We set the mode to ignore because our package has the PACKAGE_USAGE_STATS permission,
        // and default would allow in this case.
        setAppOpsMode("ignore");

        stats = mUsageStatsManager.queryUsageStats(UsageStatsManager.INTERVAL_BEST,
                startTime, endTime);
        assertTrue(stats.isEmpty());
    }

    @AppModeFull // No usage events access in instant apps
    @Test
    public void testNotificationSeen() throws Exception {
        final long startTime = System.currentTimeMillis();
        Context context = InstrumentationRegistry.getContext();
        if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            // Skip the test for wearable device.
            return;
        }
        NotificationManager mNotificationManager =
            (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        int importance = NotificationManager.IMPORTANCE_DEFAULT;
        NotificationChannel mChannel = new NotificationChannel(CHANNEL_ID, "Channel",
            importance);
        // Configure the notification channel.
        mChannel.setDescription("Test channel");
        mNotificationManager.createNotificationChannel(mChannel);
        Notification.Builder mBuilder =
                new Notification.Builder(context, CHANNEL_ID)
                    .setSmallIcon(R.drawable.ic_notification)
                    .setContentTitle("My notification")
                    .setContentText("Hello World!");
        PendingIntent pi = PendingIntent.getActivity(context, 1,
                new Intent(Settings.ACTION_SETTINGS), 0);
        mBuilder.setContentIntent(pi);
        mNotificationManager.notify(1, mBuilder.build());
        Thread.sleep(500);
        long endTime = System.currentTimeMillis();
        UsageEvents events = mUsageStatsManager.queryEvents(startTime, endTime);
        boolean found = false;
        Event event = new Event();
        while (events.hasNextEvent()) {
            events.getNextEvent(event);
            if (event.getEventType() == Event.NOTIFICATION_SEEN) {
                found = true;
            }
        }
        assertFalse(found);
        // Pull down shade
        mUiDevice.openNotification();
        outer:
        for (int i = 0; i < 5; i++) {
            Thread.sleep(500);
            endTime = System.currentTimeMillis();
            events = mUsageStatsManager.queryEvents(startTime, endTime);
            found = false;
            while (events.hasNextEvent()) {
                events.getNextEvent(event);
                if (event.getEventType() == Event.NOTIFICATION_SEEN) {
                    found = true;
                    break outer;
                }
            }
        }
        assertTrue(found);
        mUiDevice.pressBack();
    }

    static final int[] INTERACTIVE_EVENTS = new int[] {
            Event.SCREEN_INTERACTIVE,
            Event.SCREEN_NON_INTERACTIVE
    };

    static final int[] KEYGUARD_EVENTS = new int[] {
            Event.KEYGUARD_SHOWN,
            Event.KEYGUARD_HIDDEN
    };

    static final int[] ALL_EVENTS = new int[] {
            Event.SCREEN_INTERACTIVE,
            Event.SCREEN_NON_INTERACTIVE,
            Event.KEYGUARD_SHOWN,
            Event.KEYGUARD_HIDDEN
    };

    private long getEvents(int[] whichEvents, long startTime, List<Event> out) {
        final long endTime = System.currentTimeMillis();
        if (DEBUG) Log.i(TAG, "Looking for events " + Arrays.toString(whichEvents)
                + " between " + startTime + " and " + endTime);
        UsageEvents events = mUsageStatsManager.queryEvents(startTime, endTime);

        long latestTime = 0;

        // Find events.
        while (events.hasNextEvent()) {
            UsageEvents.Event event = new UsageEvents.Event();
            assertTrue(events.getNextEvent(event));
            final int ev = event.getEventType();
            for (int which : whichEvents) {
                if (ev == which) {
                    if (out != null) {
                        out.add(event);
                    }
                    if (DEBUG) Log.i(TAG, "Next event type " + event.getEventType()
                            + " time=" + event.getTimeStamp());
                    if (latestTime < event.getTimeStamp()) {
                        latestTime = event.getTimeStamp();
                    }
                    break;
                }
            }
        }

        return latestTime;
    }

    private ArrayList<Event> waitForEventCount(int[] whichEvents, long startTime, int count) {
        final ArrayList<Event> events = new ArrayList<>();
        final long endTime = SystemClock.uptimeMillis() + 2000;
        do {
            events.clear();
            getEvents(whichEvents, startTime, events);
            if (events.size() == count) {
                return events;
            }
            if (events.size() > count) {
                fail("Found too many events: got " + events.size() + ", expected " + count);
                return events;
            }
            SystemClock.sleep(10);
        } while (SystemClock.uptimeMillis() < endTime);

        fail("Timed out waiting for " + count + " events, only reached " + events.size());
        return events;
    }

    static class AggrEventData {
        final String label;
        int count;
        long duration;
        long lastEventTime;

        AggrEventData(String label) {
            this.label = label;
        }
    }

    static class AggrAllEventsData {
        final AggrEventData interactive = new AggrEventData("Interactive");
        final AggrEventData nonInteractive = new AggrEventData("Non-interactive");
        final AggrEventData keyguardShown = new AggrEventData("Keyguard shown");
        final AggrEventData keyguardHidden = new AggrEventData("Keyguard hidden");
        int interactiveCount;
        long interactiveDuration;
        long interactiveLastEventTime;
        int nonInteractiveCount;
        long nonInteractiveDuration;
        long nonInteractiveLastEventTime;
        int keyguardShownCount;
        long keyguardShownDuration;
        int keyguardHiddenCount;
        long keyguardHiddenDuration;
    }

    private SparseArray<AggrAllEventsData> getAggrEventData(long beforeTime) {
        final long endTime = System.currentTimeMillis();

        final SparseLongArray intervalLengths = new SparseLongArray();
        intervalLengths.put(UsageStatsManager.INTERVAL_DAILY, DAY);
        intervalLengths.put(UsageStatsManager.INTERVAL_WEEKLY, WEEK);
        intervalLengths.put(UsageStatsManager.INTERVAL_MONTHLY, MONTH);
        intervalLengths.put(UsageStatsManager.INTERVAL_YEARLY, YEAR);

        final SparseArray<AggrAllEventsData> allAggr = new SparseArray<>();

        final int intervalCount = intervalLengths.size();
        for (int i = 0; i < intervalCount; i++) {
            final int intervalType = intervalLengths.keyAt(i);
            final long intervalDuration = intervalLengths.valueAt(i);
            final long startTime = endTime - (2 * intervalDuration);
            List<EventStats> statsList = mUsageStatsManager.queryEventStats(intervalType,
                    startTime, endTime);
            assertFalse(statsList.isEmpty());

            final AggrAllEventsData aggr = new AggrAllEventsData();
            allAggr.put(intervalType, aggr);

            boolean foundEvent = false;
            for (EventStats stats : statsList) {
                // Verify that each period is a day long.
                //assertLessThanOrEqual(stats.getLastTimeStamp() - stats.getFirstTimeStamp(),
                //        intervalDuration);
                AggrEventData data = null;
                switch (stats.getEventType()) {
                    case Event.SCREEN_INTERACTIVE:
                        data = aggr.interactive;
                        break;
                    case Event.SCREEN_NON_INTERACTIVE:
                        data = aggr.nonInteractive;
                        break;
                    case Event.KEYGUARD_HIDDEN:
                        data = aggr.keyguardHidden;
                        break;
                    case Event.KEYGUARD_SHOWN:
                        data = aggr.keyguardShown;
                        break;
                }
                if (data != null) {
                    foundEvent = true;
                    data.count += stats.getCount();
                    data.duration += stats.getTotalTime();
                    if (data.lastEventTime < stats.getLastEventTime()) {
                        data.lastEventTime = stats.getLastEventTime();
                    }
                }
            }

            assertTrue("Did not find event data in interval " + intervalType,
                    foundEvent);
        }

        return allAggr;
    }

    private void verifyCount(int oldCount, int newCount, boolean larger, String label,
            int interval) {
        if (larger) {
            if (newCount <= oldCount) {
                fail(label + " count newer " + newCount
                        + " expected to be larger than older " + oldCount
                        + " @ interval " + interval);
            }
        } else {
            if (newCount != oldCount) {
                fail(label + " count newer " + newCount
                        + " expected to be same as older " + oldCount
                        + " @ interval " + interval);
            }
        }
    }

    private void verifyDuration(long oldDur, long newDur, boolean larger, String label,
            int interval) {
        if (larger) {
            if (newDur <= oldDur) {
                fail(label + " duration newer " + newDur
                        + " expected to be larger than older " + oldDur
                        + " @ interval " + interval);
            }
        } else {
            if (newDur != oldDur) {
                fail(label + " duration newer " + newDur
                        + " expected to be same as older " + oldDur
                        + " @ interval " + interval);
            }
        }
    }

    private void verifyAggrEventData(AggrEventData older, AggrEventData newer,
            boolean countLarger, boolean durationLarger, int interval) {
        verifyCount(older.count, newer.count, countLarger, older.label, interval);
        verifyDuration(older.duration, newer.duration, durationLarger, older.label, interval);
    }

    private void verifyAggrInteractiveEventData(SparseArray<AggrAllEventsData> older,
            SparseArray<AggrAllEventsData> newer, boolean interactiveLarger,
            boolean nonInteractiveLarger) {
        for (int i = 0; i < older.size(); i++) {
            AggrAllEventsData o = older.valueAt(i);
            AggrAllEventsData n = newer.valueAt(i);
            // When we are told something is larger, that means we have transitioned
            // *out* of that state -- so the duration of that state is expected to
            // increase, but the count should stay the same (and the count of the state
            // we transition to is increased).
            final int interval = older.keyAt(i);
            verifyAggrEventData(o.interactive, n.interactive, nonInteractiveLarger,
                    interactiveLarger, interval);
            verifyAggrEventData(o.nonInteractive, n.nonInteractive, interactiveLarger,
                    nonInteractiveLarger, interval);
        }
    }

    private void verifyAggrKeyguardEventData(SparseArray<AggrAllEventsData> older,
            SparseArray<AggrAllEventsData> newer, boolean hiddenLarger,
            boolean shownLarger) {
        for (int i = 0; i < older.size(); i++) {
            AggrAllEventsData o = older.valueAt(i);
            AggrAllEventsData n = newer.valueAt(i);
            // When we are told something is larger, that means we have transitioned
            // *out* of that state -- so the duration of that state is expected to
            // increase, but the count should stay the same (and the count of the state
            // we transition to is increased).
            final int interval = older.keyAt(i);
            verifyAggrEventData(o.keyguardHidden, n.keyguardHidden, shownLarger,
                    hiddenLarger, interval);
            verifyAggrEventData(o.keyguardShown, n.keyguardShown, hiddenLarger,
                    shownLarger, interval);
        }
    }

    @AppModeFull // No usage events access in instant apps
    @Test
    public void testInteractiveEvents() throws Exception {
        final KeyguardManager kmgr = InstrumentationRegistry.getInstrumentation()
                .getContext().getSystemService(KeyguardManager.class);

        // We need to start out with the screen on.
        if (!mUiDevice.isScreenOn()) {
            pressWakeUp();
            SystemClock.sleep(1000);
        }

        // Also want to start out with the keyguard dismissed.
        if (kmgr.isKeyguardLocked()) {
            final long startTime = getEvents(KEYGUARD_EVENTS, 0, null) + 1;
            mUiDevice.executeShellCommand("wm dismiss-keyguard");
            ArrayList<Event> events = waitForEventCount(KEYGUARD_EVENTS, startTime, 1);
            assertEquals(Event.KEYGUARD_HIDDEN, events.get(0).getEventType());
            SystemClock.sleep(500);
        }

        try {
            ArrayList<Event> events;

            // Determine time to start looking for events.
            final long startTime = getEvents(ALL_EVENTS, 0, null) + 1;
            SparseArray<AggrAllEventsData> baseAggr = getAggrEventData(0);

            // First test -- put device to sleep and make sure we see this event.
            pressSleep();

            // Do we have one event, going in to non-interactive mode?
            events = waitForEventCount(INTERACTIVE_EVENTS, startTime, 1);
            assertEquals(Event.SCREEN_NON_INTERACTIVE, events.get(0).getEventType());
            SparseArray<AggrAllEventsData> offAggr = getAggrEventData(startTime);
            verifyAggrInteractiveEventData(baseAggr, offAggr, true, false);

            // Next test -- turn screen on and make sure we have a second event.
            // XXX need to wait a bit so we don't accidentally trigger double-power
            // to launch camera.  (SHOULD FIX HOW WE WAKEUP / SLEEP TO NOT USE POWER KEY)
            SystemClock.sleep(500);
            pressWakeUp();
            events = waitForEventCount(INTERACTIVE_EVENTS, startTime, 2);
            assertEquals(Event.SCREEN_NON_INTERACTIVE, events.get(0).getEventType());
            assertEquals(Event.SCREEN_INTERACTIVE, events.get(1).getEventType());
            SparseArray<AggrAllEventsData> onAggr = getAggrEventData(startTime);
            verifyAggrInteractiveEventData(offAggr, onAggr, false, true);

            // If the device is doing a lock screen, verify that we are also seeing the
            // appropriate keyguard behavior.  We don't know the timing from when the screen
            // will go off until the keyguard is shown, so we will do this all after turning
            // the screen back on (at which point it must be shown).
            // XXX CTS seems to be preventing the keyguard from showing, so this path is
            // never being tested.
            if (kmgr.isKeyguardLocked()) {
                events = waitForEventCount(KEYGUARD_EVENTS, startTime, 1);
                assertEquals(Event.KEYGUARD_SHOWN, events.get(0).getEventType());
                SparseArray<AggrAllEventsData> shownAggr = getAggrEventData(startTime);
                verifyAggrKeyguardEventData(offAggr, shownAggr, true, false);

                // Now dismiss the keyguard and verify the resulting events.
                mUiDevice.executeShellCommand("wm dismiss-keyguard");
                events = waitForEventCount(KEYGUARD_EVENTS, startTime, 2);
                assertEquals(Event.KEYGUARD_SHOWN, events.get(0).getEventType());
                assertEquals(Event.KEYGUARD_HIDDEN, events.get(1).getEventType());
                SparseArray<AggrAllEventsData> hiddenAggr = getAggrEventData(startTime);
                verifyAggrKeyguardEventData(shownAggr, hiddenAggr, false, true);
            }

        } finally {
            // Dismiss keyguard to get device back in its normal state.
            pressWakeUp();
            mUiDevice.executeShellCommand("wm dismiss-keyguard");
        }
    }

    @Test
    public void testIgnoreNonexistentPackage() throws Exception {
        final String fakePackageName = "android.fake.package.name";
        final int defaultValue = -1;

        mUiDevice.executeShellCommand("am set-standby-bucket " + fakePackageName + " rare");
        // Verify the above does not add a new entry to the App Standby bucket map
        Map<String, Integer> bucketMap = mUsageStatsManager.getAppStandbyBuckets();
        int bucket = bucketMap.getOrDefault(fakePackageName, defaultValue);
        assertFalse("Meaningful bucket value " + bucket + " returned for " + fakePackageName
                + " after set-standby-bucket", bucket > 0);

        mUiDevice.executeShellCommand("am get-standby-bucket " + fakePackageName);
        // Verify the above does not add a new entry to the App Standby bucket map
        bucketMap = mUsageStatsManager.getAppStandbyBuckets();
        bucket = bucketMap.getOrDefault(fakePackageName, defaultValue);
        assertFalse("Meaningful bucket value " + bucket + " returned for " + fakePackageName
                + " after get-standby-bucket", bucket > 0);
    }

    @Test
    public void testObserveUsagePermission() {
        try {
            mUsageStatsManager.registerAppUsageObserver(0, new String[] {"com.android.settings"},
                    1, java.util.concurrent.TimeUnit.HOURS, null);
        } catch (SecurityException e) {
            return;
        }
        fail("Should throw SecurityException");
    }

    private void pressWakeUp() {
        mUiDevice.pressKeyCode(KeyEvent.KEYCODE_WAKEUP);
    }

    private void pressSleep() {
        mUiDevice.pressKeyCode(KeyEvent.KEYCODE_SLEEP);
    }
}
