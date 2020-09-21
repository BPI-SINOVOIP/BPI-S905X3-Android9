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
package android.content.cts;

import android.app.ActivityManager;
import android.content.Context;
import android.content.ContextWrapper;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class ContextMoreTest extends AndroidTestCase {
    private static final String TAG = "ContextMoreTest";

    /**
     * Test for {@link Context#getSystemService)}.
     *
     * Call it repeatedly from multiple threads, and:
     * - Make sure getSystemService(ActivityManager) will always return non-null.
     * - If ContextImpl.mServiceCache is accessible via reflection, clear it once in a while and
     * make sure getSystemService() still returns non-null.
     */
    @LargeTest
    public void testGetSystemService_multiThreaded() throws Exception {
        // # of times the tester Runnable has been executed.
        final AtomicInteger totalCount = new AtomicInteger(0);

        // # of times the tester Runnable has failed.
        final AtomicInteger failCount = new AtomicInteger(0);

        // Run the threads until this becomes true.
        final AtomicBoolean stop = new AtomicBoolean(false);

        final Context context = getContext();
        final Object[] serviceCache = findServiceCache(context);
        if (serviceCache == null) {
            Log.w(TAG, "mServiceCache not found.");
        }

        final Runnable tester = () -> {
            for (;;) {
                final int pass = totalCount.incrementAndGet();

                final Object service = context.getSystemService(ActivityManager.class);
                if (service == null) {
                    failCount.incrementAndGet(); // Fail!
                }

                if (stop.get()) {
                    return;
                }

                // Yield the CPU.
                try {
                    Thread.sleep(0);
                } catch (InterruptedException e) {
                }

                // Once in a while, force clear mServiceCache.
                if ((serviceCache != null) && ((pass % 7) == 0)) {
                    Arrays.fill(serviceCache, null);
                }
            }
        };

        final int NUM_THREADS = 20;

        // Create and start the threads...
        final Thread[] threads = new Thread[NUM_THREADS];
        for (int i = 0; i < NUM_THREADS; i++) {
            threads[i] = new Thread(tester);
        }
        for (int i = 0; i < NUM_THREADS; i++) {
            threads[i].start();
        }

        Thread.sleep(10 * 1000);

        stop.set(true);

        // Wait for them to stop...
        for (int i = 0; i < NUM_THREADS; i++) {
            threads[i].join();
        }

        assertEquals(0, failCount.get());
        assertTrue("totalCount must be bigger than " + NUM_THREADS
                + " but was " + totalCount.get(), totalCount.get() > NUM_THREADS);
    }

    /**
     * Find a field by name using reflection.
     */
    private static Object readField(Object instance, String fieldName) {
        final Field f;
        try {
            f = instance.getClass().getDeclaredField(fieldName);
            f.setAccessible(true);
            final Object ret = f.get(instance);
            if (ret == null) {
                return null;
            }
            return ret;
        } catch (NoSuchFieldException | IllegalAccessException e) {
            return null;
        }
    }

    /**
     * Try to find the mServiceCache field from a Context. Returns null if none found.
     */
    private static Object[] findServiceCache(Context context) {
        // Find the base context.
        while (context instanceof ContextWrapper) {
            context = ((ContextWrapper) context).getBaseContext();
        }
        // Try to find the mServiceCache field.
        final Object serviceCache = readField(context, "mServiceCache");
        if (serviceCache instanceof Object[]) {
            return (Object[]) serviceCache;
        }
        return null;
    }
}
