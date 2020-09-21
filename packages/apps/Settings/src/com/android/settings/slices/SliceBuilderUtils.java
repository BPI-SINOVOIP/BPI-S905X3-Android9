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
 * limitations under the License
 */

package com.android.settings.slices;

import static com.android.settings.core.BasePreferenceController.AVAILABLE;
import static com.android.settings.core.BasePreferenceController.CONDITIONALLY_UNAVAILABLE;
import static com.android.settings.core.BasePreferenceController.DISABLED_DEPENDENT_SETTING;
import static com.android.settings.core.BasePreferenceController.DISABLED_FOR_USER;
import static com.android.settings.core.BasePreferenceController.UNSUPPORTED_ON_DEVICE;
import static com.android.settings.slices.SettingsSliceProvider.EXTRA_SLICE_KEY;
import static com.android.settings.slices.SettingsSliceProvider.EXTRA_SLICE_PLATFORM_DEFINED;

import android.annotation.ColorInt;
import android.app.PendingIntent;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Settings;
import android.provider.SettingsSlicesContract;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.SubSettings;
import com.android.settings.Utils;
import com.android.settings.core.BasePreferenceController;
import com.android.settings.core.SliderPreferenceController;
import com.android.settings.core.TogglePreferenceController;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.search.DatabaseIndexingUtils;
import com.android.settingslib.SliceBroadcastRelay;
import com.android.settingslib.core.AbstractPreferenceController;

import android.support.v4.graphics.drawable.IconCompat;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

import androidx.slice.Slice;
import androidx.slice.builders.ListBuilder;
import androidx.slice.builders.SliceAction;


/**
 * Utility class to build Slices objects and Preference Controllers based on the Database managed
 * by {@link SlicesDatabaseHelper}
 */
public class SliceBuilderUtils {

    private static final String TAG = "SliceBuilder";

    /**
     * Build a Slice from {@link SliceData}.
     *
     * @return a {@link Slice} based on the data provided by {@param sliceData}.
     * Will build an {@link Intent} based Slice unless the Preference Controller name in
     * {@param sliceData} is an inline controller.
     */
    public static Slice buildSlice(Context context, SliceData sliceData) {
        Log.d(TAG, "Creating slice for: " + sliceData.getPreferenceController());
        final BasePreferenceController controller = getPreferenceController(context, sliceData);
        final Pair<Integer, Object> sliceNamePair =
                Pair.create(MetricsEvent.FIELD_SETTINGS_PREFERENCE_CHANGE_NAME, sliceData.getKey());
        // Log Slice requests using the same schema as SharedPreferenceLogger (but with a different
        // action name).
        FeatureFactory.getFactory(context).getMetricsFeatureProvider()
                .action(context, MetricsEvent.ACTION_SETTINGS_SLICE_REQUESTED, sliceNamePair);

        if (!controller.isAvailable()) {
            // Cannot guarantee setting page is accessible, let the presenter handle error case.
            return null;
        }

        if (controller.getAvailabilityStatus() == DISABLED_DEPENDENT_SETTING) {
            return buildUnavailableSlice(context, sliceData);
        }

        switch (sliceData.getSliceType()) {
            case SliceData.SliceType.INTENT:
                return buildIntentSlice(context, sliceData, controller);
            case SliceData.SliceType.SWITCH:
                return buildToggleSlice(context, sliceData, controller);
            case SliceData.SliceType.SLIDER:
                return buildSliderSlice(context, sliceData, controller);
            default:
                throw new IllegalArgumentException(
                        "Slice type passed was invalid: " + sliceData.getSliceType());
        }
    }

    /**
     * @return the {@link SliceData.SliceType} for the {@param controllerClassName} and key.
     */
    @SliceData.SliceType
    public static int getSliceType(Context context, String controllerClassName,
            String controllerKey) {
        BasePreferenceController controller = getPreferenceController(context, controllerClassName,
                controllerKey);
        return controller.getSliceType();
    }

    /**
     * Splits the Settings Slice Uri path into its two expected components:
     * - intent/action
     * - key
     * <p>
     * Examples of valid paths are:
     * - /intent/wifi
     * - /intent/bluetooth
     * - /action/wifi
     * - /action/accessibility/servicename
     *
     * @param uri of the Slice. Follows pattern outlined in {@link SettingsSliceProvider}.
     * @return Pair whose first element {@code true} if the path is prepended with "intent", and
     * second is a key.
     */
    public static Pair<Boolean, String> getPathData(Uri uri) {
        final String path = uri.getPath();
        final String[] split = path.split("/", 3);

        // Split should be: [{}, SLICE_TYPE, KEY].
        // Example: "/action/wifi" -> [{}, "action", "wifi"]
        //          "/action/longer/path" -> [{}, "action", "longer/path"]
        if (split.length != 3) {
            return null;
        }

        final boolean isIntent = TextUtils.equals(SettingsSlicesContract.PATH_SETTING_INTENT,
                split[1]);

        return new Pair<>(isIntent, split[2]);
    }

    /**
     * Looks at the controller classname in in {@link SliceData} from {@param sliceData}
     * and attempts to build an {@link AbstractPreferenceController}.
     */
    public static BasePreferenceController getPreferenceController(Context context,
            SliceData sliceData) {
        return getPreferenceController(context, sliceData.getPreferenceController(),
                sliceData.getKey());
    }

    /**
     * @return {@link PendingIntent} for a non-primary {@link SliceAction}.
     */
    public static PendingIntent getActionIntent(Context context, String action, SliceData data) {
        final Intent intent = new Intent(action);
        intent.setClass(context, SliceBroadcastReceiver.class);
        intent.putExtra(EXTRA_SLICE_KEY, data.getKey());
        intent.putExtra(EXTRA_SLICE_PLATFORM_DEFINED, data.isPlatformDefined());
        return PendingIntent.getBroadcast(context, 0 /* requestCode */, intent,
                PendingIntent.FLAG_CANCEL_CURRENT);
    }

    /**
     * @return {@link PendingIntent} for the primary {@link SliceAction}.
     */
    public static PendingIntent getContentPendingIntent(Context context, SliceData sliceData) {
        final Intent intent = getContentIntent(context, sliceData);
        return PendingIntent.getActivity(context, 0 /* requestCode */, intent, 0 /* flags */);
    }

    /**
     * @return the summary text for a {@link Slice} built for {@param sliceData}.
     */
    public static CharSequence getSubtitleText(Context context,
            AbstractPreferenceController controller, SliceData sliceData) {
        CharSequence summaryText = sliceData.getScreenTitle();
        if (isValidSummary(context, summaryText) && !TextUtils.equals(summaryText,
                sliceData.getTitle())) {
            return summaryText;
        }

        if (controller != null) {
            summaryText = controller.getSummary();

            if (isValidSummary(context, summaryText)) {
                return summaryText;
            }
        }

        summaryText = sliceData.getSummary();
        if (isValidSummary(context, summaryText)) {
            return summaryText;
        }

        return "";
    }

    public static Uri getUri(String path, boolean isPlatformSlice) {
        final String authority = isPlatformSlice
                ? SettingsSlicesContract.AUTHORITY
                : SettingsSliceProvider.SLICE_AUTHORITY;
        return new Uri.Builder()
                .scheme(ContentResolver.SCHEME_CONTENT)
                .authority(authority)
                .appendPath(path)
                .build();
    }

    @VisibleForTesting
    static Intent getContentIntent(Context context, SliceData sliceData) {
        final Uri contentUri = new Uri.Builder().appendPath(sliceData.getKey()).build();
        final Intent intent = DatabaseIndexingUtils.buildSearchResultPageIntent(context,
                sliceData.getFragmentClassName(), sliceData.getKey(),
                sliceData.getScreenTitle().toString(), 0 /* TODO */);
        intent.setClassName(context.getPackageName(), SubSettings.class.getName());
        intent.setData(contentUri);
        return intent;
    }

    private static Slice buildToggleSlice(Context context, SliceData sliceData,
            BasePreferenceController controller) {
        final PendingIntent contentIntent = getContentPendingIntent(context, sliceData);
        final IconCompat icon = IconCompat.createWithResource(context, sliceData.getIconResource());
        final CharSequence subtitleText = getSubtitleText(context, controller, sliceData);
        @ColorInt final int color = Utils.getColorAccent(context);
        final TogglePreferenceController toggleController =
                (TogglePreferenceController) controller;
        final SliceAction sliceAction = getToggleAction(context, sliceData,
                toggleController.isChecked());
        final List<String> keywords = buildSliceKeywords(sliceData);

        return new ListBuilder(context, sliceData.getUri(), ListBuilder.INFINITY)
                .setAccentColor(color)
                .addRow(rowBuilder -> rowBuilder
                        .setTitle(sliceData.getTitle())
                        .setSubtitle(subtitleText)
                        .setPrimaryAction(
                                new SliceAction(contentIntent, icon, sliceData.getTitle()))
                        .addEndItem(sliceAction))
                .setKeywords(keywords)
                .build();
    }

    private static Slice buildIntentSlice(Context context, SliceData sliceData,
            BasePreferenceController controller) {
        final PendingIntent contentIntent = getContentPendingIntent(context, sliceData);
        final IconCompat icon = IconCompat.createWithResource(context, sliceData.getIconResource());
        final CharSequence subtitleText = getSubtitleText(context, controller, sliceData);
        @ColorInt final int color = Utils.getColorAccent(context);
        final List<String> keywords = buildSliceKeywords(sliceData);

        return new ListBuilder(context, sliceData.getUri(), ListBuilder.INFINITY)
                .setAccentColor(color)
                .addRow(rowBuilder -> rowBuilder
                        .setTitle(sliceData.getTitle())
                        .setSubtitle(subtitleText)
                        .setPrimaryAction(
                                new SliceAction(contentIntent, icon, sliceData.getTitle())))
                .setKeywords(keywords)
                .build();
    }

    private static Slice buildSliderSlice(Context context, SliceData sliceData,
            BasePreferenceController controller) {
        final SliderPreferenceController sliderController = (SliderPreferenceController) controller;
        final PendingIntent actionIntent = getSliderAction(context, sliceData);
        final PendingIntent contentIntent = getContentPendingIntent(context, sliceData);
        final IconCompat icon = IconCompat.createWithResource(context, sliceData.getIconResource());
        final CharSequence subtitleText = getSubtitleText(context, controller, sliceData);
        @ColorInt final int color = Utils.getColorAccent(context);
        final SliceAction primaryAction = new SliceAction(contentIntent, icon,
                sliceData.getTitle());
        final List<String> keywords = buildSliceKeywords(sliceData);

        return new ListBuilder(context, sliceData.getUri(), ListBuilder.INFINITY)
                .setAccentColor(color)
                .addInputRange(builder -> builder
                        .setTitle(sliceData.getTitle())
                        .setSubtitle(subtitleText)
                        .setPrimaryAction(primaryAction)
                        .setMax(sliderController.getMaxSteps())
                        .setValue(sliderController.getSliderPosition())
                        .setInputAction(actionIntent))
                .setKeywords(keywords)
                .build();
    }

    private static BasePreferenceController getPreferenceController(Context context,
            String controllerClassName, String controllerKey) {
        try {
            return BasePreferenceController.createInstance(context, controllerClassName);
        } catch (IllegalStateException e) {
            // Do nothing
        }

        return BasePreferenceController.createInstance(context, controllerClassName, controllerKey);
    }

    private static SliceAction getToggleAction(Context context, SliceData sliceData,
            boolean isChecked) {
        PendingIntent actionIntent = getActionIntent(context,
                SettingsSliceProvider.ACTION_TOGGLE_CHANGED, sliceData);
        return new SliceAction(actionIntent, null, isChecked);
    }

    private static PendingIntent getSliderAction(Context context, SliceData sliceData) {
        return getActionIntent(context, SettingsSliceProvider.ACTION_SLIDER_CHANGED, sliceData);
    }

    private static boolean isValidSummary(Context context, CharSequence summary) {
        if (summary == null || TextUtils.isEmpty(summary.toString().trim())) {
            return false;
        }

        final CharSequence placeHolder = context.getText(R.string.summary_placeholder);
        final CharSequence doublePlaceHolder =
                context.getText(R.string.summary_two_lines_placeholder);

        return !(TextUtils.equals(summary, placeHolder)
                || TextUtils.equals(summary, doublePlaceHolder));
    }

    private static List<String> buildSliceKeywords(SliceData data) {
        final List<String> keywords = new ArrayList<>();

        keywords.add(data.getTitle());

        if (!TextUtils.equals(data.getTitle(), data.getScreenTitle())) {
            keywords.add(data.getScreenTitle().toString());
        }

        final String keywordString = data.getKeywords();
        if (keywordString != null) {
            final String[] keywordArray = keywordString.split(",");
            final List<String> strippedKeywords = Arrays.stream(keywordArray)
                    .map(s -> s = s.trim())
                    .collect(Collectors.toList());
            keywords.addAll(strippedKeywords);
        }

        return keywords;
    }

    private static Slice buildUnavailableSlice(Context context, SliceData data) {
        final String title = data.getTitle();
        final List<String> keywords = buildSliceKeywords(data);
        @ColorInt final int color = Utils.getColorAccent(context);
        final CharSequence summary = context.getText(R.string.disabled_dependent_setting_summary);
        final IconCompat icon = IconCompat.createWithResource(context, data.getIconResource());
        final SliceAction primaryAction = new SliceAction(getContentPendingIntent(context, data),
                icon, title);

        return new ListBuilder(context, data.getUri(), ListBuilder.INFINITY)
                .setAccentColor(color)
                .addRow(builder -> builder
                        .setTitle(title)
                        .setTitleItem(icon)
                        .setSubtitle(summary)
                        .setPrimaryAction(primaryAction))
                .setKeywords(keywords)
                .build();
    }
}
