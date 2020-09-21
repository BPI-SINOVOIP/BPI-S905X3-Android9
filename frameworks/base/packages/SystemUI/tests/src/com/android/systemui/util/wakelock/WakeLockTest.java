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

package com.android.systemui.util.wakelock;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.os.PowerManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.systemui.SysuiTestCase;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class WakeLockTest extends SysuiTestCase {

    WakeLock mWakeLock;
    PowerManager.WakeLock mInner;

    @Before
    public void setUp() {
        mInner = WakeLock.createPartialInner(mContext, WakeLockTest.class.getName());
        mWakeLock = WakeLock.wrap(mInner);
    }

    @After
    public void tearDown() {
        mInner.setReferenceCounted(false);
        mInner.release();
    }

    @Test
    public void createPartialInner_notHeldYet() {
        assertFalse(mInner.isHeld());
    }

    @Test
    public void wakeLock_acquire() {
        mWakeLock.acquire();
        assertTrue(mInner.isHeld());
    }

    @Test
    public void wakeLock_release() {
        mWakeLock.acquire();
        mWakeLock.release();
        assertFalse(mInner.isHeld());
    }

    @Test
    public void wakeLock_refCounted() {
        mWakeLock.acquire();
        mWakeLock.acquire();
        mWakeLock.release();
        assertTrue(mInner.isHeld());
    }

    @Test
    public void wakeLock_wrap() {
        boolean[] ran = new boolean[1];

        Runnable wrapped = mWakeLock.wrap(() -> {
            ran[0] = true;
        });

        assertTrue(mInner.isHeld());
        assertFalse(ran[0]);

        wrapped.run();

        assertTrue(ran[0]);
        assertFalse(mInner.isHeld());
    }
}