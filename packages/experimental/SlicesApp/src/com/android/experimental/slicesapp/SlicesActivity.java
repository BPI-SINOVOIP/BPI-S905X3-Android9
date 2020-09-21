/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.experimental.slicesapp;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import java.util.ArrayList;
import java.util.List;

public class SlicesActivity extends Activity {

    private static final String TAG = "SlicesActivity";

    private int mState;
    private ViewGroup mSlicePreviewFrame;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        SharedPreferences state = getSharedPreferences("slice", 0);
        mState = 0;
        if (state != null) {
            mState = Integer.parseInt(state.getString(getUri().toString(), "0"));
        }
        setContentView(R.layout.activity_layout);
        mSlicePreviewFrame = (ViewGroup) findViewById(R.id.slice_preview);
        findViewById(R.id.subtract).setOnClickListener(v -> {
            mState--;
            state.edit().putString(getUri().toString(), String.valueOf(mState)).commit();
            updateState();
            getContentResolver().notifyChange(getUri(), null);
        });
        findViewById(R.id.add).setOnClickListener(v -> {
            mState++;
            state.edit().putString(getUri().toString(), String.valueOf(mState)).commit();
            updateState();
            getContentResolver().notifyChange(getUri(), null);
        });
        findViewById(R.id.auth).setOnClickListener(v -> {
            List<ApplicationInfo> packages = getPackageManager().getInstalledApplications(0);
            packages.forEach(info -> grantUriPermission(info.packageName, getUri(),
                    Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                            | Intent.FLAG_GRANT_WRITE_URI_PERMISSION));
        });

        Spinner spinner = findViewById(R.id.spinner);
        List<String> list = new ArrayList<>();
        list.add("Default");
        list.add("Single-line");
        list.add("Single-line action");
        list.add("Two-line");
        list.add("Two-line action");
        list.add("Weather");
        list.add("Messaging");
        list.add("Keep actions");
        list.add("Maps multi");
        list.add("Settings");
        list.add("Settings content");
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, list);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        spinner.setSelection(list.indexOf(state.getString("slice_type", "Default")));
        spinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                state.edit().putString("slice_type", list.get(position)).commit();
                getContentResolver().notifyChange(getUri(), null);
                updateSlice(getUri());
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        bindCheckbox(R.id.header, state, "show_header");
        bindCheckbox(R.id.sub_header, state, "show_sub_header");
        bindCheckbox(R.id.show_action_row, state, "show_action_row");

        updateState();
    }

    private void bindCheckbox(int id, SharedPreferences state, String key) {
        CheckBox checkbox = findViewById(id);
        checkbox.setChecked(state.getBoolean(key, false));
        checkbox.setOnCheckedChangeListener((v, isChecked) -> {
            state.edit().putBoolean(key, isChecked).commit();
            getContentResolver().notifyChange(getUri(), null);
            updateSlice(getUri());
        });
    }

    private void updateState() {
        ((TextView) findViewById(R.id.summary)).setText(String.valueOf(mState));
    }

    public Uri getUri() {
        return new Uri.Builder()
                .scheme(ContentResolver.SCHEME_CONTENT)
                .authority(getPackageName())
                .appendPath("main").build();
    }

    private void updateSlice(Uri uri) {
    }
}
