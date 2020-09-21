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
package com.android.documentsui.inspector;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.LocaleList;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassificationManager;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextSelection;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Checks if a TextView has a latitude and longitude. If so shows the default maps app to open
 * those coordinates.
 *
 * Example - textView.setTextClassifier(new GpsCoordinatesTextClassifier(context, intent));
 */
final class GpsCoordinatesTextClassifier implements TextClassifier {

    // Checks for latitude and longitude points, latitude ranges -90.0 to 90.0 and longitude
    // ranges -180.0 to 180.0 Valid entries can be of the format "0,0", "0, 0" and "0.0, 0.0"
    // in the mentioned range.
    private static final String geoPattern
        = "-?(90(\\.0*)?|[1-8]?[0-9](\\.[0-9]*)?), *-?(180("
        + "\\.0*)?|([1][0-7][0-9]|[0-9]?[0-9])(\\.[0-9]*)?)";
    private static final Pattern sGeoPattern = Pattern.compile(geoPattern);

    private final TextClassifier mSystemClassifier;
    private final PackageManager mPackageManager;

    public GpsCoordinatesTextClassifier(PackageManager pm, TextClassifier classifier) {
        assert pm != null;
        assert classifier != null;
        mPackageManager = pm;
        mSystemClassifier = classifier;
    }

    public static GpsCoordinatesTextClassifier create(Context context) {
        return new GpsCoordinatesTextClassifier(
                context.getPackageManager(),
                context.getSystemService(TextClassificationManager.class).getTextClassifier());
    }

    // TODO: add support for local specific formatting
    @Override
    public TextClassification classifyText(
            CharSequence text, int start, int end, LocaleList localeList) {

        CharSequence sequence = text.subSequence(start, end);
        if (isGeoSequence(sequence)) {

            Intent intent = new Intent(Intent.ACTION_VIEW)
                .setData(Uri.parse(String.format("geo:0,0?q=%s", sequence)));

            final ResolveInfo info = mPackageManager.resolveActivity(intent, 0);
            if (info != null) {

                return new TextClassification.Builder()
                        .setText(sequence.toString())
                        .setEntityType(TextClassifier.TYPE_ADDRESS, 1.0f)
                        .setIcon(info.loadIcon(mPackageManager))
                        .setLabel(info.loadLabel(mPackageManager).toString())
                        .setIntent(intent)
                        .build();
            }
        }

        // return default if text was not latitude, longitude or we couldn't find an application
        // to handle the geo intent.
        return mSystemClassifier.classifyText(text, start, end, localeList);
    }

    @Override
    public TextSelection suggestSelection(
        CharSequence context, int start, int end, LocaleList localeList) {

        // Show map action menu popup when entire TextView is selected.
        final int[] boundaries = {0, context.length()};

        if (boundaries != null) {
            return new TextSelection.Builder(boundaries[0], boundaries[1])
                .setEntityType(TextClassifier.TYPE_ADDRESS, 1.0f)
                .build();
        }
        return mSystemClassifier.suggestSelection(context, start, end, localeList);
    }

    private static boolean isGeoSequence(CharSequence text) {
        return sGeoPattern.matcher(text).matches();
    }
}
