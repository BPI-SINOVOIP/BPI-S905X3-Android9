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

package com.android.server.telecom.settings;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.provider.BlockedNumberContract;

import com.android.server.telecom.R;

/**
 * Shows a dialog when user taps an notification in notification tray.
 */
public class CallBlockDisabledActivity extends Activity {
    private AlertDialog mDialog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        showCallBlockingOffDialog();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        dismissCallBlockingOffDialog();
    }

    /**
     * Shows the dialog that notifies user that [Call Blocking] has been turned off.
     */
    private void showCallBlockingOffDialog() {
        if (isShowingCallBlockingOffDialog()) {
            return;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        mDialog = builder
                .setTitle(R.string.phone_strings_emergency_call_made_dialog_title_txt)
                .setMessage(R.string
                        .phone_strings_emergency_call_made_dialog_call_blocking_text_txt)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        BlockedNumbersUtil.setEnhancedBlockSetting(
                                CallBlockDisabledActivity.this,
                                BlockedNumberContract.SystemContract
                                        .ENHANCED_SETTING_KEY_SHOW_EMERGENCY_CALL_NOTIFICATION,
                                false);
                        BlockedNumbersUtil.updateEmergencyCallNotification(
                                CallBlockDisabledActivity.this, false);
                        finish();
                    }
                })
                .setOnCancelListener(new DialogInterface.OnCancelListener() {
                    @Override
                    public void onCancel(DialogInterface dialog) {
                        finish();
                    }
                })
                .create();
        mDialog.setCanceledOnTouchOutside(false);
        mDialog.show();
    }

    /**
     * Dismisses the dialog that notifies user that [Call Blocking] has been turned off.
     */
    private void dismissCallBlockingOffDialog() {
        if (isShowingCallBlockingOffDialog()) {
            mDialog.dismiss();
        }
        mDialog = null;
    }

    /**
     * Checks whether the dialog is currently showing.
     *
     * @return true if the dialog is currently showing, false otherwise.
     */
    private boolean isShowingCallBlockingOffDialog() {
        return (mDialog != null && mDialog.isShowing());
    }
}
