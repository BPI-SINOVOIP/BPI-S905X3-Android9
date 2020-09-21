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

package com.android.settings.display;

import android.content.Context;
import android.os.UserHandle;
import android.provider.SearchIndexableResource;

import com.android.internal.hardware.AmbientDisplayConfiguration;
import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.gestures.DoubleTapScreenPreferenceController;
import com.android.settings.gestures.PickupGesturePreferenceController;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.instrumentation.MetricsFeatureProvider;
import com.android.settingslib.core.lifecycle.Lifecycle;

import java.util.ArrayList;
import java.util.List;

/**
 * Settings screen for Ambient display.
 */
public class AmbientDisplaySettings extends DashboardFragment {

    public static final String KEY_AMBIENT_DISPLAY_ALWAYS_ON = "ambient_display_always_on";

    private static final String TAG = "AmbientDisplaySettings";

    private AmbientDisplayConfiguration mConfig;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        use(AmbientDisplayAlwaysOnPreferenceController.class)
            .setConfig(getConfig(context))
            .setCallback(this::updatePreferenceStates);
        use(AmbientDisplayNotificationsPreferenceController.class).setConfig(getConfig(context));
        use(DoubleTapScreenPreferenceController.class).setConfig(getConfig(context));
        use(PickupGesturePreferenceController.class).setConfig(getConfig(context));
    }

    @Override
    protected String getLogTag() {
        return TAG;
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.ambient_display_settings;
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.AMBIENT_DISPLAY_SETTINGS;
    }

    public static final Indexable.SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
                        boolean enabled) {
                    final ArrayList<SearchIndexableResource> result = new ArrayList<>();

                    final SearchIndexableResource sir = new SearchIndexableResource(context);
                    sir.xmlResId = R.xml.ambient_display_settings;
                    result.add(sir);
                    return result;
                }
            };

    private AmbientDisplayConfiguration getConfig(Context context) {
        if (mConfig == null) {
            mConfig = new AmbientDisplayConfiguration(context);
        }
        return mConfig;
    }
}
