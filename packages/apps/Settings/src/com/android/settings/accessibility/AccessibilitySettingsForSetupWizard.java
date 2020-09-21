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

package com.android.settings.accessibility;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ServiceInfo;
import android.os.Bundle;
import android.support.v7.preference.Preference;
import android.text.TextUtils;
import android.view.accessibility.AccessibilityManager;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.SettingsPreferenceFragment;

import java.util.List;

/**
 * Activity with the accessibility settings specific to Setup Wizard.
 */
public class AccessibilitySettingsForSetupWizard extends SettingsPreferenceFragment
        implements Preference.OnPreferenceChangeListener {

    // Preferences.
    private static final String DISPLAY_MAGNIFICATION_PREFERENCE =
            "screen_magnification_preference";
    private static final String SCREEN_READER_PREFERENCE = "screen_reader_preference";
    private static final String SELECT_TO_SPEAK_PREFERENCE = "select_to_speak_preference";
    private static final String FONT_SIZE_PREFERENCE = "font_size_preference";

    // Package names and service names used to identify screen reader and SelectToSpeak services.
    private static final String SCREEN_READER_PACKAGE_NAME = "com.google.android.marvin.talkback";
    private static final String SCREEN_READER_SERVICE_NAME =
            "com.google.android.marvin.talkback.TalkBackService";
    private static final String SELECT_TO_SPEAK_PACKAGE_NAME = "com.google.android.marvin.talkback";
    private static final String SELECT_TO_SPEAK_SERVICE_NAME =
            "com.google.android.accessibility.selecttospeak.SelectToSpeakService";

    // Preference controls.
    private Preference mDisplayMagnificationPreference;
    private Preference mScreenReaderPreference;
    private Preference mSelectToSpeakPreference;

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.SUW_ACCESSIBILITY;
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        addPreferencesFromResource(R.xml.accessibility_settings_for_setup_wizard);

        mDisplayMagnificationPreference = findPreference(DISPLAY_MAGNIFICATION_PREFERENCE);
        mScreenReaderPreference = findPreference(SCREEN_READER_PREFERENCE);
        mSelectToSpeakPreference = findPreference(SELECT_TO_SPEAK_PREFERENCE);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateAccessibilityServicePreference(mScreenReaderPreference,
                findService(SCREEN_READER_PACKAGE_NAME, SCREEN_READER_SERVICE_NAME));
        updateAccessibilityServicePreference(mSelectToSpeakPreference,
                findService(SELECT_TO_SPEAK_PACKAGE_NAME, SELECT_TO_SPEAK_SERVICE_NAME));
        configureMagnificationPreferenceIfNeeded(mDisplayMagnificationPreference);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        setHasOptionsMenu(false);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return false;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (mDisplayMagnificationPreference == preference) {
            Bundle extras = mDisplayMagnificationPreference.getExtras();
            extras.putBoolean(AccessibilitySettings.EXTRA_LAUNCHED_FROM_SUW, true);
        }

        return super.onPreferenceTreeClick(preference);
    }

    private AccessibilityServiceInfo findService(String packageName, String serviceName) {
        final AccessibilityManager manager =
                getActivity().getSystemService(AccessibilityManager.class);
        final List<AccessibilityServiceInfo> accessibilityServices =
                manager.getInstalledAccessibilityServiceList();
        for (AccessibilityServiceInfo info : accessibilityServices) {
            ServiceInfo serviceInfo = info.getResolveInfo().serviceInfo;
            if (packageName.equals(serviceInfo.packageName)
                    && serviceName.equals(serviceInfo.name)) {
                return info;
            }
        }

        return null;
    }

    private void updateAccessibilityServicePreference(Preference preference,
            AccessibilityServiceInfo info) {
        if (info == null) {
            getPreferenceScreen().removePreference(preference);
            return;
        }

        ServiceInfo serviceInfo = info.getResolveInfo().serviceInfo;
        String title = info.getResolveInfo().loadLabel(getPackageManager()).toString();
        preference.setTitle(title);
        ComponentName componentName = new ComponentName(serviceInfo.packageName, serviceInfo.name);
        preference.setKey(componentName.flattenToString());

        // Update the extras.
        Bundle extras = preference.getExtras();
        extras.putParcelable(AccessibilitySettings.EXTRA_COMPONENT_NAME, componentName);

        extras.putString(AccessibilitySettings.EXTRA_PREFERENCE_KEY,
            preference.getKey());
        extras.putString(AccessibilitySettings.EXTRA_TITLE, title);

        String description = info.loadDescription(getPackageManager());
        if (TextUtils.isEmpty(description)) {
            description = getString(R.string.accessibility_service_default_description);
        }
        extras.putString(AccessibilitySettings.EXTRA_SUMMARY, description);
    }

    private static void configureMagnificationPreferenceIfNeeded(Preference preference) {
        // Some devices support only a single magnification mode. In these cases, we redirect to
        // the magnification mode's UI directly, rather than showing a PreferenceScreen with a
        // single list item.
        final Context context = preference.getContext();
        if (!MagnificationPreferenceFragment.isApplicable(context.getResources())) {
            preference.setFragment(
                    ToggleScreenMagnificationPreferenceFragmentForSetupWizard.class.getName());
            final Bundle extras = preference.getExtras();
            MagnificationGesturesPreferenceController
                    .populateMagnificationGesturesPreferenceExtras(extras, context);
        }
    }
}
