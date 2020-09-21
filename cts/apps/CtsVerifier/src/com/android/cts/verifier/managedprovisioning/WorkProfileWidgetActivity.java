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

package com.android.cts.verifier.managedprovisioning;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.os.Bundle;
import android.view.View;

import com.android.cts.verifier.ArrayTestListAdapter;
import com.android.cts.verifier.DialogTestListActivity;
import com.android.cts.verifier.R;

public class WorkProfileWidgetActivity extends DialogTestListActivity {

    public static final String ACTION_TEST_WORK_PROFILE_WIDGET =
            "com.android.cts.verifier.byod.test_work_profile_widget";

    private DevicePolicyManager mDpm;

    public WorkProfileWidgetActivity() {
        super(R.layout.provisioning_byod,
                R.string.provisioning_byod_work_profile_widget,
                R.string.provisioning_byod_work_profile_widget_info,
                R.string.provisioning_byod_work_profile_widget_description);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPrepareTestButton.setVisibility(View.GONE);

        mDpm = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
        allowToAddCtsVerifierWidget();
    }

    @Override
    public void finish() {
        // Revert the policy.
        disallowToAddCtsVerifierWidget();
        super.finish();
    }

    @Override
    protected void setupTests(ArrayTestListAdapter adapter) {
        // no-op
    }

    private void allowToAddCtsVerifierWidget() {
        mDpm.addCrossProfileWidgetProvider(getAdminComponent(), getPackageName());
    }

    private void disallowToAddCtsVerifierWidget() {
        mDpm.removeCrossProfileWidgetProvider(getAdminComponent(), getPackageName());
    }

    private ComponentName getAdminComponent() {
        return DeviceAdminTestReceiver.getReceiverComponentName();
    }
}
