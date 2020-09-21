/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.settings.search;

import static android.provider.SearchIndexablesContract.COLUMN_INDEX_NON_INDEXABLE_KEYS_KEY_VALUE;
import static com.google.common.truth.Truth.assertThat;
import static junit.framework.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.database.Cursor;
import android.text.TextUtils;

import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.FakeIndexProvider;
import com.android.settings.testutils.SettingsRobolectricTestRunner;
import com.android.settings.wifi.WifiSettings;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

@RunWith(SettingsRobolectricTestRunner.class)
public class SearchIndexableResourcesTest {

    private SearchFeatureProviderImpl mSearchProvider;
    private FakeFeatureFactory mFakeFeatureFactory;

    @Before
    public void setUp() {
        mSearchProvider = new SearchFeatureProviderImpl();
        mFakeFeatureFactory = FakeFeatureFactory.setupForTest();
        mFakeFeatureFactory.searchFeatureProvider = mSearchProvider;
    }

    @After
    public void cleanUp() {
        mFakeFeatureFactory.searchFeatureProvider = mock(SearchFeatureProvider.class);
    }

    @Test
    public void testAddIndex() {
        // Confirms that String.class isn't contained in SearchIndexableResources.
        assertThat(mSearchProvider.getSearchIndexableResources().getProviderValues())
                .doesNotContain(String.class);
        final int beforeCount =
                mSearchProvider.getSearchIndexableResources().getProviderValues().size();

        ((SearchIndexableResourcesImpl) mSearchProvider.getSearchIndexableResources())
                .addIndex(String.class);

        assertThat(mSearchProvider.getSearchIndexableResources().getProviderValues())
                .contains(String.class);
        final int afterCount =
                mSearchProvider.getSearchIndexableResources().getProviderValues().size();
        assertThat(afterCount).isEqualTo(beforeCount + 1);
    }

    @Test
    public void testIndexHasWifiSettings() {
        assertThat(mSearchProvider.getSearchIndexableResources().getProviderValues())
                .contains(WifiSettings.class);
    }

    @Test
    public void testNonIndexableKeys_GetsKeyFromProvider() {
        mSearchProvider.getSearchIndexableResources().getProviderValues().clear();
        ((SearchIndexableResourcesImpl) mSearchProvider.getSearchIndexableResources())
                .addIndex(FakeIndexProvider.class);

        SettingsSearchIndexablesProvider provider = spy(new SettingsSearchIndexablesProvider());

        when(provider.getContext()).thenReturn(RuntimeEnvironment.application);

        Cursor cursor = provider.queryNonIndexableKeys(null);
        boolean hasTestKey = false;
        while (cursor.moveToNext()) {
            String key = cursor.getString(COLUMN_INDEX_NON_INDEXABLE_KEYS_KEY_VALUE);
            if (TextUtils.equals(key, FakeIndexProvider.KEY)) {
                hasTestKey = true;
                break;
            }
        }

        assertThat(hasTestKey).isTrue();
    }

    @Test
    public void testAllClassNamesHaveProviders() {
        for (Class clazz : mSearchProvider.getSearchIndexableResources().getProviderValues()) {
            if (DatabaseIndexingUtils.getSearchIndexProvider(clazz) == null) {
                fail(clazz.getName() + "is not an index provider");
            }
        }
    }
}
