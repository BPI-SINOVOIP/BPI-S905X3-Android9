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
import android.os.Bundle;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.util.ArrayMap;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;
import android.telephony.ims.ImsReasonInfo;

import java.util.Map;

public class ImsRegistrationActivity extends Activity {

    private int mSelectedRegTech = ImsRegistrationImplBase.REGISTRATION_TECH_LTE;

    private static final Map<String, Integer> REG_TECH = new ArrayMap<>(2);
    static {
        REG_TECH.put("LTE", ImsRegistrationImplBase.REGISTRATION_TECH_LTE);
        REG_TECH.put("IWLAN", ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN);
    }

    private View mDeregisteredReason;
    private View mRegChangeFailedReason;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_registration);

        //Set up registration tech spinner
        Spinner regTechDropDown = findViewById(R.id.reg_tech_selector);
        regTechDropDown.setAdapter(new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_dropdown_item,
                REG_TECH.keySet().toArray(new String[REG_TECH.size()])));
        regTechDropDown.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                onTechDropDownChanged((String) parent.getItemAtPosition(position));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // Don't change selection
            }
        });

        // Map buttons to onClick listeners
        Button registeredButton = findViewById(R.id.reg_registered_button);
        registeredButton.setOnClickListener((v)->onRegisteredClicked());
        Button registeringButton = findViewById(R.id.reg_registering_button);
        registeringButton.setOnClickListener((v)->onRegisteringClicked());
        Button deregisteredButton = findViewById(R.id.reg_deregistered_button);
        deregisteredButton.setOnClickListener((v)->onDeregisteredClicked());
        Button regChangeFailedButton = findViewById(R.id.reg_changefailed_button);
        regChangeFailedButton.setOnClickListener((v)->onRegChangeFailedClicked());

        mDeregisteredReason = findViewById(R.id.deregistered_imsreasoninfo);
        mRegChangeFailedReason = findViewById(R.id.regchangefail_imsreasoninfo);
    }

    private void onRegisteredClicked() {
        if (!isFrameworkConnected()) {
            return;
        }
        TestImsRegistrationImpl.getInstance().onRegistered(mSelectedRegTech);
    }

    private void onRegisteringClicked() {
        if (!isFrameworkConnected()) {
            return;
        }
        TestImsRegistrationImpl.getInstance().onRegistering(mSelectedRegTech);
    }

    private void onDeregisteredClicked() {
        if (!isFrameworkConnected()) {
            return;
        }
        TestImsRegistrationImpl.getInstance().onDeregistered(getReasonInfo(mDeregisteredReason));
    }

    private void onRegChangeFailedClicked() {
        if (!isFrameworkConnected()) {
            return;
        }
        TestImsRegistrationImpl.getInstance().onTechnologyChangeFailed(mSelectedRegTech,
                getReasonInfo(mRegChangeFailedReason));
    }

    private void onTechDropDownChanged(String item) {
        mSelectedRegTech = REG_TECH.get(item);
    }

    private ImsReasonInfo getReasonInfo(View reasonView) {
        EditText errorCodeText = reasonView.findViewById(R.id.imsreasoninfo_error);
        EditText extraCodeText = reasonView.findViewById(R.id.imsreasoninfo_extra);
        EditText messageText = reasonView.findViewById(R.id.imsreasoninfo_message);

        int errorCode = ImsReasonInfo.CODE_UNSPECIFIED;
        try {
            errorCode = Integer.parseInt(errorCodeText.getText().toString());
        } catch (NumberFormatException e) {
            Toast.makeText(this, "Couldn't parse reason, defaulting to Unspecified.",
                    Toast.LENGTH_SHORT).show();
        }

        int extraCode = ImsReasonInfo.CODE_UNSPECIFIED;
        try {
            extraCode = Integer.parseInt(extraCodeText.getText().toString());
        } catch (NumberFormatException e) {
            Toast.makeText(this, "Couldn't parse reason, defaulting to Unspecified.",
                    Toast.LENGTH_SHORT).show();
        }

        String message = messageText.getText().toString();

        ImsReasonInfo result = new ImsReasonInfo(errorCode, extraCode, message);
        Toast.makeText(this, "getReasonInfo: " + result, Toast.LENGTH_SHORT).show();
        return result;
    }

    private boolean isFrameworkConnected() {
        if (TestImsRegistrationImpl.getInstance() == null) {
            Toast.makeText(this, "Connection to Framework Unavailable!",
                    Toast.LENGTH_LONG).show();
            return false;
        }
        return true;
    }
}
