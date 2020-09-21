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

package com.google.android.tv.partner.support;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;

/** Customizations available to partners */
@TargetApi(Build.VERSION_CODES.N)
@SuppressWarnings("AndroidApiChecker") // TODO(b/32513850) remove when error prone is updated
public final class PartnerCustomizations extends BaseCustomization {

    private static final String[] CUSTOMIZE_PERMISSIONS = {
        "com.google.android.tv.permission.CUSTOMIZE_TV_APP"
    };

    public static final String TVPROVIDER_ALLOWS_SYSTEM_INSERTS_TO_PROGRAM_TABLE =
            "tvprovider_allows_system_inserts_to_program_table";

    public PartnerCustomizations(Context context) {
        super(context, CUSTOMIZE_PERMISSIONS);
    }

    public boolean doesTvProviderAllowSystemInsertsToProgramTable(Context context) {
        return getBooleanResource(context, TVPROVIDER_ALLOWS_SYSTEM_INSERTS_TO_PROGRAM_TABLE)
                .orElse(false);
    }
}
