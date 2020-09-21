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

package com.android.car.settings.users;

import android.car.drivingstate.CarUxRestrictions;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;

import com.android.car.settings.R;
import com.android.car.settings.common.BaseFragment;
import com.android.car.settings.common.CarUxRestrictionsHelper;

/**
 * Shows a user switcher for all Users available on this device.
 */
public class UserSwitcherFragment extends BaseFragment {

    private UserGridRecyclerView mUserGridView;

    /**
     * Initializes the UserSwitcherFragment
     * @return instance of the UserSwitcherFragment
     */
    public static UserSwitcherFragment newInstance() {
        UserSwitcherFragment userSwitcherFragment = new UserSwitcherFragment();
        Bundle bundle = BaseFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.users_list_title);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        bundle.putInt(EXTRA_LAYOUT, R.layout.car_user_switcher);
        userSwitcherFragment.setArguments(bundle);
        return userSwitcherFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mUserGridView = getView().findViewById(R.id.user_grid);
        GridLayoutManager layoutManager = new GridLayoutManager(getContext(),
                getContext().getResources().getInteger(R.integer.user_switcher_num_col));
        mUserGridView.setFragment(this);
        mUserGridView.getRecyclerView().setLayoutManager(layoutManager);
        mUserGridView.buildAdapter();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    /**
     * User switcher fragment is distraction optimized, so is allowed at all times.
     */
    @Override
    public boolean canBeShown(CarUxRestrictions carUxRestrictions) {
        return true;
    }


    @Override
    public void onUxRestrictionChanged(CarUxRestrictions carUxRestrictions) {
        applyRestriction(CarUxRestrictionsHelper.isNoSetup(carUxRestrictions));
    }

    private void applyRestriction(boolean restricted) {
        if (mUserGridView != null) {
            if (restricted) {
                mUserGridView.disableAddUser();
            } else {
                mUserGridView.enableAddUser();
            }
        }
    }

}
