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

package com.android.tv.settings.core.instrumentation;

import android.os.Bundle;
import android.support.v4.app.FragmentActivity;

import com.android.settingslib.core.instrumentation.Instrumentable;
import com.android.settingslib.core.instrumentation.MetricsFeatureProvider;
import com.android.settingslib.core.instrumentation.VisibilityLoggerMixin;

/**
 * Instrumented activity that logs visibility state.
 */
public abstract class InstrumentedActivity extends FragmentActivity implements Instrumentable {

    protected MetricsFeatureProvider mMetricsFeatureProvider;
    protected VisibilityLoggerMixin mVisibilityLoggerMixin;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Mixin that logs visibility change for activity.
        mMetricsFeatureProvider = new MetricsFeatureProvider();
        mVisibilityLoggerMixin = new VisibilityLoggerMixin(getMetricsCategory(),
            mMetricsFeatureProvider);
        getLifecycle().addObserver(mVisibilityLoggerMixin);
    }
}
