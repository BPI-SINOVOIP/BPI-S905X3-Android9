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


package com.android.car.settings.system;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ExtraSettingsLoader;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.car.settingslib.language.LanguagePickerUtils;
import com.android.internal.app.LocaleHelper;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Locale;
import java.util.Map;

/**
 * Shows basic info about the system and provide some actions like update, reset etc.
 */
public class SystemSettingsFragment extends ListItemSettingsFragment {

    // Copied from hidden version in android.provider.Settings
    private static final String ACTION_SYSTEM_UPDATE_SETTINGS =
            "android.settings.SYSTEM_UPDATE_SETTINGS";

    private static final String ACTION_SETTING_VIEW_LICENSE =
            "android.settings.WEBVIEW_LICENSE";

    private ListItemProvider mItemProvider;

    public static SystemSettingsFragment getInstance() {
        SystemSettingsFragment systemSettingsFragment = new SystemSettingsFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.system_setting_title);
        systemSettingsFragment.setArguments(bundle);
        return systemSettingsFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mItemProvider = new ListItemProvider.ListProvider(getListItems());
        // super.onActivityCreated() will need itemProvider, so call it after the provider
        // is initialized.
        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }

    private ArrayList<ListItem> getListItems() {
        ArrayList<ListItem> lineItems = new ArrayList<>();

        lineItems.add(createLanguageListItem());
        lineItems.addAll(createSystemUpdateListItems());
        lineItems.add(createAboutSystemListItem());
        lineItems.add(createLegalInfoListItem());

        return lineItems;
    }

    private TextListItem createLanguageListItem() {
        Context context = getContext();
        TextListItem languageItem = new TextListItem(context);
        languageItem.setTitle(context.getString(R.string.language_settings));
        Locale locale = LanguagePickerUtils.getConfiguredLocale();
        languageItem.setBody(LocaleHelper.getDisplayName(locale, locale, /* sentenceCase= */ true));
        languageItem.setPrimaryActionIcon(
                R.drawable.ic_language, /* useLargeIcon= */ false);
        languageItem.setSupplementalIcon(R.drawable.ic_chevron_right, /* showDivider= */ false);
        languageItem.setOnClickListener(
                v -> getFragmentController().launchFragment(LanguagePickerFragment.newInstance()));
        return languageItem;
    }

    private Collection<ListItem> createSystemUpdateListItems() {
        Collection<ListItem> collection = new ArrayList<>();
        Context context = getContext();

        Intent settingsIntent = new Intent(ACTION_SYSTEM_UPDATE_SETTINGS);
        PackageManager packageManager = context.getPackageManager();
        if (settingsIntent.resolveActivity(packageManager) != null) {
            collection.add(new SystemUpdatesListItem(context, settingsIntent));
        }

        ExtraSettingsLoader extraSettingLoader = new ExtraSettingsLoader(context);
        Map<String, Collection<ListItem>> extraSettings = extraSettingLoader.load();
        collection.addAll(extraSettings.get(ExtraSettingsLoader.SYSTEM_CATEGORY));
        return collection;
    }

    private TextListItem createAboutSystemListItem() {
        Context context = getContext();
        TextListItem aboutSystemItem = new TextListItem(context);
        aboutSystemItem.setTitle(context.getString(R.string.about_settings));
        aboutSystemItem.setBody(
                context.getString(R.string.about_summary, Build.VERSION.RELEASE));
        aboutSystemItem.setPrimaryActionIcon(
                R.drawable.ic_settings_about, /* useLargeIcon= */ false);
        aboutSystemItem.setSupplementalIcon(R.drawable.ic_chevron_right, /* showDivider= */ false);
        aboutSystemItem.setOnClickListener(
                v -> getFragmentController().launchFragment(AboutSettingsFragment.getInstance()));
        return aboutSystemItem;
    }

    private TextListItem createLegalInfoListItem() {
        Context context = getContext();
        TextListItem legalInfoItem = new TextListItem(context);
        legalInfoItem.setTitle(context.getString(R.string.legal_information));
        legalInfoItem.setPrimaryActionIcon(
                R.drawable.ic_settings_about, /* useLargeIcon= */ false);
        legalInfoItem.setSupplementalIcon(R.drawable.ic_chevron_right, /* showDivider= */ false);
        legalInfoItem.setOnClickListener(v -> {
            Intent intent = new Intent();
            intent.setAction(ACTION_SETTING_VIEW_LICENSE);
            context.startActivity(intent);
        });
        return legalInfoItem;
    }
}
