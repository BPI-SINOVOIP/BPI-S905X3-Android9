/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.ui.sidepanel;

import static com.android.tv.TvFeatures.TUNER;

import android.app.ApplicationErrorReport;
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.view.View;
import android.widget.Toast;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.TvSingletons;
import com.android.tv.common.CommonPreferences;
import com.android.tv.common.customization.CustomizationManager;
import com.android.tv.common.util.PermissionUtils;
import com.android.tv.dialog.PinDialogFragment;
import com.android.tv.license.LicenseSideFragment;
import com.android.tv.license.Licenses;
import com.android.tv.util.Utils;
import java.util.ArrayList;
import java.util.List;

/** Shows Live TV settings. */
public class SettingsFragment extends SideFragment {
    private static final String TRACKER_LABEL = "settings";

    @Override
    protected String getTitle() {
        return getResources().getString(R.string.side_panel_title_settings);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        final Item customizeChannelListItem =
                new SubMenuItem(
                        getString(R.string.settings_channel_source_item_customize_channels),
                        getString(
                                R.string
                                        .settings_channel_source_item_customize_channels_description),
                        getMainActivity().getOverlayManager().getSideFragmentManager()) {
                    @Override
                    protected SideFragment getFragment() {
                        return new CustomizeChannelListFragment();
                    }

                    @Override
                    protected void onBind(View view) {
                        super.onBind(view);
                        setEnabled(false);
                    }

                    @Override
                    protected void onUpdate() {
                        super.onUpdate();
                        setEnabled(getChannelDataManager().getChannelCount() != 0);
                    }
                };
        customizeChannelListItem.setEnabled(false);
        items.add(customizeChannelListItem);
        final MainActivity activity = getMainActivity();
        boolean hasNewInput =
                TvSingletons.getSingletons(getContext())
                        .getSetupUtils()
                        .hasNewInput(activity.getTvInputManagerHelper());
        items.add(
                new ActionItem(
                        getString(R.string.settings_channel_source_item_setup),
                        hasNewInput
                                ? getString(R.string.settings_channel_source_item_setup_new_inputs)
                                : null) {
                    @Override
                    protected void onSelected() {
                        activity.getOverlayManager().showSetupFragment();
                        closeFragment();
                    }
                });
        if (PermissionUtils.hasModifyParentalControls(getMainActivity())) {
            items.add(
                    new ActionItem(
                            getString(R.string.settings_parental_controls),
                            getString(
                                    activity.getParentalControlSettings()
                                                    .isParentalControlsEnabled()
                                            ? R.string.option_toggle_parental_controls_on
                                            : R.string.option_toggle_parental_controls_off)) {
                        @Override
                        protected void onSelected() {
                            getMainActivity()
                                    .getOverlayManager()
                                    .getSideFragmentManager()
                                    .hideSidePanel(true);
                            PinDialogFragment fragment =
                                    PinDialogFragment.create(
                                            PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN);
                            getMainActivity()
                                    .getOverlayManager()
                                    .showDialogFragment(
                                            PinDialogFragment.DIALOG_TAG, fragment, true);
                        }
                    });
        } else {
            // Note: parental control is turned off, when MODIFY_PARENTAL_CONTROLS is not granted.
            // But, we may be able to turn on channel lock feature regardless of the permission.
            // It's TBD.
        }
        boolean showTrickplaySetting = false;
        if (TUNER.isEnabled(getContext())) {
            for (TvInputInfo inputInfo :
                    TvSingletons.getSingletons(getContext())
                            .getTvInputManagerHelper()
                            .getTvInputInfos(true, true)) {
                if (Utils.isInternalTvInput(getContext(), inputInfo.getId())) {
                    showTrickplaySetting = true;
                    break;
                }
            }
            if (showTrickplaySetting) {
                showTrickplaySetting =
                        CustomizationManager.getTrickplayMode(getContext())
                                == CustomizationManager.TRICKPLAY_MODE_ENABLED;
            }
        }
        if (showTrickplaySetting) {
            items.add(
                    new SwitchItem(
                            getString(R.string.settings_trickplay),
                            getString(R.string.settings_trickplay),
                            getString(R.string.settings_trickplay_description),
                            getResources().getInteger(R.integer.trickplay_description_max_lines)) {
                        @Override
                        protected void onUpdate() {
                            super.onUpdate();
                            boolean enabled =
                                    CommonPreferences.getTrickplaySetting(getContext())
                                            != CommonPreferences.TRICKPLAY_SETTING_DISABLED;
                            setChecked(enabled);
                        }

                        @Override
                        protected void onSelected() {
                            super.onSelected();
                            @CommonPreferences.TrickplaySetting
                            int setting =
                                    isChecked()
                                            ? CommonPreferences.TRICKPLAY_SETTING_ENABLED
                                            : CommonPreferences.TRICKPLAY_SETTING_DISABLED;
                            CommonPreferences.setTrickplaySetting(getContext(), setting);
                        }
                    });
        }

        //add send back option if related component has been found
        PackageManager packageManager = getMainActivity().getPackageManager();
        Intent activityIntent = new Intent(Intent.ACTION_APP_ERROR, null);
        List<ResolveInfo> resolveInfos = packageManager.queryIntentActivities(activityIntent, PackageManager.MATCH_DEFAULT_ONLY);
        boolean hasSendBack = (resolveInfos != null && resolveInfos.size() > 0) ? true : false;
        if (hasSendBack) {
            items.add(
                    new ActionItem(getString(R.string.settings_send_feedback)) {
                        @Override
                        protected void onSelected() {
                            Intent intent = new Intent(Intent.ACTION_APP_ERROR);
                            ApplicationErrorReport report = new ApplicationErrorReport();
                            report.packageName = report.processName = getContext().getPackageName();
                            report.time = System.currentTimeMillis();
                            report.type = ApplicationErrorReport.TYPE_NONE;
                            intent.putExtra(Intent.EXTRA_BUG_REPORT, report);
                            startActivityForResult(intent, 0);
                        }
                    });
        }

        if (Licenses.hasLicenses(getContext())) {
            items.add(
                    new SubMenuItem(
                            getString(R.string.settings_menu_licenses),
                            getMainActivity().getOverlayManager().getSideFragmentManager()) {
                        @Override
                        protected SideFragment getFragment() {
                            return new LicenseSideFragment();
                        }
                    });
        }
        // Show version.
        SimpleActionItem version =
                new SimpleActionItem(
                        getString(R.string.settings_menu_version),
                        ((TvApplication) activity.getApplicationContext()).getVersionName());
        version.setClickable(false);
        items.add(version);
        return items;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (getChannelDataManager().areAllChannelsHidden()) {
            Toast.makeText(getActivity(), R.string.msg_all_channels_hidden, Toast.LENGTH_SHORT)
                    .show();
        }
    }
}
