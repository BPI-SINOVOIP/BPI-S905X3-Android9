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
 * limitations under the License.
 */
package com.android.tradefed.util.hostmetric;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Map;

/** Unit tests for {@link HeapHostMonitor}. */
@RunWith(JUnit4.class)
public class HeapHostMonitorTest {

    private HeapHostMonitor mMonitor;
    private Map<String, String> mArgsLogged = null;

    @Before
    public void setUp() {
        mArgsLogged = null;
        mMonitor =
                new HeapHostMonitor() {
                    @Override
                    void logEvent(Map<String, String> args) {
                        mArgsLogged = args;
                    }
                };
    }

    /** Test that an event is logged with the memory key in it. */
    @Test
    public void testDispatchOfHeap() {
        assertNull(mArgsLogged);
        mMonitor.dispatch();
        assertNotNull(mArgsLogged);
        assertNotNull(mArgsLogged.get(HeapHostMonitor.HEAP_KEY));
        // make sure it's a positive number
        long mem = Long.parseLong(mArgsLogged.get(HeapHostMonitor.HEAP_KEY));
        assertTrue(mem > 0l);
    }
}
