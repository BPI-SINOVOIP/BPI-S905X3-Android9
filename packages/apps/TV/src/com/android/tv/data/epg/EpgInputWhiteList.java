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

package com.android.tv.data.epg;

import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.config.api.RemoteConfig;
import com.android.tv.common.experiments.Experiments;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Checks if a package or a input is white listed. */
public final class EpgInputWhiteList {
    private static final boolean DEBUG = false;
    private static final String TAG = "EpgInputWhiteList";
    @VisibleForTesting public static final String KEY = "live_channels_3rd_party_epg_inputs";
    private static final String QA_DEV_INPUTS =
            "com.example.partnersupportsampletvinput/.SampleTvInputService,"
                    + "com.android.tv.tuner.sample.dvb/.tvinput.SampleDvbTunerTvInputService";

    /** Returns the package portion of a inputId */
    @Nullable
    public static String getPackageFromInput(@Nullable String inputId) {
        return inputId == null ? null : inputId.substring(0, inputId.indexOf("/"));
    }

    private final RemoteConfig remoteConfig;

    public EpgInputWhiteList(RemoteConfig remoteConfig) {
        this.remoteConfig = remoteConfig;
    }

    public boolean isInputWhiteListed(String inputId) {
        return getWhiteListedInputs().contains(inputId);
    }

    public boolean isPackageWhiteListed(String packageName) {
        if (DEBUG) Log.d(TAG, "isPackageWhiteListed " + packageName);
        Set<String> whiteList = getWhiteListedInputs();
        for (String good : whiteList) {
            try {
                String goodPackage = getPackageFromInput(good);
                if (goodPackage.equals(packageName)) {
                    return true;
                }
            } catch (Exception e) {
                if (DEBUG) Log.d(TAG, "Error parsing package name of " + good, e);
                continue;
            }
        }
        return false;
    }

    private Set<String> getWhiteListedInputs() {
        Set<String> result = toInputSet(remoteConfig.getString(KEY));
        if (BuildConfig.ENG || Experiments.ENABLE_QA_FEATURES.get()) {
            HashSet<String> moreInputs = new HashSet<>(toInputSet(QA_DEV_INPUTS));
            if (result.isEmpty()) {
                result = moreInputs;
            } else {
                result.addAll(moreInputs);
            }
        }
        if (DEBUG) Log.d(TAG, "getWhiteListedInputs " + result);
        return result;
    }

    @VisibleForTesting
    static Set<String> toInputSet(String value) {
        if (TextUtils.isEmpty(value)) {
            return Collections.emptySet();
        }
        List<String> strings = Arrays.asList(value.split(","));
        Set<String> result = new HashSet<>(strings.size());
        for (String s : strings) {
            String trimmed = s.trim();
            if (!TextUtils.isEmpty(trimmed)) {
                result.add(trimmed);
            }
        }
        return result;
    }
}
