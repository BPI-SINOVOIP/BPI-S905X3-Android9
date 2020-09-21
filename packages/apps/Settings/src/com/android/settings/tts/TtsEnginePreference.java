/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.settings.tts;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.speech.tts.TextToSpeech.EngineInfo;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceViewHolder;
import android.util.Log;
import android.widget.Checkable;
import android.widget.CompoundButton;
import android.widget.RadioButton;

import com.android.settings.R;
import com.android.settings.SettingsActivity;


public class TtsEnginePreference extends Preference {

    private static final String TAG = "TtsEnginePreference";

    /**
     * The engine information for the engine this preference represents.
     * Contains it's name, label etc. which are used for display.
     */
    private final EngineInfo mEngineInfo;

    /**
     * The shared radio button state, which button is checked etc.
     */
    private final RadioButtonGroupState mSharedState;

    /**
     * When true, the change callbacks on the radio button will not
     * fire.
     */
    private volatile boolean mPreventRadioButtonCallbacks;

    private RadioButton mRadioButton;
    private Intent mVoiceCheckData;

    private final CompoundButton.OnCheckedChangeListener mRadioChangeListener =
        new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                onRadioButtonClicked(buttonView, isChecked);
            }
        };

    public TtsEnginePreference(Context context, EngineInfo info, RadioButtonGroupState state,
            SettingsActivity prefActivity) {
        super(context);
        setLayoutResource(R.layout.preference_tts_engine);

        mSharedState = state;
        mEngineInfo = info;
        mPreventRadioButtonCallbacks = false;

        setKey(mEngineInfo.name);
        setTitle(mEngineInfo.label);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder view) {
        super.onBindViewHolder(view);

        if (mSharedState == null) {
            throw new IllegalStateException("Call to getView() before a call to" +
                    "setSharedState()");
        }

        final RadioButton rb = (RadioButton) view.findViewById(R.id.tts_engine_radiobutton);
        rb.setOnCheckedChangeListener(mRadioChangeListener);
        rb.setText(mEngineInfo.label);

        boolean isChecked = getKey().equals(mSharedState.getCurrentKey());
        if (isChecked) {
            mSharedState.setCurrentChecked(rb);
        }

        mPreventRadioButtonCallbacks = true;
        rb.setChecked(isChecked);
        mPreventRadioButtonCallbacks = false;

        mRadioButton = rb;
    }

    public void setVoiceDataDetails(Intent data) {
        mVoiceCheckData = data;
    }

    private boolean shouldDisplayDataAlert() {
        return !mEngineInfo.system;
    }


    private void displayDataAlert(
            DialogInterface.OnClickListener positiveOnClickListener,
            DialogInterface.OnClickListener negativeOnClickListener) {
        Log.i(TAG, "Displaying data alert for :" + mEngineInfo.name);

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle(android.R.string.dialog_alert_title)
                .setMessage(getContext().getString(
                        R.string.tts_engine_security_warning, mEngineInfo.label))
                .setCancelable(true)
                .setPositiveButton(android.R.string.ok, positiveOnClickListener)
                .setNegativeButton(android.R.string.cancel, negativeOnClickListener);

        AlertDialog dialog = builder.create();
        dialog.show();
    }


    private void onRadioButtonClicked(final CompoundButton buttonView,
            boolean isChecked) {
        if (mPreventRadioButtonCallbacks ||
                (mSharedState.getCurrentChecked() == buttonView)) {
            return;
        }

        if (isChecked) {
            // Should we alert user? if that's true, delay making engine current one.
            if (shouldDisplayDataAlert()) {
                displayDataAlert(new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        makeCurrentEngine(buttonView);
                    }
                },new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // Undo the click.
                        buttonView.setChecked(false);
                    }
                });
            } else {
                // Privileged engine, set it current
                makeCurrentEngine(buttonView);
            }
        }
    }

    private void makeCurrentEngine(Checkable current) {
        if (mSharedState.getCurrentChecked() != null) {
            mSharedState.getCurrentChecked().setChecked(false);
        }
        mSharedState.setCurrentChecked(current);
        mSharedState.setCurrentKey(getKey());
        callChangeListener(mSharedState.getCurrentKey());
    }


    /**
     * Holds all state that is common to this group of radio buttons, such
     * as the currently selected key and the currently checked compound button.
     * (which corresponds to this key).
     */
    public interface RadioButtonGroupState {
        String getCurrentKey();
        Checkable getCurrentChecked();

        void setCurrentKey(String key);
        void setCurrentChecked(Checkable current);
    }

}
