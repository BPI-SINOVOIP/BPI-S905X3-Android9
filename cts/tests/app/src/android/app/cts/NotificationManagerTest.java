/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.app.cts;

import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_AMBIENT;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_FULL_SCREEN_INTENT;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_LIGHTS;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_NOTIFICATION_LIST;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_PEEK;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_SCREEN_OFF;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_SCREEN_ON;
import static android.app.NotificationManager.Policy.SUPPRESSED_EFFECT_STATUS_BAR;
import static android.content.pm.PackageManager.FEATURE_WATCH;

import android.app.ActivityManager;
import android.app.Instrumentation;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationChannelGroup;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.UiAutomation;
import android.app.stubs.MockNotificationListener;
import android.app.stubs.R;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.drawable.Icon;
import android.media.AudioAttributes;
import android.media.session.MediaSession;
import android.net.Uri;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.provider.Settings;
import android.provider.Telephony.Threads;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.support.test.InstrumentationRegistry;
import android.test.AndroidTestCase;
import android.util.Log;

import junit.framework.Assert;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class NotificationManagerTest extends AndroidTestCase {
    final String TAG = NotificationManagerTest.class.getSimpleName();
    final boolean DEBUG = false;
    final String NOTIFICATION_CHANNEL_ID = "NotificationManagerTest";

    private PackageManager mPackageManager;
    private NotificationManager mNotificationManager;
    private ActivityManager mActivityManager;
    private String mId;
    private MockNotificationListener mListener;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // This will leave a set of channels on the device with each test run.
        mId = UUID.randomUUID().toString();
        mNotificationManager = (NotificationManager) mContext.getSystemService(
                Context.NOTIFICATION_SERVICE);
        // clear the deck so that our getActiveNotifications results are predictable
        mNotificationManager.cancelAll();
        mNotificationManager.createNotificationChannel(new NotificationChannel(
                NOTIFICATION_CHANNEL_ID, "name", NotificationManager.IMPORTANCE_DEFAULT));
        mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        mPackageManager = mContext.getPackageManager();
        // delay between tests so notifications aren't dropped by the rate limiter
        try {
            Thread.sleep(500);
        } catch(InterruptedException e) {}
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mNotificationManager.cancelAll();
        List<NotificationChannel> channels = mNotificationManager.getNotificationChannels();
        // Delete all channels.
        for (NotificationChannel nc : channels) {
            if (NotificationChannel.DEFAULT_CHANNEL_ID.equals(nc.getId())) {
                continue;
            }
            mNotificationManager.deleteNotificationChannel(nc.getId());
        }

        toggleListenerAccess(MockNotificationListener.getId(),
                InstrumentationRegistry.getInstrumentation(), false);

        List<NotificationChannelGroup> groups = mNotificationManager.getNotificationChannelGroups();
        // Delete all groups.
        for (NotificationChannelGroup ncg : groups) {
            mNotificationManager.deleteNotificationChannelGroup(ncg.getId());
        }
    }

    public void testPostPCanToggleAlarmsMediaSystemTest() throws Exception {
        if (mActivityManager.isLowRamDevice()) {
            return;
        }

        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), true);

        if (mContext.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.P) {
            // Post-P can toggle alarms, media, system
            // toggle on alarms, media, system:
            mNotificationManager.setNotificationPolicy(new NotificationManager.Policy(
                    NotificationManager.Policy.PRIORITY_CATEGORY_ALARMS
                            | NotificationManager.Policy.PRIORITY_CATEGORY_MEDIA
                            | NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM, 0, 0));
            NotificationManager.Policy policy = mNotificationManager.getNotificationPolicy();
            assertTrue((policy.priorityCategories
                    & NotificationManager.Policy.PRIORITY_CATEGORY_ALARMS) != 0);
            assertTrue((policy.priorityCategories
                    & NotificationManager.Policy.PRIORITY_CATEGORY_MEDIA) != 0);
            assertTrue((policy.priorityCategories
                    & NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM) != 0);

            // toggle off alarms, media, system
            mNotificationManager.setNotificationPolicy(new NotificationManager.Policy(0, 0, 0));
            policy = mNotificationManager.getNotificationPolicy();
            assertTrue((policy.priorityCategories
                    & NotificationManager.Policy.PRIORITY_CATEGORY_ALARMS) == 0);
            assertTrue((policy.priorityCategories &
                    NotificationManager.Policy.PRIORITY_CATEGORY_MEDIA) == 0);
            assertTrue((policy.priorityCategories &
                    NotificationManager.Policy.PRIORITY_CATEGORY_SYSTEM) == 0);
        }

        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), false);
    }

    public void testCreateChannelGroup() throws Exception {
        final NotificationChannelGroup ncg = new NotificationChannelGroup("a group", "a label");
        final NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup(ncg.getId());
        mNotificationManager.createNotificationChannelGroup(ncg);
        final NotificationChannel ungrouped =
                new NotificationChannel(mId + "!", "name", NotificationManager.IMPORTANCE_DEFAULT);
        try {
            mNotificationManager.createNotificationChannel(channel);
            mNotificationManager.createNotificationChannel(ungrouped);

            List<NotificationChannelGroup> ncgs =
                    mNotificationManager.getNotificationChannelGroups();
            assertEquals(1, ncgs.size());
            assertEquals(ncg.getName(), ncgs.get(0).getName());
            assertEquals(ncg.getDescription(), ncgs.get(0).getDescription());
            assertEquals(channel.getId(), ncgs.get(0).getChannels().get(0).getId());
        } finally {
            mNotificationManager.deleteNotificationChannelGroup(ncg.getId());
        }
    }

    public void testGetChannelGroup() throws Exception {
        final NotificationChannelGroup ncg = new NotificationChannelGroup("a group", "a label");
        ncg.setDescription("bananas");
        final NotificationChannelGroup ncg2 = new NotificationChannelGroup("group 2", "label 2");
        final NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup(ncg.getId());

        mNotificationManager.createNotificationChannelGroup(ncg);
        mNotificationManager.createNotificationChannelGroup(ncg2);
        mNotificationManager.createNotificationChannel(channel);

        NotificationChannelGroup actual =
                mNotificationManager.getNotificationChannelGroup(ncg.getId());
        assertEquals(ncg.getId(), actual.getId());
        assertEquals(ncg.getName(), actual.getName());
        assertEquals(ncg.getDescription(), actual.getDescription());
        assertEquals(channel.getId(), actual.getChannels().get(0).getId());
    }

    public void testDeleteChannelGroup() throws Exception {
        final NotificationChannelGroup ncg = new NotificationChannelGroup("a group", "a label");
        final NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup(ncg.getId());
        mNotificationManager.createNotificationChannelGroup(ncg);
        mNotificationManager.createNotificationChannel(channel);

        mNotificationManager.deleteNotificationChannelGroup(ncg.getId());

        assertNull(mNotificationManager.getNotificationChannel(channel.getId()));
        assertEquals(0, mNotificationManager.getNotificationChannelGroups().size());
    }

    public void testCreateChannel() throws Exception {
        final NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setDescription("bananas");
        channel.enableVibration(true);
        channel.setVibrationPattern(new long[] {5, 8, 2, 1});
        channel.setSound(new Uri.Builder().scheme("test").build(),
                new AudioAttributes.Builder().setUsage(
                        AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_DELAYED).build());
        channel.enableLights(true);
        channel.setBypassDnd(true);
        channel.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
        mNotificationManager.createNotificationChannel(channel);
        final NotificationChannel createdChannel =
                mNotificationManager.getNotificationChannel(mId);
        compareChannels(channel, createdChannel);
        // Lockscreen Visibility and canBypassDnd no longer settable.
        assertTrue(createdChannel.getLockscreenVisibility() != Notification.VISIBILITY_SECRET);
        assertFalse(createdChannel.canBypassDnd());
    }

    public void testCreateChannel_rename() throws Exception {
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        mNotificationManager.createNotificationChannel(channel);
        channel.setName("new name");
        mNotificationManager.createNotificationChannel(channel);
        final NotificationChannel createdChannel =
                mNotificationManager.getNotificationChannel(mId);
        compareChannels(channel, createdChannel);

        channel.setImportance(NotificationManager.IMPORTANCE_HIGH);
        mNotificationManager.createNotificationChannel(channel);
        assertEquals(NotificationManager.IMPORTANCE_DEFAULT,
                mNotificationManager.getNotificationChannel(mId).getImportance());
    }

    public void testCreateChannel_addToGroup() throws Exception {
        String oldGroup = null;
        String newGroup = "new group";
        mNotificationManager.createNotificationChannelGroup(
                new NotificationChannelGroup(newGroup, newGroup));

        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup(oldGroup);
        mNotificationManager.createNotificationChannel(channel);

        channel.setGroup(newGroup);
        mNotificationManager.createNotificationChannel(channel);

        final NotificationChannel updatedChannel =
                mNotificationManager.getNotificationChannel(mId);
        assertEquals("Failed to add non-grouped channel to a group on update ",
                newGroup, updatedChannel.getGroup());
    }

    public void testCreateChannel_cannotChangeGroup() throws Exception {
        String oldGroup = "old group";
        String newGroup = "new group";
        mNotificationManager.createNotificationChannelGroup(
                new NotificationChannelGroup(oldGroup, oldGroup));
        mNotificationManager.createNotificationChannelGroup(
                new NotificationChannelGroup(newGroup, newGroup));

        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup(oldGroup);
        mNotificationManager.createNotificationChannel(channel);
        channel.setGroup(newGroup);
        mNotificationManager.createNotificationChannel(channel);
        final NotificationChannel updatedChannel =
                mNotificationManager.getNotificationChannel(mId);
        assertEquals("Channels should not be allowed to change groups",
                oldGroup, updatedChannel.getGroup());
    }

    public void testCreateSameChannelDoesNotUpdate() throws Exception {
        final NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        mNotificationManager.createNotificationChannel(channel);
        final NotificationChannel channelDupe =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_HIGH);
        mNotificationManager.createNotificationChannel(channelDupe);
        final NotificationChannel createdChannel =
                mNotificationManager.getNotificationChannel(mId);
        compareChannels(channel, createdChannel);
    }

    public void testCreateChannelAlreadyExistsNoOp() throws Exception {
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        mNotificationManager.createNotificationChannel(channel);
        NotificationChannel channelDupe =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_HIGH);
        mNotificationManager.createNotificationChannel(channelDupe);
        compareChannels(channel, mNotificationManager.getNotificationChannel(channel.getId()));
    }

    public void testCreateChannelWithGroup() throws Exception {
        NotificationChannelGroup ncg = new NotificationChannelGroup("g", "n");
        mNotificationManager.createNotificationChannelGroup(ncg);
        try {
            NotificationChannel channel =
                    new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
            channel.setGroup(ncg.getId());
            mNotificationManager.createNotificationChannel(channel);
            compareChannels(channel, mNotificationManager.getNotificationChannel(channel.getId()));
        } finally {
            mNotificationManager.deleteNotificationChannelGroup(ncg.getId());
        }
    }

    public void testCreateChannelWithBadGroup() throws Exception {
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup("garbage");
        try {
            mNotificationManager.createNotificationChannel(channel);
            fail("Created notification with bad group");
        } catch (IllegalArgumentException e) {}
    }

    public void testCreateChannelInvalidImportance() throws Exception {
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_UNSPECIFIED);
        try {
            mNotificationManager.createNotificationChannel(channel);
        } catch (IllegalArgumentException e) {
            //success
        }
    }

    public void testDeleteChannel() throws Exception {
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_LOW);
        mNotificationManager.createNotificationChannel(channel);
        compareChannels(channel, mNotificationManager.getNotificationChannel(channel.getId()));
        mNotificationManager.deleteNotificationChannel(channel.getId());
        assertNull(mNotificationManager.getNotificationChannel(channel.getId()));
    }

    public void testCannotDeleteDefaultChannel() throws Exception {
        try {
            mNotificationManager.deleteNotificationChannel(NotificationChannel.DEFAULT_CHANNEL_ID);
            fail("Deleted default channel");
        } catch (IllegalArgumentException e) {
            //success
        }
    }

    public void testGetChannel() throws Exception {
        NotificationChannel channel1 =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        NotificationChannel channel2 =
                new NotificationChannel(
                        UUID.randomUUID().toString(), "name2", NotificationManager.IMPORTANCE_HIGH);
        NotificationChannel channel3 =
                new NotificationChannel(
                        UUID.randomUUID().toString(), "name3", NotificationManager.IMPORTANCE_LOW);
        NotificationChannel channel4 =
                new NotificationChannel(
                        UUID.randomUUID().toString(), "name4", NotificationManager.IMPORTANCE_MIN);
        mNotificationManager.createNotificationChannel(channel1);
        mNotificationManager.createNotificationChannel(channel2);
        mNotificationManager.createNotificationChannel(channel3);
        mNotificationManager.createNotificationChannel(channel4);

        compareChannels(channel2,
                mNotificationManager.getNotificationChannel(channel2.getId()));
        compareChannels(channel3,
                mNotificationManager.getNotificationChannel(channel3.getId()));
        compareChannels(channel1,
                mNotificationManager.getNotificationChannel(channel1.getId()));
        compareChannels(channel4,
                mNotificationManager.getNotificationChannel(channel4.getId()));
    }

    public void testGetChannels() throws Exception {
        NotificationChannel channel1 =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        NotificationChannel channel2 =
                new NotificationChannel(
                        UUID.randomUUID().toString(), "name2", NotificationManager.IMPORTANCE_HIGH);
        NotificationChannel channel3 =
                new NotificationChannel(
                        UUID.randomUUID().toString(), "name3", NotificationManager.IMPORTANCE_LOW);
        NotificationChannel channel4 =
                new NotificationChannel(
                        UUID.randomUUID().toString(), "name4", NotificationManager.IMPORTANCE_MIN);

        Map<String, NotificationChannel> channelMap = new HashMap<>();
        channelMap.put(channel1.getId(), channel1);
        channelMap.put(channel2.getId(), channel2);
        channelMap.put(channel3.getId(), channel3);
        channelMap.put(channel4.getId(), channel4);
        mNotificationManager.createNotificationChannel(channel1);
        mNotificationManager.createNotificationChannel(channel2);
        mNotificationManager.createNotificationChannel(channel3);
        mNotificationManager.createNotificationChannel(channel4);

        mNotificationManager.deleteNotificationChannel(channel3.getId());

        List<NotificationChannel> channels = mNotificationManager.getNotificationChannels();
        for (NotificationChannel nc : channels) {
            if (NotificationChannel.DEFAULT_CHANNEL_ID.equals(nc.getId())) {
                continue;
            }
            if (NOTIFICATION_CHANNEL_ID.equals(nc.getId())) {
                continue;
            }
            assertFalse(channel3.getId().equals(nc.getId()));
            if (!channelMap.containsKey(nc.getId())) {
                // failed cleanup from prior test run; ignore
                continue;
            }
            compareChannels(channelMap.get(nc.getId()), nc);
        }
    }

    public void testRecreateDeletedChannel() throws Exception {
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setShowBadge(true);
        NotificationChannel newChannel = new NotificationChannel(
                channel.getId(), channel.getName(), NotificationManager.IMPORTANCE_HIGH);
        mNotificationManager.createNotificationChannel(channel);
        mNotificationManager.deleteNotificationChannel(channel.getId());

        mNotificationManager.createNotificationChannel(newChannel);

        compareChannels(channel,
                mNotificationManager.getNotificationChannel(newChannel.getId()));
    }

    public void testNotify() throws Exception {
        mNotificationManager.cancelAll();

        final int id = 1;
        sendNotification(id, R.drawable.black);
        // test updating the same notification
        sendNotification(id, R.drawable.blue);
        sendNotification(id, R.drawable.yellow);

        // assume that sendNotification tested to make sure individual notifications were present
        StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
        for (StatusBarNotification sbn : sbns) {
            if (sbn.getId() != id) {
                fail("we got back other notifications besides the one we posted: "
                        + sbn.getKey());
            }
        }
    }

    public void testSuspendPackage() throws Exception {
        if (mActivityManager.isLowRamDevice() && !mPackageManager.hasSystemFeature(FEATURE_WATCH)) {
            return;
        }

        toggleListenerAccess(MockNotificationListener.getId(),
                InstrumentationRegistry.getInstrumentation(), true);
        Thread.sleep(500); // wait for listener to be allowed

        mListener = MockNotificationListener.getInstance();
        assertNotNull(mListener);

        sendNotification(1, R.drawable.black);
        Thread.sleep(500); // wait for notification listener to receive notification
        assertEquals(1, mListener.mPosted.size());

        // suspend package, ranking should be updated with suspended = true
        suspendPackage(mContext.getPackageName(), InstrumentationRegistry.getInstrumentation(),
                true);
        Thread.sleep(500); // wait for notification listener to get response
        NotificationListenerService.RankingMap rankingMap = mListener.mRankingMap;
        NotificationListenerService.Ranking outRanking = new NotificationListenerService.Ranking();
        for (String key : rankingMap.getOrderedKeys()) {
            if (key.contains(mListener.getPackageName())) {
                rankingMap.getRanking(key, outRanking);
                Log.d(TAG, "key=" + key + " suspended=" + outRanking.isSuspended());
                assertTrue(outRanking.isSuspended());
            }
        }

        // unsuspend package, ranking should be updated with suspended = false
        suspendPackage(mContext.getPackageName(), InstrumentationRegistry.getInstrumentation(),
                false);
        Thread.sleep(500); // wait for notification listener to get response
        rankingMap = mListener.mRankingMap;
        for (String key : rankingMap.getOrderedKeys()) {
            if (key.contains(mListener.getPackageName())) {
                rankingMap.getRanking(key, outRanking);
                Log.d(TAG, "key=" + key + " suspended=" + outRanking.isSuspended());
                assertFalse(outRanking.isSuspended());
            }
        }

        mListener.resetData();
    }

    public void testNotify_blockedChannel() throws Exception {
        mNotificationManager.cancelAll();

        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_NONE);
        mNotificationManager.createNotificationChannel(channel);

        int id = 1;
        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ false)) {
            fail("found unexpected notification id=" + id);
        }
    }

    public void testNotify_blockedChannelGroup() throws Exception {
        mNotificationManager.cancelAll();

        NotificationChannelGroup group = new NotificationChannelGroup(mId, "group name");
        group.setBlocked(true);
        mNotificationManager.createNotificationChannelGroup(group);
        NotificationChannel channel =
                new NotificationChannel(mId, "name", NotificationManager.IMPORTANCE_DEFAULT);
        channel.setGroup(mId);
        mNotificationManager.createNotificationChannel(channel);

        int id = 1;
        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ false)) {
            fail("found unexpected notification id=" + id);
        }
    }

    public void testCancel() throws Exception {
        final int id = 9;
        sendNotification(id, R.drawable.black);
        mNotificationManager.cancel(id);

        if (!checkNotificationExistence(id, /*shouldExist=*/ false)) {
            fail("canceled notification was still alive, id=" + id);
        }
    }

    public void testCancelAll() throws Exception {
        sendNotification(1, R.drawable.black);
        sendNotification(2, R.drawable.blue);
        sendNotification(3, R.drawable.yellow);

        if (DEBUG) {
            Log.d(TAG, "posted 3 notifications, here they are: ");
            StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
            for (StatusBarNotification sbn : sbns) {
                Log.d(TAG, "  " + sbn);
            }
            Log.d(TAG, "about to cancel...");
        }
        mNotificationManager.cancelAll();

        for (int id = 1; id <= 3; id++) {
            if (!checkNotificationExistence(id, /*shouldExist=*/ false)) {
                fail("Failed to cancel notification id=" + id);
            }
        }

    }

    public void testNotifyWithTimeout() throws Exception {
        mNotificationManager.cancelAll();
        final int id = 128;
        final long timeout = 1000;

        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .setTimeoutAfter(timeout)
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ true)) {
            fail("couldn't find posted notification id=" + id);
        }

        try {
            Thread.sleep(timeout);
        } catch (InterruptedException ex) {
            // pass
        }
        checkNotificationExistence(id, false);
    }

    public void testMediaStyle() throws Exception {
        mNotificationManager.cancelAll();
        final int id = 99;
        MediaSession session = new MediaSession(getContext(), "media");

        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_black),
                                "play", getPendingIntent()).build())
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_blue),
                                "pause", getPendingIntent()).build())
                        .setStyle(new Notification.MediaStyle()
                                .setShowActionsInCompactView(0, 1)
                                .setMediaSession(session.getSessionToken()))
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ true)) {
            fail("couldn't find posted notification id=" + id);
        }
    }

    public void testInboxStyle() throws Exception {
        final int id = 100;

        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_black),
                                "a1", getPendingIntent()).build())
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_blue),
                                "a2", getPendingIntent()).build())
                        .setStyle(new Notification.InboxStyle().addLine("line")
                                .setSummaryText("summary"))
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ true)) {
            fail("couldn't find posted notification id=" + id);
        }
    }

    public void testBigTextStyle() throws Exception {
        final int id = 101;

        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_black),
                                "a1", getPendingIntent()).build())
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_blue),
                                "a2", getPendingIntent()).build())
                        .setStyle(new Notification.BigTextStyle()
                                .setBigContentTitle("big title")
                                .bigText("big text")
                                .setSummaryText("summary"))
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ true)) {
            fail("couldn't find posted notification id=" + id);
        }
    }

    public void testBigPictureStyle() throws Exception {
        final int id = 102;

        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(R.drawable.black)
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_black),
                                "a1", getPendingIntent()).build())
                        .addAction(new Notification.Action.Builder(
                                Icon.createWithResource(getContext(), R.drawable.icon_blue),
                                "a2", getPendingIntent()).build())
                        .setStyle(new Notification.BigPictureStyle()
                        .setBigContentTitle("title")
                        .bigPicture(Bitmap.createBitmap(100, 100, Bitmap.Config.RGB_565))
                        .bigLargeIcon(Icon.createWithResource(getContext(), R.drawable.icon_blue))
                        .setSummaryText("summary"))
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ true)) {
            fail("couldn't find posted notification id=" + id);
        }
    }

    public void testAutogrouping() throws Exception {
        sendNotification(1, R.drawable.black);
        sendNotification(2, R.drawable.blue);
        sendNotification(3, R.drawable.yellow);
        sendNotification(4, R.drawable.yellow);

        assertNotificationCount(5);
        assertAllPostedNotificationsAutogrouped();
    }

    public void testAutogrouping_autogroupStaysUntilAllNotificationsCanceled() throws Exception {
        sendNotification(1, R.drawable.black);
        sendNotification(2, R.drawable.blue);
        sendNotification(3, R.drawable.yellow);
        sendNotification(4, R.drawable.yellow);

        assertNotificationCount(5);
        assertAllPostedNotificationsAutogrouped();

        // Assert all notis stay in the same autogroup until all children are canceled
        for (int i = 4; i > 1; i--) {
            cancelAndPoll(i);
            assertNotificationCount(i);
            assertAllPostedNotificationsAutogrouped();
        }
        cancelAndPoll(1);
        assertNotificationCount(0);
    }

    public void testAutogrouping_autogroupStaysUntilAllNotificationsAddedToGroup()
            throws Exception {
        String newGroup = "new!";
        sendNotification(1, R.drawable.black);
        sendNotification(2, R.drawable.blue);
        sendNotification(3, R.drawable.yellow);
        sendNotification(4, R.drawable.yellow);

        List<Integer> postedIds = new ArrayList<>();
        postedIds.add(1);
        postedIds.add(2);
        postedIds.add(3);
        postedIds.add(4);

        assertNotificationCount(5);
        assertAllPostedNotificationsAutogrouped();

        // Assert all notis stay in the same autogroup until all children are canceled
        for (int i = 4; i > 1; i--) {
            sendNotification(i, newGroup, R.drawable.blue);
            postedIds.remove(postedIds.size() - 1);
            assertNotificationCount(5);
            assertOnlySomeNotificationsAutogrouped(postedIds);
        }
        sendNotification(1, newGroup, R.drawable.blue);
        assertNotificationCount(4); // no more autogroup summary
        postedIds.remove(0);
        assertOnlySomeNotificationsAutogrouped(postedIds);
    }

    public void testNewNotificationsAddedToAutogroup_ifOriginalNotificationsCanceled()
        throws Exception {
        String newGroup = "new!";
        sendNotification(10, R.drawable.black);
        sendNotification(20, R.drawable.blue);
        sendNotification(30, R.drawable.yellow);
        sendNotification(40, R.drawable.yellow);

        List<Integer> postedIds = new ArrayList<>();
        postedIds.add(10);
        postedIds.add(20);
        postedIds.add(30);
        postedIds.add(40);

        assertNotificationCount(5);
        assertAllPostedNotificationsAutogrouped();

        // regroup all but one of the children
        for (int i = postedIds.size() - 1; i > 0; i--) {
            try {
                Thread.sleep(200);
            } catch (InterruptedException ex) {
                // pass
            }
            int id = postedIds.remove(i);
            sendNotification(id, newGroup, R.drawable.blue);
            assertNotificationCount(5);
            assertOnlySomeNotificationsAutogrouped(postedIds);
        }

        // send a new non-grouped notification. since the autogroup summary still exists,
        // the notification should be added to it
        sendNotification(50, R.drawable.blue);
        postedIds.add(50);
        try {
            Thread.sleep(200);
        } catch (InterruptedException ex) {
            // pass
        }
        assertOnlySomeNotificationsAutogrouped(postedIds);
    }

    public void testSetNotificationPolicy_P_setOldFields() throws Exception {
        if (mActivityManager.isLowRamDevice()) {
            return;
        }
        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), true);
        if (mContext.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.P) {
            NotificationManager.Policy appPolicy = new NotificationManager.Policy(0, 0, 0,
                    SUPPRESSED_EFFECT_SCREEN_ON | SUPPRESSED_EFFECT_SCREEN_OFF);
            mNotificationManager.setNotificationPolicy(appPolicy);

            int expected = SUPPRESSED_EFFECT_SCREEN_ON | SUPPRESSED_EFFECT_SCREEN_OFF
                    | SUPPRESSED_EFFECT_PEEK | SUPPRESSED_EFFECT_AMBIENT
                    | SUPPRESSED_EFFECT_LIGHTS | SUPPRESSED_EFFECT_FULL_SCREEN_INTENT;

            assertEquals(expected,
                    mNotificationManager.getNotificationPolicy().suppressedVisualEffects);
        }
        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), false);
    }

    public void testSetNotificationPolicy_P_setNewFields() throws Exception {
        if (mActivityManager.isLowRamDevice()) {
            return;
        }
        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), true);
        if (mContext.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.P) {
            NotificationManager.Policy appPolicy = new NotificationManager.Policy(0, 0, 0,
                    SUPPRESSED_EFFECT_NOTIFICATION_LIST | SUPPRESSED_EFFECT_AMBIENT
                            | SUPPRESSED_EFFECT_LIGHTS | SUPPRESSED_EFFECT_FULL_SCREEN_INTENT);
            mNotificationManager.setNotificationPolicy(appPolicy);

            int expected = SUPPRESSED_EFFECT_NOTIFICATION_LIST | SUPPRESSED_EFFECT_SCREEN_OFF
                    | SUPPRESSED_EFFECT_AMBIENT | SUPPRESSED_EFFECT_LIGHTS
                    | SUPPRESSED_EFFECT_FULL_SCREEN_INTENT;
            assertEquals(expected,
                    mNotificationManager.getNotificationPolicy().suppressedVisualEffects);
        }
        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), false);
    }

    public void testSetNotificationPolicy_P_setOldNewFields() throws Exception {
        if (mActivityManager.isLowRamDevice()) {
            return;
        }
        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), true);
        if (mContext.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.P) {

            NotificationManager.Policy appPolicy = new NotificationManager.Policy(0, 0, 0,
                    SUPPRESSED_EFFECT_SCREEN_ON | SUPPRESSED_EFFECT_STATUS_BAR);
            mNotificationManager.setNotificationPolicy(appPolicy);

            int expected = SUPPRESSED_EFFECT_STATUS_BAR;
            assertEquals(expected,
                    mNotificationManager.getNotificationPolicy().suppressedVisualEffects);

            appPolicy = new NotificationManager.Policy(0, 0, 0,
                    SUPPRESSED_EFFECT_SCREEN_ON | SUPPRESSED_EFFECT_AMBIENT
                            | SUPPRESSED_EFFECT_LIGHTS | SUPPRESSED_EFFECT_FULL_SCREEN_INTENT);
            mNotificationManager.setNotificationPolicy(appPolicy);

            expected = SUPPRESSED_EFFECT_SCREEN_OFF | SUPPRESSED_EFFECT_AMBIENT
                    | SUPPRESSED_EFFECT_LIGHTS | SUPPRESSED_EFFECT_FULL_SCREEN_INTENT;
            assertEquals(expected,
                    mNotificationManager.getNotificationPolicy().suppressedVisualEffects);
        }
        toggleNotificationPolicyAccess(mContext.getPackageName(),
                InstrumentationRegistry.getInstrumentation(), false);
    }

    private PendingIntent getPendingIntent() {
        return PendingIntent.getActivity(
                getContext(), 0, new Intent(getContext(), this.getClass()), 0);
    }

    private boolean isGroupSummary(Notification n) {
        return n.getGroup() != null && (n.flags & Notification.FLAG_GROUP_SUMMARY) != 0;
    }

    private void assertOnlySomeNotificationsAutogrouped(List<Integer> autoGroupedIds) {
        String expectedGroupKey = null;
        StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
        for (StatusBarNotification sbn : sbns) {
            if (isGroupSummary(sbn.getNotification())
                    || autoGroupedIds.contains(sbn.getId())) {
                assertTrue(sbn.getKey() + " is unexpectedly not autogrouped",
                        sbn.getOverrideGroupKey() != null);
                if (expectedGroupKey == null) {
                    expectedGroupKey = sbn.getGroupKey();
                }
                assertEquals(expectedGroupKey, sbn.getGroupKey());
            } else {
                assertTrue(sbn.isGroup());
                assertTrue(sbn.getKey() + " is unexpectedly autogrouped,",
                        sbn.getOverrideGroupKey() == null);
                assertTrue(sbn.getKey() + " has an unusual group key",
                        sbn.getGroupKey() != expectedGroupKey);
            }
        }
    }

    private void assertAllPostedNotificationsAutogrouped() {
        String expectedGroupKey = null;
        StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
        for (StatusBarNotification sbn : sbns) {
            // all notis should be in a group determined by autogrouping
            assertTrue(sbn.getOverrideGroupKey() != null);
            if (expectedGroupKey == null) {
                expectedGroupKey = sbn.getGroupKey();
            }
            // all notis should be in the same group
            assertEquals(expectedGroupKey, sbn.getGroupKey());
        }
    }

    private void cancelAndPoll(int id) {
        mNotificationManager.cancel(id);

        if (!checkNotificationExistence(id, /*shouldExist=*/ false)) {
            fail("canceled notification was still alive, id=" + 1);
        }
    }

    private void sendNotification(final int id, final int icon) throws Exception {
        sendNotification(id, null, icon);
    }

    private void sendNotification(final int id, String groupKey, final int icon) throws Exception {
        final Intent intent = new Intent(Intent.ACTION_MAIN, Threads.CONTENT_URI);

        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP
                | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.setAction(Intent.ACTION_MAIN);

        final PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 0, intent, 0);
        final Notification notification =
                new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                        .setSmallIcon(icon)
                        .setWhen(System.currentTimeMillis())
                        .setContentTitle("notify#" + id)
                        .setContentText("This is #" + id + "notification  ")
                        .setContentIntent(pendingIntent)
                        .setGroup(groupKey)
                        .build();
        mNotificationManager.notify(id, notification);

        if (!checkNotificationExistence(id, /*shouldExist=*/ true)) {
            fail("couldn't find posted notification id=" + id);
        }
    }

    private boolean checkNotificationExistence(int id, boolean shouldExist) {
        // notification is a bit asynchronous so it may take a few ms to appear in
        // getActiveNotifications()
        // we will check for it for up to 300ms before giving up
        boolean found = false;
        for (int tries = 3; tries--> 0;) {
            // Need reset flag.
            found = false;
            final StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
            for (StatusBarNotification sbn : sbns) {
                Log.d(TAG, "Found " + sbn.getKey());
                if (sbn.getId() == id) {
                    found = true;
                    break;
                }
            }
            if (found == shouldExist) break;
            try {
                Thread.sleep(100);
            } catch (InterruptedException ex) {
                // pass
            }
        }
        return found == shouldExist;
    }

    private void assertNotificationCount(int expectedCount) {
        // notification is a bit asynchronous so it may take a few ms to appear in
        // getActiveNotifications()
        // we will check for it for up to 400ms before giving up
        int lastCount = 0;
        for (int tries = 4; tries-- > 0;) {
            final StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
            lastCount = sbns.length;
            if (expectedCount == lastCount) return;
            try {
                Thread.sleep(100);
            } catch (InterruptedException ex) {
                // pass
            }
        }
        fail("Expected " + expectedCount + " posted notifications, were " +  lastCount);
    }

    private void compareChannels(NotificationChannel expected, NotificationChannel actual) {
        if (actual == null) {
            fail("actual channel is null");
            return;
        }
        if (expected == null) {
            fail("expected channel is null");
            return;
        }
        assertEquals(expected.getId(), actual.getId());
        assertEquals(expected.getName(), actual.getName());
        assertEquals(expected.getDescription(), actual.getDescription());
        assertEquals(expected.shouldVibrate(), actual.shouldVibrate());
        assertEquals(expected.shouldShowLights(), actual.shouldShowLights());
        assertEquals(expected.getImportance(), actual.getImportance());
        if (expected.getSound() == null) {
            assertEquals(Settings.System.DEFAULT_NOTIFICATION_URI, actual.getSound());
            assertEquals(Notification.AUDIO_ATTRIBUTES_DEFAULT, actual.getAudioAttributes());
        } else {
            assertEquals(expected.getSound(), actual.getSound());
            assertEquals(expected.getAudioAttributes(), actual.getAudioAttributes());
        }
        assertTrue(Arrays.equals(expected.getVibrationPattern(), actual.getVibrationPattern()));
        assertEquals(expected.getGroup(), actual.getGroup());
    }

    private void toggleNotificationPolicyAccess(String packageName,
            Instrumentation instrumentation, boolean on) throws IOException {

        String command = " cmd notification " + (on ? "allow_dnd " : "disallow_dnd ") + packageName;

        runCommand(command, instrumentation);

        NotificationManager nm = mContext.getSystemService(NotificationManager.class);
        Assert.assertEquals("Notification Policy Access Grant is " +
                        nm.isNotificationPolicyAccessGranted() + " not " + on, on,
                nm.isNotificationPolicyAccessGranted());
    }

    private void suspendPackage(String packageName,
            Instrumentation instrumentation, boolean suspend) throws IOException {
        String command = " cmd notification " + (suspend ? "suspend_package "
                : "unsuspend_package ") + packageName;

        runCommand(command, instrumentation);
    }

    private void toggleListenerAccess(String componentName, Instrumentation instrumentation,
            boolean on) throws IOException {

        String command = " cmd notification " + (on ? "allow_listener " : "disallow_listener ")
                + componentName;

        runCommand(command, instrumentation);

        final NotificationManager nm = mContext.getSystemService(NotificationManager.class);
        final ComponentName listenerComponent = MockNotificationListener.getComponentName();
        assertTrue(listenerComponent + " has not been granted access",
                nm.isNotificationListenerAccessGranted(listenerComponent) == on);
    }

    private void runCommand(String command, Instrumentation instrumentation) throws IOException {
        UiAutomation uiAutomation = instrumentation.getUiAutomation();
        // Execute command
        try (ParcelFileDescriptor fd = uiAutomation.executeShellCommand(command)) {
            Assert.assertNotNull("Failed to execute shell command: " + command, fd);
            // Wait for the command to finish by reading until EOF
            try (InputStream in = new FileInputStream(fd.getFileDescriptor())) {
                byte[] buffer = new byte[4096];
                while (in.read(buffer) > 0) {}
            } catch (IOException e) {
                throw new IOException("Could not read stdout of command: " + command, e);
            }
        } finally {
            uiAutomation.destroy();
        }
    }
}
