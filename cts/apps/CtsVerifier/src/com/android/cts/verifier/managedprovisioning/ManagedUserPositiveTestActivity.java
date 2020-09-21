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

package com.android.cts.verifier.managedprovisioning;

import static com.android.cts.verifier.managedprovisioning.Utils.createInteractiveTestItem;

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.DataSetObserver;
import android.os.Bundle;
import android.provider.Settings;

import com.android.cts.verifier.ArrayTestListAdapter;
import com.android.cts.verifier.IntentDrivenTestActivity.ButtonInfo;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestListAdapter.TestListItem;
import com.android.cts.verifier.TestResult;

/**
 * Activity that lists all positive managed user tests.
 */
public class ManagedUserPositiveTestActivity extends PassFailButtons.TestListActivity {
    private static final String TAG = "ManagedUserPositiveTestActivity";

    private static final String ACTION_CHECK_AFFILIATED_PROFILE_OWNER =
            "com.android.cts.verifier.managedprovisioning.action.CHECK_AFFILIATED_PROFILE_OWNER";
    static final String EXTRA_TEST_ID = "extra-test-id";

    private static final String CHECK_AFFILIATED_PROFILE_OWNER_TEST_ID =
            "CHECK_AFFILIATED_PROFILE_OWNER";
    private static final String DEVICE_ADMIN_SETTINGS_ID = "DEVICE_ADMIN_SETTINGS";
    private static final String DISABLE_STATUS_BAR_TEST_ID = "DISABLE_STATUS_BAR";
    private static final String DISABLE_KEYGUARD_TEST_ID = "DISABLE_KEYGUARD";
    private static final String POLICY_TRANSPARENCY_TEST_ID = "POLICY_TRANSPARENCY";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (ACTION_CHECK_AFFILIATED_PROFILE_OWNER.equals(getIntent().getAction())) {
            DevicePolicyManager dpm = getSystemService(DevicePolicyManager.class);
            if (dpm.isProfileOwnerApp(getPackageName()) && dpm.isAffiliatedUser()) {
                TestResult.setPassedResult(this, getIntent().getStringExtra(EXTRA_TEST_ID),
                        null, null);
            } else {
                TestResult.setFailedResult(this, getIntent().getStringExtra(EXTRA_TEST_ID),
                        getString(R.string.managed_user_incorrect_managed_user), null);
            }
            finish();
            return;
        }

        setContentView(R.layout.positive_managed_user);
        setInfoResources(R.string.managed_user_positive_tests,
                R.string.managed_user_positive_tests_info, 0);
        setPassFailButtonClickListeners();

        final ArrayTestListAdapter adapter = new ArrayTestListAdapter(this);
        adapter.add(TestListItem.newCategory(this, R.string.managed_user_positive_category));

        addTestsToAdapter(adapter);

        adapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                updatePassButton();
            }
        });

        setTestListAdapter(adapter);
    }

    @Override
    public void finish() {
        // If this activity was started for checking profile owner status, then no need to do any
        // tear down.
        if (!ACTION_CHECK_AFFILIATED_PROFILE_OWNER.equals(getIntent().getAction())) {
            // Pass and fail buttons are known to call finish() when clicked,
            // and this is when we want to remove the managed owner.
            DevicePolicyManager dpm = getSystemService(DevicePolicyManager.class);
            dpm.logoutUser(DeviceAdminTestReceiver.getReceiverComponentName());
        }
        super.finish();
    }

    private void addTestsToAdapter(final ArrayTestListAdapter adapter) {
        adapter.add(createTestItem(this, CHECK_AFFILIATED_PROFILE_OWNER_TEST_ID,
                R.string.managed_user_check_managed_user_test,
                new Intent(ACTION_CHECK_AFFILIATED_PROFILE_OWNER)
                        .putExtra(EXTRA_TEST_ID, getIntent().getStringExtra(EXTRA_TEST_ID))));

        // device admin settings
        adapter.add(createInteractiveTestItem(this, DEVICE_ADMIN_SETTINGS_ID,
                R.string.device_owner_device_admin_visible,
                R.string.device_owner_device_admin_visible_info,
                new ButtonInfo(
                        R.string.device_owner_settings_go,
                        new Intent(Settings.ACTION_SECURITY_SETTINGS))));

        // DISABLE_STATUS_BAR_TEST
        if (isStatusBarEnabled()) {
            adapter.add(createInteractiveTestItem(this, DISABLE_STATUS_BAR_TEST_ID,
                    R.string.device_owner_disable_statusbar_test,
                    R.string.device_owner_disable_statusbar_test_info,
                    new ButtonInfo[]{
                            new ButtonInfo(
                                    R.string.device_owner_disable_statusbar_button,
                                    createManagedUserIntentWithBooleanParameter(
                                            CommandReceiverActivity.COMMAND_SET_STATUSBAR_DISABLED,
                                            true)),
                            new ButtonInfo(
                                    R.string.device_owner_reenable_statusbar_button,
                                    createManagedUserIntentWithBooleanParameter(
                                            CommandReceiverActivity.COMMAND_SET_STATUSBAR_DISABLED,
                                            false))}));
        }

        // setKeyguardDisabled
        adapter.add(createInteractiveTestItem(this, DISABLE_KEYGUARD_TEST_ID,
                R.string.device_owner_disable_keyguard_test,
                R.string.device_owner_disable_keyguard_test_info,
                new ButtonInfo[]{
                        new ButtonInfo(
                                R.string.device_owner_disable_keyguard_button,
                                createManagedUserIntentWithBooleanParameter(
                                        CommandReceiverActivity.COMMAND_SET_KEYGUARD_DISABLED,
                                        true)),
                        new ButtonInfo(
                                R.string.device_owner_reenable_keyguard_button,
                                createManagedUserIntentWithBooleanParameter(
                                        CommandReceiverActivity.COMMAND_SET_KEYGUARD_DISABLED,
                                        false))}));

        // Policy Transparency
        final Intent policyTransparencyTestIntent = new Intent(this,
                PolicyTransparencyTestListActivity.class);
        policyTransparencyTestIntent.putExtra(
                PolicyTransparencyTestListActivity.EXTRA_MODE,
                PolicyTransparencyTestListActivity.MODE_MANAGED_USER);
        // So that PolicyTransparencyTestListActivity knows which test to update with the result:
        policyTransparencyTestIntent.putExtra(
                PolicyTransparencyTestActivity.EXTRA_TEST_ID, POLICY_TRANSPARENCY_TEST_ID);
        adapter.add(createTestItem(this, POLICY_TRANSPARENCY_TEST_ID,
                R.string.device_profile_owner_policy_transparency_test,
                policyTransparencyTestIntent));

    }


    static TestListItem createTestItem(Activity activity, String id, int titleRes,
            Intent intent) {
        intent.putExtra(EXTRA_TEST_ID, id);
        return TestListItem.newTest(activity, titleRes, id, intent, null);
    }

    private Intent createManagedUserIntentWithBooleanParameter(String command, boolean value) {
        return new Intent(this, CommandReceiverActivity.class)
                .putExtra(CommandReceiverActivity.EXTRA_COMMAND, command)
                .putExtra(CommandReceiverActivity.EXTRA_ENFORCED, value);
    }

    private boolean isStatusBarEnabled() {
        // Watches don't support the status bar so this is an ok proxy, but this is not the most
        // general test for that. TODO: add a test API to do a real check for status bar support.
        return !getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH);
    }
}
