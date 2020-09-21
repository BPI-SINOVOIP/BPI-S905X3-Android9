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

package com.android.car.settings.system;

import android.os.Build;
import android.os.Bundle;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.settings.R;
import com.android.car.settings.common.ListItemSettingsFragment;
import com.android.settingslib.DeviceInfoUtils;

import java.util.ArrayList;
import java.util.List;

/**
 * Shows basic info about the system and provide some actions like update, reset etc.
 */
public class AboutSettingsFragment extends ListItemSettingsFragment {

    private ListItemProvider mItemProvider;

    public static AboutSettingsFragment getInstance() {
        AboutSettingsFragment aboutSettingsFragment = new AboutSettingsFragment();
        Bundle bundle = ListItemSettingsFragment.getBundle();
        bundle.putInt(EXTRA_TITLE_ID, R.string.about_settings);
        aboutSettingsFragment.setArguments(bundle);
        return aboutSettingsFragment;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        mItemProvider = new ListItemProvider.ListProvider(getListItems());
        // super.onActivityCreated() will need itemProvider, so call it after the provider
        // is initialized.
        super.onActivityCreated(savedInstanceState);
    }

    @Override
    public ListItemProvider getItemProvider() {
        return mItemProvider;
    }

    private List<ListItem> getListItems() {
        List<ListItem> listItems = new ArrayList<>();
        TextListItem modelItem = new TextListItem(getContext());
        modelItem.setTitle(getString(R.string.model_info));
        modelItem.setBody(Build.MODEL + DeviceInfoUtils.getMsvSuffix());
        listItems.add(modelItem);

        TextListItem androidVersionItem = new TextListItem(getContext());
        androidVersionItem.setTitle(getString(R.string.firmware_version));
        androidVersionItem.setBody(getString(R.string.about_summary, Build.VERSION.RELEASE));
        listItems.add(androidVersionItem);

        TextListItem securityLevelItem = new TextListItem(getContext());
        securityLevelItem.setTitle(getString(R.string.security_patch));
        securityLevelItem.setBody(DeviceInfoUtils.getSecurityPatch());
        listItems.add(securityLevelItem);

        TextListItem kernelVersionItem = new TextListItem(getContext());
        kernelVersionItem.setTitle(getString(R.string.kernel_version));
        kernelVersionItem.setBody(DeviceInfoUtils.getFormattedKernelVersion(getContext()));
        listItems.add(kernelVersionItem);

        TextListItem buildNumberItem = new TextListItem(getContext());
        buildNumberItem.setTitle(getString(R.string.build_number));
        buildNumberItem.setBody(Build.DISPLAY);
        listItems.add(buildNumberItem);
        return listItems;
    }
}
