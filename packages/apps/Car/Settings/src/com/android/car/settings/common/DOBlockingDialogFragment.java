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
package com.android.car.settings.common;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;

import androidx.car.app.CarAlertDialog;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment.FragmentController;
import com.android.car.settings.quicksettings.QuickSettingFragment;

/**
 * A dialog to block non-distraction optimized view when restriction is applied.
 */
public class DOBlockingDialogFragment extends DialogFragment implements
        DialogInterface.OnClickListener {
    public static final String DIALOG_TAG = "block_dialog_tag";
    private static final String MESSAGE_ARG_KEY = "message";
    private boolean mShowQuickSettingsMainScreen = true;

    /**
     * Creates a DOBlockingDialogFragment with a specified message
     *
     * @param message
     * @return an instance of DOBlockingDialogFragment
     */
    public static DOBlockingDialogFragment newInstance(String message) {
        DOBlockingDialogFragment fragment = new DOBlockingDialogFragment();

        Bundle args = new Bundle();
        args.putString(MESSAGE_ARG_KEY, message);
        fragment.setArguments(args);

        return fragment;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Context context = getContext();
        // If a message is not set, use the default message.
        String message = getArguments().getString(MESSAGE_ARG_KEY);
        if (message == null) {
            message = getContext().getString(R.string.restricted_while_driving);
        }
        Dialog dialog = new CarAlertDialog.Builder(context)
                .setBody(message)
                .setPositiveButton(context.getString(R.string.okay),  /* listener= */ this)
                .setCancelable(false)
                .create();
        return dialog;
    }

    /**
     * Return to the quick settings main screen after the dialog is dismissed.
     * @param showQuickSettingsMainScreen whether to return to the quick settings main screen, the
     * default value is true
     */
    public void goBackToQuickSettingsMainScreen(boolean showQuickSettingsMainScreen) {
        mShowQuickSettingsMainScreen = showQuickSettingsMainScreen;
    }

    // only one button, no need to check on negative.
    @Override
    public void onClick(DialogInterface dialog, int which) {
        if (mShowQuickSettingsMainScreen) {
            ((FragmentController) getActivity()).launchFragment(
                    QuickSettingFragment.newInstance());
        }
        dismiss();
    }
}
