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

package com.android.settings.development;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.text.TextUtils;
import android.util.Log;

import com.android.settings.R;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.webview.WebViewUpdateServiceWrapper;
import com.android.settingslib.applications.DefaultAppInfo;
import com.android.settingslib.development.DeveloperOptionsPreferenceController;
import com.android.settingslib.wrapper.PackageManagerWrapper;

public class WebViewAppPreferenceController extends DeveloperOptionsPreferenceController implements
        PreferenceControllerMixin {

    private static final String TAG = "WebViewAppPrefCtrl";
    private static final String WEBVIEW_APP_KEY = "select_webview_provider";

    private final PackageManagerWrapper mPackageManager;
    private final WebViewUpdateServiceWrapper mWebViewUpdateServiceWrapper;

    public WebViewAppPreferenceController(Context context) {
        super(context);

        mPackageManager = new PackageManagerWrapper(context.getPackageManager());
        mWebViewUpdateServiceWrapper = new WebViewUpdateServiceWrapper();
    }

    @Override
    public String getPreferenceKey() {
        return WEBVIEW_APP_KEY;
    }

    @Override
    public void updateState(Preference preference) {
        final CharSequence defaultAppLabel = getDefaultAppLabel();
        if (!TextUtils.isEmpty(defaultAppLabel)) {
            mPreference.setSummary(defaultAppLabel);
        } else {
            Log.d(TAG, "No default app");
            mPreference.setSummary(R.string.app_list_preference_none);
        }
    }

    @VisibleForTesting
    DefaultAppInfo getDefaultAppInfo() {
        final PackageInfo currentPackage = mWebViewUpdateServiceWrapper.getCurrentWebViewPackage();
        return new DefaultAppInfo(mContext, mPackageManager,
                currentPackage == null ? null : currentPackage.applicationInfo);
    }

    private CharSequence getDefaultAppLabel() {
        final DefaultAppInfo app = getDefaultAppInfo();
        return app.loadLabel();
    }
}
