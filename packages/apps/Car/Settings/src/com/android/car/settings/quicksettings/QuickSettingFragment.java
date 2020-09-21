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
 * limitations under the License.
 */
package com.android.car.settings.quicksettings;

import android.car.drivingstate.CarUxRestrictions;
import android.car.user.CarUserManagerHelper;
import android.content.pm.UserInfo;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.car.widget.PagedListView;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.common.CarUxRestrictionsHelper;
import com.android.car.settings.home.HomepageFragment;
import com.android.car.settings.users.UserIconProvider;
import com.android.car.settings.users.UserSwitcherFragment;

/**
 * Shows a page to access frequently used settings.
 */
public class QuickSettingFragment extends BaseFragment {
    private CarUserManagerHelper  mCarUserManagerHelper;
    private UserIconProvider mUserIconProvider;
    private QuickSettingGridAdapter mGridAdapter;
    private PagedListView mListView;
    private View mFullSettingBtn;
    private View mUserSwitcherBtn;
    private HomeFragmentLauncher mHomeFragmentLauncher;
    private float mOpacityDisabled;
    private float mOpacityEnabled;

    /**
     * Returns an instance of this class.
     */
    public static QuickSettingFragment newInstance() {
        QuickSettingFragment quickSettingFragment = new QuickSettingFragment();
        Bundle bundle = quickSettingFragment.getBundle();
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_quick_settings);
        bundle.putInt(EXTRA_LAYOUT, R.layout.quick_settings);
        bundle.putInt(EXTRA_TITLE_ID, R.string.settings_label);
        quickSettingFragment.setArguments(bundle);
        return quickSettingFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mHomeFragmentLauncher = new HomeFragmentLauncher();
        getActivity().findViewById(R.id.action_bar_icon_container).setOnClickListener(
                v -> getActivity().finish());

        mOpacityDisabled = getContext().getResources().getFloat(R.dimen.opacity_disabled);
        mOpacityEnabled = getContext().getResources().getFloat(R.dimen.opacity_enabled);
        mCarUserManagerHelper = new CarUserManagerHelper(getContext());
        mUserIconProvider = new UserIconProvider(mCarUserManagerHelper);
        mListView = (PagedListView) getActivity().findViewById(R.id.list);
        mGridAdapter = new QuickSettingGridAdapter(getContext());
        mListView.getRecyclerView().setLayoutManager(mGridAdapter.getGridLayoutManager());

        mFullSettingBtn = getActivity().findViewById(R.id.full_setting_btn);
        mFullSettingBtn.setOnClickListener(mHomeFragmentLauncher);
        mUserSwitcherBtn = getActivity().findViewById(R.id.user_switcher_btn);
        mUserSwitcherBtn.setOnClickListener(v -> {
            getFragmentController().launchFragment(UserSwitcherFragment.newInstance());
        });

        View exitBtn = getActivity().findViewById(R.id.exit_button);
        exitBtn.setOnClickListener(v -> getFragmentController().goBack());

        mGridAdapter
                .addTile(new WifiTile(getContext(), mGridAdapter, getFragmentController()))
                .addTile(new BluetoothTile(getContext(), mGridAdapter))
                .addTile(new DayNightTile(getContext(), mGridAdapter))
                .addTile(new CelluarTile(getContext(), mGridAdapter))
                .addSeekbarTile(new BrightnessTile(getContext()));
        mListView.setAdapter(mGridAdapter);
    }

    @Override
    public void onStop() {
        super.onStop();
        mGridAdapter.stop();
    }

    private void setupAccountButton() {
        ImageView userIcon = (ImageView) getActivity().findViewById(R.id.user_icon);
        UserInfo currentUserInfo = mCarUserManagerHelper.getCurrentForegroundUserInfo();
        userIcon.setImageDrawable(mUserIconProvider.getUserIcon(currentUserInfo, getContext()));
        userIcon.clearColorFilter();

        TextView userSwitcherText = (TextView) getActivity().findViewById(R.id.user_switcher_text);
        userSwitcherText.setText(currentUserInfo.name);
    }

    /**
     * Quick setting fragment is distraction optimized, so is allowed at all times.
     */
    @Override
    public boolean canBeShown(CarUxRestrictions carUxRestrictions) {
        return true;
    }

    @Override
    public void onUxRestrictionChanged(CarUxRestrictions carUxRestrictions) {
        // TODO: update tiles
        applyRestriction(CarUxRestrictionsHelper.isNoSetup(carUxRestrictions));
    }

    private void applyRestriction(boolean restricted) {
        mHomeFragmentLauncher.showDOBlockingMessage(restricted);
        mFullSettingBtn.setAlpha(restricted ? mOpacityDisabled : mOpacityEnabled);
    }

    private class HomeFragmentLauncher implements OnClickListener {
        private boolean mShowDOBlockingMessage;

        private void showDOBlockingMessage(boolean show) {
            mShowDOBlockingMessage = show;
        }

        @Override
        public void onClick(View v) {
            if (mShowDOBlockingMessage) {
                getFragmentController().showDOBlockingMessage();
            } else {
                getFragmentController().launchFragment(HomepageFragment.newInstance());
            }
        }
    }
}
