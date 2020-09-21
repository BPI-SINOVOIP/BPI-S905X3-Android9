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

package com.android.settings.inputmethod;

import android.app.AlertDialog.Builder;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.support.v7.preference.PreferenceViewHolder;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.textservice.SpellCheckerInfo;

import com.android.settings.CustomListPreference;
import com.android.settings.R;

/**
 * Spell checker service preference.
 *
 * This preference represents a spell checker service. It is used for two purposes. 1) A radio
 * button on the left side is used to choose the current spell checker service. 2) A settings
 * icon on the right side is used to invoke the setting activity of the spell checker service.
 */
class SpellCheckerPreference extends CustomListPreference {

    private final SpellCheckerInfo[] mScis;
    private Intent mIntent;

    public SpellCheckerPreference(final Context context, final SpellCheckerInfo[] scis) {
        super(context, null);
        mScis = scis;
        setWidgetLayoutResource(R.layout.preference_widget_gear);
        CharSequence[] labels = new CharSequence[scis.length];
        CharSequence[] values = new CharSequence[scis.length];
        for (int i = 0 ; i < scis.length; i++) {
            labels[i] = scis[i].loadLabel(context.getPackageManager());
            // Use values as indexing since ListPreference doesn't support generic objects.
            values[i] = String.valueOf(i);
        }
        setEntries(labels);
        setEntryValues(values);
    }

    @Override
    protected void onPrepareDialogBuilder(Builder builder,
            DialogInterface.OnClickListener listener) {
        builder.setTitle(R.string.choose_spell_checker);
        builder.setSingleChoiceItems(getEntries(), findIndexOfValue(getValue()), listener);
    }

    public void setSelected(SpellCheckerInfo currentSci) {
        if (currentSci == null) {
            setValue(null);
            return;
        }
        for (int i = 0; i < mScis.length; i++) {
            if (mScis[i].getId().equals(currentSci.getId())) {
                setValueIndex(i);
                return;
            }
        }
    }

    @Override
    public void setValue(String value) {
        super.setValue(value);
        int index = value != null ? Integer.parseInt(value) : -1;
        if (index == -1) {
            mIntent = null;
            return;
        }
        SpellCheckerInfo sci = mScis[index];
        final String settingsActivity = sci.getSettingsActivity();
        if (TextUtils.isEmpty(settingsActivity)) {
            mIntent = null;
        } else {
            mIntent = new Intent(Intent.ACTION_MAIN);
            mIntent.setClassName(sci.getPackageName(), settingsActivity);
        }
    }

    @Override
    public boolean callChangeListener(Object newValue) {
        newValue = newValue != null ? mScis[Integer.parseInt((String) newValue)] : null;
        return super.callChangeListener(newValue);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder view) {
        super.onBindViewHolder(view);
        View settingsButton = view.findViewById(R.id.settings_button);
        settingsButton.setVisibility(mIntent != null ? View.VISIBLE : View.INVISIBLE);
        settingsButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                onSettingsButtonClicked();
            }
        });
    }

    private void onSettingsButtonClicked() {
        final Context context = getContext();
        try {
            final Intent intent = mIntent;
            if (intent != null) {
                // Invoke a settings activity of an spell checker.
                context.startActivity(intent);
            }
        } catch (final ActivityNotFoundException e) {
        }
    }
}
