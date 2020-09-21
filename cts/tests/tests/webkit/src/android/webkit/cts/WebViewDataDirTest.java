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

package android.webkit.cts;


import android.content.Context;
import android.test.ActivityInstrumentationTestCase2;
import android.webkit.CookieManager;
import android.webkit.WebView;

import com.android.compatibility.common.util.NullWebViewUtils;

public class WebViewDataDirTest extends ActivityInstrumentationTestCase2<WebViewCtsActivity> {

    private static final long REMOTE_TIMEOUT_MS = 5000;
    private static final String ALTERNATE_DIR_NAME = "test";
    private static final String COOKIE_URL = "https://www.example.com/";
    private static final String COOKIE_VALUE = "foo=main";
    private static final String SET_COOKIE_PARAMS = "; Max-Age=86400";

    public WebViewDataDirTest() throws Exception {
        super("android.webkit.cts", WebViewCtsActivity.class);
    }

    static class TestDisableThenUseImpl extends TestProcessClient.TestRunnable {
        @Override
        public void run(Context ctx) {
            WebView.disableWebView();
            try {
                new WebView(ctx);
                fail("didn't throw IllegalStateException");
            } catch (IllegalStateException e) {}
        }
    }

    public void testDisableThenUse() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        try (TestProcessClient process = TestProcessClient.createProcessA(getActivity())) {
            process.run(TestDisableThenUseImpl.class, REMOTE_TIMEOUT_MS);
        }
    }

    public void testUseThenDisable() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        assertNotNull(getActivity().getWebView());
        try {
            WebView.disableWebView();
            fail("didn't throw IllegalStateException");
        } catch (IllegalStateException e) {}
    }

    public void testUseThenChangeDir() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        assertNotNull(getActivity().getWebView());
        try {
            WebView.setDataDirectorySuffix(ALTERNATE_DIR_NAME);
            fail("didn't throw IllegalStateException");
        } catch (IllegalStateException e) {}
    }

    static class TestInvalidDirImpl extends TestProcessClient.TestRunnable {
        @Override
        public void run(Context ctx) {
            try {
                WebView.setDataDirectorySuffix("no/path/separators");
                fail("didn't throw IllegalArgumentException");
            } catch (IllegalArgumentException e) {}
        }
    }

    public void testInvalidDir() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        try (TestProcessClient process = TestProcessClient.createProcessA(getActivity())) {
            process.run(TestInvalidDirImpl.class, REMOTE_TIMEOUT_MS);
        }
    }

    static class TestDefaultDirDisallowed extends TestProcessClient.TestRunnable {
        @Override
        public void run(Context ctx) {
            try {
                new WebView(ctx);
                fail("didn't throw RuntimeException");
            } catch (RuntimeException e) {}
        }
    }

    public void testSameDirTwoProcesses() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        assertNotNull(getActivity().getWebView());

        try (TestProcessClient processA = TestProcessClient.createProcessA(getActivity())) {
            processA.run(TestDefaultDirDisallowed.class, REMOTE_TIMEOUT_MS);
        }
    }

    static class TestCookieInAlternateDir extends TestProcessClient.TestRunnable {
        @Override
        public void run(Context ctx) {
            WebView.setDataDirectorySuffix(ALTERNATE_DIR_NAME);
            CookieManager cm = CookieManager.getInstance();
            String cookie = cm.getCookie(COOKIE_URL);
            assertNull("cookie leaked to alternate cookie jar", cookie);
        }
    }

    public void testCookieJarsSeparate() throws Throwable {
        if (!NullWebViewUtils.isWebViewAvailable()) {
            return;
        }

        CookieManager cm = CookieManager.getInstance();
        cm.setCookie(COOKIE_URL, COOKIE_VALUE + SET_COOKIE_PARAMS);
        cm.flush();
        String cookie = cm.getCookie(COOKIE_URL);
        assertEquals("wrong cookie in default cookie jar", COOKIE_VALUE, cookie);

        try (TestProcessClient processA = TestProcessClient.createProcessA(getActivity())) {
            processA.run(TestCookieInAlternateDir.class, REMOTE_TIMEOUT_MS);
        }
    }
}
