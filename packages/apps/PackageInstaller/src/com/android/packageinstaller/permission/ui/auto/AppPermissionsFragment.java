/**
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

package com.android.packageinstaller.permission.ui.auto;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Switch;
import android.widget.Toast;

import com.android.car.list.IconToggleLineItem;
import com.android.car.list.TypedPagedListAdapter;
import com.android.packageinstaller.R;
import com.android.packageinstaller.permission.model.AppPermissionGroup;
import com.android.packageinstaller.permission.model.AppPermissions;
import com.android.packageinstaller.permission.utils.Utils;

import java.util.ArrayList;

import androidx.car.widget.PagedListView;

/**
 * Contains all permissions in a list for a given application.
 */
public final class AppPermissionsFragment extends Fragment{

    private static final String LOG_TAG = "ManagePermsFragment";
    public static final String EXTRA_LAYOUT = "extra_layout";

    private AppPermissions mAppPermissions;

    private String mPackageName;

    protected PagedListView mListView;
    protected TypedPagedListAdapter mPagedListAdapter;


    /**
     * Creates a new instance.
     * @param packageName the packageName of the application that we are listing the
     *                    permissions here.
     */
    public static AppPermissionsFragment newInstance(String packageName) {
        AppPermissionsFragment fragment = new AppPermissionsFragment();
        Bundle arguments = new Bundle();
        arguments.putInt(EXTRA_LAYOUT, R.layout.car_app_permissions);
        arguments.putString(Intent.EXTRA_PACKAGE_NAME, packageName);
        fragment.setArguments(arguments);
        return fragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        getView().findViewById(R.id.action_bar_icon_container).setOnClickListener(
                v -> getActivity().onBackPressed());

        mListView = (PagedListView) getView().findViewById(R.id.list);
        mPagedListAdapter = new TypedPagedListAdapter(getLineItems());
        mListView.setAdapter(mPagedListAdapter);
    }

    protected void notifyDataSetChanged() {
        mPagedListAdapter.notifyDataSetChanged();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null
                && savedInstanceState.containsKey(Intent.EXTRA_PACKAGE_NAME)) {
            mPackageName = savedInstanceState.getString(Intent.EXTRA_PACKAGE_NAME);
        } else if (getArguments() != null
                && getArguments().containsKey(Intent.EXTRA_PACKAGE_NAME)) {
            mPackageName = getArguments().getString(Intent.EXTRA_PACKAGE_NAME);
        }

        if (mPackageName == null) {
            Log.e(LOG_TAG, "package name is missing");
            return;
        }
        Activity activity = getActivity();
        PackageInfo packageInfo = getPackageInfo(activity, mPackageName);
        if (packageInfo == null) {
            Toast.makeText(activity, R.string.app_not_found_dlg_title, Toast.LENGTH_LONG).show();
            activity.finish();
            return;
        }

        mAppPermissions = new AppPermissions(activity, packageInfo, null, true, new Runnable() {
            @Override
            public void run() {
                activity.finish();
            }
        });
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putString(Intent.EXTRA_PACKAGE_NAME, mPackageName);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(getArguments().getInt(EXTRA_LAYOUT), container, false);
    }

    @Override
    public void onResume() {
        super.onResume();
        mAppPermissions.refresh();
    }

    /**
     * Gets the list of the LineItems to show up in the list
     */
    public ArrayList<TypedPagedListAdapter.LineItem> getLineItems() {
        ArrayList<TypedPagedListAdapter.LineItem> items = new ArrayList<>();
        Context context = getContext();
        if (context == null) {
            return items;
        }

        for (AppPermissionGroup group : mAppPermissions.getPermissionGroups()) {
            if (!Utils.shouldShowPermission(group, mAppPermissions.getPackageInfo().packageName)) {
                continue;
            }
            items.add(new PermissionLineItem(group, context));
        }
        return items;
    }

    private static PackageInfo getPackageInfo(Activity activity, String packageName) {
        try {
            return activity.getPackageManager().getPackageInfo(
                    packageName, PackageManager.GET_PERMISSIONS);
        } catch (PackageManager.NameNotFoundException e) {
            if (Log.isLoggable(LOG_TAG, Log.INFO)) {
                Log.i(LOG_TAG, "No package:" + activity.getCallingPackage(), e);
            }
            return null;
        }
    }

    private class PermissionLineItem extends IconToggleLineItem {
        private final AppPermissionGroup mPermissionGroup;
        private final Context mContext;

        PermissionLineItem(AppPermissionGroup permissionGroup, Context context) {
            super(permissionGroup.getLabel(), context);
            mContext = context;
            mPermissionGroup = permissionGroup;
        }

        @Override
        public boolean onToggleTouched(Switch toggleSwitch, MotionEvent event) {
            if (event.getAction() != MotionEvent.ACTION_DOWN) {
                return true;
            }
            if (!isChecked()) {
                mPermissionGroup.grantRuntimePermissions(false);
                toggleSwitch.performClick();
            } else {
                final boolean grantedByDefault =
                        mPermissionGroup.hasGrantedByDefaultPermission();
                if (grantedByDefault || !mPermissionGroup.doesSupportRuntimePermissions()) {
                    new AlertDialog.Builder(mContext)
                            .setMessage(grantedByDefault
                                    ? R.string.system_warning : R.string.old_sdk_deny_warning)
                            .setNegativeButton(R.string.cancel, null /* listener */)
                            .setPositiveButton(R.string.grant_dialog_button_deny_anyway,
                                    (dialog, which) -> {
                                        mPermissionGroup.revokeRuntimePermissions(false);
                                        toggleSwitch.performClick();
                                    })
                            .show();
                } else {
                    mPermissionGroup.revokeRuntimePermissions(false);
                    toggleSwitch.performClick();
                }
            }
            return true;
        }

        @DrawableRes
        public int getIcon() {
            return mPermissionGroup.getIconResId();
        }

        @Override
        public boolean isChecked() {
            return mPermissionGroup.areRuntimePermissionsGranted();
        }

        @Override
        public CharSequence getDesc() {
            return null;
        }

        @Override
        public boolean isEnabled() {
            return true;
        }

        @Override
        public boolean isExpandable() {
            return false;
        }
    }
}
