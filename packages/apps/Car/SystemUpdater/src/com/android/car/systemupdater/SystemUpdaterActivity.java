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
package com.android.car.systemupdater;

import static com.android.car.systemupdater.UpdateLayoutFragment.EXTRA_RESUME_UPDATE;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.MenuItem;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;

/**
 * Apply a system update using an ota package on internal or external storage.
 */
public class SystemUpdaterActivity extends AppCompatActivity
        implements DeviceListFragment.SystemUpdater {

    private static final String FRAGMENT_TAG = "FRAGMENT_TAG";
    private static final int STORAGE_PERMISSIONS_REQUEST_CODE = 0;
    private static final String[] REQUIRED_STORAGE_PERMISSIONS = new String[]{
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, REQUIRED_STORAGE_PERMISSIONS,
                    STORAGE_PERMISSIONS_REQUEST_CODE);
        }

        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        if (savedInstanceState == null) {
            Bundle intentExtras = getIntent().getExtras();
            if (intentExtras != null && intentExtras.getBoolean(EXTRA_RESUME_UPDATE)) {
                UpdateLayoutFragment fragment = UpdateLayoutFragment.newResumedInstance();
                getSupportFragmentManager().beginTransaction()
                        .replace(R.id.device_container, fragment, FRAGMENT_TAG)
                        .commitNow();
            } else {
                DeviceListFragment fragment = new DeviceListFragment();
                getSupportFragmentManager().beginTransaction()
                        .replace(R.id.device_container, fragment, FRAGMENT_TAG)
                        .commitNow();
            }
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            UpFragment upFragment =
                    (UpFragment) getSupportFragmentManager().findFragmentByTag(FRAGMENT_TAG);
            if (!upFragment.goUp()) {
                onBackPressed();
            }
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[],
            int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (STORAGE_PERMISSIONS_REQUEST_CODE == requestCode) {
            if (grantResults.length == 0) {
                finish();
            }
            for (int grantResult : grantResults) {
                if (grantResult != PackageManager.PERMISSION_GRANTED) {
                    finish();
                }
            }
        }
    }

    @Override
    public void applyUpdate(File file) {
        UpdateLayoutFragment fragment = UpdateLayoutFragment.getInstance(file);
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.device_container, fragment, FRAGMENT_TAG)
                .addToBackStack(null)
                .commit();
    }
}
