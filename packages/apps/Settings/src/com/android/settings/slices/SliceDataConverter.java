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

import static com.android.settings.core.PreferenceXmlParserUtils.METADATA_CONTROLLER;
import static com.android.settings.core.PreferenceXmlParserUtils.METADATA_ICON;
import static com.android.settings.core.PreferenceXmlParserUtils.METADATA_KEY;
import static com.android.settings.core.PreferenceXmlParserUtils.METADATA_PLATFORM_SLICE_FLAG;
import static com.android.settings.core.PreferenceXmlParserUtils.METADATA_SUMMARY;
import static com.android.settings.core.PreferenceXmlParserUtils.METADATA_TITLE;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.os.Bundle;
import android.provider.SearchIndexableResource;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Xml;
import android.view.accessibility.AccessibilityManager;

import com.android.internal.annotations.VisibleForTesting;
import com.android.settings.R;
import com.android.settings.accessibility.AccessibilitySettings;
import com.android.settings.accessibility.AccessibilitySlicePreferenceController;
import com.android.settings.core.BasePreferenceController;
import com.android.settings.core.PreferenceXmlParserUtils;
import com.android.settings.core.PreferenceXmlParserUtils.MetadataFlag;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.search.DatabaseIndexingUtils;
import com.android.settings.search.Indexable.SearchIndexProvider;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Converts all Slice sources into {@link SliceData}.
 * This includes:
 * - All {@link DashboardFragment DashboardFragments} indexed by settings search
 * - Accessibility services
 */
class SliceDataConverter {

    private static final String TAG = "SliceDataConverter";

    private static final String NODE_NAME_PREFERENCE_SCREEN = "PreferenceScreen";

    private Context mContext;

    private List<SliceData> mSliceData;

    public SliceDataConverter(Context context) {
        mContext = context;
        mSliceData = new ArrayList<>();
    }

    /**
     * @return a list of {@link SliceData} to be indexed and later referenced as a Slice.
     *
     * The collection works as follows:
     * - Collects a list of Fragments from
     * {@link FeatureFactory#getSearchFeatureProvider()}.
     * - From each fragment, grab a {@link SearchIndexProvider}.
     * - For each provider, collect XML resource layout and a list of
     * {@link com.android.settings.core.BasePreferenceController}.
     */
    public List<SliceData> getSliceData() {
        if (!mSliceData.isEmpty()) {
            return mSliceData;
        }

        final Collection<Class> indexableClasses = FeatureFactory.getFactory(mContext)
                .getSearchFeatureProvider().getSearchIndexableResources().getProviderValues();

        for (Class clazz : indexableClasses) {
            final String fragmentName = clazz.getName();

            final SearchIndexProvider provider = DatabaseIndexingUtils.getSearchIndexProvider(
                    clazz);

            // CodeInspection test guards against the null check. Keep check in case of bad actors.
            if (provider == null) {
                Log.e(TAG, fragmentName + " dose not implement Search Index Provider");
                continue;
            }

            final List<SliceData> providerSliceData = getSliceDataFromProvider(provider,
                    fragmentName);
            mSliceData.addAll(providerSliceData);
        }

        final List<SliceData> a11ySliceData = getAccessibilitySliceData();
        mSliceData.addAll(a11ySliceData);
        return mSliceData;
    }

    private List<SliceData> getSliceDataFromProvider(SearchIndexProvider provider,
            String fragmentName) {
        final List<SliceData> sliceData = new ArrayList<>();

        final List<SearchIndexableResource> resList =
                provider.getXmlResourcesToIndex(mContext, true /* enabled */);

        if (resList == null) {
            return sliceData;
        }

        // TODO (b/67996923) get a list of permanent NIKs and skip the invalid keys.

        for (SearchIndexableResource resource : resList) {
            int xmlResId = resource.xmlResId;
            if (xmlResId == 0) {
                Log.e(TAG, fragmentName + " provides invalid XML (0) in search provider.");
                continue;
            }

            List<SliceData> xmlSliceData = getSliceDataFromXML(xmlResId, fragmentName);
            sliceData.addAll(xmlSliceData);
        }

        return sliceData;
    }

    private List<SliceData> getSliceDataFromXML(int xmlResId, String fragmentName) {
        XmlResourceParser parser = null;

        final List<SliceData> xmlSliceData = new ArrayList<>();

        try {
            parser = mContext.getResources().getXml(xmlResId);

            int type;
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && type != XmlPullParser.START_TAG) {
                // Parse next until start tag is found
            }

            String nodeName = parser.getName();
            if (!NODE_NAME_PREFERENCE_SCREEN.equals(nodeName)) {
                throw new RuntimeException(
                        "XML document must start with <PreferenceScreen> tag; found"
                                + nodeName + " at " + parser.getPositionDescription());
            }

            final AttributeSet attrs = Xml.asAttributeSet(parser);
            final String screenTitle = PreferenceXmlParserUtils.getDataTitle(mContext, attrs);

            // TODO (b/67996923) Investigate if we need headers for Slices, since they never
            // correspond to an actual setting.

            final List<Bundle> metadata = PreferenceXmlParserUtils.extractMetadata(mContext,
                    xmlResId,
                    MetadataFlag.FLAG_NEED_KEY
                            | MetadataFlag.FLAG_NEED_PREF_CONTROLLER
                            | MetadataFlag.FLAG_NEED_PREF_TYPE
                            | MetadataFlag.FLAG_NEED_PREF_TITLE
                            | MetadataFlag.FLAG_NEED_PREF_ICON
                            | MetadataFlag.FLAG_NEED_PREF_SUMMARY
                            | MetadataFlag.FLAG_NEED_PLATFORM_SLICE_FLAG);

            for (Bundle bundle : metadata) {
                // TODO (b/67996923) Non-controller Slices should become intent-only slices.
                // Note that without a controller, dynamic summaries are impossible.
                final String controllerClassName = bundle.getString(METADATA_CONTROLLER);
                if (TextUtils.isEmpty(controllerClassName)) {
                    continue;
                }

                final String key = bundle.getString(METADATA_KEY);
                final String title = bundle.getString(METADATA_TITLE);
                final String summary = bundle.getString(METADATA_SUMMARY);
                final int iconResId = bundle.getInt(METADATA_ICON);
                final int sliceType = SliceBuilderUtils.getSliceType(mContext, controllerClassName,
                        key);
                final boolean isPlatformSlice = bundle.getBoolean(METADATA_PLATFORM_SLICE_FLAG);

                final SliceData xmlSlice = new SliceData.Builder()
                        .setKey(key)
                        .setTitle(title)
                        .setSummary(summary)
                        .setIcon(iconResId)
                        .setScreenTitle(screenTitle)
                        .setPreferenceControllerClassName(controllerClassName)
                        .setFragmentName(fragmentName)
                        .setSliceType(sliceType)
                        .setPlatformDefined(isPlatformSlice)
                        .build();

                final BasePreferenceController controller =
                        SliceBuilderUtils.getPreferenceController(mContext, xmlSlice);

                // Only add pre-approved Slices available on the device.
                if (controller.isAvailable() && controller.isSliceable()) {
                    xmlSliceData.add(xmlSlice);
                }
            }
        } catch (SliceData.InvalidSliceDataException e) {
            Log.w(TAG, "Invalid data when building SliceData for " + fragmentName, e);
        } catch (XmlPullParserException e) {
            Log.w(TAG, "XML Error parsing PreferenceScreen: ", e);
        } catch (IOException e) {
            Log.w(TAG, "IO Error parsing PreferenceScreen: ", e);
        } catch (Resources.NotFoundException e) {
            Log.w(TAG, "Resource not found error parsing PreferenceScreen: ", e);
        } finally {
            if (parser != null) parser.close();
        }
        return xmlSliceData;
    }

    private List<SliceData> getAccessibilitySliceData() {
        final List<SliceData> sliceData = new ArrayList<>();

        final String accessibilityControllerClassName =
                AccessibilitySlicePreferenceController.class.getName();
        final String fragmentClassName = AccessibilitySettings.class.getName();
        final CharSequence screenTitle = mContext.getText(R.string.accessibility_settings);

        final SliceData.Builder sliceDataBuilder = new SliceData.Builder()
                .setFragmentName(fragmentClassName)
                .setScreenTitle(screenTitle)
                .setPreferenceControllerClassName(accessibilityControllerClassName);

        final Set<String> a11yServiceNames = new HashSet<>();
        Collections.addAll(a11yServiceNames, mContext.getResources()
                .getStringArray(R.array.config_settings_slices_accessibility_components));
        final List<AccessibilityServiceInfo> installedServices = getAccessibilityServiceInfoList();
        final PackageManager packageManager = mContext.getPackageManager();

        for (AccessibilityServiceInfo a11yServiceInfo : installedServices) {
            final ResolveInfo resolveInfo = a11yServiceInfo.getResolveInfo();
            final ServiceInfo serviceInfo = resolveInfo.serviceInfo;
            final String packageName = serviceInfo.packageName;
            final ComponentName componentName = new ComponentName(packageName, serviceInfo.name);
            final String flattenedName = componentName.flattenToString();

            if (!a11yServiceNames.contains(flattenedName)) {
                continue;
            }

            final String title = resolveInfo.loadLabel(packageManager).toString();
            int iconResource = resolveInfo.getIconResource();
            if (iconResource == 0) {
                iconResource = R.mipmap.ic_accessibility_generic;
            }

            sliceDataBuilder.setKey(flattenedName)
                    .setTitle(title)
                    .setIcon(iconResource)
                    .setSliceType(SliceData.SliceType.SWITCH);
            try {
                sliceData.add(sliceDataBuilder.build());
            } catch (SliceData.InvalidSliceDataException e) {
                Log.w(TAG, "Invalid data when building a11y SliceData for " + flattenedName, e);
            }
        }

        return sliceData;
    }

    @VisibleForTesting
    List<AccessibilityServiceInfo> getAccessibilityServiceInfoList() {
        final AccessibilityManager accessibilityManager = AccessibilityManager.getInstance(
                mContext);
        return accessibilityManager.getInstalledAccessibilityServiceList();
    }
}