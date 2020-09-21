/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.dialog;

import android.app.DialogFragment;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.webkit.WebView;
import android.webkit.WebViewClient;

/** A DialogFragment that shows a web view. */
public class WebDialogFragment extends SafeDismissDialogFragment {
    private static final String TAG = "WebDialogFragment";
    private static final String URL = "URL";
    private static final String TITLE = "TITLE";
    private static final String TRACKER_LABEL = "TRACKER_LABEL";

    private WebView mWebView;
    private String mTrackerLabel;

    /**
     * Create a new WebDialogFragment to show a particular web page.
     *
     * @param url The URL of the content to show.
     * @param title Optional title for the dialog.
     */
    public static WebDialogFragment newInstance(
            String url, @Nullable String title, String trackerLabel) {
        WebDialogFragment f = new WebDialogFragment();
        Bundle args = new Bundle();
        args.putString(URL, url);
        args.putString(TITLE, title);
        args.putString(TRACKER_LABEL, trackerLabel);
        f.setArguments(args);
        return f;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        String title = getArguments().getString(TITLE);
        mTrackerLabel = getArguments().getString(TRACKER_LABEL);
        int style =
                TextUtils.isEmpty(title)
                        ? DialogFragment.STYLE_NO_TITLE
                        : DialogFragment.STYLE_NORMAL;
        setStyle(style, 0);
    }

    @Nullable
    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        String title = getArguments().getString(TITLE);
        getDialog().setTitle(title);

        mWebView = new WebView(getActivity());
        mWebView.setWebViewClient(new WebViewClient());
        String url = getArguments().getString(URL);
        mWebView.loadUrl(url);
        Log.d(TAG, "Loading web content from " + url);

        return mWebView;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mWebView != null) {
            mWebView.destroy();
        }
    }

    @Override
    public void onStart() {
        super.onStart();
        // Ensure the dialog is fullscreen, even if the webview doesn't have its content yet.
        getDialog().getWindow().setLayout(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
    }

    @Override
    public String getTrackerLabel() {
        return mTrackerLabel;
    }
}
