/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.cts.verifier.notifications;

import static android.app.NotificationManager.IMPORTANCE_LOW;
import static android.app.NotificationManager.IMPORTANCE_NONE;
import static android.provider.Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS;
import static android.provider.Settings.EXTRA_APP_PACKAGE;
import static android.provider.Settings.EXTRA_CHANNEL_ID;

import static com.android.cts.verifier.notifications.MockListener.JSON_FLAGS;
import static com.android.cts.verifier.notifications.MockListener.JSON_ICON;
import static com.android.cts.verifier.notifications.MockListener.JSON_ID;
import static com.android.cts.verifier.notifications.MockListener.JSON_PACKAGE;
import static com.android.cts.verifier.notifications.MockListener.JSON_REASON;
import static com.android.cts.verifier.notifications.MockListener.JSON_STATS;
import static com.android.cts.verifier.notifications.MockListener.JSON_TAG;
import static com.android.cts.verifier.notifications.MockListener.JSON_WHEN;
import static com.android.cts.verifier.notifications.MockListener.REASON_LISTENER_CANCEL;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationChannelGroup;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.provider.Settings;
import android.provider.Settings.Secure;
import android.service.notification.StatusBarNotification;
import androidx.core.app.NotificationCompat;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.android.cts.verifier.R;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public class NotificationListenerVerifierActivity extends InteractiveVerifierActivity
        implements Runnable {
    private static final String TAG = "NoListenerVerifier";
    private static final String NOTIFICATION_CHANNEL_ID = TAG;
    protected static final String PREFS = "listener_prefs";
    final int NUM_NOTIFICATIONS_SENT = 3; // # notifications sent by sendNotifications()

    private String mTag1;
    private String mTag2;
    private String mTag3;
    private int mIcon1;
    private int mIcon2;
    private int mIcon3;
    private int mId1;
    private int mId2;
    private int mId3;
    private long mWhen1;
    private long mWhen2;
    private long mWhen3;
    private int mFlag1;
    private int mFlag2;
    private int mFlag3;

    @Override
    protected int getTitleResource() {
        return R.string.nls_test;
    }

    @Override
    protected int getInstructionsResource() {
        return R.string.nls_info;
    }

    // Test Setup

    @Override
    protected List<InteractiveTestCase> createTestItems() {
        List<InteractiveTestCase> tests = new ArrayList<>(17);
        ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        if (am.isLowRamDevice()) {
            tests.add(new CannotBeEnabledTest());
            tests.add(new ServiceStoppedTest());
            tests.add(new NotificationNotReceivedTest());
        } else {
            tests.add(new IsEnabledTest());
            tests.add(new ServiceStartedTest());
            tests.add(new NotificationReceivedTest());
            tests.add(new DataIntactTest());
            tests.add(new DismissOneTest());
            tests.add(new DismissOneWithReasonTest());
            tests.add(new DismissOneWithStatsTest());
            tests.add(new DismissAllTest());
            tests.add(new SnoozeNotificationForTimeTest());
            tests.add(new SnoozeNotificationForTimeCancelTest());
            tests.add(new GetSnoozedNotificationTest());
            tests.add(new EnableHintsTest());
            tests.add(new ReceiveAppBlockNoticeTest());
            tests.add(new ReceiveAppUnblockNoticeTest());
            tests.add(new ReceiveChannelBlockNoticeTest());
            tests.add(new ReceiveGroupBlockNoticeTest());
            tests.add(new RequestUnbindTest());
            tests.add(new RequestBindTest());
            tests.add(new MessageBundleTest());
            tests.add(new EnableHintsTest());
            tests.add(new IsDisabledTest());
            tests.add(new ServiceStoppedTest());
            tests.add(new NotificationNotReceivedTest());
        }
        return tests;
    }

    private void createChannel() {
        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID,
                NOTIFICATION_CHANNEL_ID, IMPORTANCE_LOW);
        mNm.createNotificationChannel(channel);
    }

    private void deleteChannel() {
        mNm.deleteNotificationChannel(NOTIFICATION_CHANNEL_ID);
    }

    @SuppressLint("NewApi")
    private void sendNotifications() {
        mTag1 = UUID.randomUUID().toString();
        Log.d(TAG, "Sending " + mTag1);
        mTag2 = UUID.randomUUID().toString();
        Log.d(TAG, "Sending " + mTag2);
        mTag3 = UUID.randomUUID().toString();
        Log.d(TAG, "Sending " + mTag3);

        mWhen1 = System.currentTimeMillis() + 1;
        mWhen2 = System.currentTimeMillis() + 2;
        mWhen3 = System.currentTimeMillis() + 3;

        mIcon1 = R.drawable.ic_stat_alice;
        mIcon2 = R.drawable.ic_stat_bob;
        mIcon3 = R.drawable.ic_stat_charlie;

        mId1 = NOTIFICATION_ID + 1;
        mId2 = NOTIFICATION_ID + 2;
        mId3 = NOTIFICATION_ID + 3;

        mPackageString = "com.android.cts.verifier";

        Notification n1 = new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                .setContentTitle("ClearTest 1")
                .setContentText(mTag1)
                .setSmallIcon(mIcon1)
                .setWhen(mWhen1)
                .setDeleteIntent(makeIntent(1, mTag1))
                .setOnlyAlertOnce(true)
                .build();
        mNm.notify(mTag1, mId1, n1);
        mFlag1 = Notification.FLAG_ONLY_ALERT_ONCE;

        Notification n2 = new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                .setContentTitle("ClearTest 2")
                .setContentText(mTag2)
                .setSmallIcon(mIcon2)
                .setWhen(mWhen2)
                .setDeleteIntent(makeIntent(2, mTag2))
                .setAutoCancel(true)
                .build();
        mNm.notify(mTag2, mId2, n2);
        mFlag2 = Notification.FLAG_AUTO_CANCEL;

        Notification n3 = new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                .setContentTitle("ClearTest 3")
                .setContentText(mTag3)
                .setSmallIcon(mIcon3)
                .setWhen(mWhen3)
                .setDeleteIntent(makeIntent(3, mTag3))
                .setAutoCancel(true)
                .setOnlyAlertOnce(true)
                .build();
        mNm.notify(mTag3, mId3, n3);
        mFlag3 = Notification.FLAG_ONLY_ALERT_ONCE | Notification.FLAG_AUTO_CANCEL;
    }

    // Tests
    private class NotificationReceivedTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_note_received);

        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            MockListener.getInstance().resetData();
            deleteChannel();
        }

        @Override
        protected void test() {
            List<String> result = new ArrayList<>(MockListener.getInstance().mPosted);
            if (result.size() > 0 && result.contains(mTag1)) {
                status = PASS;
            } else {
                logFail();
                status = FAIL;
            }
        }
    }

    /**
     * Creates a notification channel. Sends the user to settings to block the channel. Waits
     * to receive the broadcast that the channel was blocked, and confirms that the broadcast
     * contains the correct extras.
     */
    protected class ReceiveChannelBlockNoticeTest extends InteractiveTestCase {
        private String mChannelId;
        private int mRetries = 2;
        private View mView;
        @Override
        protected View inflate(ViewGroup parent) {
            mView = createNlsSettingsItem(parent, R.string.nls_block_channel);
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(false);
            return mView;
        }

        @Override
        protected void setUp() {
            mChannelId = UUID.randomUUID().toString();
            NotificationChannel channel = new NotificationChannel(
                    mChannelId, "ReceiveChannelBlockNoticeTest", IMPORTANCE_LOW);
            mNm.createNotificationChannel(channel);
            status = READY;
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(true);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            NotificationChannel channel = mNm.getNotificationChannel(mChannelId);
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);

            if (channel.getImportance() == IMPORTANCE_NONE) {
                if (prefs.contains(mChannelId) && prefs.getBoolean(mChannelId, false)) {
                    status = PASS;
                } else {
                    if (mRetries > 0) {
                        mRetries--;
                        status = RETEST;
                    } else {
                        status = FAIL;
                    }
                }
            } else {
                // user hasn't jumped to settings to block the channel yet
                status = WAIT_FOR_USER;
            }

            next();
        }

        protected void tearDown() {
            MockListener.getInstance().resetData();
            mNm.deleteNotificationChannel(mChannelId);
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.remove(mChannelId);
        }

        @Override
        protected Intent getIntent() {
         return new Intent(Settings.ACTION_CHANNEL_NOTIFICATION_SETTINGS)
                 .putExtra(EXTRA_APP_PACKAGE, mContext.getPackageName())
                 .putExtra(EXTRA_CHANNEL_ID, mChannelId);
        }
    }

    /**
     * Creates a notification channel group. Sends the user to settings to block the group. Waits
     * to receive the broadcast that the group was blocked, and confirms that the broadcast contains
     * the correct extras.
     */
    protected class ReceiveGroupBlockNoticeTest extends InteractiveTestCase {
        private String mGroupId;
        private int mRetries = 2;
        private View mView;
        @Override
        protected View inflate(ViewGroup parent) {
            mView = createNlsSettingsItem(parent, R.string.nls_block_group);
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(false);
            return mView;
        }

        @Override
        protected void setUp() {
            mGroupId = UUID.randomUUID().toString();
            NotificationChannelGroup group
                    = new NotificationChannelGroup(mGroupId, "ReceiveChannelGroupBlockNoticeTest");
            mNm.createNotificationChannelGroup(group);
            NotificationChannel channel = new NotificationChannel(
                    mGroupId, "ReceiveChannelBlockNoticeTest", IMPORTANCE_LOW);
            channel.setGroup(mGroupId);
            mNm.createNotificationChannel(channel);
            status = READY;
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(true);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            NotificationChannelGroup group = mNm.getNotificationChannelGroup(mGroupId);
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);

            if (group.isBlocked()) {
                if (prefs.contains(mGroupId) && prefs.getBoolean(mGroupId, false)) {
                    status = PASS;
                } else {
                    if (mRetries > 0) {
                        mRetries--;
                        status = RETEST;
                    } else {
                        status = FAIL;
                    }
                }
            } else {
                // user hasn't jumped to settings to block the group yet
                status = WAIT_FOR_USER;
            }

            next();
        }

        protected void tearDown() {
            MockListener.getInstance().resetData();
            mNm.deleteNotificationChannelGroup(mGroupId);
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.remove(mGroupId);
        }

        @Override
        protected Intent getIntent() {
            return new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS)
                    .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK)
                    .putExtra(EXTRA_APP_PACKAGE, mContext.getPackageName());
        }
    }

    /**
     * Sends the user to settings to block the app. Waits to receive the broadcast that the app was
     * blocked, and confirms that the broadcast contains the correct extras.
     */
    protected class ReceiveAppBlockNoticeTest extends InteractiveTestCase {
        private int mRetries = 2;
        private View mView;
        @Override
        protected View inflate(ViewGroup parent) {
            mView = createNlsSettingsItem(parent, R.string.nls_block_app);
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(false);
            return mView;
        }

        @Override
        protected void setUp() {
            status = READY;
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(true);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);

            if (!mNm.areNotificationsEnabled()) {
                Log.d(TAG, "Got broadcast " + prefs.contains(mContext.getPackageName()));
                Log.d(TAG, "Broadcast contains correct data? " +
                        prefs.getBoolean(mContext.getPackageName(), false));
                if (prefs.contains(mContext.getPackageName())
                        && prefs.getBoolean(mContext.getPackageName(), false)) {
                    status = PASS;
                } else {
                    if (mRetries > 0) {
                        mRetries--;
                        status = RETEST;
                    } else {
                        status = FAIL;
                    }
                }
            } else {
                Log.d(TAG, "Notifications still enabled");
                // user hasn't jumped to settings to block the app yet
                status = WAIT_FOR_USER;
            }

            next();
        }

        protected void tearDown() {
            MockListener.getInstance().resetData();
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.remove(mContext.getPackageName());
        }

        @Override
        protected Intent getIntent() {
            return new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS)
                    .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK)
                    .putExtra(EXTRA_APP_PACKAGE, mContext.getPackageName());
        }
    }

    /**
     * Sends the user to settings to unblock the app. Waits to receive the broadcast that the app
     * was unblocked, and confirms that the broadcast contains the correct extras.
     */
    protected class ReceiveAppUnblockNoticeTest extends InteractiveTestCase {
        private int mRetries = 2;
        private View mView;
        @Override
        protected View inflate(ViewGroup parent) {
            mView = createNlsSettingsItem(parent, R.string.nls_unblock_app);
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(false);
            return mView;
        }

        @Override
        protected void setUp() {
            status = READY;
            Button button = mView.findViewById(R.id.nls_action_button);
            button.setEnabled(true);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);

            if (mNm.areNotificationsEnabled()) {
                if (prefs.contains(mContext.getPackageName())
                        && !prefs.getBoolean(mContext.getPackageName(), true)) {
                    status = PASS;
                } else {
                    if (mRetries > 0) {
                        mRetries--;
                        status = RETEST;
                    } else {
                        status = FAIL;
                    }
                }
            } else {
                // user hasn't jumped to settings to block the app yet
                status = WAIT_FOR_USER;
            }

            next();
        }

        protected void tearDown() {
            MockListener.getInstance().resetData();
            SharedPreferences prefs = mContext.getSharedPreferences(
                    NotificationListenerVerifierActivity.PREFS, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.remove(mContext.getPackageName());
        }

        @Override
        protected Intent getIntent() {
            return new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS)
                    .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK)
                    .putExtra(EXTRA_APP_PACKAGE, mContext.getPackageName());
        }
    }

    private class DataIntactTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_payload_intact);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void test() {
            List<JSONObject> result = new ArrayList<>(MockListener.getInstance().getPosted());

            Set<String> found = new HashSet<String>();
            if (result.size() == 0) {
                status = FAIL;
                return;
            }
            boolean pass = true;
            for (JSONObject payload : result) {
                try {
                    pass &= checkEquals(mPackageString,
                            payload.getString(JSON_PACKAGE),
                            "data integrity test: notification package (%s, %s)");
                    String tag = payload.getString(JSON_TAG);
                    if (mTag1.equals(tag)) {
                        found.add(mTag1);
                        pass &= checkEquals(mIcon1, payload.getInt(JSON_ICON),
                                "data integrity test: notification icon (%d, %d)");
                        pass &= checkFlagSet(mFlag1, payload.getInt(JSON_FLAGS),
                                "data integrity test: notification flags (%d, %d)");
                        pass &= checkEquals(mId1, payload.getInt(JSON_ID),
                                "data integrity test: notification ID (%d, %d)");
                        pass &= checkEquals(mWhen1, payload.getLong(JSON_WHEN),
                                "data integrity test: notification when (%d, %d)");
                    } else if (mTag2.equals(tag)) {
                        found.add(mTag2);
                        pass &= checkEquals(mIcon2, payload.getInt(JSON_ICON),
                                "data integrity test: notification icon (%d, %d)");
                        pass &= checkFlagSet(mFlag2, payload.getInt(JSON_FLAGS),
                                "data integrity test: notification flags (%d, %d)");
                        pass &= checkEquals(mId2, payload.getInt(JSON_ID),
                                "data integrity test: notification ID (%d, %d)");
                        pass &= checkEquals(mWhen2, payload.getLong(JSON_WHEN),
                                "data integrity test: notification when (%d, %d)");
                    } else if (mTag3.equals(tag)) {
                        found.add(mTag3);
                        pass &= checkEquals(mIcon3, payload.getInt(JSON_ICON),
                                "data integrity test: notification icon (%d, %d)");
                        pass &= checkFlagSet(mFlag3, payload.getInt(JSON_FLAGS),
                                "data integrity test: notification flags (%d, %d)");
                        pass &= checkEquals(mId3, payload.getInt(JSON_ID),
                                "data integrity test: notification ID (%d, %d)");
                        pass &= checkEquals(mWhen3, payload.getLong(JSON_WHEN),
                                "data integrity test: notification when (%d, %d)");
                    }
                } catch (JSONException e) {
                    pass = false;
                    Log.e(TAG, "failed to unpack data from mocklistener", e);
                }
            }

            pass &= found.size() >= 3;
            status = pass ? PASS : FAIL;
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            MockListener.getInstance().resetData();
            deleteChannel();
        }
    }

    private class DismissOneTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_clear_one);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.getInstance().cancelNotification(
                        MockListener.getInstance().getKeyForTag(mTag1));
                status = RETEST;
            } else {
                List<String> result = new ArrayList<>(MockListener.getInstance().mRemoved);
                if (result.size() != 0
                        && result.contains(mTag1)
                        && !result.contains(mTag2)
                        && !result.contains(mTag3)) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
        }
    }

    private class DismissOneWithReasonTest extends InteractiveTestCase {
        int mRetries = 3;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_clear_one_reason);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.getInstance().cancelNotification(
                        MockListener.getInstance().getKeyForTag(mTag1));
                status = RETEST;
            } else {
                List<JSONObject> result =
                        new ArrayList<>(MockListener.getInstance().mRemovedReason.values());
                boolean pass = false;
                for (JSONObject payload : result) {
                    try {
                        pass |= (checkEquals(mTag1,
                                payload.getString(JSON_TAG),
                                "data dismissal test: notification tag (%s, %s)")
                                && checkEquals(REASON_LISTENER_CANCEL,
                                payload.getInt(JSON_REASON),
                                "data dismissal test: reason (%d, %d)"));
                        if(pass) {
                            break;
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
                if (pass) {
                    status = PASS;
                } else {
                    if (--mRetries > 0) {
                        sleep(100);
                        status = RETEST;
                    } else {
                        status = FAIL;
                    }
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
        }
    }

    private class DismissOneWithStatsTest extends InteractiveTestCase {
        int mRetries = 3;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_clear_one_stats);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.getInstance().cancelNotification(
                        MockListener.getInstance().getKeyForTag(mTag1));
                status = RETEST;
            } else {
                List<JSONObject> result =
                        new ArrayList<>(MockListener.getInstance().mRemovedReason.values());
                boolean pass = true;
                for (JSONObject payload : result) {
                    try {
                        pass &= (payload.getBoolean(JSON_STATS) == false);
                    } catch (JSONException e) {
                        e.printStackTrace();
                        pass = false;
                    }
                }
                if (pass) {
                    status = PASS;
                } else {
                    if (--mRetries > 0) {
                        sleep(100);
                        status = RETEST;
                    } else {
                        logFail("Notification listener got populated stats object.");
                        status = FAIL;
                    }
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
        }
    }

    private class DismissAllTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_clear_all);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.getInstance().cancelAllNotifications();
                status = RETEST;
            } else {
                List<String> result = new ArrayList<>(MockListener.getInstance().mRemoved);
                if (result.size() != 0
                        && result.contains(mTag1)
                        && result.contains(mTag2)
                        && result.contains(mTag3)) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
        }
    }

    private class IsDisabledTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createNlsSettingsItem(parent, R.string.nls_disable_service);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        protected void test() {
            String listeners = Secure.getString(getContentResolver(),
                    ENABLED_NOTIFICATION_LISTENERS);
            if (listeners == null || !listeners.contains(LISTENER_PATH)) {
                status = PASS;
            } else {
                status = WAIT_FOR_USER;
            }
        }

        @Override
        protected void tearDown() {
            MockListener.getInstance().resetData();
        }

        @Override
        protected Intent getIntent() {
            return new Intent(ACTION_NOTIFICATION_LISTENER_SETTINGS);
        }
    }

    private class ServiceStoppedTest extends InteractiveTestCase {
        int mRetries = 3;
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_service_stopped);
        }

        @Override
        protected void test() {
            if (mNm.getEffectsSuppressor() == null && (MockListener.getInstance() == null
                    || !MockListener.getInstance().isConnected)) {
                status = PASS;
            } else {
                if (--mRetries > 0) {
                    sleep(100);
                    status = RETEST;
                } else {
                    status = FAIL;
                }
            }
        }

        @Override
        protected Intent getIntent() {
            return new Intent(ACTION_NOTIFICATION_LISTENER_SETTINGS);
        }
    }

    private class NotificationNotReceivedTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_note_missed);

        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
        }

        @Override
        protected void test() {
            if (MockListener.getInstance() == null) {
                status = PASS;
            } else {
                List<String> result = new ArrayList<>(MockListener.getInstance().mPosted);
                if (result.size() == 0) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
            next();
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            if (MockListener.getInstance() != null) {
                MockListener.getInstance().resetData();
            }
        }
    }

    private class RequestUnbindTest extends InteractiveTestCase {
        int mRetries = 5;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_snooze);

        }

        @Override
        protected void setUp() {
            status = READY;
            MockListener.getInstance().requestInterruptionFilter(
                    MockListener.HINT_HOST_DISABLE_CALL_EFFECTS);
        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.getInstance().requestUnbind();
                status = RETEST;
            } else {
                if (mNm.getEffectsSuppressor() == null && !MockListener.getInstance().isConnected) {
                    status = PASS;
                } else {
                    if (--mRetries > 0) {
                        status = RETEST;
                    } else {
                        logFail();
                        status = FAIL;
                    }
                }
                next();
            }
        }
    }

    private class RequestBindTest extends InteractiveTestCase {
        int mRetries = 5;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_unsnooze);

        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.requestRebind(MockListener.COMPONENT_NAME);
                status = RETEST;
            } else {
                if (MockListener.getInstance().isConnected) {
                    status = PASS;
                    next();
                } else {
                    if (--mRetries > 0) {
                        status = RETEST;
                        next();
                    } else {
                        logFail();
                        status = FAIL;
                    }
                }
            }
        }
    }

    private class EnableHintsTest extends InteractiveTestCase {
        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_hints);

        }

        @Override
        protected void test() {
            if (status == READY) {
                MockListener.getInstance().requestListenerHints(
                        MockListener.HINT_HOST_DISABLE_CALL_EFFECTS);
                status = RETEST;
            } else {
                int result = MockListener.getInstance().getCurrentListenerHints();
                if ((result & MockListener.HINT_HOST_DISABLE_CALL_EFFECTS)
                        == MockListener.HINT_HOST_DISABLE_CALL_EFFECTS) {
                    status = PASS;
                    next();
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }
    }

    private class SnoozeNotificationForTimeTest extends InteractiveTestCase {
        final static int READY_TO_SNOOZE = 0;
        final static int SNOOZED = 1;
        final static int READY_TO_CHECK_FOR_UNSNOOZE = 2;
        int state = -1;
        long snoozeTime = 3000;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_snooze_one_time);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
            state = READY_TO_SNOOZE;
            delay();
        }

        @Override
        protected void test() {
            status = RETEST;
            if (state == READY_TO_SNOOZE) {
                MockListener.getInstance().snoozeNotification(
                        MockListener.getInstance().getKeyForTag(mTag1), snoozeTime);
                state = SNOOZED;
            } else if (state == SNOOZED) {
                List<JSONObject> result =
                        new ArrayList<>(MockListener.getInstance().mRemovedReason.values());
                boolean pass = false;
                for (JSONObject payload : result) {
                    try {
                        pass |= (checkEquals(mTag1,
                                payload.getString(JSON_TAG),
                                "data dismissal test: notification tag (%s, %s)")
                                && checkEquals(MockListener.REASON_SNOOZED,
                                payload.getInt(JSON_REASON),
                                "data dismissal test: reason (%d, %d)"));
                        if (pass) {
                            break;
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
                if (!pass) {
                    logFail();
                    status = FAIL;
                    next();
                    return;
                } else {
                    state = READY_TO_CHECK_FOR_UNSNOOZE;
                }
            } else {
                List<String> result = new ArrayList<>(MockListener.getInstance().mPosted);
                if (result.size() > 0 && result.contains(mTag1)) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
            delay();
        }
    }

    /**
     * Posts notifications, snoozes one of them. Verifies that it is snoozed. Cancels all
     * notifications and reposts them. Confirms that the original notification is still snoozed.
     */
    private class SnoozeNotificationForTimeCancelTest extends InteractiveTestCase {

        final static int READY_TO_SNOOZE = 0;
        final static int SNOOZED = 1;
        final static int READY_TO_CHECK_FOR_SNOOZE = 2;
        int state = -1;
        long snoozeTime = 10000;
        private String tag;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_snooze_one_time);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            tag = mTag1;
            status = READY;
            state = READY_TO_SNOOZE;
            delay();
        }

        @Override
        protected void test() {
            status = RETEST;
            if (state == READY_TO_SNOOZE) {
                MockListener.getInstance().snoozeNotification(
                        MockListener.getInstance().getKeyForTag(tag), snoozeTime);
                state = SNOOZED;
            } else if (state == SNOOZED) {
                List<String> result = getSnoozed();
                if (result.size() >= 1
                        && result.contains(tag)) {
                    // cancel and repost
                    sendNotifications();
                    state = READY_TO_CHECK_FOR_SNOOZE;
                } else {
                    logFail();
                    status = FAIL;
                }
            } else {
                List<String> result = getSnoozed();
                if (result.size() >= 1
                        && result.contains(tag)) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }

        private List<String> getSnoozed() {
            List<String> result = new ArrayList<>();
            StatusBarNotification[] snoozed = MockListener.getInstance().getSnoozedNotifications();
            for (StatusBarNotification sbn : snoozed) {
                result.add(sbn.getTag());
            }
            return result;
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
        }
    }

    private class GetSnoozedNotificationTest extends InteractiveTestCase {
        final static int READY_TO_SNOOZE = 0;
        final static int SNOOZED = 1;
        final static int READY_TO_CHECK_FOR_GET_SNOOZE = 2;
        int state = -1;
        long snoozeTime = 30000;

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_get_snoozed);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendNotifications();
            status = READY;
            state = READY_TO_SNOOZE;
        }

        @Override
        protected void test() {
            status = RETEST;
            if (state == READY_TO_SNOOZE) {
                MockListener.getInstance().snoozeNotification(
                        MockListener.getInstance().getKeyForTag(mTag1), snoozeTime);
                MockListener.getInstance().snoozeNotification(
                        MockListener.getInstance().getKeyForTag(mTag2), snoozeTime);
                state = SNOOZED;
            } else if (state == SNOOZED) {
                List<JSONObject> result =
                        new ArrayList<>(MockListener.getInstance().mRemovedReason.values());
                if (result.size() == 0) {
                    status = FAIL;
                    return;
                }
                boolean pass = false;
                for (JSONObject payload : result) {
                    try {
                        pass |= (checkEquals(mTag1,
                                payload.getString(JSON_TAG),
                                "data dismissal test: notification tag (%s, %s)")
                                && checkEquals(MockListener.REASON_SNOOZED,
                                payload.getInt(JSON_REASON),
                                "data dismissal test: reason (%d, %d)"));
                        if (pass) {
                            break;
                        }
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
                if (!pass) {
                    logFail();
                    status = FAIL;
                } else {
                    state = READY_TO_CHECK_FOR_GET_SNOOZE;
                }
            } else {
                List<String> result = new ArrayList<>();
                StatusBarNotification[] snoozed =
                        MockListener.getInstance().getSnoozedNotifications();
                for (StatusBarNotification sbn : snoozed) {
                    result.add(sbn.getTag());
                }
                if (result.size() >= 2
                        && result.contains(mTag1)
                        && result.contains(mTag2)) {
                    status = PASS;
                } else {
                    logFail();
                    status = FAIL;
                }
            }
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            MockListener.getInstance().resetData();
            delay();
        }
    }

    /** Tests that the extras {@link Bundle} in a MessagingStyle#Message is preserved. */
    private class MessageBundleTest extends InteractiveTestCase {
        private final String extrasKey1 = "extras_key_1";
        private final CharSequence extrasValue1 = "extras_value_1";
        private final String extrasKey2 = "extras_key_2";
        private final CharSequence extrasValue2 = "extras_value_2";

        @Override
        protected View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.msg_extras_preserved);
        }

        @Override
        protected void setUp() {
            createChannel();
            sendMessagingNotification();
            status = READY;
        }

        @Override
        protected void tearDown() {
            mNm.cancelAll();
            deleteChannel();
            delay();
        }

        private void sendMessagingNotification() {
            mTag1 = UUID.randomUUID().toString();
            mNm.cancelAll();
            mWhen1 = System.currentTimeMillis() + 1;
            mIcon1 = R.drawable.ic_stat_alice;
            mId1 = NOTIFICATION_ID + 1;

            Notification.MessagingStyle.Message msg1 =
                    new Notification.MessagingStyle.Message("text1", 0 /* timestamp */, "sender1");
            msg1.getExtras().putCharSequence(extrasKey1, extrasValue1);

            Notification.MessagingStyle.Message msg2 =
                    new Notification.MessagingStyle.Message("text2", 1 /* timestamp */, "sender2");
            msg2.getExtras().putCharSequence(extrasKey2, extrasValue2);

            Notification.MessagingStyle style = new Notification.MessagingStyle("display_name");
            style.addMessage(msg1);
            style.addMessage(msg2);

            Notification n1 = new Notification.Builder(mContext, NOTIFICATION_CHANNEL_ID)
                    .setContentTitle("ClearTest 1")
                    .setContentText(mTag1.toString())
                    .setPriority(Notification.PRIORITY_LOW)
                    .setSmallIcon(mIcon1)
                    .setWhen(mWhen1)
                    .setDeleteIntent(makeIntent(1, mTag1))
                    .setOnlyAlertOnce(true)
                    .setStyle(style)
                    .build();
            mNm.notify(mTag1, mId1, n1);
            mFlag1 = Notification.FLAG_ONLY_ALERT_ONCE;
        }

        // Returns true on success.
        private boolean verifyMessage(
                NotificationCompat.MessagingStyle.Message message,
                String extrasKey,
                CharSequence extrasValue) {
            return message.getExtras() != null
                    && message.getExtras().getCharSequence(extrasKey) != null
                    && message.getExtras().getCharSequence(extrasKey).equals(extrasValue);
        }

        @Override
        protected void test() {
            List<Notification> result =
                    new ArrayList<>(MockListener.getInstance().mPostedNotifications);
            if (result.size() != 1 || result.get(0) == null) {
                logFail();
                status = FAIL;
                next();
                return;
            }
            // Can only read in MessagingStyle using the compat class.
            NotificationCompat.MessagingStyle readStyle =
                    NotificationCompat.MessagingStyle
                            .extractMessagingStyleFromNotification(
                                    result.get(0));
            if (readStyle == null || readStyle.getMessages().size() != 2) {
                status = FAIL;
                logFail();
                next();
                return;
            }

            if (!verifyMessage(readStyle.getMessages().get(0), extrasKey1,
                    extrasValue1)
                    || !verifyMessage(
                    readStyle.getMessages().get(1), extrasKey2, extrasValue2)) {
                status = FAIL;
                logFail();
                next();
                return;
            }

            status = PASS;
        }
    }
}
