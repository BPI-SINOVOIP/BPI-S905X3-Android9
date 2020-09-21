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
 * limitations under the License
 */

package com.android.server.am;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.annotation.Nullable;
import android.app.ActivityOptions;
import android.os.Handler;
import android.platform.test.annotations.Presubmit;
import android.support.test.filters.FlakyTest;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.ArrayMap;
import android.view.RemoteAnimationAdapter;

import com.android.server.testutils.OffsettableClock;
import com.android.server.testutils.TestHandler;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

/**
 * atest PendingRemoteAnimationRegistryTest
 */
@SmallTest
@Presubmit
@FlakyTest
@RunWith(AndroidJUnit4.class)
public class PendingRemoteAnimationRegistryTest extends ActivityTestsBase {

    @Mock RemoteAnimationAdapter mAdapter;
    private PendingRemoteAnimationRegistry mRegistry;
    private final OffsettableClock mClock = new OffsettableClock.Stopped();
    private TestHandler mHandler;
    private ActivityManagerService mService;

    @Before
    public void setUp() throws Exception {
        super.setUp();
        MockitoAnnotations.initMocks(this);
        mService = createActivityManagerService();
        mService.mHandlerThread.getThreadHandler().runWithScissors(() -> {
            mHandler = new TestHandler(null, mClock);
        }, 0);
        mRegistry = new PendingRemoteAnimationRegistry(mService, mHandler);
    }

    @Test
    public void testOverrideActivityOptions() {
        mRegistry.addPendingAnimation("com.android.test", mAdapter);
        ActivityOptions opts = ActivityOptions.makeBasic();
        opts = mRegistry.overrideOptionsIfNeeded("com.android.test", opts);
        assertEquals(mAdapter, opts.getRemoteAnimationAdapter());
    }

    @Test
    public void testOverrideActivityOptions_null() {
        mRegistry.addPendingAnimation("com.android.test", mAdapter);
        final ActivityOptions opts = mRegistry.overrideOptionsIfNeeded("com.android.test", null);
        assertNotNull(opts);
        assertEquals(mAdapter, opts.getRemoteAnimationAdapter());
    }

    @Test
    public void testTimeout() {
        mRegistry.addPendingAnimation("com.android.test", mAdapter);
        mClock.fastForward(5000);
        mHandler.timeAdvance();
        assertNull(mRegistry.overrideOptionsIfNeeded("com.android.test", null));
    }

    @Test
    public void testTimeout_overridenEntry() {
        mRegistry.addPendingAnimation("com.android.test", mAdapter);
        mClock.fastForward(2500);
        mHandler.timeAdvance();
        mRegistry.addPendingAnimation("com.android.test", mAdapter);
        mClock.fastForward(1000);
        mHandler.timeAdvance();
        final ActivityOptions opts = mRegistry.overrideOptionsIfNeeded("com.android.test", null);
        assertEquals(mAdapter, opts.getRemoteAnimationAdapter());
    }
}
