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

package com.android.settings.development;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.DialogInterface;
import android.os.Bundle;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.core.instrumentation.InstrumentedDialogFragment;

public class ClearAdbKeysWarningDialog extends InstrumentedDialogFragment implements
        DialogInterface.OnClickListener, DialogInterface.OnDismissListener {

    public static final String TAG = "ClearAdbKeysDlg";

    public static void show(Fragment host) {
        final FragmentManager manager = host.getActivity().getFragmentManager();
        if (manager.findFragmentByTag(TAG) == null) {
            final ClearAdbKeysWarningDialog dialog =
                    new ClearAdbKeysWarningDialog();
            dialog.setTargetFragment(host, 0 /* requestCode */);
            dialog.show(manager, TAG);
        }
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.DIALOG_CLEAR_ADB_KEYS;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        return new AlertDialog.Builder(getActivity())
                .setMessage(R.string.adb_keys_warning_message)
                .setPositiveButton(android.R.string.ok, this /* onClickListener */)
                .setNegativeButton(android.R.string.cancel, null /* onClickListener */)
                .create();
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        final AdbClearKeysDialogHost host = (AdbClearKeysDialogHost) getTargetFragment();
        if (host == null) {
            return;
        }
        host.onAdbClearKeysDialogConfirmed();
    }
}
