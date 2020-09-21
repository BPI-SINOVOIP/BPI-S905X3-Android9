/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package com.android.settings.enterprise;

import android.content.Context;

import com.android.settings.core.BasePreferenceController;
import com.android.settings.overlay.FeatureFactory;

public class BackupsEnabledPreferenceController extends BasePreferenceController {

    private static final String KEY_BACKUPS_ENABLED = "backups_enabled";
    private final EnterprisePrivacyFeatureProvider mFeatureProvider;

    public BackupsEnabledPreferenceController(Context context) {
        super(context, KEY_BACKUPS_ENABLED);
        mFeatureProvider = FeatureFactory.getFactory(context)
                .getEnterprisePrivacyFeatureProvider(context);
    }

    @Override
    public int getAvailabilityStatus() {
        return mFeatureProvider.areBackupsMandatory() ? AVAILABLE : DISABLED_FOR_USER;
    }
}

