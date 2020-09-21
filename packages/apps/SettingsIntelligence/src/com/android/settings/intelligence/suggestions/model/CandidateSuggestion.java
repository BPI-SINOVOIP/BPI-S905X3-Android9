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

package com.android.settings.intelligence.suggestions.model;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Bundle;
import android.service.settings.suggestions.Suggestion;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;

import com.android.settings.intelligence.suggestions.eligibility.AccountEligibilityChecker;
import com.android.settings.intelligence.suggestions.eligibility.AutomotiveEligibilityChecker;
import com.android.settings.intelligence.suggestions.eligibility.ConnectivityEligibilityChecker;
import com.android.settings.intelligence.suggestions.eligibility.DismissedChecker;
import com.android.settings.intelligence.suggestions.eligibility.FeatureEligibilityChecker;
import com.android.settings.intelligence.suggestions.eligibility.ProviderEligibilityChecker;

import java.util.List;

/**
 * A wrapper to {@link android.content.pm.ResolveInfo} that matches Suggestion signature.
 * <p/>
 * This class contains necessary metadata to eventually be
 * processed into a {@link android.service.settings.suggestions.Suggestion}.
 */
public class CandidateSuggestion {

    private static final String TAG = "CandidateSuggestion";

    /**
     * Name of the meta-data item that should be set in the AndroidManifest.xml
     * to specify the title text that should be displayed for the preference.
     */
    @VisibleForTesting
    public static final String META_DATA_PREFERENCE_TITLE = "com.android.settings.title";

    /**
     * Name of the meta-data item that should be set in the AndroidManifest.xml
     * to specify the summary text that should be displayed for the preference.
     */
    @VisibleForTesting
    public static final String META_DATA_PREFERENCE_SUMMARY = "com.android.settings.summary";

    /**
     * Name of the meta-data item that should be set in the AndroidManifest.xml
     * to specify the content provider providing the summary text that should be displayed for the
     * preference.
     *
     * Summary provided by the content provider overrides any static summary.
     */
    @VisibleForTesting
    public static final String META_DATA_PREFERENCE_SUMMARY_URI =
            "com.android.settings.summary_uri";

    /**
     * Name of the meta-data item that should be set in the AndroidManifest.xml
     * to specify the icon that should be displayed for the preference.
     */
    @VisibleForTesting
    public static final String META_DATA_PREFERENCE_ICON = "com.android.settings.icon";

    /**
     * Hint for type of suggestion UI to be displayed.
     */
    @VisibleForTesting
    public static final String META_DATA_PREFERENCE_CUSTOM_VIEW =
            "com.android.settings.custom_view";

    private final String mId;
    private final Context mContext;
    private final ResolveInfo mResolveInfo;
    private final ComponentName mComponent;
    private final Intent mIntent;
    private final boolean mIsEligible;
    private final boolean mIgnoreAppearRule;

    public CandidateSuggestion(Context context, ResolveInfo resolveInfo,
            boolean ignoreAppearRule) {
        mContext = context;
        mIgnoreAppearRule = ignoreAppearRule;
        mResolveInfo = resolveInfo;
        mIntent = new Intent().setClassName(
                resolveInfo.activityInfo.packageName, resolveInfo.activityInfo.name);
        mComponent = mIntent.getComponent();
        mId = generateId();
        mIsEligible = initIsEligible();
    }

    public String getId() {
        return mId;
    }

    public ComponentName getComponent() {
        return mComponent;
    }

    /**
     * Whether or not this candidate is eligible for display.
     * <p/>
     * Note: eligible doesn't mean it will be displayed.
     */
    public boolean isEligible() {
        return mIsEligible;
    }

    public Suggestion toSuggestion() {
        if (!mIsEligible) {
            return null;
        }
        final Suggestion.Builder builder = new Suggestion.Builder(mId);
        updateBuilder(builder);
        return builder.build();
    }

    /**
     * Checks device condition against suggestion requirement. Returns true if the suggestion is
     * eligible.
     * <p/>
     * Note: eligible doesn't mean it will be displayed.
     */
    private boolean initIsEligible() {
        if (!ProviderEligibilityChecker.isEligible(mContext, mId, mResolveInfo)) {
            return false;
        }
        if (!ConnectivityEligibilityChecker.isEligible(mContext, mId, mResolveInfo)) {
            return false;
        }
        if (!FeatureEligibilityChecker.isEligible(mContext, mId, mResolveInfo)) {
            return false;
        }
        if (!AccountEligibilityChecker.isEligible(mContext, mId, mResolveInfo)) {
            return false;
        }
        if (!DismissedChecker.isEligible(mContext, mId, mResolveInfo, mIgnoreAppearRule)) {
            return false;
        }
        if (!AutomotiveEligibilityChecker.isEligible(mContext, mId, mResolveInfo)) {
            return false;
        }
        return true;
    }

    private void updateBuilder(Suggestion.Builder builder) {
        final PackageManager pm = mContext.getPackageManager();
        final String packageName = mComponent.getPackageName();

        int iconRes = 0;
        int flags = 0;
        CharSequence title = null;
        CharSequence summary = null;
        Icon icon = null;

        // Get the activity's meta-data
        try {
            final Resources res = pm.getResourcesForApplication(packageName);
            final Bundle metaData = mResolveInfo.activityInfo.metaData;

            if (res != null && metaData != null) {
                // First get override data
                final Bundle overrideData = getOverrideData(metaData);
                // Get icon
                if (metaData.containsKey(META_DATA_PREFERENCE_ICON)) {
                    iconRes = metaData.getInt(META_DATA_PREFERENCE_ICON);
                } else {
                    iconRes = mResolveInfo.activityInfo.icon;
                }
                if (iconRes != 0) {
                    icon = Icon.createWithResource(
                            mResolveInfo.activityInfo.packageName, iconRes);
                }
                // Get title
                title = getStringFromBundle(overrideData, META_DATA_PREFERENCE_TITLE);
                if (TextUtils.isEmpty(title) && metaData.containsKey(META_DATA_PREFERENCE_TITLE)) {
                    if (metaData.get(META_DATA_PREFERENCE_TITLE) instanceof Integer) {
                        title = res.getString(metaData.getInt(META_DATA_PREFERENCE_TITLE));
                    } else {
                        title = metaData.getString(META_DATA_PREFERENCE_TITLE);
                    }
                }
                // Get summary
                summary = getStringFromBundle(overrideData, META_DATA_PREFERENCE_SUMMARY);
                if (TextUtils.isEmpty(summary)
                        && metaData.containsKey(META_DATA_PREFERENCE_SUMMARY)) {
                    if (metaData.get(META_DATA_PREFERENCE_SUMMARY) instanceof Integer) {
                        summary = res.getString(metaData.getInt(META_DATA_PREFERENCE_SUMMARY));
                    } else {
                        summary = metaData.getString(META_DATA_PREFERENCE_SUMMARY);
                    }
                }
                // Detect remote view
                flags = metaData.containsKey(META_DATA_PREFERENCE_CUSTOM_VIEW)
                        ? Suggestion.FLAG_HAS_BUTTON : 0;
            }
        } catch (PackageManager.NameNotFoundException | Resources.NotFoundException e) {
            Log.w(TAG, "Couldn't find info", e);
        }

        // Set the preference title to the activity's label if no
        // meta-data is found
        if (TextUtils.isEmpty(title)) {
            title = mResolveInfo.activityInfo.loadLabel(pm);
        }
        builder.setTitle(title)
                .setSummary(summary)
                .setFlags(flags)
                .setIcon(icon)
                .setPendingIntent(PendingIntent
                        .getActivity(mContext, 0 /* requestCode */, mIntent, 0 /* flags */));
    }

    /**
     * Extracts a string from bundle.
     */
    private CharSequence getStringFromBundle(Bundle bundle, String key) {
        if (bundle == null || TextUtils.isEmpty(key)) {
            return null;
        }
        return bundle.getString(key);
    }

    private Bundle getOverrideData(Bundle metadata) {
        if (metadata == null || !metadata.containsKey(META_DATA_PREFERENCE_SUMMARY_URI)) {
            Log.d(TAG, "Metadata null or has no info about summary_uri");
            return null;
        }

        final String uriString = metadata.getString(META_DATA_PREFERENCE_SUMMARY_URI);
        final Bundle bundle = getBundleFromUri(uriString);
        return bundle;
    }

    /**
     * Calls method through ContentProvider and expects a bundle in return.
     */
    private Bundle getBundleFromUri(String uriString) {
        final Uri uri = Uri.parse(uriString);

        final String method = getMethodFromUri(uri);
        if (TextUtils.isEmpty(method)) {
            return null;
        }
        try {
            return mContext.getContentResolver().call(uri, method, null /* args */,
                    null /* bundle */);
        } catch (IllegalArgumentException e){
            Log.d(TAG, "Unknown summary_uri", e);
            return null;
        }
    }

    /**
     * Returns the first path segment of the uri if it exists as the method, otherwise null.
     */
    private String getMethodFromUri(Uri uri) {
        if (uri == null) {
            return null;
        }
        final List<String> pathSegments = uri.getPathSegments();
        if ((pathSegments == null) || pathSegments.isEmpty()) {
            return null;
        }
        return pathSegments.get(0);
    }

    private String generateId() {
        return mComponent.flattenToString();
    }
}
