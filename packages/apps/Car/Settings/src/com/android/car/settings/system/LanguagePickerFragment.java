/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.settings.system;

import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.MutableLiveData;
import android.arch.lifecycle.ViewModel;
import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;

import androidx.car.widget.ListItemProvider;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.car.settingslib.language.LanguagePickerUtils;
import com.android.car.settingslib.language.LocaleListItemProvider;
import com.android.car.settingslib.language.LocaleSelectionListener;
import com.android.internal.app.LocalePicker;
import com.android.internal.app.LocaleStore;

import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

/**
 * Fragment for showing the list of languages.
 */
public class LanguagePickerFragment extends ListItemSettingsFragment implements
        LocaleSelectionListener {

    private LocaleListItemProvider mLocaleListItemProvider;
    private final HashSet<String> mLangTagsToIgnore = new HashSet<>();

    /**
     * Factory method for creating LanguagePickerFragment.
     */
    public static LanguagePickerFragment newInstance() {
        LanguagePickerFragment LanguagePickerFragment = new LanguagePickerFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.language_settings);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar);
        LanguagePickerFragment.setArguments(bundle);
        return LanguagePickerFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        LocaleViewModel viewModel = getLocaleViewModel();
        viewModel.getLocaleInfos(getContext(), mLangTagsToIgnore).observe(this,
                localeInfos -> resetLocaleList(localeInfos));
    }

    @Override
    public ListItemProvider getItemProvider() {
        if (mLocaleListItemProvider == null) {
            mLocaleListItemProvider = new LocaleListItemProvider(
                    getContext(),
                    new HashSet<LocaleStore.LocaleInfo>(),
                    /* localeSelectionListener= */ this,
                    mLangTagsToIgnore);
        }
        return mLocaleListItemProvider;
    }

    @Override
    public void onLocaleSelected(LocaleStore.LocaleInfo localeInfo) {
        if (localeInfo == null || localeInfo.getLocale() == null) {
            return;
        }

        Locale locale = localeInfo.getLocale();

        Resources res = getResources();
        Configuration baseConfig = res.getConfiguration();
        Configuration config = new Configuration(baseConfig);
        config.setLocale(locale);
        res.updateConfiguration(config, null);

        // Apply locale to system.
        LocalePicker.updateLocale(locale);
        getFragmentController().goBack();
    }

    @Override
    public void onParentWithChildrenLocaleSelected(LocaleStore.LocaleInfo localeInfo) {
        if (localeInfo != null) {
            setTitle(localeInfo.getFullNameNative());
            refreshList();
        }
    }

    @Override
    protected void onBackPressed() {
        if (isChildLocaleDisplayed()) {
            setTitle(getString(R.string.language_settings));
            getLocaleViewModel().reloadLocales(getContext(), mLangTagsToIgnore);
        } else {
            super.onBackPressed();
        }
    }

    private LocaleViewModel getLocaleViewModel() {
        return ViewModelProviders.of(this).get(LocaleViewModel.class);
    }

    private boolean isChildLocaleDisplayed() {
        return mLocaleListItemProvider != null && mLocaleListItemProvider.isChildLocale();
    }

    /**
     * Add a pseudo locale in debug build for testing RTL.
     *
     * @param localeInfos the set of {@link LocaleStore.LocaleInfo} to which the locale is added.
     */
    private void maybeAddPseudoLocale(Set<LocaleStore.LocaleInfo> localeInfos) {
        if (Build.IS_USERDEBUG) {
            // The ar-XB pseudo-locale is RTL.
            localeInfos.add(LocaleStore.getLocaleInfo(new Locale("ar", "XB")));
        }
    }

    private void resetLocaleList(Set<LocaleStore.LocaleInfo> localeInfos) {
        if (mLocaleListItemProvider != null) {
            maybeAddPseudoLocale(localeInfos);
            mLocaleListItemProvider.updateSuggestedLocaleAdapter(
                    LanguagePickerUtils.createSuggestedLocaleAdapter(
                            getContext(), localeInfos, /* parent= */ null),
                    /* isChildLocale= */ false);
            refreshList();
        }
    }

    /**
     * ViewModel for holding the LocaleInfos.
     */
    public static class LocaleViewModel extends ViewModel {

        private MutableLiveData<Set<LocaleStore.LocaleInfo>> mLocaleInfos;

        /**
         * Returns LiveData holding a set of LocaleInfo.
         */
        public LiveData<Set<LocaleStore.LocaleInfo>> getLocaleInfos(Context context,
                Set<String> ignorables) {

            if (mLocaleInfos == null) {
                mLocaleInfos = new MutableLiveData<Set<LocaleStore.LocaleInfo>>();
                reloadLocales(context, ignorables);
            }
            return mLocaleInfos;
        }

        /**
         * Reload the locales based on the current context.
         */
        public void reloadLocales(Context context, Set<String> ignorables) {
            new AsyncTask<Context, Void, Set<LocaleStore.LocaleInfo>>() {
                @Override
                protected Set<LocaleStore.LocaleInfo> doInBackground(Context... contexts) {
                    return LocaleStore.getLevelLocales(
                            contexts[0],
                            ignorables,
                            /* parent= */ null,
                            /* translatedOnly= */ true);
                }

                @Override
                protected void onPostExecute(Set<LocaleStore.LocaleInfo> localeInfos) {
                    LocaleViewModel.this.mLocaleInfos.setValue(localeInfos);
                }
            }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, context);
        }
    }
}
