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

package com.android.cts.ephemeralapp1;

import static android.media.AudioFormat.CHANNEL_IN_MONO;
import static android.media.AudioFormat.ENCODING_PCM_16BIT;
import static android.media.MediaRecorder.AudioSource.MIC;
import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.hasItems;
import static org.hamcrest.CoreMatchers.notNullValue;
import static org.hamcrest.CoreMatchers.nullValue;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.fail;

import android.Manifest;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.SearchManager;
import android.app.SearchableInfo;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.location.Criteria;
import android.location.LocationManager;
import android.media.AudioRecord;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.CellLocation;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;

import com.android.compatibility.common.util.SystemUtil;
import com.android.compatibility.common.util.TestThread;
import com.android.cts.util.TestResult;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
public class ClientTest {
    /** Action to start normal test activities */
    private static final String ACTION_START_NORMAL =
            "com.android.cts.ephemeraltest.START_NORMAL";
    /** Action to start normal, exposed test activities */
    private static final String ACTION_START_EXPOSED =
            "com.android.cts.ephemeraltest.START_EXPOSED";
    /** Action to start ephemeral test activities */
    private static final String ACTION_START_EPHEMERAL =
            "com.android.cts.ephemeraltest.START_EPHEMERAL";
    /** Action to start private ephemeral test activities */
    private static final String ACTION_START_EPHEMERAL_PRIVATE =
            "com.android.cts.ephemeraltest.START_EPHEMERAL_PRIVATE";
    private static final String ACTION_START_EPHEMERAL_ACTIVITY =
            "com.android.cts.ephemeraltest.START_OTHER_EPHEMERAL";
    /** Action to query for test activities */
    private static final String ACTION_QUERY =
            "com.android.cts.ephemeraltest.QUERY";
    private static final String EXTRA_ACTIVITY_NAME =
            "com.android.cts.ephemeraltest.EXTRA_ACTIVITY_NAME";
    private static final String EXTRA_ACTIVITY_RESULT =
            "com.android.cts.ephemeraltest.EXTRA_ACTIVITY_RESULT";

    private BroadcastReceiver mReceiver;
    private PhoneStateListener mPhoneStateListener;
    private final SynchronousQueue<TestResult> mResultQueue = new SynchronousQueue<>();

    @Before
    public void setUp() throws Exception {
        final IntentFilter filter =
                new IntentFilter("com.android.cts.ephemeraltest.START_ACTIVITY");
        filter.addCategory(Intent.CATEGORY_DEFAULT);
        mReceiver = new ActivityBroadcastReceiver(mResultQueue);
        InstrumentationRegistry.getContext()
                .registerReceiver(mReceiver, filter, Context.RECEIVER_VISIBLE_TO_INSTANT_APPS);
    }

    @After
    public void tearDown() throws Exception {
        InstrumentationRegistry.getContext().unregisterReceiver(mReceiver);
    }

    @Test
    public void testQuery() throws Exception {
        // query normal activities
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry.getContext()
                    .getPackageManager().queryIntentActivities(queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(2));
            assertThat(resolveInfo.get(0).activityInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).activityInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralActivity"));
            assertThat(resolveInfo.get(0).isInstantAppAvailable,
                    is(true));
            assertThat(resolveInfo.get(1).activityInfo.packageName,
                    is("com.android.cts.normalapp"));
            assertThat(resolveInfo.get(1).activityInfo.name,
                    is("com.android.cts.normalapp.ExposedActivity"));
            assertThat(resolveInfo.get(1).isInstantAppAvailable,
                    is(false));
        }

        // query normal activities; directed package
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry.getContext()
                    .getPackageManager().queryIntentActivities(queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(2));
            assertThat(resolveInfo.get(0).activityInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).activityInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralActivity"));
            assertThat(resolveInfo.get(0).isInstantAppAvailable,
                    is(true));
            assertThat(resolveInfo.get(1).activityInfo.packageName,
                    is("com.android.cts.normalapp"));
            assertThat(resolveInfo.get(1).activityInfo.name,
                    is("com.android.cts.normalapp.ExposedActivity"));
            assertThat(resolveInfo.get(1).isInstantAppAvailable,
                    is(false));
        }

        // query normal activities; directed component
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry.getContext()
                    .getPackageManager().queryIntentActivities(queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(2));
            assertThat(resolveInfo.get(0).activityInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).activityInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralActivity"));
            assertThat(resolveInfo.get(0).isInstantAppAvailable,
                    is(true));
            assertThat(resolveInfo.get(1).activityInfo.packageName,
                    is("com.android.cts.normalapp"));
            assertThat(resolveInfo.get(1).activityInfo.name,
                    is("com.android.cts.normalapp.ExposedActivity"));
            assertThat(resolveInfo.get(1).isInstantAppAvailable,
                    is(false));
        }

        // query own ephemeral application activities with a web URI
        {
            final Intent queryIntent = new Intent(Intent.ACTION_VIEW);
            queryIntent.addCategory(Intent.CATEGORY_BROWSABLE);
            queryIntent.setData(Uri.parse("https://cts.google.com/ephemeral"));
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry.getContext()
                    .getPackageManager().queryIntentActivities(
                            queryIntent, PackageManager.GET_RESOLVED_FILTER);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            for (ResolveInfo info: resolveInfo) {
                assertThat(info.filter, is(notNullValue()));
                if (handlesAllWebData(info.filter)) {
                    continue;
                }
                assertThat(info.activityInfo.packageName,
                        is("com.android.cts.ephemeralapp1"));
                assertThat(info.activityInfo.name,
                        is("com.android.cts.ephemeralapp1.EphemeralActivity"));
                assertThat(info.isInstantAppAvailable,
                        is(true));
            }
        }

        // query other ephemeral application activities with a web URI
        {
            final Intent queryIntent = new Intent(Intent.ACTION_VIEW);
            queryIntent.addCategory(Intent.CATEGORY_BROWSABLE);
            queryIntent.setData(Uri.parse("https://cts.google.com/other"));
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry.getContext()
                    .getPackageManager().queryIntentActivities(
                            queryIntent, PackageManager.GET_RESOLVED_FILTER);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            for (ResolveInfo info: resolveInfo) {
                assertThat(info.filter, is(notNullValue()));
                if (handlesAllWebData(info.filter)) {
                    continue;
                }
                fail("resolution should have only matched browsers");
            }
        }

        // query services
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry
                    .getContext().getPackageManager().queryIntentServices(queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(2));
            assertThat(resolveInfo.get(0).serviceInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).serviceInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralService"));
            assertThat(resolveInfo.get(1).serviceInfo.packageName,
                    is("com.android.cts.normalapp"));
            assertThat(resolveInfo.get(1).serviceInfo.name,
                    is("com.android.cts.normalapp.ExposedService"));
            assertThat(resolveInfo.get(1).isInstantAppAvailable,
                    is(false));
        }

        // query services; directed package
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            queryIntent.setPackage("com.android.cts.ephemeralapp1");
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry
                    .getContext().getPackageManager().queryIntentServices(queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(1));
            assertThat(resolveInfo.get(0).serviceInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).serviceInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralService"));
        }

        // query services; directed component
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            queryIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralService"));
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry
                    .getContext().getPackageManager().queryIntentServices(queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(1));
            assertThat(resolveInfo.get(0).serviceInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).serviceInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralService"));
        }

        // query instant application provider
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry
                    .getContext().getPackageManager().queryIntentContentProviders(
                            queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(2));
            assertThat(resolveInfo.get(0).providerInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).providerInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralProvider"));
            assertThat(resolveInfo.get(1).providerInfo.packageName,
                    is("com.android.cts.normalapp"));
            assertThat(resolveInfo.get(1).providerInfo.name,
                    is("com.android.cts.normalapp.ExposedProvider"));
            assertThat(resolveInfo.get(1).isInstantAppAvailable,
                    is(false));
        }

        // query instant application provider ; directed package
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            queryIntent.setPackage("com.android.cts.ephemeralapp1");
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry
                    .getContext().getPackageManager().queryIntentContentProviders(
                            queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(1));
            assertThat(resolveInfo.get(0).providerInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).providerInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralProvider"));
        }

        // query instant application provider ; directed component
        {
            final Intent queryIntent = new Intent(ACTION_QUERY);
            queryIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralProvider"));
            final List<ResolveInfo> resolveInfo = InstrumentationRegistry
                    .getContext().getPackageManager().queryIntentContentProviders(
                            queryIntent, 0 /*flags*/);
            if (resolveInfo == null || resolveInfo.size() == 0) {
                fail("didn't resolve any intents");
            }
            assertThat(resolveInfo.size(), is(1));
            assertThat(resolveInfo.get(0).providerInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(resolveInfo.get(0).providerInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralProvider"));
        }

        // resolve normal provider
        {
            final ProviderInfo providerInfo = InstrumentationRegistry
                    .getContext().getPackageManager().resolveContentProvider(
                            "com.android.cts.normalapp.provider", 0 /*flags*/);
            assertThat(providerInfo, is(nullValue()));
        }

        // resolve exposed provider
        {
            final ProviderInfo providerInfo = InstrumentationRegistry
                    .getContext().getPackageManager().resolveContentProvider(
                            "com.android.cts.normalapp.exposed.provider", 0 /*flags*/);
            assertThat(providerInfo, is(notNullValue()));
            assertThat(providerInfo.packageName,
                    is("com.android.cts.normalapp"));
            assertThat(providerInfo.name,
                    is("com.android.cts.normalapp.ExposedProvider"));
        }

        // resolve instant application provider
        {
            final ProviderInfo providerInfo = InstrumentationRegistry
                    .getContext().getPackageManager().resolveContentProvider(
                            "com.android.cts.ephemeralapp1.provider", 0 /*flags*/);
            assertThat(providerInfo, is(notNullValue()));
            assertThat(providerInfo.packageName,
                    is("com.android.cts.ephemeralapp1"));
            assertThat(providerInfo.name,
                    is("com.android.cts.ephemeralapp1.EphemeralProvider"));
        }
    }

    @Test
    public void testStartNormal() throws Exception {
        // start the normal activity
        try {
            final Intent startNormalIntent = new Intent(ACTION_START_NORMAL)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            InstrumentationRegistry
                    .getContext().startActivity(startNormalIntent, null /*options*/);
            final TestResult testResult = getResult();
            fail();
        } catch (ActivityNotFoundException expected) {
        }

        // start the normal activity; directed package
        try {
            final Intent startNormalIntent = new Intent(ACTION_START_NORMAL)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startNormalIntent.setPackage("com.android.cts.normalapp");
            InstrumentationRegistry
                    .getContext().startActivity(startNormalIntent, null /*options*/);
            final TestResult testResult = getResult();
            fail();
        } catch (ActivityNotFoundException expected) {
        }

        // start the normal activity; directed component
        try {
            final Intent startNormalIntent = new Intent(ACTION_START_NORMAL)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startNormalIntent.setComponent(new ComponentName(
                    "com.android.cts.normalapp", "com.android.cts.normalapp.NormalActivity"));
            InstrumentationRegistry
                    .getContext().startActivity(startNormalIntent, null /*options*/);
            final TestResult testResult = getResult();
            fail();
        } catch (ActivityNotFoundException expected) {
        }

// TODO: Ideally we should have a test for this. However, it shows a disambig between the
//       the normal app and chrome; for which there is no easy solution.
//        // start the normal activity; using VIEW/BROWSABLE
//        {
//            final Intent startViewIntent = new Intent(Intent.ACTION_VIEW)
//                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//            startViewIntent.addCategory(Intent.CATEGORY_BROWSABLE);
//            startViewIntent.setData(Uri.parse("https://cts.google.com/normal"));
//            InstrumentationRegistry.getContext().startActivity(startViewIntent, null /*options*/);
//            final TestResult testResult = getResult();
//            assertThat(testResult.getPackageName(),
//                    is("com.android.cts.normalapp"));
//            assertThat(testResult.getComponentName(),
//                    is("NormalWebActivity"));
//            assertThat(testResult.getStatus(),
//                    is("PASS"));
//            assertThat(testResult.getEphemeralPackageInfoExposed(),
//                    is(false));
//            assertThat(testResult.getException(),
//                    is(nullValue()));
//        }

        // We don't attempt to start the service since it will merely return and not
        // provide any feedback. The alternative is to wait for the broadcast timeout
        // but it's silly to artificially slow down CTS. We'll rely on queryIntentService
        // to check whether or not the service is actually exposed

        // bind to the normal service; directed package
        {
            final Intent startNormalIntent = new Intent(ACTION_START_NORMAL);
            startNormalIntent.setPackage("com.android.cts.normalapp");
            final TestServiceConnection connection = new TestServiceConnection();
            try {
                assertThat(InstrumentationRegistry.getContext().bindService(
                        startNormalIntent, connection, Context.BIND_AUTO_CREATE /*flags*/),
                        is(false));
            } finally {
                InstrumentationRegistry.getContext().unbindService(connection);
            }
        }

        // bind to the normal service; directed component
        {
            final Intent startNormalIntent = new Intent(ACTION_START_NORMAL);
            startNormalIntent.setComponent(new ComponentName(
                    "com.android.cts.normalapp", "com.android.cts.normalapp.NormalService"));
            final TestServiceConnection connection = new TestServiceConnection();
            try {
                assertThat(InstrumentationRegistry.getContext().bindService(
                        startNormalIntent, connection, Context.BIND_AUTO_CREATE /*flags*/),
                        is(false));
            } finally {
                InstrumentationRegistry.getContext().unbindService(connection);
            }
        }

        // connect to the normal provider
        {
            final String provider = "content://com.android.cts.normalapp.provider/table";
            final Cursor testCursor = InstrumentationRegistry
                    .getContext().getContentResolver().query(
                            Uri.parse(provider),
                            null /*projection*/,
                            null /*selection*/,
                            null /*selectionArgs*/,
                            null /*sortOrder*/);
            assertThat(testCursor, is(nullValue()));
        }
    }

    @Test
    public void testStartExposed01() throws Exception {
        // start the explicitly exposed activity
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        InstrumentationRegistry
                .getContext().startActivity(startExposedIntent, null /*options*/);
        final TestResult testResult = getResult();
        assertThat(testResult.getPackageName(),
                is("com.android.cts.normalapp"));
        assertThat(testResult.getComponentName(),
                is("ExposedActivity"));
        assertThat(testResult.getStatus(),
                is("PASS"));
        assertThat(testResult.getEphemeralPackageInfoExposed(),
                is(true));
        assertThat(testResult.getException(),
                is(nullValue()));
    }

    @Test
    public void testStartExposed02() throws Exception {
        // start the explicitly exposed activity; directed package
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startExposedIntent.setPackage("com.android.cts.normalapp");
        InstrumentationRegistry
                .getContext().startActivity(startExposedIntent, null /*options*/);
        final TestResult testResult = getResult();
        assertThat(testResult.getPackageName(),
                is("com.android.cts.normalapp"));
        assertThat(testResult.getComponentName(),
                is("ExposedActivity"));
        assertThat(testResult.getStatus(),
                is("PASS"));
        assertThat(testResult.getEphemeralPackageInfoExposed(),
                is(true));
        assertThat(testResult.getException(),
                is(nullValue()));
    }

    @Test
    public void testStartExposed03() throws Exception {
        // start the explicitly exposed activity; directed component
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startExposedIntent.setComponent(new ComponentName(
                "com.android.cts.normalapp", "com.android.cts.normalapp.ExposedActivity"));
        InstrumentationRegistry
                .getContext().startActivity(startExposedIntent, null /*options*/);
        final TestResult testResult = getResult();
        assertThat(testResult.getPackageName(),
                is("com.android.cts.normalapp"));
        assertThat(testResult.getComponentName(),
                is("ExposedActivity"));
        assertThat(testResult.getStatus(),
                is("PASS"));
        assertThat(testResult.getEphemeralPackageInfoExposed(),
                is(true));
        assertThat(testResult.getException(),
                is(nullValue()));
    }

    @Test
    public void testStartExposed04() throws Exception {
        // start the implicitly exposed activity; directed package
        try {
            final Intent startExposedIntent = new Intent(Intent.ACTION_VIEW)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startExposedIntent.setPackage("com.android.cts.implicitapp");
            startExposedIntent.addCategory(Intent.CATEGORY_BROWSABLE);
            startExposedIntent.setData(Uri.parse("https://cts.google.com/implicit"));
            InstrumentationRegistry
                    .getContext().startActivity(startExposedIntent, null /*options*/);
            fail("activity started");
        } catch (ActivityNotFoundException expected) { }
    }

    @Test
    public void testStartExposed05() throws Exception {
        // start the implicitly exposed activity; directed component
        try {
            final Intent startExposedIntent = new Intent(Intent.ACTION_VIEW)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startExposedIntent.setComponent(new ComponentName(
                    "com.android.cts.implicitapp",
                    "com.android.cts.implicitapp.ImplicitActivity"));
            startExposedIntent.addCategory(Intent.CATEGORY_BROWSABLE);
            startExposedIntent.setData(Uri.parse("https://cts.google.com/implicit"));
            InstrumentationRegistry
                    .getContext().startActivity(startExposedIntent, null /*options*/);
            fail("activity started");
        } catch (ActivityNotFoundException expected) { }
    }

    @Test
    public void testStartExposed06() throws Exception {
        // start the exposed service; directed package
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startExposedIntent.setPackage("com.android.cts.normalapp");
        InstrumentationRegistry.getContext().startForegroundService(startExposedIntent);
        final TestResult testResult = getResult();
        assertThat(testResult.getPackageName(),
                is("com.android.cts.normalapp"));
        assertThat(testResult.getComponentName(),
                is("ExposedService"));
        assertThat(testResult.getStatus(),
                is("PASS"));
        assertThat(testResult.getEphemeralPackageInfoExposed(),
                is(true));
        assertThat(testResult.getException(),
                is(nullValue()));
    }

    @Test
    public void testStartExposed07() throws Exception {
        // start the exposed service; directed component
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED);
        startExposedIntent.setComponent(new ComponentName(
                "com.android.cts.normalapp", "com.android.cts.normalapp.ExposedService"));
        InstrumentationRegistry.getContext().startForegroundService(startExposedIntent);
        final TestResult testResult = getResult();
        assertThat(testResult.getPackageName(),
                is("com.android.cts.normalapp"));
        assertThat(testResult.getComponentName(),
                is("ExposedService"));
        assertThat(testResult.getMethodName(),
                is("onStartCommand"));
        assertThat(testResult.getStatus(),
                is("PASS"));
        assertThat(testResult.getEphemeralPackageInfoExposed(),
                is(true));
        assertThat(testResult.getException(),
                is(nullValue()));
    }

    @Test
    public void testStartExposed08() throws Exception {
        // bind to the exposed service; directed package
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED);
        startExposedIntent.setPackage("com.android.cts.normalapp");
        final TestServiceConnection connection = new TestServiceConnection();
        try {
            assertThat(InstrumentationRegistry.getContext().bindService(
                    startExposedIntent, connection, Context.BIND_AUTO_CREATE /*flags*/),
                    is(true));
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.normalapp"));
            assertThat(testResult.getComponentName(),
                    is("ExposedService"));
            assertThat(testResult.getMethodName(),
                    is("onBind"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getEphemeralPackageInfoExposed(),
                    is(true));
            assertThat(testResult.getException(),
                    is(nullValue()));
        } finally {
            InstrumentationRegistry.getContext().unbindService(connection);
        }
    }

    @Test
    public void testStartExposed09() throws Exception {
        // bind to the exposed service; directed component
        final Intent startExposedIntent = new Intent(ACTION_START_EXPOSED);
        startExposedIntent.setComponent(new ComponentName(
                "com.android.cts.normalapp", "com.android.cts.normalapp.ExposedService"));
        final TestServiceConnection connection = new TestServiceConnection();
        try {
            assertThat(InstrumentationRegistry.getContext().bindService(
                    startExposedIntent, connection, Context.BIND_AUTO_CREATE /*flags*/),
                    is(true));
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.normalapp"));
            assertThat(testResult.getComponentName(),
                    is("ExposedService"));
            assertThat(testResult.getMethodName(),
                    is("onBind"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getEphemeralPackageInfoExposed(),
                    is(true));
            assertThat(testResult.getException(),
                    is(nullValue()));
        } finally {
            InstrumentationRegistry.getContext().unbindService(connection);
        }
    }

    @Test
    public void testStartExposed10() throws Exception {
        // connect to exposed provider
        final String provider = "content://com.android.cts.normalapp.exposed.provider/table";
        final Cursor testCursor = InstrumentationRegistry
                .getContext().getContentResolver().query(
                        Uri.parse(provider),
                        null /*projection*/,
                        null /*selection*/,
                        null /*selectionArgs*/,
                        null /*sortOrder*/);
        assertThat(testCursor, is(notNullValue()));
        assertThat(testCursor.getCount(), is(1));
        assertThat(testCursor.getColumnCount(), is(2));
        assertThat(testCursor.moveToFirst(), is(true));
        assertThat(testCursor.getInt(0), is(1));
        assertThat(testCursor.getString(1), is("ExposedProvider"));
        final TestResult testResult = getResult();
        assertThat(testResult.getPackageName(),
                is("com.android.cts.normalapp"));
        assertThat(testResult.getComponentName(),
                is("ExposedProvider"));
        assertThat(testResult.getStatus(),
                is("PASS"));
        assertThat(testResult.getEphemeralPackageInfoExposed(),
                is(true));
        assertThat(testResult.getException(),
                is(nullValue()));
    }

    @Test
    public void testStartEphemeral() throws Exception {
        // start the ephemeral activity
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start the ephemeral activity; directed package
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startEphemeralIntent.setPackage("com.android.cts.ephemeralapp1");
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start the ephemeral activity; directed component
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startEphemeralIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralActivity"));
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start a private ephemeral activity
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL_PRIVATE)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity2"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start a private ephemeral activity; directed package
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL_PRIVATE)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startEphemeralIntent.setPackage("com.android.cts.ephemeralapp1");
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity2"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start a private ephemeral activity; directed component
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL_PRIVATE)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startEphemeralIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralActivity2"));
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity2"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start a private ephemeral activity; directed component
        {
            final Intent startEphemeralIntent = new Intent()
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startEphemeralIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralActivity3"));
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp1"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity3"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start an ephemeral activity; VIEW / BROWSABLE intent
        {
            final Intent startEphemeralIntent = new Intent(Intent.ACTION_VIEW)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startEphemeralIntent.addCategory(Intent.CATEGORY_BROWSABLE);
            startEphemeralIntent.setData(Uri.parse("https://cts.google.com/other"));
            InstrumentationRegistry
                    .getContext().startActivity(startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp2"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity"));
            assertThat(testResult.getIntent().getAction(),
                    is(Intent.ACTION_VIEW));
            assertThat(testResult.getIntent().getCategories(),
                    hasItems(Intent.CATEGORY_BROWSABLE));
            assertThat(testResult.getIntent().getData().toString(),
                    is("https://cts.google.com/other"));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start an ephemeral activity; EXTERNAL flag
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL_ACTIVITY)
                    .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MATCH_EXTERNAL);
            InstrumentationRegistry.getContext().startActivity(
                    startEphemeralIntent, null /*options*/);
            final TestResult testResult = getResult();
            assertThat(testResult.getPackageName(),
                    is("com.android.cts.ephemeralapp2"));
            assertThat(testResult.getComponentName(),
                    is("EphemeralActivity"));
            assertThat(testResult.getIntent().getAction(),
                    is(ACTION_START_EPHEMERAL_ACTIVITY));
            assertThat(testResult.getIntent().getData(),
                    is(nullValue()));
            assertThat(testResult.getStatus(),
                    is("PASS"));
            assertThat(testResult.getException(),
                    is(nullValue()));
        }

        // start the ephemeral service; directed package
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL);
            startEphemeralIntent.setPackage("com.android.cts.ephemeralapp1");
            try {
                InstrumentationRegistry.getContext().startService(startEphemeralIntent);
                final TestResult testResult = getResult();
                assertThat(testResult.getPackageName(),
                        is("com.android.cts.ephemeralapp1"));
                assertThat(testResult.getComponentName(),
                        is("EphemeralService"));
                assertThat(testResult.getMethodName(),
                        is("onStartCommand"));
                assertThat(testResult.getStatus(),
                        is("PASS"));
                assertThat(testResult.getException(),
                        is(nullValue()));
            } finally {
                InstrumentationRegistry.getContext().stopService(startEphemeralIntent);
            }
        }

        // start the ephemeral service; directed component
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL);
            startEphemeralIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralService"));
            try {
                assertThat(InstrumentationRegistry.getContext().startService(startEphemeralIntent),
                        is(notNullValue()));
                final TestResult testResult = getResult();
                assertThat(testResult.getPackageName(),
                        is("com.android.cts.ephemeralapp1"));
                assertThat(testResult.getComponentName(),
                        is("EphemeralService"));
                assertThat(testResult.getMethodName(),
                        is("onStartCommand"));
                assertThat(testResult.getStatus(),
                        is("PASS"));
                assertThat(testResult.getException(),
                        is(nullValue()));
            } finally {
                InstrumentationRegistry.getContext().stopService(startEphemeralIntent);
            }
        }

        // bind to the ephemeral service; directed package
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL);
            startEphemeralIntent.setPackage("com.android.cts.ephemeralapp1");
            final TestServiceConnection connection = new TestServiceConnection();
            try {
                assertThat(InstrumentationRegistry.getContext().bindService(
                        startEphemeralIntent, connection, Context.BIND_AUTO_CREATE /*flags*/),
                        is(true));
                final TestResult testResult = getResult();
                assertThat(testResult.getPackageName(),
                        is("com.android.cts.ephemeralapp1"));
                assertThat(testResult.getComponentName(),
                        is("EphemeralService"));
                assertThat(testResult.getMethodName(),
                        is("onBind"));
                assertThat(testResult.getStatus(),
                        is("PASS"));
                assertThat(testResult.getException(),
                        is(nullValue()));
            } finally {
                InstrumentationRegistry.getContext().unbindService(connection);
            }
        }

        // bind to the ephemeral service; directed component
        {
            final Intent startEphemeralIntent = new Intent(ACTION_START_EPHEMERAL);
            startEphemeralIntent.setComponent(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralService"));
            final TestServiceConnection connection = new TestServiceConnection();
            try {
                assertThat(InstrumentationRegistry.getContext().bindService(
                        startEphemeralIntent, connection, Context.BIND_AUTO_CREATE /*flags*/),
                        is(true));
                final TestResult testResult = getResult();
                assertThat(testResult.getPackageName(),
                        is("com.android.cts.ephemeralapp1"));
                assertThat(testResult.getComponentName(),
                        is("EphemeralService"));
                assertThat(testResult.getMethodName(),
                        is("onBind"));
                assertThat(testResult.getStatus(),
                        is("PASS"));
                assertThat(testResult.getException(),
                        is(nullValue()));
            } finally {
                InstrumentationRegistry.getContext().unbindService(connection);
            }
        }

        // connect to the instant app provider
        {
            final String provider = "content://com.android.cts.ephemeralapp1.provider/table";
            final Cursor testCursor = InstrumentationRegistry
                    .getContext().getContentResolver().query(
                            Uri.parse(provider),
                            null /*projection*/,
                            null /*selection*/,
                            null /*selectionArgs*/,
                            null /*sortOrder*/);
            assertThat(testCursor, is(notNullValue()));
            assertThat(testCursor.getCount(), is(1));
            assertThat(testCursor.getColumnCount(), is(2));
            assertThat(testCursor.moveToFirst(), is(true));
            assertThat(testCursor.getInt(0), is(1));
            assertThat(testCursor.getString(1), is("InstantAppProvider"));
        }
    }

    @Test
    public void testBuildSerialUnknown() throws Exception {
        assertThat(Build.SERIAL, is(Build.UNKNOWN));
    }

    @Test
    public void testPackageInfo() throws Exception {
        PackageInfo info;
        // own package info
        info = InstrumentationRegistry.getContext().getPackageManager()
                .getPackageInfo("com.android.cts.ephemeralapp1", 0);
        assertThat(info.packageName,
                is("com.android.cts.ephemeralapp1"));

        // exposed application package info
        info = InstrumentationRegistry.getContext().getPackageManager()
                .getPackageInfo("com.android.cts.normalapp", 0);
        assertThat(info.packageName,
                is("com.android.cts.normalapp"));

        // implicitly exposed application package info not accessible
        try {
            info = InstrumentationRegistry.getContext().getPackageManager()
                    .getPackageInfo("com.android.cts.implicitapp", 0);
            fail("Instant apps should not be able to access PackageInfo for an app that does not" +
                    " expose itself to Instant Apps.");
        } catch (PackageManager.NameNotFoundException expected) {
        }

        // unexposed application package info not accessible
        try {
            info = InstrumentationRegistry.getContext().getPackageManager()
                    .getPackageInfo("com.android.cts.unexposedapp", 0);
            fail("Instant apps should not be able to access PackageInfo for an app that does not" +
                    " expose itself to Instant Apps.");
        } catch (PackageManager.NameNotFoundException expected) {
        }

        // instant application (with visibleToInstantApp component) package info not accessible
        try {
            info = InstrumentationRegistry.getContext().getPackageManager()
                    .getPackageInfo("com.android.cts.ephemeralapp2", 0);
            fail("Instant apps should not be able to access PackageInfo for another Instant App.");
        } catch (PackageManager.NameNotFoundException expected) {
        }
    }

    @Test
    public void testActivityInfo() throws Exception {
        // own package info
        {
            final ComponentName component = new ComponentName(
                    "com.android.cts.ephemeralapp1",
                    "com.android.cts.ephemeralapp1.EphemeralActivity");
            final ActivityInfo info = InstrumentationRegistry.getContext().getPackageManager()
                .getActivityInfo(component, 0);
            assertThat(info.packageName,
                    is("com.android.cts.ephemeralapp1"));
        }

        // exposed application package info
        {
            final ComponentName component = new ComponentName(
                    "com.android.cts.normalapp",
                    "com.android.cts.normalapp.ExposedActivity");
            final ActivityInfo info = InstrumentationRegistry.getContext().getPackageManager()
                .getActivityInfo(component, 0);
            assertThat(info.packageName,
                    is("com.android.cts.normalapp"));
        }

        // implicitly exposed application package info not accessible
        {
            try {
                final ComponentName component = new ComponentName(
                        "com.android.cts.normalapp",
                        "com.android.cts.normalapp.NormalWebActivity");
                final ActivityInfo info = InstrumentationRegistry.getContext().getPackageManager()
                        .getActivityInfo(component, 0);
                fail("Instant apps should not be able to access ActivityInfo for"
                        + " an activity that implicitly exposes itself to Instant Apps.");
            } catch (PackageManager.NameNotFoundException expected) {
            }
        }

        // unexposed application package info not accessible
        {
            try {
                final ComponentName component = new ComponentName(
                        "com.android.cts.normalapp",
                        "com.android.cts.normalapp.NormalActivity");
                final ActivityInfo info = InstrumentationRegistry.getContext().getPackageManager()
                        .getActivityInfo(component, 0);
                fail("Instant apps should not be able to access ActivityInfo for"
                        + " an activity that does not expose itself to Instant Apps.");
            } catch (PackageManager.NameNotFoundException expected) {
            }
        }

        // instant application (with visibleToInstantApp component) package info not accessible
        {
            try {
                final ComponentName component = new ComponentName(
                        "com.android.cts.ephemeralapp2",
                        "com.android.cts.ephemeralapp2.ExposedActivity");
                final ActivityInfo info = InstrumentationRegistry.getContext().getPackageManager()
                        .getActivityInfo(component, 0);
                fail("Instant apps should not be able to access ActivityInfo for"
                        + " another Instant App.");
            } catch (PackageManager.NameNotFoundException expected) {
            }
        }
    }

    @Test
    public void testInstallPermissionNotGranted() throws Exception {
        assertThat(InstrumentationRegistry.getContext()
                    .checkCallingOrSelfPermission(Manifest.permission.SET_ALARM),
                is(PackageManager.PERMISSION_DENIED));
    }

    @Test
    public void testInstallPermissionGranted() throws Exception {
        assertThat(InstrumentationRegistry.getContext()
                    .checkCallingOrSelfPermission(Manifest.permission.INTERNET),
                is(PackageManager.PERMISSION_GRANTED));
    }

    @Test
    public void testExposedActivity() throws Exception {
        final Bundle testArgs = InstrumentationRegistry.getArguments();
        assertThat(testArgs, is(notNullValue()));
        final String action = testArgs.getString("action");
        final String category = testArgs.getString("category");
        final String mimeType = testArgs.getString("mime_type");
        assertThat(action, is(notNullValue()));
        final Intent queryIntent = makeIntent(action, category, mimeType);
        final List<ResolveInfo> resolveInfo = InstrumentationRegistry.getContext()
                .getPackageManager().queryIntentActivities(queryIntent, 0 /*flags*/);
        if (resolveInfo == null || resolveInfo.size() == 0) {
            fail("No activies found for Intent: " + queryIntent);
        }
    }

    @Test
    public void testGetInstaller01() throws Exception {
        // test we can read our own installer
        final String installerPackageName = InstrumentationRegistry.getContext().getPackageManager()
                .getInstallerPackageName("com.android.cts.ephemeralapp1");
        assertThat(installerPackageName, is("com.android.cts.normalapp"));
    }
    @Test
    public void testGetInstaller02() throws Exception {
        // test we can read someone else's installer if they're exposed to instant applications
        final String installerPackageName = InstrumentationRegistry.getContext().getPackageManager()
                .getInstallerPackageName("com.android.cts.normalapp");
        assertThat(installerPackageName, is("com.android.cts.normalapp"));
    }
    @Test
    public void testGetInstaller03() throws Exception {
        // test we can't read installer if they're not exposed to instant applications
        final String installerPackageName = InstrumentationRegistry.getContext().getPackageManager()
                .getInstallerPackageName("com.android.cts.unexposedapp");
        assertThat(installerPackageName, is(nullValue()));
    }

    @Test
    public void testStartForegroundService() throws Exception {
        final Context context = InstrumentationRegistry.getContext();
        final Intent intent = new Intent(context, SomeService.class);

        // Create a notification channel for the foreground notification
        final NotificationChannel channel = new NotificationChannel("foo", "foo",
                NotificationManager.IMPORTANCE_DEFAULT);
        final NotificationManager notificationManager = context.getSystemService(
                NotificationManager.class);
        notificationManager.createNotificationChannel(channel);

        // Shouldn't be able to start without a permission
        final CountDownLatch latch1 = new CountDownLatch(1);
        SomeService.setOnStartCommandCallback((int result) -> {
            assertSame("Shouldn't be able to start without "
                    + " INSTANT_APP_FOREGROUND_SERVICE permission", 0, result);
            latch1.countDown();
        });
        context.startForegroundService(intent);
        latch1.await(5, TimeUnit.SECONDS);

        // Now grant ourselves INSTANT_APP_FOREGROUND_SERVICE
        grantInstantAppForegroundServicePermission();

        // Should be able to start with a permission
        final CountDownLatch latch2 = new CountDownLatch(1);
        SomeService.setOnStartCommandCallback((int result) -> {
            assertSame("Should be able to start with "
                    + " INSTANT_APP_FOREGROUND_SERVICE permission", 1, result);
            latch2.countDown();
        });
        context.startForegroundService(intent);
        latch2.await(5, TimeUnit.SECONDS);
    }

    @Test
    public void testRecordAudioPermission() throws Throwable {
        final AudioRecord record =
                new AudioRecord(MIC, 8000, CHANNEL_IN_MONO, ENCODING_PCM_16BIT, 4096);
        try {
            assertThat("audio record not initialized",
                    record.getState(), is(AudioRecord.STATE_INITIALIZED));
        } finally {
            record.release();
        }
    }

    @Test
    public void testReadPhoneNumbersPermission() throws Throwable {
        final Context context = InstrumentationRegistry.getContext();
        if (!context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            return;
        }

        try {
            final TelephonyManager telephonyManager =
                    (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            final String nmbr = telephonyManager.getLine1Number();
        } catch (SecurityException e) {
            fail("Permission not granted");
        }
    }

    @Test
    public void testAccessCoarseLocationPermission() {
        final Context context = InstrumentationRegistry.getContext();

        final LocationManager locationManager =
                (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);

        final Criteria criteria = new Criteria();
        criteria.setAccuracy(Criteria.ACCURACY_COARSE);
        final String bestProvider = locationManager.getBestProvider(criteria, false);
        try {
            final String provider =
                    bestProvider == null ? LocationManager.NETWORK_PROVIDER : bestProvider;
            locationManager.getLastKnownLocation(provider);
        } catch (SecurityException e) {
            fail("Permission not granted.");
        }
    }

    @Test
    public void testCameraPermission() throws Throwable {
        final Context context = InstrumentationRegistry.getContext();
        final CameraManager manager =
                (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
        final String[] cameraIds = manager.getCameraIdList();
        if (cameraIds.length == 0) {
            return;
        }
        final CountDownLatch latch = new CountDownLatch(1);
        final HandlerThread backgroundThread = new HandlerThread("camera_bg");
        backgroundThread.start();
        final CameraDevice.StateCallback callback = new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice camera) {
                latch.countDown();
                camera.close();
            }
            @Override
            public void onDisconnected(CameraDevice camera) {
                camera.close();
            }
            @Override
            public void onError(CameraDevice camera, int error) {
                camera.close();
            }
        };
        manager.openCamera(cameraIds[0], callback, new Handler(backgroundThread.getLooper()));
        assertThat(latch.await(1000, TimeUnit.MILLISECONDS), is(true));
    }

    @Test
    public void testInternetPermission() throws Throwable {
        final ConnectivityManager manager = (ConnectivityManager) InstrumentationRegistry.getContext()
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        manager.reportNetworkConnectivity(null, false);
    }

    @Test
    public void testVibratePermission() throws Throwable {
        final Vibrator vibrator = (Vibrator) InstrumentationRegistry.getContext()
                .getSystemService(Context.VIBRATOR_SERVICE);
        final VibrationEffect effect =
                VibrationEffect.createOneShot(100, VibrationEffect.DEFAULT_AMPLITUDE);
        vibrator.vibrate(effect);
    }

    @Test
    public void testWakeLockPermission() throws Throwable {
        WakeLock wakeLock = null;
        try {
            final PowerManager powerManager = (PowerManager) InstrumentationRegistry.getContext()
                    .getSystemService(Context.POWER_SERVICE);
            wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "test");
            wakeLock.acquire();
        }
        finally {
            if (wakeLock != null &&  wakeLock.isHeld()) {
                wakeLock.release();
            }
        }
    }

    @Test
    public void testGetSearchableInfo() throws Throwable {
        final SearchManager searchManager = (SearchManager) InstrumentationRegistry.getContext()
                .getSystemService(Context.SEARCH_SERVICE);
        if (searchManager == null) {
            return;
        }
        // get searchable info for a component in ourself; pass
        {
            final SearchableInfo info = searchManager.getSearchableInfo(
                    new ComponentName("com.android.cts.ephemeralapp1",
                            "com.android.cts.ephemeralapp1.EphemeralActivity"));
            assertThat(info, is(notNullValue()));
            assertThat(info.getSearchActivity(),
                    is(equalTo(
                            new ComponentName("com.android.cts.ephemeralapp1",
                                    "com.android.cts.ephemeralapp1.EphemeralActivity"))));
        }

        // get searchable info for a component in a different instant application; fail
        {
            final SearchableInfo info = searchManager.getSearchableInfo(
                    new ComponentName("com.android.cts.ephemeralapp2",
                            "com.android.cts.ephemeralapp2.EphemeralActivity"));
            assertThat(info, is(nullValue()));
        }

        // get searchable info for an exposed in a full application; pass
        {
            final SearchableInfo info = searchManager.getSearchableInfo(
                    new ComponentName("com.android.cts.normalapp",
                            "com.android.cts.normalapp.ExposedActivity"));
            assertThat(info, is(notNullValue()));
            assertThat(info.getSearchActivity(),
                    is(equalTo(
                            new ComponentName("com.android.cts.normalapp",
                                    "com.android.cts.normalapp.ExposedActivity"))));
        }

        // get searchable info for an unexposed component in a full application; fail
        {
            final SearchableInfo info = searchManager.getSearchableInfo(
                    new ComponentName("com.android.cts.normalapp",
                            "com.android.cts.normalapp.NormalActivity"));
            assertThat(info, is(nullValue()));
        }
    }

    /** Returns {@code true} if the given filter handles all web URLs, regardless of host. */
    private boolean handlesAllWebData(IntentFilter filter) {
        return filter.hasCategory(Intent.CATEGORY_APP_BROWSER) ||
                (handlesWebUris(filter) && filter.countDataAuthorities() == 0);
    }

    /** Returns {@code true} if the given filter handles at least one web URL. */
    private boolean handlesWebUris(IntentFilter filter) {
        // Require ACTION_VIEW, CATEGORY_BROWSEABLE, and at least one scheme
        if (!filter.hasAction(Intent.ACTION_VIEW)
            || !filter.hasCategory(Intent.CATEGORY_BROWSABLE)
            || filter.countDataSchemes() == 0) {
            return false;
        }
        // Now allow only the schemes "http" and "https"
        final Iterator<String> schemesIterator = filter.schemesIterator();
        while (schemesIterator.hasNext()) {
            final String scheme = schemesIterator.next();
            final boolean isWebScheme = "http".equals(scheme) || "https".equals(scheme);
            if (isWebScheme) {
                return true;
            }
        }
        return false;
    }

    private TestResult getResult() {
        final TestResult result;
        try {
            result = mResultQueue.poll(5, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
        if (result == null) {
            throw new IllegalStateException("Activity didn't receive a Result in 5 seconds");
        }
        return result;
    }

    private static void grantInstantAppForegroundServicePermission() throws IOException {
        InstrumentationRegistry.getInstrumentation().getUiAutomation().grantRuntimePermission(
                InstrumentationRegistry.getContext().getPackageName(),
                android.Manifest.permission.INSTANT_APP_FOREGROUND_SERVICE);
    }

    private static Intent makeIntent(String action, String category, String mimeType) {
        Intent intent = new Intent(action);
        if (category != null) {
            intent.addCategory(category);
        }
        if (mimeType != null) {
            intent.setType(mimeType);
        }
        return intent;
    }

    private static class ActivityBroadcastReceiver extends BroadcastReceiver {
        private final SynchronousQueue<TestResult> mQueue;
        public ActivityBroadcastReceiver(SynchronousQueue<TestResult> queue) {
            mQueue = queue;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                mQueue.offer(intent.getParcelableExtra(TestResult.EXTRA_TEST_RESULT),
                        5, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private static class TestServiceConnection implements ServiceConnection {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
        }
        @Override
        public void onServiceDisconnected(ComponentName name) {
        }
    }
}
