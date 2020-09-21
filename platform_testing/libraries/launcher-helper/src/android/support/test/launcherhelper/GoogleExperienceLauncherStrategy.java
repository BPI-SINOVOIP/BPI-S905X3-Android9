/*
 * Copyright (C) 2015 The Android Open Source Project
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
package android.support.test.launcherhelper;

import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;

/**
 * Implementation of {@link ILauncherStrategy} to support Google experience launcher
 */
public class GoogleExperienceLauncherStrategy extends BaseLauncher3Strategy {

    private static final String LAUNCHER_PKG = "com.google.android.googlequicksearchbox";

    @Override
    public String getSupportedLauncherPackage() {
        return LAUNCHER_PKG;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getAllAppsButtonSelector() {
        return By.desc("Apps");
    }
}
