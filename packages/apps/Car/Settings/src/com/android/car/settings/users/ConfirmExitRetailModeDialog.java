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

package com.android.car.settings.users;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import androidx.car.app.CarAlertDialog;

import com.android.car.settings.R;

/**
 * A dialog that asks the user to confirm that they want to exit retail mode, which will result in
 * a factory reset.
 */
public class ConfirmExitRetailModeDialog extends DialogFragment implements
        DialogInterface.OnClickListener {
    private static final String DIALOG_TAG = "ConfirmExitRetailModeDialog";
    private ConfirmExitRetailModeListener mListener;

    /**
     * Shows the dialog.
     *
     * @param parent Fragment associated with the dialog.
     */
    public void show(Fragment parent) {
        setTargetFragment(parent, 0);
        show(parent.getFragmentManager(), DIALOG_TAG);
    }

    /**
     * Sets a listener for onExitRetailModeConfirmed that will get called if user confirms
     * the dialog.
     *
     * @param listener Instance of {@link ConfirmExitRetailModeListener} to call when confirmed.
     */
    public void setConfirmExitRetailModeListener(ConfirmExitRetailModeListener listener) {
        mListener = listener;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        return new CarAlertDialog.Builder(getContext())
                .setTitle(R.string.exit_retail_mode_dialog_title)
                .setBody(R.string.exit_retail_mode_dialog_body)
                .setPositiveButton(R.string.exit_retail_mode_dialog_confirmation_button_text, this)
                .setNegativeButton(android.R.string.cancel, null)
                .create();
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        if (mListener != null) {
            mListener.onExitRetailModeConfirmed();
        }
        dialog.dismiss();
    }

    /**
     * Interface for listeners that want to receive a callback when user confirms exit of retail
     * mode in a dialog.
     */
    public interface ConfirmExitRetailModeListener {
        /**
         * Called when the user confirms that they want to exit retail mode.
         */
        void onExitRetailModeConfirmed();
    }
}
