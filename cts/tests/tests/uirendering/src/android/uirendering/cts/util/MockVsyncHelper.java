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

package android.uirendering.cts.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.view.animation.AnimationUtils;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class MockVsyncHelper {

    private static ExecutorService sExecutor = Executors.newSingleThreadExecutor();
    private static Future<Thread> sExecutorThread;

    static {
        // We can't wait on the future here because the lambda cannot be executed until
        // the class has finished loading
        sExecutorThread = sExecutor.submit(() -> {
            AnimationUtils.lockAnimationClock(16);
            return Thread.currentThread();
        });
    }

    private static boolean isOnExecutorThread() {
        try {
            return Thread.currentThread().equals(sExecutorThread.get());
        } catch (InterruptedException | ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    public static void nextFrame() {
        assertTrue("nextFrame() must be called inside #unOnVsyncThread block",
                isOnExecutorThread());
        AnimationUtils.lockAnimationClock(AnimationUtils.currentAnimationTimeMillis() + 16);
    }

    public static void runOnVsyncThread(CallableVoid callable) {
        assertFalse("Cannot runOnVsyncThread inside #runOnVsyncThread block",
                isOnExecutorThread());
        try {
            sExecutor.submit(() -> {
                callable.call();
                return (Void) null;
            }).get();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } catch (ExecutionException e) {
            SneakyThrow.sneakyThrow(e.getCause() != null ? e.getCause() : e);
        }
    }

    public interface CallableVoid {
        void call() throws Exception;
    }
}
