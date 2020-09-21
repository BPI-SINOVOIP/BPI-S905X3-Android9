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

import android.car.user.CarUserManagerHelper;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;

import java.util.ArrayList;
import java.util.List;

/**
 * Shows details for a guest session, including ability to switch to guest.
 */
public class GuestFragment extends ListItemSettingsFragment {
    private CarUserManagerHelper mCarUserManagerHelper;
    private ListItemProvider mItemProvider;

    /**
     * Create new GuestFragment instance.
     */
    public static GuestFragment newInstance() {
        GuestFragment guestFragment = new GuestFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.user_details_title);
        bundle.putInt(EXTRA_ACTION_BAR_LAYOUT, R.layout.action_bar_with_button);
        guestFragment.setArguments(bundle);
        return guestFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mCarUserManagerHelper = new CarUserManagerHelper(getContext());
        mItemProvider = new ListItemProvider.ListProvider(getListItems());

        // Super class's onActivityCreated need to be called after mItemProvider is initialized.
        // Because getItemProvider is called in there.
        super.onActivityCreated(savedInstanceState);

        showSwitchButton();
    }

    private void showSwitchButton() {
        Button switchUserBtn = (Button) getActivity().findViewById(R.id.action_button1);
        // If the current process is not allowed to switch to another user, doe not show the switch
        // button.
        if (!mCarUserManagerHelper.canCurrentProcessSwitchUsers()) {
            switchUserBtn.setVisibility(View.GONE);
            return;
        }
        switchUserBtn.setVisibility(View.VISIBLE);
        switchUserBtn.setText(R.string.user_switch);
        switchUserBtn.setOnClickListener(v -> {
            getActivity().onBackPressed();
            mCarUserManagerHelper.startNewGuestSession(getContext().getString(R.string.user_guest));
        });
    }

    private List<ListItem> getListItems() {
        Drawable icon = UserIconProvider.scaleUserIcon(mCarUserManagerHelper.getGuestDefaultIcon(),
                mCarUserManagerHelper, getContext());

        TextListItem item = new TextListItem(getContext());
        item.setPrimaryActionIcon(icon, /* useLargeIcon= */ false);
        item.setTitle(getContext().getString(R.string.user_guest));

        List<ListItem> items = new ArrayList<>();
        items.add(item);
        return items;
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }
}
