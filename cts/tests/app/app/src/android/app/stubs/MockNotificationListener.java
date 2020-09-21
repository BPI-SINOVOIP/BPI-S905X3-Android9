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
package android.app.stubs;

import android.content.ComponentName;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.util.Log;

import java.util.ArrayList;

public class MockNotificationListener extends NotificationListenerService {
    public static final String TAG = "TestNotificationListener";
    public static final String PKG = "android.app.stubs";

    private ArrayList<String> mTestPackages = new ArrayList<>();

    public ArrayList<StatusBarNotification> mPosted = new ArrayList<>();
    public ArrayList<StatusBarNotification> mRemoved = new ArrayList<>();
    public RankingMap mRankingMap;

    private static MockNotificationListener sNotificationListenerInstance = null;
    boolean isConnected;

    public static String getId() {
        return String.format("%s/%s", MockNotificationListener.class.getPackage().getName(),
                MockNotificationListener.class.getName());
    }

    public static ComponentName getComponentName() {
        return new ComponentName(MockNotificationListener.class.getPackage().getName(),
                MockNotificationListener.class.getName());
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mTestPackages.add(PKG);
    }

    @Override
    public void onListenerConnected() {
        super.onListenerConnected();
        sNotificationListenerInstance = this;
        isConnected = true;
    }

    @Override
    public void onListenerDisconnected() {
        isConnected = false;
    }

    public static MockNotificationListener getInstance() {
        return sNotificationListenerInstance;
    }

    public void resetData() {
        mPosted.clear();
        mRemoved.clear();
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn, RankingMap rankingMap) {
        if (!mTestPackages.contains(sbn.getPackageName())) { return; }
        mPosted.add(sbn);
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn, RankingMap rankingMap) {
        if (!mTestPackages.contains(sbn.getPackageName())) { return; }
        mRemoved.add(sbn);
    }

    @Override
    public void onNotificationRankingUpdate(RankingMap rankingMap) {
        mRankingMap = rankingMap;
    }
}
