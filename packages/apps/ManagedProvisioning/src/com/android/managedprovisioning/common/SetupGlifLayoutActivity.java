/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.common;

import android.annotation.Nullable;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.os.SystemProperties;
import android.support.annotation.VisibleForTesting;

import com.android.managedprovisioning.R;
import com.android.setupwizardlib.GlifLayout;
import com.android.setupwizardlib.util.WizardManagerHelper;

/**
 * Base class for setting up the layout.
 */
public abstract class SetupGlifLayoutActivity extends SetupLayoutActivity {
    public SetupGlifLayoutActivity() {
        super();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setDefaultTheme();
    }

    @VisibleForTesting
    protected SetupGlifLayoutActivity(Utils utils) {
        super(utils);
    }

    protected void initializeLayoutParams(int layoutResourceId, @Nullable Integer headerResourceId,
            int mainColor, int statusBarColor) {
        setContentView(layoutResourceId);
        GlifLayout layout = findViewById(R.id.setup_wizard_layout);

        setStatusBarColor(statusBarColor);
        layout.setPrimaryColor(ColorStateList.valueOf(mainColor));

        if (headerResourceId != null) {
            layout.setHeaderText(headerResourceId);
        }

        layout.setIcon(LogoUtils.getOrganisationLogo(this, mainColor));
    }

    private void setDefaultTheme() {
        // Take Glif light as default theme like
        // com.google.android.setupwizard.util.ThemeHelper.getDefaultTheme
        setTheme(WizardManagerHelper.getThemeRes(SystemProperties.get("setupwizard.theme"),
                R.style.SuwThemeGlif_Light));
    }

}