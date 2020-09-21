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

package com.android.settings.applications.appinfo;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.applications.AppStoreUtil;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.core.BasePreferenceController;
import com.android.settings.overlay.FeatureFactory;
import com.android.settingslib.applications.AppUtils;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnCreateOptionsMenu;
import com.android.settingslib.core.lifecycle.events.OnOptionsItemSelected;
import com.android.settingslib.core.lifecycle.events.OnPrepareOptionsMenu;
import com.android.settingslib.wrapper.PackageManagerWrapper;

import java.util.List;

public class InstantAppButtonsPreferenceController extends BasePreferenceController implements
        LifecycleObserver, OnCreateOptionsMenu, OnPrepareOptionsMenu, OnOptionsItemSelected,
        DialogInterface.OnClickListener {

    private static final String KEY_INSTANT_APP_BUTTONS = "instant_app_buttons";
    private static final String META_DATA_DEFAULT_URI = "default-url";

    private final AppInfoDashboardFragment mParent;
    private final String mPackageName;
    private final PackageManagerWrapper mPackageManagerWrapper;
    private String mLaunchUri;
    private LayoutPreference mPreference;
    private MenuItem mInstallMenu;

    public InstantAppButtonsPreferenceController(Context context, AppInfoDashboardFragment parent,
            String packageName, Lifecycle lifecycle) {
        super(context, KEY_INSTANT_APP_BUTTONS);
        mParent = parent;
        mPackageName = packageName;
        mPackageManagerWrapper = new PackageManagerWrapper(context.getPackageManager());
        mLaunchUri = getDefaultLaunchUri();
        if (lifecycle != null) {
            lifecycle.addObserver(this);
        }
    }

    @Override
    public int getAvailabilityStatus() {
        return AppUtils.isInstant(mParent.getPackageInfo().applicationInfo)
                ? AVAILABLE : DISABLED_FOR_USER;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mPreference = (LayoutPreference) screen.findPreference(KEY_INSTANT_APP_BUTTONS);
        initButtons(mPreference.findViewById(R.id.instant_app_button_container));
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        if (!TextUtils.isEmpty(mLaunchUri)) {
            menu.add(0, AppInfoDashboardFragment.INSTALL_INSTANT_APP_MENU, 2, R.string.install_text)
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem menuItem) {
        if (menuItem.getItemId() == AppInfoDashboardFragment.INSTALL_INSTANT_APP_MENU) {
            final Intent appStoreIntent = AppStoreUtil.getAppStoreLink(mContext, mPackageName);
            if (appStoreIntent != null) {
                mParent.startActivity(appStoreIntent);
            }
            return true;
        }
        return false;
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        mInstallMenu = menu.findItem(AppInfoDashboardFragment.INSTALL_INSTANT_APP_MENU);
        if (mInstallMenu != null) {
            if (AppStoreUtil.getAppStoreLink(mContext, mPackageName) == null) {
                mInstallMenu.setEnabled(false);
            }
        }
    }

    @Override
    public void onClick(DialogInterface dialog, int which) {
        FeatureFactory.getFactory(mContext).getMetricsFeatureProvider()
            .action(mContext, MetricsEvent.ACTION_SETTINGS_CLEAR_INSTANT_APP, mPackageName);
        mPackageManagerWrapper.deletePackageAsUser(
            mPackageName, null, 0, UserHandle.myUserId());
    }

    AlertDialog createDialog(int id) {
        if (id == AppInfoDashboardFragment.DLG_CLEAR_INSTANT_APP) {
            AlertDialog confirmDialog = new AlertDialog.Builder(mContext)
                .setPositiveButton(R.string.clear_instant_app_data, this)
                .setNegativeButton(R.string.cancel, null)
                .setTitle(R.string.clear_instant_app_data)
                .setMessage(mContext.getString(R.string.clear_instant_app_confirmation))
                .create();
            return confirmDialog;
        }
        return null;
    }

    private void initButtons(View view) {
        final Button installButton = view.findViewById(R.id.install);
        final Button clearDataButton = view.findViewById(R.id.clear_data);
        final Button launchButton = view.findViewById(R.id.launch);
        if (!TextUtils.isEmpty(mLaunchUri)) {
            installButton.setVisibility(View.GONE);
            final Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.addCategory(Intent.CATEGORY_BROWSABLE);
            intent.setPackage(mPackageName);
            intent.setData(Uri.parse(mLaunchUri));
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            launchButton.setOnClickListener(v -> mParent.startActivity(intent));
        } else {
            launchButton.setVisibility(View.GONE);
            final Intent appStoreIntent = AppStoreUtil.getAppStoreLink(mContext, mPackageName);
            if (appStoreIntent != null) {
                installButton.setOnClickListener(v -> mParent.startActivity(appStoreIntent));
            } else {
                installButton.setEnabled(false);
            }
        }
        clearDataButton.setOnClickListener(
            v -> mParent.showDialogInner(mParent.DLG_CLEAR_INSTANT_APP, 0));
    }

    private String getDefaultLaunchUri() {
        final PackageManager manager = mContext.getPackageManager();
        final Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        intent.setPackage(mPackageName);
        final List<ResolveInfo> infos = manager.queryIntentActivities(
            intent, PackageManager.GET_META_DATA | PackageManager.MATCH_INSTANT);
        for (ResolveInfo info : infos) {
            final Bundle metaData = info.activityInfo.metaData;
            if (metaData != null) {
                final String launchUri = metaData.getString(META_DATA_DEFAULT_URI);
                if (!TextUtils.isEmpty(launchUri)) {
                    return launchUri;
                }
            }
        }
        return null;
    }
}
