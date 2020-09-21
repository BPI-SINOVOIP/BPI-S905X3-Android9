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
 * limitations under the License
 */
package com.android.car.settings.applications;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.UserHandle;
import android.view.View;
import android.widget.Button;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.car.settings.common.Logger;
import com.android.settingslib.Utils;

import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.List;

/**
 * Shows details about an application and action associated with that application,
 * like uninstall, forceStop.
 */
public class ApplicationDetailFragment extends ListItemSettingsFragment {
    private static final Logger LOG = new Logger(ApplicationDetailFragment.class);
    public static final String EXTRA_RESOLVE_INFO = "extra_resolve_info";

    private ResolveInfo mResolveInfo;
    private PackageInfo mPackageInfo;

    private Button mDisableToggle;
    private Button mForceStopButton;
    private DevicePolicyManager mDpm;

    public static ApplicationDetailFragment getInstance(ResolveInfo resolveInfo) {
        ApplicationDetailFragment applicationDetailFragment = new ApplicationDetailFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putParcelable(EXTRA_RESOLVE_INFO, resolveInfo);
        bundle.putInt(EXTRA_TITLE_ID, R.string.applications_settings);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        applicationDetailFragment.setArguments(bundle);
        return applicationDetailFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mResolveInfo = getArguments().getParcelable(EXTRA_RESOLVE_INFO);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mPackageInfo = getPackageInfo();
        super.onActivityCreated(savedInstanceState);
        if (mResolveInfo == null) {
            LOG.w("No application info set.");
            return;
        }

        mDisableToggle = (Button) getActivity().findViewById(R.id.action_button1);
        mForceStopButton = (Button) getActivity().findViewById(R.id.action_button2);
        mForceStopButton.setText(R.string.force_stop);
        mForceStopButton.setVisibility(View.VISIBLE);

        mDpm = (DevicePolicyManager) getContext().getSystemService(Context.DEVICE_POLICY_SERVICE);
        updateForceStopButton();
        mForceStopButton.setOnClickListener(
                v -> forceStopPackage(mResolveInfo.activityInfo.packageName));
    }

    @Override
    public void onStart() {
        super.onStart();
        updateForceStopButton();
        updateDisableable();
    }

    @Override
    public ListItemProvider getItemProvider() {
        return new ListItemProvider.ListProvider(getListItems());
    }

    private List<ListItem> getListItems() {
        ArrayList<ListItem> items = new ArrayList<>();
        items.add(new ApplicationLineItem(
                getContext(),
                getContext().getPackageManager(),
                mResolveInfo,
                /* fragmentController= */ null,
                false));
        items.add(new ApplicationPermissionLineItem(getContext(), mResolveInfo, this));
        TextListItem versionItem = new TextListItem(getContext());
        versionItem.setTitle(getContext().getString(
                R.string.application_version_label, mPackageInfo.versionName));
        items.add(versionItem);
        return items;
    }

    // fetch the latest ApplicationInfo instead of caching it so it reflects the current state.
    private ApplicationInfo getAppInfo() {
        try {
            return getContext().getPackageManager().getApplicationInfo(
                    mResolveInfo.activityInfo.packageName, 0 /* flag */);
        } catch (PackageManager.NameNotFoundException e) {
            LOG.e("incorrect packagename: " + mResolveInfo.activityInfo.packageName, e);
            throw new IllegalArgumentException(e);
        }
    }

    private PackageInfo getPackageInfo() {
        try {
            return getContext().getPackageManager().getPackageInfo(
                    mResolveInfo.activityInfo.packageName, 0 /* flag */);
        } catch (PackageManager.NameNotFoundException e) {
            LOG.e("incorrect packagename: " + mResolveInfo.activityInfo.packageName, e);
            throw new IllegalArgumentException(e);
        }
    }

    private void updateDisableable() {
        boolean disableable = false;
        boolean disabled = false;
        // Try to prevent the user from bricking their phone
        // by not allowing disabling of apps in the system.
        if (Utils.isSystemPackage(
                getResources(), getContext().getPackageManager(), mPackageInfo)) {
            // Disable button for core system applications.
            mDisableToggle.setText(R.string.disable_text);
            disabled = false;
        } else if (getAppInfo().enabled && !isDisabledUntilUsed()) {
            mDisableToggle.setText(R.string.disable_text);
            disableable = true;
            disabled = false;
        } else {
            mDisableToggle.setText(R.string.enable_text);
            disableable = true;
            disabled = true;
        }
        mDisableToggle.setEnabled(disableable);
        final int enableState = disabled
                ? PackageManager.COMPONENT_ENABLED_STATE_DEFAULT
                : PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER;
        mDisableToggle.setOnClickListener(v -> {
            getContext().getPackageManager().setApplicationEnabledSetting(
                    mResolveInfo.activityInfo.packageName,
                    enableState,
                    0);
            updateDisableable();
        });
    }

    private boolean isDisabledUntilUsed() {
        return getAppInfo().enabledSetting
                == PackageManager.COMPONENT_ENABLED_STATE_DISABLED_UNTIL_USED;
    }

    private void forceStopPackage(String pkgName) {
        ActivityManager am = (ActivityManager) getContext().getSystemService(
                Context.ACTIVITY_SERVICE);
        LOG.d("Stopping package " + pkgName);
        am.forceStopPackage(pkgName);
        updateForceStopButton();
    }

    // enable or disable the force stop button:
    // - disabled if it's a device admin
    // - if the application is stopped unexplicitly, enabled the button
    // - if there's a reason for the system to restart the application, that indicates the app
    //   can be force stopped.
    private void updateForceStopButton() {
        if (mDpm.packageHasActiveAdmins(mResolveInfo.activityInfo.packageName)) {
            // User can't force stop device admin.
            LOG.d("Disabling button, user can't force stop device admin");
            mForceStopButton.setEnabled(false);
        } else if ((getAppInfo().flags & ApplicationInfo.FLAG_STOPPED) == 0) {
            // If the app isn't explicitly stopped, then always show the
            // force stop button.
            LOG.w("App is not explicitly stopped");
            mForceStopButton.setEnabled(true);
        } else {
            Intent intent = new Intent(Intent.ACTION_QUERY_PACKAGE_RESTART,
                    Uri.fromParts("package", mResolveInfo.activityInfo.packageName, null));
            intent.putExtra(Intent.EXTRA_PACKAGES, new String[]{
                    mResolveInfo.activityInfo.packageName
            });
            LOG.d("Sending broadcast to query restart for "
                    + mResolveInfo.activityInfo.packageName);
            getActivity().sendOrderedBroadcastAsUser(intent, UserHandle.CURRENT, null,
                    mCheckKillProcessesReceiver, null, Activity.RESULT_CANCELED, null, null);
        }
    }

    private final BroadcastReceiver mCheckKillProcessesReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final boolean enabled = getResultCode() != Activity.RESULT_CANCELED;
            LOG.d(MessageFormat.format("Got broadcast response: Restart status for {0} {1}",
                    mResolveInfo.activityInfo.packageName, enabled));
            mForceStopButton.setEnabled(enabled);
        }
    };
}
