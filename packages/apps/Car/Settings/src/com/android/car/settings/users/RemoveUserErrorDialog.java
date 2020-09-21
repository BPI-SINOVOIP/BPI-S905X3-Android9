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
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;

import androidx.car.app.CarAlertDialog;

import com.android.car.settings.R;

/**
 * Dialog to inform that user deletion failed and offers to retry.
 */
public class RemoveUserErrorDialog extends DialogFragment {
    private DialogInterface.OnClickListener mRemoveUserRetryListener = new OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
            if (mListener != null && which == DialogInterface.BUTTON_POSITIVE) {
                mListener.onRetryRemoveUser();
            }
            dialog.dismiss();
        }
    };

    private RemoveUserErrorListener mListener;

    /**
     * Sets a listener for onRetryRemoveUser that will get called if user presses positive
     * button.
     *
     * @param listener Instance of {@link RemoveUserErrorListener} to call when confirmed.
     */
    public void setRetryListener(@Nullable RemoveUserErrorListener listener) {
        mListener = listener;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        return new CarAlertDialog.Builder(getContext())
                .setTitle(R.string.remove_user_error_title)
                .setBody(R.string.remove_user_error_message)
                .setPositiveButton(R.string.remove_user_error_retry, mRemoveUserRetryListener)
                .setNegativeButton(R.string.remove_user_error_dismiss, null)
                .create();
    }

    /**
     * Interface for listeners that want to receive a callback when user removal fails.
     */
    public interface RemoveUserErrorListener {

        /**
         * Method called only when user presses retry button.
         */
        void onRetryRemoveUser();
    }
}
