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

package com.android.settings.core;

import static junit.framework.Assert.fail;

import android.content.Context;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.ArraySet;

import com.android.settings.overlay.FeatureFactory;
import com.android.settings.search.DatabaseIndexingUtils;
import com.android.settings.search.Indexable;
import com.android.settings.search.SearchIndexableResources;
import com.android.settingslib.core.AbstractPreferenceController;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;
import java.util.Set;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class PreferenceControllerContractTest {

    private Context mContext;

    @Before
    public void setUp() {
        mContext = InstrumentationRegistry.getTargetContext();
    }

    @Test
    @Presubmit
    public void controllersInSearchShouldImplementPreferenceControllerMixin() {
        final Set<String> errorClasses = new ArraySet<>();

        final SearchIndexableResources resources =
                FeatureFactory.getFactory(mContext).getSearchFeatureProvider()
                        .getSearchIndexableResources();
        for (Class<?> clazz : resources.getProviderValues()) {

            final Indexable.SearchIndexProvider provider =
                    DatabaseIndexingUtils.getSearchIndexProvider(clazz);
            if (provider == null) {
                continue;
            }

            final List<AbstractPreferenceController> controllers =
                    provider.getPreferenceControllers(mContext);
            if (controllers == null) {
                continue;
            }
            for (AbstractPreferenceController controller : controllers) {
                if (!(controller instanceof PreferenceControllerMixin)
                        && !(controller instanceof BasePreferenceController)) {
                    errorClasses.add(controller.getClass().getName());
                }
            }
        }

        if (!errorClasses.isEmpty()) {
            final StringBuilder errorMessage = new StringBuilder()
                    .append("Each preference must implement PreferenceControllerMixin ")
                    .append("or extend BasePreferenceController, ")
                    .append("the following classes don't:\n");
            for (String c : errorClasses) {
                errorMessage.append(c).append("\n");
            }
            fail(errorMessage.toString());
        }
    }
}
