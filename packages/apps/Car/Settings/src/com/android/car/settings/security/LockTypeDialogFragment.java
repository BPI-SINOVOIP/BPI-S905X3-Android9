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

package com.android.car.settings.security;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;

import androidx.car.app.CarListDialog;

import com.android.car.settings.R;

/**
 * Dialog that allows user to switch lock type.  The containing Activity must implement the
 * OnLockSelectListener interface
 */
public class LockTypeDialogFragment extends DialogFragment {
    // Please make sure the order is sync with LOCK_TYPES array
    public static final int POSITION_NONE = 0;
    public static final int POSITION_PIN = 1;
    public static final int POSITION_PATTERN = 2;
    public static final int POSITION_PASSWORD = 3;

    private static final int[] LOCK_TYPES = {
            R.string.security_lock_none,
            R.string.security_lock_pin,
            R.string.security_lock_pattern,
            R.string.security_lock_password };

    private OnLockSelectListener mOnLockSelectListener;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);

        if (!(getActivity() instanceof OnLockSelectListener)) {
            throw new RuntimeException("Parent must implement OnLockSelectListener");
        }

        mOnLockSelectListener = (OnLockSelectListener) getActivity();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String[] items = new String[LOCK_TYPES.length];
        for (int i = 0; i < LOCK_TYPES.length; ++i) {
            items[i] = getResources().getString(LOCK_TYPES[i]);
        }

        return new CarListDialog.Builder(getContext())
                .setItems(items, (dialog, pos) -> mOnLockSelectListener.onLockTypeSelected(pos))
                .create();
    }

    /**
     * Handles when a lock type is selected
     */
    interface OnLockSelectListener {
        void onLockTypeSelected(int position);
    }
}
