/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.statusbartest;

import static android.app.NotificationManager.IMPORTANCE_DEFAULT;
import static android.app.NotificationManager.IMPORTANCE_HIGH;
import static android.app.NotificationManager.IMPORTANCE_LOW;
import static android.app.NotificationManager.IMPORTANCE_MIN;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Icon;
import android.media.AudioAttributes;
import android.os.Bundle;
import android.os.Vibrator;
import android.os.Handler;
import android.os.UserHandle;
import android.util.Log;
import android.net.Uri;
import android.os.SystemClock;
import android.widget.RemoteViews;
import android.os.PowerManager;

// private NM API
import android.app.INotificationManager;
import android.widget.Toast;

public class NotificationTestList extends TestActivity
{
    private final static String TAG = "NotificationTestList";

    NotificationManager mNM;
    Vibrator mVibrator;
    Handler mHandler = new Handler();

    long mActivityCreateTime;
    long mChronometerBase = 0;

    boolean mProgressDone = true;

    final int[] kNumberedIconResIDs = {
            R.drawable.notification0,
            R.drawable.notification1,
            R.drawable.notification2,
            R.drawable.notification3,
            R.drawable.notification4,
            R.drawable.notification5,
            R.drawable.notification6,
            R.drawable.notification7,
            R.drawable.notification8,
            R.drawable.notification9
    };
    final int kUnnumberedIconResID = R.drawable.notificationx;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mVibrator = (Vibrator)getSystemService(VIBRATOR_SERVICE);
        mActivityCreateTime = System.currentTimeMillis();
        mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        mNM.createNotificationChannel(new NotificationChannel("min", "Min", IMPORTANCE_MIN));
        mNM.createNotificationChannel(new NotificationChannel("low", "Low", IMPORTANCE_LOW));
        mNM.createNotificationChannel(
                new NotificationChannel("default", "Default", IMPORTANCE_DEFAULT));
        mNM.createNotificationChannel(new NotificationChannel("high", "High", IMPORTANCE_HIGH));
    }

    @Override
    protected String tag() {
        return TAG;
    }

    @Override
    protected Test[] tests() {
        return mTests;
    }

    private Test[] mTests = new Test[] {
            new Test("cancel all") {
                public void run() {
                    mNM.cancelAll();
                }
            },
            new Test("Phone call") {
                public void run()
                {
                    NotificationChannel phoneCall =
                            new NotificationChannel("phone call", "Phone Call", IMPORTANCE_HIGH);
                    phoneCall.setVibrationPattern(new long[] {
                            300, 400, 300, 400, 300, 400, 300, 400, 300, 400, 300, 400,
                            300, 400, 300, 400, 300, 400, 300, 400, 300, 400, 300, 400,
                            300, 400, 300, 400, 300, 400, 300, 400, 300, 400, 300, 400 });
                    phoneCall.enableVibration(true);
                    phoneCall.setLightColor(0xff0000ff);
                    phoneCall.enableLights(true);
                    phoneCall.setSound(Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://" +
                                    getPackageName() + "/raw/ringer"),
                            new AudioAttributes.Builder().setUsage(
                                    AudioAttributes.USAGE_NOTIFICATION_RINGTONE).build());
                    Notification n = new Notification.Builder(NotificationTestList.this,
                            "phone call")
                            .setSmallIcon(R.drawable.icon2)
                            .setFullScreenIntent(makeIntent2(), true)
                            .build();
                    mNM.notify(7001, n);
                }
            },
            new Test("with zen") {
                public void run()
                {
                    mNM.setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALARMS);
                    Notification n = new Notification.Builder(NotificationTestList.this,
                            "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Default priority")
                            .build();
                    mNM.notify("default", 7004, n);
                    try {
                        Thread.sleep(8000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    mNM.setInterruptionFilter(NotificationManager.INTERRUPTION_FILTER_ALL);
                }
            },
            new Test("repeated") {
                public void run()
                {
                    for (int i = 0; i < 50; i++) {
                        Notification n = new Notification.Builder(NotificationTestList.this,
                                "default")
                                .setSmallIcon(R.drawable.icon2)
                                .setContentTitle("Default priority")
                                .build();
                        mNM.notify("default", 7004, n);
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }
            },
            new Test("Post a group") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "min")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Min priority group 1")
                            .setGroup("group1")
                            .build();
                    mNM.notify(6000, n);
                    n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("low priority group 1")
                            .setGroup("group1")
                            .build();
                    mNM.notify(6001, n);
                    n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("default priority group 1")
                            .setGroup("group1")
                            .setOngoing(true)
                            .setColorized(true)
                            .build();
                    mNM.notify(6002, n);
                    n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("summary group 1")
                            .setGroup("group1")
                            .setGroupSummary(true)
                            .build();
                    mNM.notify(6003, n);
                }
            },
            new Test("Post a group (2) w/o summary") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "min")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Min priority group 2")
                            .setGroup("group2")
                            .build();
                    mNM.notify(6100, n);
                    n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("low priority group 2")
                            .setGroup("group2")
                            .build();
                    mNM.notify(6101, n);
                    n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("default priority group 2")
                            .setGroup("group2")
                            .build();
                    mNM.notify(6102, n);
                }
            },
            new Test("Summary for group 2") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "min")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("summary group 2")
                            .setGroup("group2")
                            .setGroupSummary(true)
                            .build();
                    mNM.notify(6103, n);
                }
            },
            new Test("Group up public-secret") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("public notification")
                            .setVisibility(Notification.VISIBILITY_PUBLIC)
                            .setGroup("public-secret")
                            .build();
                    mNM.notify("public", 7009, n);
                    n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("private only notification")
                            .setVisibility(Notification.VISIBILITY_PRIVATE)
                            .setGroup("public-secret")
                            .build();
                    mNM.notify("no public", 7010, n);
                    n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("private version of notification")
                            .setVisibility(Notification.VISIBILITY_PRIVATE)
                            .setGroup("public-secret")
                            .setPublicVersion(new Notification.Builder(
                                    NotificationTestList.this, "default")
                                    .setSmallIcon(R.drawable.icon2)
                                    .setContentTitle("public notification of private notification")
                                    .setVisibility(Notification.VISIBILITY_PUBLIC)
                                    .build())
                            .build();
                    mNM.notify("priv with pub", 7011, n);
                    n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("secret notification")
                            .setVisibility(Notification.VISIBILITY_SECRET)
                            .setGroup("public-secret")
                            .build();
                    mNM.notify("secret", 7012, n);

                    Notification s = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("summary group public-secret")
                            .setGroup("public-secret")
                            .setGroupSummary(true)
                            .build();
                    mNM.notify(7113, s);
                }
            },
            new Test("Cancel priority autogroup") {
                public void run()
                {
                    try {
                        mNM.cancel(Integer.MAX_VALUE);
                    } catch (Exception e) {
                        Toast.makeText(NotificationTestList.this, "cancel failed (yay)",
                                Toast.LENGTH_LONG).show();
                    }
                }
            },
            new Test("Min priority") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "min")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Min priority")
                            .setTicker("Min priority")
                            .build();
                    mNM.notify("min", 7000, n);
                }
            },
            new Test("Low priority") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Low priority")
                            .setTicker("Low priority")
                            .build();
                    mNM.notify("low", 7002, n);
                }
            },
            new Test("Default priority") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Default priority")
                            .build();
                    mNM.notify("default", 7004, n);
                }
            },
            new Test("High priority") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "high")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("High priority")
                            .setTicker("High priority")
                            .build();
                    mNM.notify("high", 7006, n);
                }
            },
            new Test("high priority with delay") {
                public void run()
                {
                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException e) {
                    }
                    Notification n = new Notification.Builder(NotificationTestList.this, "high")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("High priority")
                            .setFullScreenIntent(makeIntent2(), false)
                            .build();
                    mNM.notify(7008, n);
                }
            },
            new Test("public notification") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("public notification")
                            .setVisibility(Notification.VISIBILITY_PUBLIC)
                            .build();
                    mNM.notify("public", 7009, n);
                }
            },
            new Test("private notification, no public") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("private only notification")
                            .setVisibility(Notification.VISIBILITY_PRIVATE)
                            .build();
                    mNM.notify("no public", 7010, n);
                }
            },
            new Test("private notification, has public") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("private version of notification")
                            .setVisibility(Notification.VISIBILITY_PRIVATE)
                            .setPublicVersion(new Notification.Builder(
                                    NotificationTestList.this, "low")
                                    .setSmallIcon(R.drawable.icon2)
                                    .setContentTitle("public notification of private notification")
                                    .setVisibility(Notification.VISIBILITY_PUBLIC)
                                    .build())
                            .build();
                    mNM.notify("priv with pub", 7011, n);
                }
            },
            new Test("secret notification") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("secret notification")
                            .setVisibility(Notification.VISIBILITY_SECRET)
                            .build();
                    mNM.notify("secret", 7012, n);
                }
            },
            new Test("1 minute timeout") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("timeout in a minute")
                            .setTimeoutAfter(System.currentTimeMillis() + (1000 * 60))
                            .build();
                    mNM.notify("timeout_min", 7013, n);
                }
            },
            new Test("Colorized") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("RED IS BEST")
                            .setContentText("or is blue?")
                            .setTimeoutAfter(System.currentTimeMillis() + (1000 * 60))
                            .setColor(Color.RED)
                            .setFlag(Notification.FLAG_ONGOING_EVENT, true)
                            .setColorized(true)
                            .build();
                    mNM.notify("timeout_min", 7013, n);
                }
            },
            new Test("Too many cancels") {
                public void run()
                {
                    mNM.cancelAll();
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle("Cancel then post")
                            .setContentText("instead of just updating the existing notification")
                            .build();
                    mNM.notify("cancel_madness", 7014, n);
                }
            },
            new Test("Off") {
                public void run() {
                    PowerManager pm = (PowerManager) NotificationTestList.this.getSystemService(
                            Context.POWER_SERVICE);
                    PowerManager.WakeLock wl =
                            pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "sound");
                    wl.acquire();

                    pm.goToSleep(SystemClock.uptimeMillis());

                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.stat_sys_phone)
                            .setContentTitle(name)
                            .build();
                    Log.d(TAG, "n.sound=" + n.sound);

                    mNM.notify(1, n);

                    Log.d(TAG, "releasing wake lock");
                    wl.release();
                    Log.d(TAG, "released wake lock");
                }
            },

            new Test("Cancel #1") {
                public void run()
                {
                    mNM.cancel(1);
                }
            },

            new Test("Custom Button") {
                public void run() {
                    RemoteViews view = new RemoteViews(getPackageName(), R.layout.button_notification);
                    view.setOnClickPendingIntent(R.id.button, makeIntent2());
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setOngoing(true)
                            .setCustomContentView(view)
                            .build();

                    mNM.notify(1, n);
                }
            },

            new Test("Action Button") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setOngoing(true)
                            .addAction(new Notification.Action.Builder(
                                    Icon.createWithResource(NotificationTestList.this,
                                            R.drawable.ic_statusbar_chat),
                                    "Button", makeIntent2())
                                    .build())
                            .build();

                    mNM.notify(1, n);
                }
            },

            new Test("with intent") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle("Persistent #1")
                            .setContentText("This is a notification!!!")
                            .setContentIntent(makeIntent2())
                            .setOngoing(true)
                            .build();

                    mNM.notify(1, n);
                }
            },

            new Test("Is blocked?") {
                public void run() {
                    Toast.makeText(NotificationTestList.this,
                            "package enabled? " + mNM.areNotificationsEnabled(),
                            Toast.LENGTH_LONG).show();
                }
            },

            new Test("importance?") {
                public void run() {
                    Toast.makeText(NotificationTestList.this,
                            "importance? " + mNM.getImportance(),
                            Toast.LENGTH_LONG).show();
                }
            },

            new Test("Whens") {
                public void run()
                {
                    Notification.Builder n = new Notification.Builder(
                            NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon1)
                            .setContentTitle(name)
                            .setOngoing(true);

                    mNM.notify(1, n.setContentTitle("(453) 123-2328")
                            .setWhen(System.currentTimeMillis()-(1000*60*60*24))
                            .build());

                    mNM.notify(1, n.setContentTitle("Mark Willem, Me (2)")
                            .setWhen(System.currentTimeMillis())
                            .build());

                    mNM.notify(1, n.setContentTitle("Sophia Winterlanden")
                            .setWhen(System.currentTimeMillis() + (1000 * 60 * 60 * 24))
                            .build());
                }
            },

            new Test("Bad Icon #1 (when=create)") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.layout.chrono_notification /* not an icon */)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle("Persistent #1")
                            .setContentText("This is the same notification!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Bad Icon #1 (when=now)") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.layout.chrono_notification /* not an icon */)
                            .setWhen(System.currentTimeMillis())
                            .setContentTitle("Persistent #1")
                            .setContentText("This is the same notification!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Null Icon #1 (when=now)") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(0)
                            .setWhen(System.currentTimeMillis())
                            .setContentTitle("Persistent #1")
                            .setContentText("This is the same notification!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Bad resource #1 (when=create)") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle("Persistent #1")
                            .setContentText("This is the same notification!!")
                            .setContentIntent(makeIntent())
                            .build();
                    n.contentView.setInt(1 /*bogus*/, "bogus method", 666);
                    mNM.notify(1, n);
                }
            },

            new Test("Bad resource #1 (when=now)") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setWhen(System.currentTimeMillis())
                            .setContentTitle("Persistent #1")
                            .setContentText("This is the same notification!!")
                            .setContentIntent(makeIntent())
                            .build();
                    n.contentView.setInt(1 /*bogus*/, "bogus method", 666);
                    mNM.notify(1, n);
                }
            },

            new Test("Times") {
                public void run()
                {
                    long now = System.currentTimeMillis();

                    timeNotification(7, "24 hours from now", now+(1000*60*60*24));
                    timeNotification(6, "12:01:00 from now", now+(1000*60*60*12)+(60*1000));
                    timeNotification(5, "12 hours from now", now+(1000*60*60*12));
                    timeNotification(4, "now", now);
                    timeNotification(3, "11:59:00 ago", now-((1000*60*60*12)-(60*1000)));
                    timeNotification(2, "12 hours ago", now-(1000*60*60*12));
                    timeNotification(1, "24 hours ago", now-(1000*60*60*24));
                }
            },
            new StateStress("Stress - Ongoing / Latest", 100, 100, new Runnable[] {
                    new Runnable() {
                        public void run() {
                            Log.d(TAG, "Stress - Ongoing/Latest 0");
                            Notification n = new Notification.Builder(NotificationTestList.this, "low")
                                    .setSmallIcon(R.drawable.icon3)
                                    .setWhen(System.currentTimeMillis())
                                    .setContentTitle("Stress - Ongoing")
                                    .setContentText("Notify me!!!")
                                    .setOngoing(true)
                                    .build();
                            mNM.notify(1, n);
                        }
                    },
                    new Runnable() {
                        public void run() {
                            Log.d(TAG, "Stress - Ongoing/Latest 1");
                            Notification n = new Notification.Builder(NotificationTestList.this, "low")
                                    .setSmallIcon(R.drawable.icon4)
                                    .setWhen(System.currentTimeMillis())
                                    .setContentTitle("Stress - Latest")
                                    .setContentText("Notify me!!!")
                                    .build();
                            mNM.notify(1, n);
                        }
                    }
            }),

            new Test("Long") {
                public void run()
                {
                    NotificationChannel channel = new NotificationChannel("v. noisy",
                            "channel for sound and a custom vibration", IMPORTANCE_DEFAULT);
                    channel.enableVibration(true);
                    channel.setVibrationPattern(new long[] {
                            300, 400, 300, 400, 300, 400, 300, 400, 300, 400, 300, 400,
                            300, 400, 300, 400, 300, 400, 300, 400, 300, 400, 300, 400,
                            300, 400, 300, 400, 300, 400, 300, 400, 300, 400, 300, 400 });
                    mNM.createNotificationChannel(channel);

                    Notification n = new Notification.Builder(NotificationTestList.this, "v. noisy")
                            .setSmallIcon(R.drawable.icon1)
                            .setContentTitle(name)
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Progress #1") {
                public void run() {
                    final boolean PROGRESS_UPDATES_WHEN = true;
                    if (!mProgressDone) return;
                    mProgressDone = false;
                    Thread t = new Thread() {
                        public void run() {
                            int x = 0;
                            final Notification.Builder n = new Notification.Builder(
                                    NotificationTestList.this, "low")
                                    .setSmallIcon(R.drawable.icon1)
                                    .setContentTitle(name)
                                    .setOngoing(true);

                            while (!mProgressDone) {
                                n.setWhen(PROGRESS_UPDATES_WHEN
                                        ? System.currentTimeMillis()
                                        : mActivityCreateTime);
                                n.setProgress(100, x, false);
                                n.setContentText("Progress: " + x + "%");

                                mNM.notify(500, n.build());
                                x = (x + 7) % 100;

                                try {
                                    Thread.sleep(1000);
                                } catch (InterruptedException e) {
                                    break;
                                }
                            }
                        }
                    };
                    t.start();
                }
            },

            new Test("Stop Progress") {
                public void run() {
                    mProgressDone = true;
                    mNM.cancel(500);
                }
            },

            new Test("Blue Lights") {
                public void run()
                {
                    NotificationChannel channel = new NotificationChannel("blue",
                            "blue", IMPORTANCE_DEFAULT);
                    channel.enableLights(true);
                    channel.setLightColor(0xff0000ff);
                    mNM.createNotificationChannel(channel);

                    Notification n = new Notification.Builder(NotificationTestList.this, "blue")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle(name)
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Red Lights") {
                public void run()
                {
                    NotificationChannel channel = new NotificationChannel("red",
                            "red", IMPORTANCE_DEFAULT);
                    channel.enableLights(true);
                    channel.setLightColor(0xffff0000);
                    mNM.createNotificationChannel(channel);

                    Notification n = new Notification.Builder(NotificationTestList.this, "red")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle(name)
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Lights off") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "default")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle(name)
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Alert once") {
                public void run()
                {
                    Notification n = new Notification.Builder(NotificationTestList.this, "high")
                            .setSmallIcon(R.drawable.icon2)
                            .setContentTitle(name)
                            .setOnlyAlertOnce(true)
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Resource Sound") {
                public void run()
                {
                    NotificationChannel channel = new NotificationChannel("res_sound",
                            "resource sound", IMPORTANCE_DEFAULT);
                    channel.setSound(Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://" +
                            getPackageName() + "/raw/ringer"), Notification.AUDIO_ATTRIBUTES_DEFAULT);
                    mNM.createNotificationChannel(channel);

                    Notification n = new Notification.Builder(NotificationTestList.this, "res_sound")
                            .setSmallIcon(R.drawable.stat_sys_phone)
                            .setContentTitle(name)
                            .build();
                    Log.d(TAG, "n.sound=" + n.sound);

                    mNM.notify(1, n);
                }
            },

            new Test("Sound and Cancel") {
                public void run()
                {
                    NotificationChannel channel = new NotificationChannel("res_sound",
                            "resource sound", IMPORTANCE_DEFAULT);
                    channel.setSound(Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://" +
                            getPackageName() + "/raw/ringer"), Notification.AUDIO_ATTRIBUTES_DEFAULT);
                    mNM.createNotificationChannel(channel);

                    Notification n = new Notification.Builder(NotificationTestList.this, "res_sound")
                            .setSmallIcon(R.drawable.stat_sys_phone)
                            .setContentTitle(name)
                            .build();

                    mNM.notify(1, n);
                    SystemClock.sleep(600);
                    mNM.cancel(1);
                }
            },

            new Test("Vibrate and cancel") {
                public void run()
                {
                    NotificationChannel channel = new NotificationChannel("vibrate",
                            "vibrate", IMPORTANCE_DEFAULT);
                    channel.enableVibration(true);
                    channel.setVibrationPattern(new long[] {0, 700, 500, 1000, 0, 700, 500, 1000,
                            0, 700, 500, 1000, 0, 700, 500, 1000, 0, 700, 500, 1000, 0, 700, 500, 1000,
                            0, 700, 500, 1000, 0, 700, 500, 1000});
                    mNM.createNotificationChannel(channel);

                    Notification n = new Notification.Builder(NotificationTestList.this, "vibrate")
                            .setSmallIcon(R.drawable.stat_sys_phone)
                            .setContentTitle(name)
                            .build();

                    mNM.notify(1, n);
                    SystemClock.sleep(500);
                    mNM.cancel(1);
                }
            },

            new Test("Vibrate pattern") {
                public void run()
                {
                    mVibrator.vibrate(new long[] { 250, 1000, 500, 2000 }, -1);
                }
            },

            new Test("Vibrate pattern repeating") {
                public void run()
                {
                    mVibrator.vibrate(new long[] { 250, 1000, 500 }, 1);
                }
            },

            new Test("Vibrate 3s") {
                public void run()
                {
                    mVibrator.vibrate(3000);
                }
            },

            new Test("Vibrate 100s") {
                public void run()
                {
                    mVibrator.vibrate(100000);
                }
            },

            new Test("Vibrate off") {
                public void run()
                {
                    mVibrator.cancel();
                }
            },

            new Test("Cancel #1") {
                public void run() {
                    mNM.cancel(1);
                }
            },

            new Test("Cancel #1 in 3 sec") {
                public void run() {
                    mHandler.postDelayed(new Runnable() {
                        public void run() {
                            Log.d(TAG, "Cancelling now...");
                            mNM.cancel(1);
                        }
                    }, 3000);
                }
            },

            new Test("Cancel #2") {
                public void run() {
                    mNM.cancel(2);
                }
            },

            new Test("Persistent #1") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this)
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setContentText("This is a notification!!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Persistent #1 in 3 sec") {
                public void run() {
                    mHandler.postDelayed(new Runnable() {
                        public void run() {
                            String message = "            "
                                    + "tick tock tick tock\n\nSometimes notifications can "
                                    + "be really long and wrap to more than one line.\n"
                                    + "Sometimes."
                                    + "Ohandwhathappensifwehaveonereallylongstringarewesure"
                                    + "thatwesegmentitcorrectly?\n";
                            Notification n = new Notification.Builder(
                                    NotificationTestList.this, "low")
                                    .setSmallIcon(R.drawable.icon1)
                                    .setContentTitle(name)
                                    .setContentText("This is still a notification!!!")
                                    .setContentIntent(makeIntent())
                                    .setStyle(new Notification.BigTextStyle().bigText(message))
                                    .build();
                            mNM.notify(1, n);
                        }
                    }, 3000);
                }
            },

            new Test("Persistent #2") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setContentText("This is a notification!!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(2, n);
                }
            },

            new Test("Persistent #3") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setContentText("This is a notification!!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(3, n);
                }
            },

            new Test("Persistent #2 Vibrate") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setContentText("This is a notification!!!")
                            .setContentIntent(makeIntent())
                            .setDefaults(Notification.DEFAULT_VIBRATE)
                            .build();
                    mNM.notify(2, n);
                }
            },

            new Test("Persistent #1 - different icon") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon2)
                            .setWhen(mActivityCreateTime)
                            .setContentTitle(name)
                            .setContentText("This is a notification!!!")
                            .setContentIntent(makeIntent())
                            .build();
                    mNM.notify(1, n);
                }
            },

            new Test("Chronometer Start") {
                public void run() {
                    Notification n = new Notification.Builder(NotificationTestList.this, "low")
                            .setSmallIcon(R.drawable.icon1)
                            .setWhen(System.currentTimeMillis())
                            .setContentTitle(name)
                            .setContentIntent(makeIntent())
                            .setOngoing(true)
                            .setUsesChronometer(true)
                            .build();
                    mNM.notify(2, n);
                }
            },

            new Test("Chronometer Stop") {
                public void run() {
                    mHandler.postDelayed(new Runnable() {
                        public void run() {
                            Log.d(TAG, "Chronometer Stop");
                            Notification n = new Notification.Builder(
                                    NotificationTestList.this, "low")
                                    .setSmallIcon(R.drawable.icon1)
                                    .setWhen(System.currentTimeMillis())
                                    .setContentTitle(name)
                                    .setContentIntent(makeIntent())
                                    .build();
                            mNM.notify(2, n);
                        }
                    }, 3000);
                }
            },

            new Test("Sequential Persistent") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 1));
                    mNM.notify(2, notificationWithNumbers(name, 2));
                }
            },

            new Test("Replace Persistent") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 1));
                    mNM.notify(1, notificationWithNumbers(name, 1));
                }
            },

            new Test("Run and Cancel (n=1)") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 1));
                    mNM.cancel(1);
                }
            },

            new Test("Run an Cancel (n=2)") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 1));
                    mNM.notify(2, notificationWithNumbers(name, 2));
                    mNM.cancel(2);
                }
            },

            // Repeatedly notify and cancel -- triggers bug #670627
            new Test("Bug 670627") {
                public void run() {
                    for (int i = 0; i < 10; i++) {
                        Log.d(TAG, "Add two notifications");
                        mNM.notify(1, notificationWithNumbers(name, 1));
                        mNM.notify(2, notificationWithNumbers(name, 2));
                        Log.d(TAG, "Cancel two notifications");
                        mNM.cancel(1);
                        mNM.cancel(2);
                    }
                }
            },

            new Test("Ten Notifications") {
                public void run() {
                    for (int i = 0; i < 10; i++) {
                        Notification n = new Notification.Builder(NotificationTestList.this, "low")
                                .setSmallIcon(kNumberedIconResIDs[i])
                                .setContentTitle("Persistent #" + i)
                                .setContentText("Notify me!!!" + i)
                                .setOngoing(i < 2)
                                .setNumber(i)
                                .build();
                        mNM.notify((i+1)*10, n);
                    }
                }
            },

            new Test("Cancel eight notifications") {
                public void run() {
                    for (int i = 1; i < 9; i++) {
                        mNM.cancel((i+1)*10);
                    }
                }
            },

            new Test("Cancel the other two notifications") {
                public void run() {
                    mNM.cancel(10);
                    mNM.cancel(100);
                }
            },

            new Test("Persistent with numbers 1") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 1));
                }
            },

            new Test("Persistent with numbers 22") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 22));
                }
            },

            new Test("Persistent with numbers 333") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 333));
                }
            },

            new Test("Persistent with numbers 4444") {
                public void run() {
                    mNM.notify(1, notificationWithNumbers(name, 4444));
                }
            },

            new Test("Crash") {
                public void run()
                {
                    PowerManager.WakeLock wl =
                            ((PowerManager) NotificationTestList.this.getSystemService(Context.POWER_SERVICE))
                                    .newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "crasher");
                    wl.acquire();
                    mHandler.postDelayed(new Runnable() {
                        public void run() {
                            throw new RuntimeException("Die!");
                        }
                    }, 10000);

                }
            },

    };

    private Notification notificationWithNumbers(String name, int num) {
        Notification n = new Notification.Builder(NotificationTestList.this, "low")
                .setSmallIcon((num >= 0 && num < kNumberedIconResIDs.length)
                        ? kNumberedIconResIDs[num]
                        : kUnnumberedIconResID)
                .setContentTitle(name)
                .setContentText("Number=" + num)
                .setNumber(num)
                .build();
        return n;
    }

    private PendingIntent makeIntent() {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);
        return PendingIntent.getActivity(this, 0, intent, 0);
    }

    private PendingIntent makeIntent2() {
        Intent intent = new Intent(this, StatusBarTest.class);
        return PendingIntent.getActivity(this, 0, intent, 0);
    }


    class StateStress extends Test {
        StateStress(String name, int pause, int iterations, Runnable[] tasks) {
            super(name);
            mPause = pause;
            mTasks = tasks;
            mIteration = iterations;
        }
        Runnable[] mTasks;
        int mNext;
        int mIteration;
        long mPause;
        Runnable mRunnable = new Runnable() {
            public void run() {
                mTasks[mNext].run();
                mNext++;
                if (mNext >= mTasks.length) {
                    mNext = 0;
                    mIteration--;
                    if (mIteration <= 0) {
                        return;
                    }
                }
                mHandler.postDelayed(mRunnable, mPause);
            }
        };
        public void run() {
            mNext = 0;
            mHandler.postDelayed(mRunnable, mPause);
        }
    }

    void timeNotification(int n, String label, long time) {
        mNM.notify(n, new Notification.Builder(NotificationTestList.this, "low")
                .setSmallIcon(R.drawable.ic_statusbar_missedcall)
                .setWhen(time)
                .setContentTitle(label)
                .setContentText(new java.util.Date(time).toString())
                .build());

    }

    Bitmap loadBitmap(int resId) {
        BitmapDrawable bd = (BitmapDrawable)getResources().getDrawable(resId);
        return Bitmap.createBitmap(bd.getBitmap());
    }
}
