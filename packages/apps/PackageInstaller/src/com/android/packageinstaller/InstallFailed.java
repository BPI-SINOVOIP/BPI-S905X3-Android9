/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package com.android.packageinstaller;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.util.Log;
import android.widget.TextView;

import java.io.File;

/**
 * Installation failed: Return status code to the caller or display failure UI to user
 */
public class InstallFailed extends Activity {
    private static final String LOG_TAG = InstallFailed.class.getSimpleName();

    /** Label of the app that failed to install */
    private CharSequence mLabel;

    /**
     * Convert an package installer status code into the user friendly label.
     *
     * @param statusCode The status code from the package installer.
     *
     * @return The user friendly label for the status code
     */
    private int getExplanationFromErrorCode(int statusCode) {
        Log.d(LOG_TAG, "Installation status code: " + statusCode);

        switch (statusCode) {
            case PackageInstaller.STATUS_FAILURE_BLOCKED:
                return R.string.install_failed_blocked;
            case PackageInstaller.STATUS_FAILURE_CONFLICT:
                return R.string.install_failed_conflict;
            case PackageInstaller.STATUS_FAILURE_INCOMPATIBLE:
                return R.string.install_failed_incompatible;
            case PackageInstaller.STATUS_FAILURE_INVALID:
                return R.string.install_failed_invalid_apk;
            default:
                return R.string.install_failed;
        }
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        int statusCode = getIntent().getIntExtra(PackageInstaller.EXTRA_STATUS,
                PackageInstaller.STATUS_FAILURE);

        if (getIntent().getBooleanExtra(Intent.EXTRA_RETURN_RESULT, false)) {
            int legacyStatus = getIntent().getIntExtra(PackageInstaller.EXTRA_LEGACY_STATUS,
                    PackageManager.INSTALL_FAILED_INTERNAL_ERROR);

            // Return result if requested
            Intent result = new Intent();
            result.putExtra(Intent.EXTRA_INSTALL_RESULT, legacyStatus);
            setResult(Activity.RESULT_FIRST_USER, result);
            finish();
        } else {
            Intent intent = getIntent();
            ApplicationInfo appInfo = intent
                    .getParcelableExtra(PackageUtil.INTENT_ATTR_APPLICATION_INFO);
            Uri packageURI = intent.getData();

            setContentView(R.layout.install_failed);

            // Set header icon and title
            PackageUtil.AppSnippet as;
            PackageManager pm = getPackageManager();

            if ("package".equals(packageURI.getScheme())) {
                as = new PackageUtil.AppSnippet(pm.getApplicationLabel(appInfo),
                        pm.getApplicationIcon(appInfo));
            } else {
                final File sourceFile = new File(packageURI.getPath());
                as = PackageUtil.getAppSnippet(this, appInfo, sourceFile);
            }

            // Store label for dialog
            mLabel = as.label;

            PackageUtil.initSnippetForNewApp(this, as, R.id.app_snippet);

            // Show out of space dialog if needed
            if (statusCode == PackageInstaller.STATUS_FAILURE_STORAGE) {
                (new OutOfSpaceDialog()).show(getFragmentManager(), "outofspace");
            }

            // Get status messages
            ((TextView) findViewById(R.id.simple_status)).setText(
                    getExplanationFromErrorCode(statusCode));

            // Set up "done" button
            findViewById(R.id.done_button).setOnClickListener(view -> finish());
        }
    }

    /**
     * Dialog shown when we ran out of space during installation. This contains a link to the
     * "manage applications" settings page.
     */
    public static class OutOfSpaceDialog extends DialogFragment {
        private InstallFailed mActivity;

        @Override
        public void onAttach(Context context) {
            super.onAttach(context);

            mActivity = (InstallFailed) context;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            return new AlertDialog.Builder(mActivity)
                    .setTitle(R.string.out_of_space_dlg_title)
                    .setMessage(getString(R.string.out_of_space_dlg_text, mActivity.mLabel))
                    .setPositiveButton(R.string.manage_applications, (dialog, which) -> {
                        // launch manage applications
                        Intent intent = new Intent("android.intent.action.MANAGE_PACKAGE_STORAGE");
                        startActivity(intent);
                        mActivity.finish();
                    })
                    .setNegativeButton(R.string.cancel, (dialog, which) -> mActivity.finish())
                    .create();
        }

        @Override
        public void onCancel(DialogInterface dialog) {
            super.onCancel(dialog);

            mActivity.finish();
        }
    }
}
