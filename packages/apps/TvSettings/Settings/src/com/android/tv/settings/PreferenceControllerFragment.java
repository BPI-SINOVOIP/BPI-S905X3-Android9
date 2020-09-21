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

package com.android.tv.settings;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.XmlRes;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.PreferenceScreen;
import android.util.ArraySet;
import android.util.Log;

import com.android.settingslib.core.AbstractPreferenceController;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Fragment for managing a screen of preferences controlled entirely through
 * {@link AbstractPreferenceController} objects. Based on DashboardFragment from mobile settings,
 * but without the Tile support.
 */
public abstract class PreferenceControllerFragment extends SettingsPreferenceFragment {
    private static final String TAG = "PrefControllerFrag";

    private final Set<AbstractPreferenceController> mPreferenceControllers = new ArraySet<>();

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        List<AbstractPreferenceController> controllers = onCreatePreferenceControllers(context);
        if (controllers == null) {
            controllers = new ArrayList<>();
        }
        mPreferenceControllers.addAll(controllers);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getPreferenceManager().setPreferenceComparisonCallback(
                new PreferenceManager.SimplePreferenceComparisonCallback());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(getPreferenceScreenResId(), null);
        refreshAllPreferences();
    }

    @Override
    public void onResume() {
        super.onResume();
        updatePreferenceStates();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        // Copy before iterating, in case a preference controller tries to update the list
        Collection<AbstractPreferenceController> controllers =
                new ArrayList<>(mPreferenceControllers);
        // If preference contains intent, log it before handling.
        // Give all controllers a chance to handle click.
        for (AbstractPreferenceController controller : controllers) {
            if (controller.handlePreferenceTreeClick(preference)) {
                return true;
            }
        }
        return super.onPreferenceTreeClick(preference);
    }

    @SuppressWarnings("unchecked")
    protected <T extends AbstractPreferenceController> T getOnePreferenceController(
            Class<T> clazz) {
        final List<AbstractPreferenceController> foundControllers =
                mPreferenceControllers.stream()
                        .filter(clazz::isInstance).collect(Collectors.toList());
        if (foundControllers.size() > 0) {
            return (T) foundControllers.get(0);
        } else {
            return null;
        }
    }

    @SuppressWarnings("unchecked")
    protected  <T extends AbstractPreferenceController> Collection<T> getPreferenceControllers(
            Class<T> clazz) {
        return (Collection<T>) mPreferenceControllers.stream()
                .filter(clazz::isInstance).collect(Collectors.toList());
    }

    protected void addPreferenceController(AbstractPreferenceController controller) {
        mPreferenceControllers.add(controller);
    }

    /**
     * Update state of each preference managed by PreferenceController.
     */
    protected void updatePreferenceStates() {
        Collection<AbstractPreferenceController> controllers =
                new ArrayList<>(mPreferenceControllers);
        final PreferenceScreen screen = getPreferenceScreen();
        for (AbstractPreferenceController controller : controllers) {
            if (!controller.isAvailable()) {
                continue;
            }
            final String key = controller.getPreferenceKey();

            final Preference preference = screen.findPreference(key);
            if (preference == null) {
                Log.d(TAG, "Cannot find preference with key " + key
                        + " in Controller " + controller.getClass().getSimpleName());
                continue;
            }
            controller.updateState(preference);
        }
    }

    private void refreshAllPreferences() {
        final PreferenceScreen screen = getPreferenceScreen();
        Collection<AbstractPreferenceController> controllers =
                new ArrayList<>(mPreferenceControllers);
        for (AbstractPreferenceController controller : controllers) {
            controller.displayPreference(screen);
        }
    }

    /**
     * Get the res id for static preference xml for this fragment.
     */
    @XmlRes
    protected abstract int getPreferenceScreenResId();

    /**
     * Get a list of {@link AbstractPreferenceController} for this fragment.
     */
    protected abstract List<AbstractPreferenceController> onCreatePreferenceControllers(
            Context context);
}
