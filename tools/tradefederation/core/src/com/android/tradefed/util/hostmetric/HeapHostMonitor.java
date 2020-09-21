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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.log.ILogRegistry.EventType;

import com.google.common.annotations.VisibleForTesting;

import java.util.HashMap;
import java.util.Map;

import com.android.tradefed.log.LogRegistry;

/**
 * {@link AbstractHostMonitor} implementation that monitors the heap memory on the host and log it
 * periodically to the history log.
 */
public class HeapHostMonitor extends AbstractHostMonitor {

    protected static final String HEAP_KEY = "heap_memory_Mbytes";

    public HeapHostMonitor() {
        super();
        setName("HeapHostMonitor");
    }

    /** {@inheritDoc} */
    @Override
    public void dispatch() {
        // This host monitor does not care about events, so we flush them out.
        mHostEvents.clear();
        // Collect the current JVM memory usage in bytes
        Runtime rt = Runtime.getRuntime();
        long totalJvmMemory = (rt.totalMemory() - rt.freeMemory()) / (1024l * 1024l);
        Map<String, String> args = new HashMap<>();
        args.put(HEAP_KEY, Long.toString(totalJvmMemory));
        logEvent(args);
    }

    /** Log the event to the history log. */
    @VisibleForTesting
    void logEvent(Map<String, String> args) {
        LogRegistry.getLogRegistry().logEvent(LogLevel.INFO, EventType.HEAP_MEMORY, args);
    }
}
