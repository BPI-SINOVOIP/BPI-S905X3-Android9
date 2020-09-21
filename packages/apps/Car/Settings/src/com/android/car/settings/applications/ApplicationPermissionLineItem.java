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

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.icu.text.ListFormatter;
import android.text.TextUtils;

import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.AnimationUtil;
import com.android.car.settings.common.ListController;
import com.android.car.settings.common.Logger;
import com.android.settingslib.applications.PermissionsSummaryHelper;
import com.android.settingslib.applications.PermissionsSummaryHelper.PermissionsResultCallback;

import java.util.ArrayList;
import java.util.List;

/**
 * Shows details about an application and action associated with that application,
 * like uninstall, forceStop.
 */
public class ApplicationPermissionLineItem extends TextListItem {
    private static final Logger LOG = new Logger(ApplicationPermissionLineItem.class);

    private final ResolveInfo mResolveInfo;
    private final Context mContext;
    private final ListController mListController;
    private String mSummary;

    public ApplicationPermissionLineItem(Context context, ResolveInfo resolveInfo,
            ListController listController) {
        super(context);
        mResolveInfo = resolveInfo;
        mContext = context;
        mListController = listController;

        PermissionsSummaryHelper.getPermissionSummary(mContext,
                mResolveInfo.activityInfo.packageName, mPermissionCallback);
        setTitle(context.getString(R.string.permissions_label));
        setSupplementalIcon(R.drawable.ic_chevron_right, /* showDivider= */ false);
        updateBody();
    }

    private void updateBody() {
        if (TextUtils.isEmpty(mSummary)) {
            setBody(mContext.getString(R.string.computing_size));
            setOnClickListener(null);
        } else {
            setBody(mSummary);
            setOnClickListener(view -> {
                // start new activity to manage app permissions
                Intent intent = new Intent(Intent.ACTION_MANAGE_APP_PERMISSIONS);
                intent.putExtra(Intent.EXTRA_PACKAGE_NAME, mResolveInfo.activityInfo.packageName);
                try {
                    mContext.startActivity(
                            intent, AnimationUtil.slideInFromRightOption(mContext).toBundle());
                } catch (ActivityNotFoundException e) {
                    LOG.w("No app can handle android.intent.action.MANAGE_APP_PERMISSIONS");
                }
            });
        }
    }

    private final PermissionsResultCallback mPermissionCallback = new PermissionsResultCallback() {
        @Override
        public void onPermissionSummaryResult(int standardGrantedPermissionCount,
                int requestedPermissionCount, int additionalGrantedPermissionCount,
                List<CharSequence> grantedGroupLabels) {
            Resources res = mContext.getResources();

            if (requestedPermissionCount == 0) {
                mSummary = res.getString(
                        R.string.runtime_permissions_summary_no_permissions_requested);
            } else {
                ArrayList<CharSequence> list = new ArrayList<>(grantedGroupLabels);
                if (additionalGrantedPermissionCount > 0) {
                    // N additional permissions.
                    list.add(res.getQuantityString(
                            R.plurals.runtime_permissions_additional_count,
                            additionalGrantedPermissionCount, additionalGrantedPermissionCount));
                }
                if (list.isEmpty()) {
                    mSummary = res.getString(
                            R.string.runtime_permissions_summary_no_permissions_granted);
                } else {
                    mSummary = ListFormatter.getInstance().format(list);
                }
            }
            updateBody();
            mListController.refreshList();
        }
    };
}
