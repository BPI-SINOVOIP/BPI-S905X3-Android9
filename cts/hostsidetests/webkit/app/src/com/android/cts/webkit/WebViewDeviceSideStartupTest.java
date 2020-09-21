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

package com.android.cts.webkit;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.http.SslError;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.StrictMode;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.util.Log;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.SslErrorHandler;
import android.webkit.WebView;
import android.webkit.cts.CtsTestServer;
import android.webkit.cts.WebViewOnUiThread;
import android.webkit.cts.WebViewOnUiThread.WaitForLoadedClient;
import android.webkit.WebView;

import com.android.compatibility.common.util.NullWebViewUtils;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Test class testing different aspects of WebView loading.
 * The test methods in this class should be run one-and-one from the host-side to ensure we
 * don't run the tests in the same process (since we can only load WebView into a process
 * once - after that we will reuse the same webview provider).
 * This works because the instrumentation used to run device-tests from the host-side terminates the
 * testing process after each run.
 * OBS! When adding a test here - remember to add a corresponding host-side test that will start the
 * device-test added here! See com.android.cts.webkit.WebViewHostSideStartupTest.
 */
public class WebViewDeviceSideStartupTest
        extends ActivityInstrumentationTestCase2<WebViewStartupCtsActivity> {

    private static final String TAG = WebViewDeviceSideStartupTest.class.getSimpleName();
    private static final long TEST_TIMEOUT_MS = 3000;

    private WebViewStartupCtsActivity mActivity;

    public WebViewDeviceSideStartupTest() {
        super("com.android.cts.webkit", WebViewStartupCtsActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
    }

    @UiThreadTest
    public void testCookieManagerBlockingUiThread() throws Throwable {
        // Instant app can only have https connection.
        CtsTestServer server = new CtsTestServer(mActivity, true);
        final String url = server.getCookieUrl("death.html");

        Thread background = new Thread(new Runnable() {
            @Override
            public void run() {
                CookieSyncManager csm = CookieSyncManager.createInstance(mActivity);
                CookieManager cookieManager = CookieManager.getInstance();

                cookieManager.removeAllCookie();
                cookieManager.setAcceptCookie(true);
                cookieManager.setCookie(url, "count=41");
                Log.i(TAG, "done setting cookie before creating webview");
            }
        });
        NullWebViewUtils.NullWebViewFromThreadExceptionHandler h =
                new NullWebViewUtils.NullWebViewFromThreadExceptionHandler();

        background.setUncaughtExceptionHandler(h);
        background.start();
        background.join();

        if (!h.isWebViewAvailable(mActivity)) {
            return;
        }

        // Now create WebView and test that setting the cookie beforehand really worked.
        mActivity.createAndAttachWebView();
        WebView webView = mActivity.getWebView();
        WebViewOnUiThread onUiThread = new WebViewOnUiThread(this, mActivity.getWebView());
        webView.setWebViewClient(new WaitForLoadedClient(onUiThread) {
            @Override
            public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
                // Not intended to verify server certificate, ignore the error.
                if (error.getPrimaryError() == SslError.SSL_IDMISMATCH) handler.proceed();
            }
        });
        onUiThread.loadUrlAndWaitForCompletion(url);
        assertEquals("1|count=41", onUiThread.getTitle()); // outgoing cookie
        CookieManager cookieManager = CookieManager.getInstance();
        String cookie = cookieManager.getCookie(url);
        assertNotNull(cookie);
        final Pattern pat = Pattern.compile("count=(\\d+)");
        Matcher m = pat.matcher(cookie);
        assertTrue(m.matches());
        assertEquals("42", m.group(1)); // value got incremented
    }

    @UiThreadTest
    public void testGetCurrentWebViewPackageOnUiThread() throws Throwable {
        runCurrentWebViewPackageTest(true /* alreadyOnMainThread */);
    }

    public void testGetCurrentWebViewPackage() throws Throwable {
        runCurrentWebViewPackageTest(false /* alreadyOnMainThread */);
    }

    private void runCurrentWebViewPackageTest(boolean alreadyOnMainThread) throws Exception {
        PackageManager pm = mActivity.getPackageManager();
        if (pm.hasSystemFeature(PackageManager.FEATURE_WEBVIEW)) {
            PackageInfo webViewPackage = WebView.getCurrentWebViewPackage();
            // Ensure that getCurrentWebViewPackage returns a package recognized by the package
            // manager.
            assertPackageEquals(pm.getPackageInfo(webViewPackage.packageName, 0), webViewPackage);

            // Create WebView on the app's main thread
            if (alreadyOnMainThread) {
                mActivity.createAndAttachWebView();
            } else {
                getInstrumentation().runOnMainSync(new Runnable() {
                    @Override
                    public void run() {
                        mActivity.createAndAttachWebView();
                    }
                });
            }

            // Ensure we are still using the same WebView package.
            assertPackageEquals(webViewPackage, WebView.getCurrentWebViewPackage());
        } else {
            // if WebView isn't supported the API should return null.
            assertNull(WebView.getCurrentWebViewPackage());
        }
    }

    private void assertPackageEquals(PackageInfo expected, PackageInfo actual) {
        if (expected == null) assertNull(actual);
        assertEquals(expected.packageName, actual.packageName);
        assertEquals(expected.versionCode, actual.versionCode);
        assertEquals(expected.versionName, actual.versionName);
        assertEquals(expected.lastUpdateTime, actual.lastUpdateTime);
    }

    @UiThreadTest
    public void testStrictModeNotViolatedOnStartup() throws Throwable {
        StrictMode.ThreadPolicy oldThreadPolicy = StrictMode.getThreadPolicy();
        StrictMode.VmPolicy oldVmPolicy = StrictMode.getVmPolicy();
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                .detectAll()
                .penaltyLog()
                .penaltyDeath()
                .build());
        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                .detectAll()
                .penaltyLog()
                .penaltyDeath()
                .build());

        try {
            createWebViewAndNavigate();
            // Try to force Garbage Collection to catch any StrictMode violations triggered in
            // finalizers.
            for(int n = 0; n < 5; n++) {
                Runtime.getRuntime().gc();
                Thread.sleep(200);
            }
        } finally {
            StrictMode.setThreadPolicy(oldThreadPolicy);
            StrictMode.setVmPolicy(oldVmPolicy);
        }
    }

    private void createWebViewAndNavigate() {
        try {
            mActivity.createAndAttachWebView();
        } catch (Throwable t) {
            NullWebViewUtils.determineIfWebViewAvailable(mActivity, t);
            if (NullWebViewUtils.isWebViewAvailable()) {
                // Rethrow t if WebView is available (because then we failed in some way that
                // indicates that the device supports WebView but couldn't load it for some reason).
                throw t;
            } else {
                // No WebView available - bail out!
                return;
            }
        }

        // WebView is available, so try to call some WebView APIs to ensure they don't cause
        // strictmode violations

        WebViewOnUiThread onUiThread = new WebViewOnUiThread(this, mActivity.getWebView());
        onUiThread.loadUrlAndWaitForCompletion("about:blank");
        onUiThread.loadUrlAndWaitForCompletion("");
    }

    @UiThreadTest
    public void testGetWebViewLooperOnUiThread() {
        PackageManager pm = mActivity.getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_WEBVIEW)) return;

        createAndCheckWebViewLooper();
    }

    /**
     * Ensure that a WebView created on the UI thread returns that thread as its creator thread.
     * This ensures WebView.getWebViewLooper() is not implemented as 'return Looper.myLooper();'.
     */
    public void testGetWebViewLooperCreatedOnUiThreadFromInstrThread() {
        PackageManager pm = mActivity.getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_WEBVIEW)) return;

        WebView[] webviewHolder = new WebView[1];
        // Create the WebView on the UI thread and then ensure webview.getWebViewLooper() returns
        // the UI thread.
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                webviewHolder[0] = createAndCheckWebViewLooper();
            }
        });
        assertEquals(Looper.getMainLooper(), webviewHolder[0].getWebViewLooper());
    }

    /**
     * Ensure that a WebView created on a background thread returns that thread as its creator
     * thread.
     * This ensures WebView.getWebViewLooper() is not bound to the UI thread regardless of the
     * thread it is created on..
     */
    public void testGetWebViewLooperCreatedOnBackgroundThreadFromInstThread()
            throws InterruptedException {
        PackageManager pm = mActivity.getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_WEBVIEW)) return;

        // Create a WebView on a background thread, check it from the UI thread
        final WebView webviewHolder[] = new WebView[1];

        // Use a HandlerThread, because such a thread owns a Looper.
        HandlerThread backgroundThread = new HandlerThread("WebViewLooperCtsHandlerThread");
        backgroundThread.start();
        new Handler(backgroundThread.getLooper()).post(new Runnable() {
            @Override
            public void run() {
                webviewHolder[0] = createAndCheckWebViewLooper();
            }
        });
        backgroundThread.join(TEST_TIMEOUT_MS);
        assertEquals(backgroundThread.getLooper(), webviewHolder[0].getWebViewLooper());
    }

    private WebView createAndCheckWebViewLooper() {
        // Ensure we are running this on a thread with a Looper - otherwise there's no point.
        assertNotNull(Looper.myLooper());
        WebView webview = new WebView(mActivity);
        assertEquals(Looper.myLooper(), webview.getWebViewLooper());
        return webview;
    }
}
