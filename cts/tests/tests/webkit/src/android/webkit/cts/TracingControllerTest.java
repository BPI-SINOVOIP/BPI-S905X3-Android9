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

package android.webkit.cts;

import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.webkit.TracingConfig;
import android.webkit.TracingController;
import android.webkit.WebView;
import android.webkit.cts.WebViewOnUiThread.WaitForLoadedClient;

import com.android.compatibility.common.util.NullWebViewUtils;
import com.android.compatibility.common.util.PollingCheck;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

public class TracingControllerTest extends ActivityInstrumentationTestCase2<WebViewCtsActivity> {

    public static class TracingReceiver extends OutputStream {
        private int mChunkCount;
        private boolean mComplete;
        private ByteArrayOutputStream outputStream;

        public TracingReceiver() {
            outputStream = new ByteArrayOutputStream();
        }

        @Override
        public void write(byte[] chunk) {
            validateThread();
            mChunkCount++;
            try {
                outputStream.write(chunk);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        @Override
        public void close() {
            validateThread();
            mComplete = true;
        }

        @Override
        public void flush() {
            fail("flush should not be called");
        }

        @Override
        public void write(int b) {
            fail("write(int) should not be called");
        }

        @Override
        public void write(byte[] b, int off, int len) {
            fail("write(byte[], int, int) should not be called");
        }

        private void validateThread() {
            // Ensure the callbacks are called on the correct (executor) thread.
            assertTrue(Thread.currentThread().getName().startsWith(EXECUTOR_THREAD_PREFIX));
        }

        int getNbChunks() { return mChunkCount; }
        boolean getComplete() { return mComplete; }

        Callable<Boolean> getCompleteCallable() {
            return new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    return getComplete();
                }
            };
        }

        ByteArrayOutputStream getOutputStream() { return outputStream; }
    }

    private static final int POLLING_TIMEOUT = 60 * 1000;
    private static final int EXECUTOR_TIMEOUT = 10; // timeout of executor shutdown in seconds
    private static final String EXECUTOR_THREAD_PREFIX = "TracingExecutorThread";
    private WebViewOnUiThread mOnUiThread;
    private ExecutorService singleThreadExecutor;

    public TracingControllerTest() throws Exception {
        super("android.webkit.cts", WebViewCtsActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        WebView webview = getActivity().getWebView();
        if (webview == null) return;
        mOnUiThread = new WebViewOnUiThread(this, webview);
        singleThreadExecutor = Executors.newSingleThreadExecutor(getCustomThreadFactory());
    }

    @Override
    protected void tearDown() throws Exception {
        // make sure to stop everything and clean up
        ensureTracingStopped();
        if (singleThreadExecutor != null) {
            singleThreadExecutor.shutdown();
            if (!singleThreadExecutor.awaitTermination(EXECUTOR_TIMEOUT, TimeUnit.SECONDS)) {
                fail("Failed to shutdown executor");
            }
        }
        if (mOnUiThread != null) {
            mOnUiThread.cleanUp();
        }
        super.tearDown();
    }

    private void ensureTracingStopped() throws Exception {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        TracingController.getInstance().stop(null, singleThreadExecutor);
        Callable<Boolean> tracingStopped = new Callable<Boolean>() {
            @Override
            public Boolean call() {
                return !TracingController.getInstance().isTracing();
            }
        };
        PollingCheck.check("Tracing did not stop", POLLING_TIMEOUT, tracingStopped);
    }

    private ThreadFactory getCustomThreadFactory() {
        return new ThreadFactory() {
            private final AtomicInteger threadCount = new AtomicInteger(0);
            @Override
            public Thread newThread(Runnable r) {
                Thread thread = new Thread(r);
                thread.setName(EXECUTOR_THREAD_PREFIX + "_" + threadCount.incrementAndGet());
                return thread;
            }
        };
    }

    // Test that callbacks are invoked and tracing data is returned on the correct thread
    // (via executor). Tracing start/stop and webview loading happens on the UI thread.
    public void testTracingControllerCallbacksOnUI() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }
        final TracingReceiver tracingReceiver = new TracingReceiver();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                runTracingTestWithCallbacks(tracingReceiver, singleThreadExecutor);
            }
        });
        PollingCheck.check("Tracing did not complete", POLLING_TIMEOUT, tracingReceiver.getCompleteCallable());
        assertTrue(tracingReceiver.getNbChunks() > 0);
        assertTrue(tracingReceiver.getOutputStream().size() > 0);
        // currently the output is json (as of April 2018), but this could change in the future
        // so we don't explicitly test the contents of output stream.
    }

    // Test that callbacks are invoked and tracing data is returned on the correct thread
    // (via executor). Tracing start/stop happens on the testing thread; webview loading
    // happens on the UI thread.
    public void testTracingControllerCallbacks() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        final TracingReceiver tracingReceiver = new TracingReceiver();
        runTracingTestWithCallbacks(tracingReceiver, singleThreadExecutor);
        PollingCheck.check("Tracing did not complete", POLLING_TIMEOUT, tracingReceiver.getCompleteCallable());
        assertTrue(tracingReceiver.getNbChunks() > 0);
        assertTrue(tracingReceiver.getOutputStream().size() > 0);
    }

    // Test that tracing stop has no effect if tracing has not been started.
    public void testTracingStopFalseIfNotTracing() {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        TracingController tracingController = TracingController.getInstance();
        assertFalse(tracingController.stop(null, singleThreadExecutor));
        assertFalse(tracingController.isTracing());
    }

    // Test that tracing cannot be started if already tracing.
    public void testTracingCannotStartIfAlreadyTracing() throws Exception {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        TracingController tracingController = TracingController.getInstance();
        TracingConfig config = new TracingConfig.Builder().build();

        tracingController.start(config);
        assertTrue(tracingController.isTracing());
        try {
            tracingController.start(config);
        } catch (IllegalStateException e) {
            // as expected
            return;
        }
        assertTrue(tracingController.stop(null, singleThreadExecutor));
        fail("Tracing start should throw an exception when attempting to start while already tracing");
    }

    // Test that tracing cannot be invoked with excluded categories.
    public void testTracingInvalidCategoriesPatternExclusion() {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        TracingController tracingController = TracingController.getInstance();
        TracingConfig config = new TracingConfig.Builder()
                .addCategories("android_webview","-blink")
                .build();
        try {
            tracingController.start(config);
        } catch (IllegalArgumentException e) {
            // as expected;
            assertFalse(tracingController.isTracing());
            return;
        }

        fail("Tracing start should throw an exception due to invalid category pattern");
    }

    // Test that tracing cannot be invoked with categories containing commas.
    public void testTracingInvalidCategoriesPatternComma() {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        TracingController tracingController = TracingController.getInstance();
        TracingConfig config = new TracingConfig.Builder()
                .addCategories("android_webview, blink")
                .build();
        try {
            tracingController.start(config);
        } catch (IllegalArgumentException e) {
            // as expected;
            assertFalse(tracingController.isTracing());
            return;
        }

        fail("Tracing start should throw an exception due to invalid category pattern");
    }

    // Test that tracing cannot start with a configuration that is null.
    public void testTracingWithNullConfig() {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        TracingController tracingController = TracingController.getInstance();
        try {
            tracingController.start(null);
        } catch (IllegalArgumentException e) {
            // as expected
            assertFalse(tracingController.isTracing());
            return;
        }
        fail("Tracing start should throw exception if TracingConfig is null");
    }

    // Generic helper function for running tracing.
    private void runTracingTestWithCallbacks(TracingReceiver tracingReceiver, Executor executor) {
        TracingController tracingController = TracingController.getInstance();
        assertNotNull(tracingController);

        TracingConfig config = new TracingConfig.Builder()
                .addCategories(TracingConfig.CATEGORIES_WEB_DEVELOPER)
                .setTracingMode(TracingConfig.RECORD_CONTINUOUSLY)
                .build();
        assertFalse(tracingController.isTracing());
        tracingController.start(config);
        assertTrue(tracingController.isTracing());

        mOnUiThread.loadUrlAndWaitForCompletion("about:blank");
        assertTrue(tracingController.stop(tracingReceiver, executor));
    }
}

