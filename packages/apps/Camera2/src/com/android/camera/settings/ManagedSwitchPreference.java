/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.camera.settings;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.preference.SwitchPreference;
import android.util.AttributeSet;

import com.android.camera.app.CameraApp;
import com.android.camera.app.CameraServicesImpl;

/**
 * This class allows Settings UIs to display and set boolean values controlled
 * via the {@link SettingManager}. The Default {@link SwitchPreference} uses
 * {@link android.content.SharedPreferences} as a backing store; since the
 * {@link SettingManager} stores all settings as Strings we need to ensure we
 * get and set boolean settings through the manager.
 */
public class ManagedSwitchPreference extends SwitchPreference {
    public ManagedSwitchPreference(Context context) {
        super(context);
    }

    public ManagedSwitchPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ManagedSwitchPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public boolean getPersistedBoolean(boolean defaultReturnValue) {
        CameraApp cameraApp = getCameraApp();
        if (cameraApp == null) {
            // The context and app may not be initialized upon initial inflation of the
            // preference from XML. In that case return the default value.
            return defaultReturnValue;
        }
        SettingsManager settingsManager = CameraServicesImpl.instance().getSettingsManager();
        if (settingsManager != null) {
            return settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL, getKey());
        } else {
            // If the SettingsManager is for some reason not initialized,
            // perhaps triggered by a monkey, return default value.
            return defaultReturnValue;
        }
    }

    @Override
    public boolean persistBoolean(boolean value) {
        CameraApp cameraApp = getCameraApp();
        if (cameraApp == null) {
            // The context may not be initialized upon initial inflation of the
            // preference from XML. In that case return false to note the value won't
            // be persisted.
            return false;
        }
        SettingsManager settingsManager = CameraServicesImpl.instance().getSettingsManager();
        if (settingsManager != null) {
            settingsManager.set(SettingsManager.SCOPE_GLOBAL, getKey(), value);
            return true;
        } else {
            // If the SettingsManager is for some reason not initialized,
            // perhaps triggered by a monkey, return false to note the value
            // was not persisted.
            return false;
        }
    }

    private CameraApp getCameraApp() {
        Context context = getContext();
        if (context instanceof Activity) {
            Application application = ((Activity) context).getApplication();
            if (application instanceof CameraApp) {
                return (CameraApp) application;
            }
        }
        return null;
    }
}
