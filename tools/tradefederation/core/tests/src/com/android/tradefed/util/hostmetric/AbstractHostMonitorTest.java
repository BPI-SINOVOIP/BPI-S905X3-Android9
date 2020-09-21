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

import com.android.tradefed.util.hostmetric.IHostMonitor.HostDataPoint;
import com.android.tradefed.util.hostmetric.IHostMonitor.HostMetricType;

import junit.framework.TestCase;

/** Unit tests for {@link AbstractHostMonitor}. */
public class AbstractHostMonitorTest extends TestCase {

    AbstractHostMonitor mHostMonitor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mHostMonitor =
                new AbstractHostMonitor() {
                    @Override
                    public void dispatch() {
                        // ignore
                    }
                };
    }

    /**
     * Test {@link AbstractHostMonitor#addHostEvent(HostMetricType, HostDataPoint)} when the event
     * is properly added.
     */
    public void testaddHostEvent() {
        assertTrue(mHostMonitor.getQueueSize() == 0);
        HostDataPoint fakeDataPoint = new HostDataPoint("test", 5);
        mHostMonitor.addHostEvent(mHostMonitor.getTag(), fakeDataPoint);
        assertTrue(mHostMonitor.getQueueSize() == 1);
        mHostMonitor.addHostEvent(mHostMonitor.getTag(), fakeDataPoint);
        assertTrue(mHostMonitor.getQueueSize() == 2);
    }

    /**
     * Test {@link AbstractHostMonitor#addHostEvent(HostMetricType, HostDataPoint)} when the event
     * has a different tag than the Monitor, it should not be added.
     */
    public void testaddHostEvent_differentTag() {
        assertTrue(mHostMonitor.getQueueSize() == 0);
        HostDataPoint fakeDataPoint = new HostDataPoint("test", 5);
        // expected NONE key for hostmonitor
        mHostMonitor.addHostEvent(HostMetricType.INVOCATION_STRAY_THREAD, fakeDataPoint);
        assertTrue(mHostMonitor.getQueueSize() == 0);
        mHostMonitor.addHostEvent(null, fakeDataPoint);
        assertTrue(mHostMonitor.getQueueSize() == 0);
    }
}
