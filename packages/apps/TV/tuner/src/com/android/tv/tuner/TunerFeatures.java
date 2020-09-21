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

package com.android.tv.tuner;

import static com.android.tv.common.feature.FeatureUtils.OFF;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.common.BaseApplication;
import com.android.tv.common.config.api.RemoteConfig;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.feature.Feature;
import com.android.tv.common.feature.Model;
import com.android.tv.common.feature.PropertyFeature;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.common.util.LocationUtils;
import java.util.Locale;

/**
 * List of {@link Feature} for Tuner.
 *
 * <p>Remove the {@code Feature} once it is launched.
 */
public class TunerFeatures extends CommonFeatures {
    private static final String TAG = "TunerFeatures";
    private static final boolean DEBUG = false;

    /** Use network tuner if it is available and there is no other tuner types. */
    public static final Feature NETWORK_TUNER =
            new Feature() {
                @Override
                public boolean isEnabled(Context context) {
                    if (!TUNER.isEnabled(context)) {
                        return false;
                    }
                    if (CommonUtils.isDeveloper()) {
                        // Network tuner will be enabled for developers.
                        return true;
                    }
                    return Locale.US
                            .getCountry()
                            .equalsIgnoreCase(LocationUtils.getCurrentCountry(context));
                }
            };

    /**
     * USE_SW_CODEC_FOR_SD
     *
     * <p>Prefer software based codec for SD channels.
     */
    public static final Feature USE_SW_CODEC_FOR_SD =
            PropertyFeature.create(
                    "use_sw_codec_for_sd",
                    false
                    );

    /** Use AC3 software decode. */
    public static final Feature AC3_SOFTWARE_DECODE =
            new Feature() {
                private final String[] SUPPORTED_REGIONS = {};

                private Boolean mEnabled;

                @Override
                public boolean isEnabled(Context context) {
                    if (mEnabled == null) {
                        if (mEnabled == null) {
                            // We will not cache the result of fallback solution.
                            String country = LocationUtils.getCurrentCountry(context);
                            for (int i = 0; i < SUPPORTED_REGIONS.length; ++i) {
                                if (SUPPORTED_REGIONS[i].equalsIgnoreCase(country)) {
                                    return true;
                                }
                            }
                            if (DEBUG) Log.d(TAG, "AC3 flag false after country check");
                            return false;
                        }
                    }
                    if (DEBUG) Log.d(TAG, "AC3 flag " + mEnabled);
                    return mEnabled;
                }
            };

    /** Enable Dvb parsers and listeners. */
    public static final Feature ENABLE_FILE_DVB = OFF;

    private TunerFeatures() {}
}
