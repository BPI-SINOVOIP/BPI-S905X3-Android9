/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.tv.settings.autofill;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.UserHandle;
import android.support.annotation.Keep;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.Html;
import android.widget.Button;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.applications.DefaultAppInfo;
import com.android.settingslib.wrapper.PackageManagerWrapper;
import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.List;

/**
 * Picker Fragment to select Autofill service
 */
@Keep
public class AutofillPickerFragment extends SettingsPreferenceFragment {

    private static final String AUTOFILL_SERVICE_RADIO_GROUP = "autofill_service_group";

    @VisibleForTesting
    static final String KEY_FOR_NONE = "_none_";

    private static final int FINISH_ACTIVITY_DELAY = 300;

    private PackageManagerWrapper mPm;

    private final Handler mHandler = new Handler();

    /**
     * Set when the fragment is implementing ACTION_REQUEST_SET_AUTOFILL_SERVICE.
     */
    private DialogInterface.OnClickListener mCancelListener;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Activity activity = getActivity();
        if (activity != null && activity.getIntent()
                .getStringExtra(AutofillPickerActivity.EXTRA_PACKAGE_NAME) != null) {
            mCancelListener = (d, w) -> {
                activity.setResult(Activity.RESULT_CANCELED);
                activity.finish();
            };
        }
    }

    /**
     * @return new AutofillPickerFragment instance
     */
    public static AutofillPickerFragment newInstance() {
        return new AutofillPickerFragment();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mPm = new PackageManagerWrapper(context.getPackageManager());
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.autofill_picker, null);
        final PreferenceScreen screen = getPreferenceScreen();
        bind(screen, savedInstanceState == null);
        setPreferenceScreen(screen);
    }

    @VisibleForTesting
    void bind(PreferenceScreen screen, boolean scrollToSelection) {
        Context context = getContext();

        List<DefaultAppInfo> candidates = AutofillHelper.getAutofillCandidates(context,
                mPm, UserHandle.myUserId());
        DefaultAppInfo current = AutofillHelper.getCurrentAutofill(context, candidates);

        RadioPreference activePref = null;
        for (final DefaultAppInfo appInfo : candidates) {
            final RadioPreference radioPreference = new RadioPreference(context);
            radioPreference.setKey(appInfo.getKey());
            radioPreference.setPersistent(false);
            radioPreference.setTitle(appInfo.loadLabel());
            radioPreference.setRadioGroup(AUTOFILL_SERVICE_RADIO_GROUP);
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);

            if (current == appInfo) {
                radioPreference.setChecked(true);
                activePref = radioPreference;
            }
            screen.addPreference(radioPreference);
        }
        if (activePref == null) {
            // select the none
            activePref = ((RadioPreference) screen.findPreference(KEY_FOR_NONE));
            activePref.setChecked(true);
        }

        if (activePref != null && scrollToSelection) {
            scrollToPreference(activePref);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {

            final Context context = getContext();
            List<DefaultAppInfo> candidates = AutofillHelper.getAutofillCandidates(context,
                    mPm, UserHandle.myUserId());
            final DefaultAppInfo current = AutofillHelper.getCurrentAutofill(context,
                    candidates);
            final String currentKey = current != null ? current.getKey() : KEY_FOR_NONE;

            final RadioPreference newPref = (RadioPreference) preference;
            final String newKey = newPref.getKey();
            final boolean clickOnCurrent = currentKey.equals(newKey);

            if (!clickOnCurrent && !KEY_FOR_NONE.equals(newKey)) {
                // Undo checked state change and wait dialog to confirm click
                RadioPreference currentPref = (RadioPreference) findPreference(currentKey);
                currentPref.setChecked(true);
                currentPref.clearOtherRadioPreferences(getPreferenceScreen());
                CharSequence confirmationMessage = Html.fromHtml(getContext().getString(
                        R.string.autofill_confirmation_message, newPref.getTitle()));
                displayAlert(confirmationMessage, (dialog, which) -> {
                    RadioPreference pref = (RadioPreference) findPreference(newKey);
                    if (pref != null) {
                        pref.setChecked(true);
                        pref.clearOtherRadioPreferences(getPreferenceScreen());
                        setAutofillService(newKey);
                    }
                });
            } else {
                // Either clicked on already checked or click on "none": just close the fragment
                newPref.setChecked(true);
                newPref.clearOtherRadioPreferences(getPreferenceScreen());
                setAutofillService(newKey);
            }
        }
        return true;
    }

    private void setAutofillService(String key) {
        AutofillHelper.setCurrentAutofill(getContext(), key);
        // Check if activity was launched from Settings.ACTION_REQUEST_SET_AUTOFILL_SERVICE
        // intent, and set proper result if so...
        final Activity activity = getActivity();
        if (activity != null) {
            final String packageName = activity.getIntent()
                    .getStringExtra(AutofillPickerActivity.EXTRA_PACKAGE_NAME);
            if (packageName != null) {
                ComponentName componentName = ComponentName.unflattenFromString(key);
                final int result = componentName != null
                        && componentName.getPackageName().equals(packageName)
                        ? Activity.RESULT_OK : Activity.RESULT_CANCELED;
                finishActivity(true, result);
            } else {
                if (!getFragmentManager().popBackStackImmediate()) {
                    finishActivity(false, 0);
                }
            }
        }
    }

    private void finishActivity(final boolean sendResult, final int result) {
        // RadioPreference does not update UI if activity is marked as finished.
        // Wait a little bit for the RadioPreference UI update.
        mHandler.postDelayed(() -> {
            if (sendResult) {
                getActivity().setResult(result);
            }
            getActivity().finish();
        }, FINISH_ACTIVITY_DELAY);
    }

    private void displayAlert(
            CharSequence message,
            DialogInterface.OnClickListener positiveOnClickListener) {

        final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity())
                .setMessage(message)
                .setCancelable(true)
                .setPositiveButton(android.R.string.ok, positiveOnClickListener)
                .setNegativeButton(android.R.string.cancel, mCancelListener);
        final AlertDialog dialog = builder.create();
        dialog.setOnShowListener((dialogInterface) -> {
            final Button negative = dialog.getButton(AlertDialog.BUTTON_NEGATIVE);
            negative.requestFocus();
        });
        dialog.show();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.DEFAULT_AUTOFILL_PICKER;
    }
}
