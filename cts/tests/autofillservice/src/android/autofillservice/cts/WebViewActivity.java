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
package android.autofillservice.cts;

import static android.autofillservice.cts.Timeouts.WEBVIEW_TIMEOUT;

import android.content.Context;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import android.widget.LinearLayout;

import java.io.IOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class WebViewActivity extends AbstractAutoFillActivity {

    private static final String TAG = "WebViewActivity";
    static final String FAKE_DOMAIN = "y.u.no.real.server";
    private static final String FAKE_URL = "https://" + FAKE_DOMAIN + ":666/login.html";
    static final String ID_WEBVIEW = "webview";

    static final String HTML_NAME_USERNAME = "username";
    static final String HTML_NAME_PASSWORD = "password";

    static final String ID_OUTSIDE1 = "outside1";
    static final String ID_OUTSIDE2 = "outside2";

    private MyWebView mWebView;

    private LinearLayout mParent;
    private LinearLayout mOutsideContainer1;
    private LinearLayout mOutsideContainer2;
    EditText mOutside1;
    EditText mOutside2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.webview_activity);

        mParent = findViewById(R.id.parent);
        mOutsideContainer1 = findViewById(R.id.outsideContainer1);
        mOutsideContainer2 = findViewById(R.id.outsideContainer2);
        mOutside1 = findViewById(R.id.outside1);
        mOutside2 = findViewById(R.id.outside2);
    }

    public MyWebView loadWebView(UiBot uiBot) throws Exception {
        return loadWebView(uiBot, false);
    }

    public MyWebView loadWebView(UiBot uiBot, boolean usingAppContext) throws Exception {
        final CountDownLatch latch = new CountDownLatch(1);
        syncRunOnUiThread(() -> {
            final Context context = usingAppContext ? getApplicationContext() : this;
            mWebView = new MyWebView(context);
            mParent.addView(mWebView);
            mWebView.setWebViewClient(new WebViewClient() {
                // WebView does not set the WebDomain on file:// requests, so we need to use an
                // https:// request and intercept it to provide the real data.
                @Override
                public WebResourceResponse shouldInterceptRequest(WebView view,
                        WebResourceRequest request) {
                    final String url = request.getUrl().toString();
                    if (!url.equals(FAKE_URL)) {
                        Log.d(TAG, "Ignoring " + url);
                        return super.shouldInterceptRequest(view, request);
                    }

                    final String rawPath = request.getUrl().getPath()
                            .substring(1); // Remove leading /
                    Log.d(TAG, "Converting " + url + " to " + rawPath);
                    // NOTE: cannot use try-with-resources because it would close the stream before
                    // WebView uses it.
                    try {
                        return new WebResourceResponse("text/html", "utf-8",
                                getAssets().open(rawPath));
                    } catch (IOException e) {
                        throw new IllegalArgumentException("Error opening " + rawPath, e);
                    }
                }

                @Override
                public void onPageFinished(WebView view, String url) {
                    Log.v(TAG, "onPageFinished(): " + url);
                    latch.countDown();
                }

            });
            mWebView.loadUrl(FAKE_URL);
        });

        // Wait until it's loaded.

        if (!latch.await(WEBVIEW_TIMEOUT.ms(), TimeUnit.MILLISECONDS)) {
            throw new RetryableException(WEBVIEW_TIMEOUT, "WebView not loaded");
        }

        // TODO(b/80317628): re-add check below
        // NOTE: we cannot search by resourceId because WebView does not set them...
        // uiBot.assertShownByText("Login"); // Login button

        return mWebView;
    }

    public void loadOutsideViews() {
        syncRunOnUiThread(() -> {
            mOutsideContainer1.setVisibility(View.VISIBLE);
            mOutsideContainer2.setVisibility(View.VISIBLE);
        });
    }

    public UiObject2 getUsernameLabel(UiBot uiBot) throws Exception {
        return getLabel(uiBot, "Username: ");
    }

    public UiObject2 getPasswordLabel(UiBot uiBot) throws Exception {
        return getLabel(uiBot, "Password: ");
    }

    public UiObject2 getUsernameInput(UiBot uiBot) throws Exception {
        return getInput(uiBot, "Username: ");
    }

    public UiObject2 getPasswordInput(UiBot uiBot) throws Exception {
        return getInput(uiBot, "Password: ");
    }

    public UiObject2 getLoginButton(UiBot uiBot) throws Exception {
        return getLabel(uiBot, "Login");
    }

    private UiObject2 getLabel(UiBot uiBot, String label) throws Exception {
        return uiBot.assertShownByText(label, Timeouts.WEBVIEW_TIMEOUT);
    }

    private UiObject2 getInput(UiBot uiBot, String contentDescription) throws Exception {
        // First get the label..
        final UiObject2 label = getLabel(uiBot, contentDescription);

        // Then the input is next.
        final UiObject2 parent = label.getParent();
        UiObject2 previous = null;
        for (UiObject2 child : parent.getChildren()) {
            if (label.equals(previous)) {
                if (child.getClassName().equals(EditText.class.getName())) {
                    return child;
                }
                uiBot.dumpScreen("getInput() for " + child + "failed");
                throw new IllegalStateException("Invalid class for " + child);
            }
            previous = child;
        }
        uiBot.dumpScreen("getInput() for label " + label + "failed");
        throw new IllegalStateException("could not find username (label=" + label + ")");
    }

    public void dispatchKeyPress(int keyCode) {
        runOnUiThread(() -> {
            KeyEvent keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
            mWebView.dispatchKeyEvent(keyEvent);
            keyEvent = new KeyEvent(KeyEvent.ACTION_UP, keyCode);
            mWebView.dispatchKeyEvent(keyEvent);
        });
        // wait webview to process the key event.
        SystemClock.sleep(300);
    }
}
