/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.documentsui;

import static android.os.Environment.isStandardDirectory;
import static android.os.storage.StorageVolume.EXTRA_DIRECTORY_NAME;
import static android.os.storage.StorageVolume.EXTRA_STORAGE_VOLUME;

import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_DENIED;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_ERROR;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_GRANTED;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_INVALID_ARGUMENTS;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_INVALID_DIRECTORY;
import static com.android.documentsui.ScopedAccessMetrics.logInvalidScopedAccessRequest;
import static com.android.documentsui.ScopedAccessMetrics.logValidScopedAccessRequest;
import static com.android.documentsui.base.SharedMinimal.DEBUG;
import static com.android.documentsui.base.SharedMinimal.DIRECTORY_ROOT;
import static com.android.documentsui.base.SharedMinimal.getUriPermission;
import static com.android.documentsui.base.SharedMinimal.getInternalDirectoryName;
import static com.android.documentsui.prefs.ScopedAccessLocalPreferences.PERMISSION_ASK_AGAIN;
import static com.android.documentsui.prefs.ScopedAccessLocalPreferences.PERMISSION_NEVER_ASK;
import static com.android.documentsui.prefs.ScopedAccessLocalPreferences.getScopedAccessPermissionStatus;
import static com.android.documentsui.prefs.ScopedAccessLocalPreferences.setScopedAccessPermissionStatus;

import android.annotation.Nullable;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.GrantedUriPermission;
import android.content.ContentProviderClient;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.UriPermission;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.storage.VolumeInfo;
import android.provider.DocumentsContract;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;

import com.android.documentsui.base.Providers;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * Activity responsible for handling {@link StorageVolume#createAccessIntent(String)}.
 */
public class ScopedAccessActivity extends Activity {
    private static final String TAG = "ScopedAccessActivity";
    private static final String FM_TAG = "open_external_directory";
    private static final String EXTRA_FILE = "com.android.documentsui.FILE";
    private static final String EXTRA_APP_LABEL = "com.android.documentsui.APP_LABEL";
    private static final String EXTRA_VOLUME_LABEL = "com.android.documentsui.VOLUME_LABEL";
    private static final String EXTRA_VOLUME_UUID = "com.android.documentsui.VOLUME_UUID";
    private static final String EXTRA_IS_ROOT = "com.android.documentsui.IS_ROOT";
    private static final String EXTRA_IS_PRIMARY = "com.android.documentsui.IS_PRIMARY";

    private ContentProviderClient mExternalStorageClient;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null) {
            if (DEBUG) Log.d(TAG, "activity.onCreateDialog(): reusing instance");
            return;
        }

        final Intent intent = getIntent();
        if (intent == null) {
            if (DEBUG) Log.d(TAG, "missing intent");
            logInvalidScopedAccessRequest(this, SCOPED_DIRECTORY_ACCESS_INVALID_ARGUMENTS);
            setResult(RESULT_CANCELED);
            finish();
            return;
        }
        final Parcelable storageVolume = intent.getParcelableExtra(EXTRA_STORAGE_VOLUME);
        if (!(storageVolume instanceof StorageVolume)) {
            if (DEBUG)
                Log.d(TAG, "extra " + EXTRA_STORAGE_VOLUME + " is not a StorageVolume: "
                        + storageVolume);
            logInvalidScopedAccessRequest(this, SCOPED_DIRECTORY_ACCESS_INVALID_ARGUMENTS);
            setResult(RESULT_CANCELED);
            finish();
            return;
        }
        String directoryName =
                getInternalDirectoryName(intent.getStringExtra(EXTRA_DIRECTORY_NAME));
        final StorageVolume volume = (StorageVolume) storageVolume;
        if (getScopedAccessPermissionStatus(getApplicationContext(), getCallingPackage(),
                volume.getUuid(), directoryName) == PERMISSION_NEVER_ASK) {
            logValidScopedAccessRequest(this, directoryName,
                    SCOPED_DIRECTORY_ACCESS_ALREADY_DENIED);
            setResult(RESULT_CANCELED);
            finish();
            return;
        }

        final int userId = UserHandle.myUserId();
        if (!showFragment(this, userId, volume, directoryName)) {
            setResult(RESULT_CANCELED);
            finish();
            return;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mExternalStorageClient != null) {
            mExternalStorageClient.close();
        }
    }

    /**
     * Validates the given path (volume + directory) and display the appropriate dialog asking the
     * user to grant access to it.
     */
    private static boolean showFragment(ScopedAccessActivity activity, int userId,
            StorageVolume storageVolume, String directoryName) {
        return getUriPermission(activity,
                activity.getExternalStorageClient(), storageVolume, directoryName, userId, true,
                (file, volumeLabel, isRoot, isPrimary, grantedUri, rootUri) -> {
                    // Checks if the user has granted the permission already.
                    final Intent intent = getIntentForExistingPermission(activity,
                            activity.getCallingPackage(), grantedUri, rootUri);
                    if (intent != null) {
                        logValidScopedAccessRequest(activity, isRoot ? "." : directoryName,
                                SCOPED_DIRECTORY_ACCESS_ALREADY_GRANTED);
                        activity.setResult(RESULT_OK, intent);
                        activity.finish();
                        return true;
                    }

                    // Gets the package label.
                    final String appLabel = getAppLabel(activity);
                    if (appLabel == null) {
                        // Error already logged.
                        return false;
                    }

                    // Sets args that will be retrieve on onCreate()
                    final Bundle args = new Bundle();
                    args.putString(EXTRA_FILE, file.getAbsolutePath());
                    args.putString(EXTRA_VOLUME_LABEL, volumeLabel);
                    args.putString(EXTRA_VOLUME_UUID, storageVolume.getUuid());
                    args.putString(EXTRA_APP_LABEL, appLabel);
                    args.putBoolean(EXTRA_IS_ROOT, isRoot);
                    args.putBoolean(EXTRA_IS_PRIMARY, isPrimary);

                    final FragmentManager fm = activity.getFragmentManager();
                    final FragmentTransaction ft = fm.beginTransaction();
                    final ScopedAccessDialogFragment fragment = new ScopedAccessDialogFragment();
                    fragment.setArguments(args);
                    ft.add(fragment, FM_TAG);
                    ft.commitAllowingStateLoss();

                    return true;
                });
    }

    private static String getAppLabel(Activity activity) {
        final String packageName = activity.getCallingPackage();
        final PackageManager pm = activity.getPackageManager();
        try {
            return pm.getApplicationLabel(pm.getApplicationInfo(packageName, 0)).toString();
        } catch (NameNotFoundException e) {
            logInvalidScopedAccessRequest(activity, SCOPED_DIRECTORY_ACCESS_ERROR);
            Log.w(TAG, "Could not get label for package " + packageName);
            return null;
        }
    }

    private static Intent createGrantedUriPermissionsIntent(Context context,
            ContentProviderClient provider, File file) {
        final Uri uri = getUriPermission(context, provider, file);
        return createGrantedUriPermissionsIntent(uri);
    }

    private static Intent createGrantedUriPermissionsIntent(Uri uri) {
        final Intent intent = new Intent();
        intent.setData(uri);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        return intent;
    }

    private static Intent getIntentForExistingPermission(Context context, String packageName,
            Uri grantedUri, Uri rootUri) {
        if (DEBUG) {
            Log.d(TAG, "checking if " + packageName + " already has permission for " + grantedUri
                    + " or its root (" + rootUri + ")");
        }
        final ActivityManager am = context.getSystemService(ActivityManager.class);
        for (GrantedUriPermission uriPermission : am.getGrantedUriPermissions(packageName)
                .getList()) {
            final Uri uri = uriPermission.uri;
            if (uri == null) {
                Log.w(TAG, "null URI for " + uriPermission);
                continue;
            }
            if (uri.equals(grantedUri) || uri.equals(rootUri)) {
                if (DEBUG) Log.d(TAG, packageName + " already has permission: " + uriPermission);
                return createGrantedUriPermissionsIntent(grantedUri);
            }
        }
        if (DEBUG) Log.d(TAG, packageName + " does not have permission for " + grantedUri);
        return null;
    }

    public static class ScopedAccessDialogFragment extends DialogFragment {

        private File mFile;
        private String mVolumeUuid;
        private String mVolumeLabel;
        private String mAppLabel;
        private boolean mIsRoot;
        private boolean mIsPrimary;
        private CheckBox mDontAskAgain;
        private ScopedAccessActivity mActivity;
        private AlertDialog mDialog;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setRetainInstance(true);
            final Bundle args = getArguments();
            if (args != null) {
                mFile = new File(args.getString(EXTRA_FILE));
                mVolumeUuid = args.getString(EXTRA_VOLUME_UUID);
                mVolumeLabel = args.getString(EXTRA_VOLUME_LABEL);
                mAppLabel = args.getString(EXTRA_APP_LABEL);
                mIsRoot = args.getBoolean(EXTRA_IS_ROOT);
                mIsPrimary= args.getBoolean(EXTRA_IS_PRIMARY);
            }
            mActivity = (ScopedAccessActivity) getActivity();
        }

        @Override
        public void onDestroyView() {
            // Workaround for https://code.google.com/p/android/issues/detail?id=17423
            if (mDialog != null && getRetainInstance()) {
                mDialog.setDismissMessage(null);
            }
            super.onDestroyView();
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            if (mDialog != null) {
                if (DEBUG) Log.d(TAG, "fragment.onCreateDialog(): reusing dialog");
                return mDialog;
            }
            if (mActivity != getActivity()) {
                // Sanity check.
                Log.wtf(TAG, "activity references don't match on onCreateDialog(): mActivity = "
                        + mActivity + " , getActivity() = " + getActivity());
                mActivity = (ScopedAccessActivity) getActivity();
            }
            final String directory = mFile.getName();
            final String directoryName = mIsRoot ? DIRECTORY_ROOT : directory;
            final Context context = mActivity.getApplicationContext();
            final OnClickListener listener = new OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    Intent intent = null;
                    if (which == DialogInterface.BUTTON_POSITIVE) {
                        intent = createGrantedUriPermissionsIntent(mActivity,
                                mActivity.getExternalStorageClient(), mFile);
                    }
                    if (which == DialogInterface.BUTTON_NEGATIVE || intent == null) {
                        logValidScopedAccessRequest(mActivity, directoryName,
                                SCOPED_DIRECTORY_ACCESS_DENIED);
                        final boolean checked = mDontAskAgain.isChecked();
                        if (checked) {
                            logValidScopedAccessRequest(mActivity, directory,
                                    SCOPED_DIRECTORY_ACCESS_DENIED_AND_PERSIST);
                            setScopedAccessPermissionStatus(context, mActivity.getCallingPackage(),
                                    mVolumeUuid, directoryName, PERMISSION_NEVER_ASK);
                        } else {
                            setScopedAccessPermissionStatus(context, mActivity.getCallingPackage(),
                                    mVolumeUuid, directoryName, PERMISSION_ASK_AGAIN);
                        }
                        mActivity.setResult(RESULT_CANCELED);
                    } else {
                        logValidScopedAccessRequest(mActivity, directory,
                                SCOPED_DIRECTORY_ACCESS_GRANTED);
                        mActivity.setResult(RESULT_OK, intent);
                    }
                    mActivity.finish();
                }
            };

            @SuppressLint("InflateParams")
            // It's ok pass null ViewRoot on AlertDialogs.
            final View view = View.inflate(mActivity, R.layout.dialog_open_scoped_directory, null);
            final CharSequence message;
            if (mIsRoot) {
                message = TextUtils.expandTemplate(getText(
                        R.string.open_external_dialog_root_request), mAppLabel, mVolumeLabel);
            } else {
                message = TextUtils.expandTemplate(
                        getText(mIsPrimary ? R.string.open_external_dialog_request_primary_volume
                                : R.string.open_external_dialog_request),
                                mAppLabel, directory, mVolumeLabel);
            }
            final TextView messageField = (TextView) view.findViewById(R.id.message);
            messageField.setText(message);
            mDialog = new AlertDialog.Builder(mActivity, R.style.Theme_AppCompat_Light_Dialog_Alert)
                    .setView(view)
                    .setPositiveButton(R.string.allow, listener)
                    .setNegativeButton(R.string.deny, listener)
                    .create();

            mDontAskAgain = (CheckBox) view.findViewById(R.id.do_not_ask_checkbox);
            if (getScopedAccessPermissionStatus(context, mActivity.getCallingPackage(),
                    mVolumeUuid, directoryName) == PERMISSION_ASK_AGAIN) {
                mDontAskAgain.setVisibility(View.VISIBLE);
                mDontAskAgain.setOnCheckedChangeListener(new OnCheckedChangeListener() {

                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        mDialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(!isChecked);
                    }
                });
            }

            return mDialog;
        }

        @Override
        public void onCancel(DialogInterface dialog) {
            super.onCancel(dialog);
            final Activity activity = getActivity();
            logValidScopedAccessRequest(activity, mFile.getName(), SCOPED_DIRECTORY_ACCESS_DENIED);
            activity.setResult(RESULT_CANCELED);
            activity.finish();
        }
    }

    private synchronized ContentProviderClient getExternalStorageClient() {
        if (mExternalStorageClient == null) {
            mExternalStorageClient =
                    getContentResolver().acquireContentProviderClient(Providers.AUTHORITY_STORAGE);
        }
        return mExternalStorageClient;
    }
}
