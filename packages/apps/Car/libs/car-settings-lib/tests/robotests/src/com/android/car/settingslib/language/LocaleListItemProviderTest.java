/*
 * Copyright (C) 2018 Google Inc.
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
package com.android.car.settingslib.language;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.verify;
import static org.robolectric.RuntimeEnvironment.application;

import android.content.Context;
import android.telephony.TelephonyManager;

import com.android.car.settingslib.robolectric.BaseRobolectricTest;
import com.android.car.settingslib.robolectric.CarSettingsLibRobolectricTestRunner;
import com.android.car.settingslib.shadows.ShadowLocalePicker;
import com.android.internal.app.LocaleStore;
import com.android.internal.app.SuggestedLocaleAdapter;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.shadows.ShadowTelephonyManager;

import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

import androidx.car.widget.ListItemAdapter;
import androidx.car.widget.PagedListView;

/**
 * Tests {@link LocaleListItemProvider} to validate
 */
@RunWith(CarSettingsLibRobolectricTestRunner.class)
@Config(shadows = ShadowLocalePicker.class)
public class LocaleListItemProviderTest extends BaseRobolectricTest {
    private static final int TYPE_HEADER_SUGGESTED = 0;
    private static final int TYPE_HEADER_ALL_OTHERS = 1;
    private static final int TYPE_LOCALE = 2;
    private static final String[] TEST_LOCALES = new String[] {
            "en-AU",
            "en-CA",
            "en-GB",
            "en-IN",
            "en-NZ",
            "en-US",
            "es-ES",
            "es-MX",
            "es-US",
            "ja-JP"};
    private static final HashSet<String> IGNORABLE_TAGS = new HashSet<>();

    @Mock
    private LocaleSelectionListener mLocaleSelectionListener;
    @Mock private ListItemAdapter mMockListItemAdapter;

    private Context mContext;
    private LocaleListItemProvider mLocaleListItemProvider;
    private SuggestedLocaleAdapter mSuggestedLocaleAdapter;
    private Set<LocaleStore.LocaleInfo> mLocaleInfos = new HashSet<>();

    @Before
    public void setUpComponents() {
        ShadowLocalePicker.setSystemAssetLocales(TEST_LOCALES);
        ShadowLocalePicker.setSupportedLocales(TEST_LOCALES);
        ShadowApplication shadowApplication = Shadows.shadowOf(application);
        ShadowTelephonyManager shadowTelephonyManager =
                Shadows.shadowOf(application.getSystemService(TelephonyManager.class));
        shadowTelephonyManager.setSimCountryIso("");
        shadowTelephonyManager.setNetworkCountryIso("");
        mContext = shadowApplication.getApplicationContext();
        Locale.setDefault(new Locale("en", "US"));
        LocaleStore.fillCache(mContext);
        mLocaleInfos = LocaleStore.getLevelLocales(mContext, new HashSet<>(), null, true);
        PagedListView pagedListView = new PagedListView(mContext, null);
        pagedListView.setAdapter(mMockListItemAdapter);
        mLocaleListItemProvider = new LocaleListItemProvider(
                mContext,
                mLocaleInfos,
                mLocaleSelectionListener,
                IGNORABLE_TAGS);
        mSuggestedLocaleAdapter = mLocaleListItemProvider.getSuggestedLocaleAdapter();
    }

    @Test
    public void testHeaders() {
        // Validate headers are present by checking the number of ViewTypes, when headers are
        // present there should be 3 types:
        //    TYPE_HEADER_SUGGESTED = 0
        //    TYPE_HEADER_ALL_OTHERS = 1;
        //    TYPE_LOCALE = 2;
        assertThat(mSuggestedLocaleAdapter.getViewTypeCount()).isEqualTo(3);

        // Validate provider size, when using TEST_LOCALES the breakdown of the items is as follows:
        // There are 10 total locales, 3 distinct languages, and US will be a suggested country.
        // This yields:
        //    2 headers - "Suggested", "All languages"
        //    2 suggestions - en-us, es-us
        //    3 languages - en, es, ja
        // Which means we should a size of 7 for the provider.
        assertThat(mLocaleListItemProvider.size()).isEqualTo(7);

        // The TYPE_HEADER_SUGGESTED = 0 "Suggested" should be located at the 0th position.
        assertThat(mSuggestedLocaleAdapter.getItemViewType(0)).isEqualTo(TYPE_HEADER_SUGGESTED);

        // The TYPE_LOCALE = 2, which is a localeInfo entry [example: "English (United States)]
        // should be located at the 1st and 2nd position, for brevity only checking the 1st.
        assertThat(mSuggestedLocaleAdapter.getItemViewType(1)).isEqualTo(TYPE_LOCALE);

        // The TYPE_HEADER_ALL_OTHERS = 1 "All languages" should be located at the 3rd position.
        assertThat(mSuggestedLocaleAdapter.getItemViewType(3)).isEqualTo(TYPE_HEADER_ALL_OTHERS);
    }

    @Test
    public void testLocalesWithChildren() {
        LocaleStore.LocaleInfo localeInfo =
                (LocaleStore.LocaleInfo) mSuggestedLocaleAdapter.getItem(1);
        // en-US
        assertThat(localeInfo.getId()).isEqualTo("en-US");
        assertThat(localeInfo.getParent()).isNotNull();

        // en
        localeInfo = (LocaleStore.LocaleInfo) mSuggestedLocaleAdapter.getItem(4);
        assertThat(localeInfo.getId()).isEqualTo("en");
        assertThat(localeInfo.getParent()).isNull();

        // es
        localeInfo = (LocaleStore.LocaleInfo) mSuggestedLocaleAdapter.getItem(5);
        assertThat(localeInfo.getId()).isEqualTo("es");
        assertThat(localeInfo.getParent()).isNull();

        // ja
        localeInfo = (LocaleStore.LocaleInfo) mSuggestedLocaleAdapter.getItem(6);
        Set<LocaleStore.LocaleInfo> localeInfoSet =
                LocaleStore.getLevelLocales(mContext, IGNORABLE_TAGS, localeInfo,true);
        assertThat(localeInfo.getId()).isEqualTo("ja");
        assertThat(localeInfo.getParent()).isNull();
        // In TEST_LOCALES the language ja has only one locale
        assertThat(localeInfoSet).hasSize(1);
    }

    @Test
    public void testUpdateSuggestedLocaleAdapter() {
        LocaleStore.LocaleInfo localeInfo = LocaleStore.getLocaleInfo(new Locale("es"));
        Set<LocaleStore.LocaleInfo> localeInfoSet =
                LocaleStore.getLevelLocales(mContext, IGNORABLE_TAGS, localeInfo,true);
        SuggestedLocaleAdapter suggestedLocaleAdapter =
                LanguagePickerUtils.createSuggestedLocaleAdapter(
                        mContext, localeInfoSet, localeInfo);
        mLocaleListItemProvider.updateSuggestedLocaleAdapter(suggestedLocaleAdapter, true);
        assertThat(mLocaleListItemProvider.isChildLocale()).isTrue();
        assertThat(mLocaleListItemProvider.getSuggestedLocaleAdapter())
                .isEqualTo(suggestedLocaleAdapter);
    }
}
