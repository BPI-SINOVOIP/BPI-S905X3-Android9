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
 * limitations under the License
 */

package com.android.systemui.statusbar;

import static com.android.systemui.statusbar.RemoteInputController.processForRemoteInput;
import static com.android.systemui.statusbar.phone.StatusBar.DEBUG;
import static com.android.systemui.statusbar.phone.StatusBar.ENABLE_CHILD_NOTIFICATIONS;

import android.content.ComponentName;
import android.content.Context;
import android.os.RemoteException;
import android.os.UserHandle;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import com.android.systemui.Dependency;
import com.android.systemui.statusbar.phone.NotificationListenerWithPlugins;

/**
 * This class handles listening to notification updates and passing them along to
 * NotificationPresenter to be displayed to the user.
 */
public class NotificationListener extends NotificationListenerWithPlugins {
    private static final String TAG = "NotificationListener";

    // Dependencies:
    private final NotificationRemoteInputManager mRemoteInputManager =
            Dependency.get(NotificationRemoteInputManager.class);

    private final Context mContext;

    protected NotificationPresenter mPresenter;
    protected NotificationEntryManager mEntryManager;

    public NotificationListener(Context context) {
        mContext = context;
    }

    @Override
    public void onListenerConnected() {
        if (DEBUG) Log.d(TAG, "onListenerConnected");
        onPluginConnected();
        final StatusBarNotification[] notifications = getActiveNotifications();
        if (notifications == null) {
            Log.w(TAG, "onListenerConnected unable to get active notifications.");
            return;
        }
        final RankingMap currentRanking = getCurrentRanking();
        mPresenter.getHandler().post(() -> {
            for (StatusBarNotification sbn : notifications) {
                mEntryManager.addNotification(sbn, currentRanking);
            }
        });
    }

    @Override
    public void onNotificationPosted(final StatusBarNotification sbn,
            final RankingMap rankingMap) {
        if (DEBUG) Log.d(TAG, "onNotificationPosted: " + sbn);
        if (sbn != null && !onPluginNotificationPosted(sbn, rankingMap)) {
            mPresenter.getHandler().post(() -> {
                processForRemoteInput(sbn.getNotification(), mContext);
                String key = sbn.getKey();
                mEntryManager.removeKeyKeptForRemoteInput(key);
                boolean isUpdate =
                        mEntryManager.getNotificationData().get(key) != null;
                // In case we don't allow child notifications, we ignore children of
                // notifications that have a summary, since` we're not going to show them
                // anyway. This is true also when the summary is canceled,
                // because children are automatically canceled by NoMan in that case.
                if (!ENABLE_CHILD_NOTIFICATIONS
                        && mPresenter.getGroupManager().isChildInGroupWithSummary(sbn)) {
                    if (DEBUG) {
                        Log.d(TAG, "Ignoring group child due to existing summary: " + sbn);
                    }

                    // Remove existing notification to avoid stale data.
                    if (isUpdate) {
                        mEntryManager.removeNotification(key, rankingMap);
                    } else {
                        mEntryManager.getNotificationData()
                                .updateRanking(rankingMap);
                    }
                    return;
                }
                if (isUpdate) {
                    mEntryManager.updateNotification(sbn, rankingMap);
                } else {
                    mEntryManager.addNotification(sbn, rankingMap);
                }
            });
        }
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn,
            final RankingMap rankingMap) {
        if (DEBUG) Log.d(TAG, "onNotificationRemoved: " + sbn);
        if (sbn != null && !onPluginNotificationRemoved(sbn, rankingMap)) {
            final String key = sbn.getKey();
            mPresenter.getHandler().post(() -> {
                mEntryManager.removeNotification(key, rankingMap);
            });
        }
    }

    @Override
    public void onNotificationRankingUpdate(final RankingMap rankingMap) {
        if (DEBUG) Log.d(TAG, "onRankingUpdate");
        if (rankingMap != null) {
            RankingMap r = onPluginRankingUpdate(rankingMap);
            mPresenter.getHandler().post(() -> {
                mEntryManager.updateNotificationRanking(r);
            });
        }
    }

    public void setUpWithPresenter(NotificationPresenter presenter,
            NotificationEntryManager entryManager) {
        mPresenter = presenter;
        mEntryManager = entryManager;

        try {
            registerAsSystemService(mContext,
                    new ComponentName(mContext.getPackageName(), getClass().getCanonicalName()),
                    UserHandle.USER_ALL);
        } catch (RemoteException e) {
            Log.e(TAG, "Unable to register notification listener", e);
        }
    }
}
