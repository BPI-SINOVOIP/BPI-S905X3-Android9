/*
 * Copyright 2014, The Android Open Source Project
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
package com.android.managedprovisioning.task;

import static com.android.internal.logging.nano.MetricsProto.MetricsEvent
        .PROVISIONING_INSTALL_PACKAGE_TASK_MS;
import static com.android.internal.util.Preconditions.checkNotNull;

import android.annotation.NonNull;
import android.app.PendingIntent;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.text.TextUtils;

import com.android.internal.annotations.VisibleForTesting;
import com.android.managedprovisioning.R;
import com.android.managedprovisioning.common.ProvisionLogger;
import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.model.ProvisioningParams;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Installs the management app apk from a download location provided by
 * {@link DownloadPackageTask#getDownloadedPackageLocation()}.
 */
public class InstallPackageTask extends AbstractProvisioningTask {
    private static final String ACTION_INSTALL_DONE = InstallPackageTask.class.getName() + ".DONE.";

    public static final int ERROR_PACKAGE_INVALID = 0;
    public static final int ERROR_INSTALLATION_FAILED = 1;

    private final SettingsFacade mSettingsFacade;
    private final DownloadPackageTask mDownloadPackageTask;

    private final PackageManager mPm;
    private final DevicePolicyManager mDpm;
    private boolean mInitialPackageVerifierEnabled;

    /**
     * Create an InstallPackageTask. When run, this will attempt to install the device admin package
     * if it is non-null.
     *
     * {@see #run(String, String)} for more detail on package installation.
     */
    public InstallPackageTask(
            DownloadPackageTask downloadPackageTask,
            Context context,
            ProvisioningParams params,
            Callback callback) {
        this(new SettingsFacade(), downloadPackageTask, context, params, callback);
    }

    @VisibleForTesting
    InstallPackageTask(
            SettingsFacade settingsFacade,
            DownloadPackageTask downloadPackageTask,
            Context context,
            ProvisioningParams params,
            Callback callback) {
        super(context, params, callback);

        mPm = context.getPackageManager();
        mDpm = context.getSystemService(DevicePolicyManager.class);
        mSettingsFacade = checkNotNull(settingsFacade);
        mDownloadPackageTask = checkNotNull(downloadPackageTask);
    }

    @Override
    public int getStatusMsgId() {
        return R.string.progress_install;
    }

    private static void copyStream(@NonNull InputStream in, @NonNull OutputStream out)
            throws IOException {
        byte[] buffer = new byte[16 * 1024];
        int numRead;
        while ((numRead = in.read(buffer)) != -1) {
            out.write(buffer, 0, numRead);
        }
    }

    /**
     * Installs a package. The package will be installed from the given location if one is provided.
     * If a null or empty location is provided, and the package is installed for a different user,
     * it will be enabled for the calling user. If the package location is not provided and the
     * package is not installed for any other users, this task will produce an error.
     *
     * Errors will be indicated if a downloaded package is invalid, or installation fails.
     */
    @Override
    public void run(int userId) {
        startTaskTimer();
        String packageLocation = mDownloadPackageTask.getDownloadedPackageLocation();
        String packageName = mProvisioningParams.inferDeviceAdminPackageName();

        ProvisionLogger.logi("Installing package " + packageName);
        mInitialPackageVerifierEnabled = mSettingsFacade.isPackageVerifierEnabled(mContext);
        if (TextUtils.isEmpty(packageLocation)) {
            // Do not log time if not installing any package, as that isn't useful.
            success();
            return;
        }

        // Temporarily turn off package verification.
        mSettingsFacade.setPackageVerifierEnabled(mContext, false);

        int installFlags = PackageManager.INSTALL_REPLACE_EXISTING;
        // Current device owner (if exists) must be test-only, so it is fine to replace it with a
        // test-only package of same package name. No need to further verify signature as
        // installation will fail if signatures don't match.
        if (mDpm.isDeviceOwnerApp(packageName)) {
            installFlags |= PackageManager.INSTALL_ALLOW_TEST;
        }

        PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_FULL_INSTALL);
        params.installFlags = installFlags;

        File source = new File(packageLocation);
        PackageInstaller pi = mPm.getPackageInstaller();
        try {
            int sessionId = pi.createSession(params);
            try (PackageInstaller.Session session = pi.openSession(sessionId)) {
                try (FileInputStream in = new FileInputStream(source);
                     OutputStream out = session.openWrite(source.getName(), 0, -1)) {
                    copyStream(in, out);
                } catch (IOException e) {
                    session.abandon();
                    throw e;
                }

                String action = ACTION_INSTALL_DONE + sessionId;
                mContext.registerReceiver(new PackageInstallReceiver(packageName),
                        new IntentFilter(action));

                PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, sessionId,
                        new Intent(action),
                        PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_UPDATE_CURRENT);
                session.commit(pendingIntent.getIntentSender());
            }
        } catch (IOException e) {
            mSettingsFacade.setPackageVerifierEnabled(mContext, mInitialPackageVerifierEnabled);

            ProvisionLogger.loge("Installing package " + packageName + " failed.", e);
            error(ERROR_INSTALLATION_FAILED);
        } finally {
            source.delete();
        }
    }

    @Override
    protected int getMetricsCategory() {
        return PROVISIONING_INSTALL_PACKAGE_TASK_MS;
    }

    private class PackageInstallReceiver extends BroadcastReceiver {
        private final String mPackageName;

        public PackageInstallReceiver(String packageName) {
            mPackageName = packageName;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            mSettingsFacade.setPackageVerifierEnabled(mContext, mInitialPackageVerifierEnabled);

            // Should not happen as we use a one shot pending intent specifically for this receiver
            if (intent.getAction() == null || !intent.getAction().startsWith(ACTION_INSTALL_DONE)) {
                ProvisionLogger.logw("Incorrect action");

                error(ERROR_INSTALLATION_FAILED);
                return;
            }

            // Should not happen as we use a one shot pending intent specifically for this receiver
            if (!intent.getStringExtra(PackageInstaller.EXTRA_PACKAGE_NAME).equals(mPackageName)) {
                ProvisionLogger.loge("Package doesn't have expected package name.");
                error(ERROR_PACKAGE_INVALID);
                return;
            }

            int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, 0);
            String statusMessage = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE);
            int legacyStatus = intent.getIntExtra(PackageInstaller.EXTRA_LEGACY_STATUS, 0);

            mContext.unregisterReceiver(this);
            ProvisionLogger.logi(status + " " + legacyStatus + " " + statusMessage);

            if (status == PackageInstaller.STATUS_SUCCESS) {
                ProvisionLogger.logd("Package " + mPackageName + " is succesfully installed.");
                stopTaskTimer();
                success();
            } else if (legacyStatus == PackageManager.INSTALL_FAILED_VERSION_DOWNGRADE) {
                ProvisionLogger.logd("Current version of " + mPackageName
                        + " higher than the version to be installed. It was not reinstalled.");
                // If the package is already at a higher version: success.
                // Do not log time if package is already at a higher version, as that isn't useful.
                success();
            } else {
                ProvisionLogger.logd("Installing package " + mPackageName + " failed.");
                ProvisionLogger.logd("Status message returned  = " + statusMessage);
                error(ERROR_INSTALLATION_FAILED);
            }
        }
    }
}
