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

package com.android.cts.verifier.location.base;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.location.cts.GnssTestCase;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.LayoutInflater;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestResult;
import java.util.concurrent.TimeUnit;

/**
 * An Activity that allows Gnss CTS tests to be executed inside CtsVerifier.
 *
 * Sub-classes pass the test class as part of construction.
 * One JUnit test class is executed per Activity, the test class can still be executed outside
 * CtsVerifier.
 */
public abstract class EmergencyCallBaseTestActivity extends GnssCtsTestActivity {
    private static final String PHONE_NUMBER_KEY = "android.cts.emergencycall.phonenumber";
    private static final String defaultPhonePackageName = "com.google.android.dialer";

    /**
     * Constructor for a CTS test executor. It will execute a standalone CTS test class.
     *
     * @param testClass The test class to execute, it must be a subclass of {@link AndroidTestCase}.
     */
    protected EmergencyCallBaseTestActivity(Class<? extends GnssTestCase> testClass) {
        super(testClass, R.layout.gnss_emergency_test);
    }

    protected abstract long getPhoneCallDurationMs();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // override the test info
        mTextView.setText(R.string.location_emergency_call_test_info);
        EmergencyCallUtil.setDefaultDialer(this, this.getPackageName());
        setPassFailButtonClickListeners();

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EmergencyCallUtil.setDefaultDialer(this, defaultPhonePackageName);
    }

    protected abstract boolean showLocalNumberInputbox();

    @Override
    public void onClick(View target) {
        // skip current test if device doesn't support cellular
        if (!EmergencyCallUtil.isPhoneDevice(this)) {
            String skipInfo = getResources().getString(R.string.emergency_call_skip_info);
            TestResult.setPassedResult(this, super.getClass().getName(), skipInfo);
            Toast toast = Toast.makeText(getApplicationContext(), skipInfo, Toast.LENGTH_LONG);
            toast.show();
            this.finish();
            return;
        }
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        final FrameLayout frameView = new FrameLayout(this);
        builder.setView(frameView);

        final boolean enableLocalNumberInputBox = showLocalNumberInputbox();
        final AlertDialog alertDialog = builder.create();
        LayoutInflater inflater = alertDialog.getLayoutInflater();

        View dialogView;
        if (enableLocalNumberInputBox) {
            dialogView =
                inflater.inflate(R.layout.emergency_call_msg_test_confirm_dialog, frameView);
        } else {
            dialogView = inflater.inflate(R.layout.emergency_call_confirm_dialog, frameView);
        }
        final EditText targetNumberEditText =
            (EditText) dialogView.findViewById(R.id.emergency_number);
        final Button dialButton = (Button) dialogView.findViewById(R.id.dial_button);
        dialButton.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (enableLocalNumberInputBox) {
                    final EditText currentNumberEditText =
                        (EditText) dialogView.findViewById(R.id.local_phone_number);
                    String currentNumber = currentNumberEditText.getText().toString();
                    // pass the number to cts tests for cts verifier UI, through System property.
                    System.setProperty(PHONE_NUMBER_KEY, currentNumber);
                }
                int targetPhoneNumber =
                    Integer.parseInt(targetNumberEditText.getText().toString());
                long callDurationMs = EmergencyCallBaseTestActivity.this.getPhoneCallDurationMs();
                EmergencyCallUtil.makePhoneCall(
                    EmergencyCallBaseTestActivity.this, targetPhoneNumber);
                EmergencyCallBaseTestActivity.super.onClick(target);
                EmergencyCallUtil.endCallWithDelay(
                    EmergencyCallBaseTestActivity.this.getApplicationContext(), callDurationMs);
                alertDialog.dismiss();
            }
        });
        alertDialog.show();
    }
}
