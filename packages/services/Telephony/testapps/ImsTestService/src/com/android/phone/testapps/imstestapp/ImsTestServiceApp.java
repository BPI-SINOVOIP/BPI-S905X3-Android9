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
 * limitations under the License.
 */

package com.android.phone.testapps.imstestapp;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.LinearLayout;

/**
 * Main activity for Test ImsService Application.
 */

public class ImsTestServiceApp extends Activity {

    private LinearLayout mConnections;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        ((Button) findViewById(R.id.control_launch_reg)).setOnClickListener(
                (view) -> launchRegistrationActivity());

        ((Button) findViewById(R.id.control_launch_calling)).setOnClickListener(
                (view) -> launchCallingActivity());

        ((Button) findViewById(R.id.control_launch_config)).setOnClickListener(
                (view) -> launchConfigActivity());

        // Adds Card view for testing
        mConnections = findViewById(R.id.connections_list);
        mConnections.addView(getLayoutInflater().inflate(R.layout.ims_connection, null, false));
    }

    private void launchRegistrationActivity() {
        Intent intent = new Intent(this, ImsRegistrationActivity.class);
        startActivity(intent);
    }

    private void launchCallingActivity() {
        Intent intent = new Intent(this, ImsCallingActivity.class);
        startActivity(intent);
    }

    private void launchConfigActivity() {
        Intent intent = new Intent(this, ImsConfigActivity.class);
        startActivity(intent);
    }
}
