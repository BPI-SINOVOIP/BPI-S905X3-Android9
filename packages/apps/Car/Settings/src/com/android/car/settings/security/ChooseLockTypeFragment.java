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
 * limitations under the License
 */

package com.android.car.settings.security;

import android.app.admin.DevicePolicyManager;
import android.os.Bundle;
import android.os.UserHandle;
import android.text.TextUtils;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.internal.widget.LockPatternUtils;

import java.util.ArrayList;
import java.util.List;

/**
 * Give user choices of lock screen type: Pin/Pattern/Password or None.
 */
public class ChooseLockTypeFragment extends ListItemSettingsFragment {

    public static final String EXTRA_CURRENT_PASSWORD_QUALITY = "extra_current_password_quality";
    private static final String DIALOG_TAG = "ConfirmRemoveScreenLockDialog";

    private ListItemProvider mItemProvider;
    private String mCurrPassword;
    private int mPasswordQuality;

    private final ConfirmRemoveScreenLockDialog.ConfirmRemoveLockListener mConfirmListener = () -> {
        int userId = UserHandle.myUserId();
        new LockPatternUtils(getContext()).clearLock(mCurrPassword, userId);
        getFragmentController().goBack();
    };

    public static ChooseLockTypeFragment newInstance() {
        ChooseLockTypeFragment chooseLockTypeFragment = new ChooseLockTypeFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.lock_settings_picker_title);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        chooseLockTypeFragment.setArguments(bundle);
        return chooseLockTypeFragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Bundle args = getArguments();
        if (args != null) {
            mCurrPassword = args.getString(SettingsScreenLockActivity.EXTRA_CURRENT_SCREEN_LOCK);
            mPasswordQuality = args.getInt(EXTRA_CURRENT_PASSWORD_QUALITY);
        }

        if (savedInstanceState != null) {
            ConfirmRemoveScreenLockDialog dialog = (ConfirmRemoveScreenLockDialog)
                    getFragmentManager().findFragmentByTag(DIALOG_TAG);
            if (dialog != null) {
                dialog.setConfirmRemoveLockListener(mConfirmListener);
            }
        }
    }

    @Override
    public ListItemProvider getItemProvider() {
        if (mItemProvider == null) {
            mItemProvider = new ListItemProvider.ListProvider(getListItems());
        }
        return mItemProvider;
    }

    private List<ListItem> getListItems() {
        List<ListItem> items = new ArrayList<>();
        items.add(createNoneLineItem());
        items.add(createLockPatternLineItem());
        items.add(createLockPasswordLineItem());
        items.add(createLockPinLineItem());
        return items;
    }

    private ListItem createNoneLineItem() {
        TextListItem item = new TextListItem(getContext());
        item.setTitle(getString(R.string.security_lock_none));
        if (mPasswordQuality == DevicePolicyManager.PASSWORD_QUALITY_UNSPECIFIED) {
            item.setBody(getString(R.string.current_screen_lock));
            // TODO set item disabled after b/78784323 is fixed
        } else {
            item.setOnClickListener(view -> {
                ConfirmRemoveScreenLockDialog dialog = new ConfirmRemoveScreenLockDialog();
                dialog.setConfirmRemoveLockListener(mConfirmListener);
                dialog.show(getFragmentManager(), DIALOG_TAG);
            });
        }
        return item;
    }

    private ListItem createLockPatternLineItem() {
        TextListItem item = new TextListItem(getContext());
        item.setTitle(getString(R.string.security_lock_pattern));
        if (mPasswordQuality == DevicePolicyManager.PASSWORD_QUALITY_SOMETHING) {
            item.setBody(getString(R.string.current_screen_lock));
        }
        item.setOnClickListener(view -> launchFragment(ChooseLockPatternFragment.newInstance()));
        return item;
    }

    private ListItem createLockPasswordLineItem() {
        TextListItem item = new TextListItem(getContext());
        item.setTitle(getString(R.string.security_lock_password));
        if (mPasswordQuality == DevicePolicyManager.PASSWORD_QUALITY_ALPHABETIC
                || mPasswordQuality == DevicePolicyManager.PASSWORD_QUALITY_ALPHANUMERIC) {
            item.setBody(getString(R.string.current_screen_lock));
        }
        item.setOnClickListener(view -> launchFragment(
                ChooseLockPinPasswordFragment.newPasswordInstance()));
        return item;
    }

    private ListItem createLockPinLineItem() {
        TextListItem item = new TextListItem(getContext());
        item.setTitle(getString(R.string.security_lock_pin));
        if (mPasswordQuality == DevicePolicyManager.PASSWORD_QUALITY_NUMERIC
                || mPasswordQuality == DevicePolicyManager.PASSWORD_QUALITY_NUMERIC_COMPLEX) {
            item.setBody(getString(R.string.current_screen_lock));
        }
        item.setOnClickListener(view -> launchFragment(
                ChooseLockPinPasswordFragment.newPinInstance()));
        return item;
    }

    private void launchFragment(BaseFragment fragment) {
        if (!TextUtils.isEmpty(mCurrPassword)) {
            Bundle args = fragment.getArguments();
            if (args == null) {
                args = new Bundle();
            }
            args.putString(SettingsScreenLockActivity.EXTRA_CURRENT_SCREEN_LOCK, mCurrPassword);
            fragment.setArguments(args);
        }

        getFragmentController().launchFragment(fragment);
    }
}
