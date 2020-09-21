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
package com.android.car;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.car.SystemActivityMonitoringService.TopTaskInfoContainer;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class SystemActivityMonitoringServiceTest {
    private static final long ACTIVITY_TIME_OUT = 5000;

    private SystemActivityMonitoringService mService;
    private Semaphore mSemaphore = new Semaphore(0);

    private final TopTaskInfoContainer[] mTopTaskInfo = new TopTaskInfoContainer[1];

    @Before
    public void setUp() throws Exception {
        mService = new SystemActivityMonitoringService(getContext());
        mService.registerActivityLaunchListener(topTask -> {
            if (!getTestContext().getPackageName().equals(topTask.topActivity.getPackageName())) {
                return; // Ignore activities outside of this test case.
            }
            synchronized (mTopTaskInfo) {
                mTopTaskInfo[0] = topTask;
            }
            mSemaphore.release();
        });
    }

    @After
    public void tearDown() throws Exception {
        mService.registerActivityLaunchListener(null);
        mService = null;
    }

    @Test
    public void testActivityLaunch() throws Exception {
        ComponentName activityA = toComponentName(getTestContext(), ActivityA.class);
        startActivity(getContext(), activityA);
        assertTopTaskActivity(activityA);

        ComponentName activityB = toComponentName(getTestContext(), ActivityB.class);
        startActivity(getContext(), activityB);
        assertTopTaskActivity(activityB);
    }

    @Test
    public void testActivityBlocking() throws Exception {
        ComponentName blackListedActivity = toComponentName(getTestContext(), ActivityC.class);
        ComponentName blockingActivity = toComponentName(getTestContext(), BlockingActivity.class);
        Intent blockingIntent = new Intent();
        blockingIntent.setComponent(blockingActivity);

        // start a black listed activity
        startActivity(getContext(), blackListedActivity);
        assertTopTaskActivity(blackListedActivity);

        // Instead of start activity, invoke blockActivity.
        mService.blockActivity(mTopTaskInfo[0], blockingIntent);
        assertTopTaskActivity(blockingActivity);
    }

    /** Activity that closes itself after some timeout to clean up the screen. */
    public static class TempActivity extends Activity {
        @Override
        protected void onResume() {
            super.onResume();
            getMainThreadHandler().postDelayed(this::finish, ACTIVITY_TIME_OUT);
        }
    }

    public static class ActivityA extends TempActivity {}
    public static class ActivityB extends TempActivity {}
    public static class ActivityC extends TempActivity {}
    public static class BlockingActivity extends TempActivity {}

    private void assertTopTaskActivity(ComponentName activity) throws Exception{
        assertTrue(mSemaphore.tryAcquire(2, TimeUnit.SECONDS));
        synchronized (mTopTaskInfo) {
            assertEquals(activity, mTopTaskInfo[0].topActivity);
        }
    }

    private Context getContext() {
        return InstrumentationRegistry.getTargetContext();
    }

    private Context getTestContext() {
        return InstrumentationRegistry.getContext();
    }

    private static ComponentName toComponentName(Context ctx, Class<?> cls) {
        return ComponentName.createRelative(ctx, cls.getName());
    }

    private static void startActivity(Context ctx, ComponentName name) {
        Intent intent = new Intent();
        intent.setComponent(name);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ctx.startActivity(intent);
    }
}
