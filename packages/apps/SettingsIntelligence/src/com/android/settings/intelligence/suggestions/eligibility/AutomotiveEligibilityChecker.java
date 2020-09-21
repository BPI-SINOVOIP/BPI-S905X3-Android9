package com.android.settings.intelligence.suggestions.eligibility;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.util.Log;

public class AutomotiveEligibilityChecker {
    private static final String TAG = "AutomotiveEligibility";

    /**
     * When running with {@link PackageManager#FEATURE_AUTOMOTIVE}, suggestion must have
     * {@link #META_DATA_AUTOMOTIVE_ELIGIBLE} defined as true for the suggestion to be eligible.
     */
    private static final String META_DATA_AUTOMOTIVE_ELIGIBLE =
            "com.android.settings.automotive_eligible";

    public static boolean isEligible(Context context, String id, ResolveInfo info) {
        PackageManager packageManager = context.getPackageManager();
        boolean isAutomotive = packageManager.hasSystemFeature(PackageManager.FEATURE_AUTOMOTIVE);
        boolean isAutomotiveEligible =
                info.activityInfo.metaData.getBoolean(META_DATA_AUTOMOTIVE_ELIGIBLE, false);
        if (isAutomotive) {
            if (!isAutomotiveEligible) {
                Log.i(TAG, "Suggestion is ineligible for FEATURE_AUTOMOTIVE: " + id);
            }
            return isAutomotiveEligible;
        }
        return true;
    }
}
