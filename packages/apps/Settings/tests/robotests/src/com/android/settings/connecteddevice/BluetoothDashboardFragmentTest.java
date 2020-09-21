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
 * limitations under the License
 */

package com.android.settings.connecteddevice;

import static com.google.common.truth.Truth.assertThat;

import android.content.Context;
import android.text.TextUtils;

import com.android.settings.R;
import com.android.settings.search.SearchIndexableRaw;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;

import java.util.List;
import java.util.stream.Collectors;

@RunWith(SettingsRobolectricTestRunner.class)
public class BluetoothDashboardFragmentTest {

    private Context mContext;

    @Before
    public void setUp() {
        mContext = RuntimeEnvironment.application;
    }

    @Test
    public void rawData_includesFragmentResult() {
        final List<SearchIndexableRaw> rawList =
                BluetoothDashboardFragment.SEARCH_INDEX_DATA_PROVIDER.getRawDataToIndex(mContext,
                        true /* enabled */);

        final SearchIndexableRaw fragmentResult = rawList.stream().filter(
                raw -> TextUtils.equals(raw.title,
                        mContext.getString(R.string.bluetooth_settings))).findFirst().get();


        assertThat(fragmentResult).isNotNull();
    }

}
