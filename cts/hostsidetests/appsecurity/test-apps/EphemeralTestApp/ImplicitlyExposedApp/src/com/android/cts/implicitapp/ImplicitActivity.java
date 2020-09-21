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

package com.android.cts.implicitapp;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import com.android.cts.util.TestResult;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.List;

public class ImplicitActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        boolean canAccessInstantApp = false;
        String exception = null;
        try {
            canAccessInstantApp = tryAccessingInstantApp();
        } catch (Throwable t) {
            exception = t.getClass().getName();
        }

        TestResult.getBuilder()
                .setPackageName("com.android.cts.implicitapp")
                .setComponentName("ImplicitActivity")
                .setStatus("PASS")
                .setException(exception)
                .setEphemeralPackageInfoExposed(canAccessInstantApp)
                .build()
                .broadcast(this);
        finish();
    }

    private boolean tryAccessingInstantApp() throws NameNotFoundException {
        final PackageInfo info = getPackageManager()
                .getPackageInfo("com.android.cts.ephemeralapp1", 0 /*flags*/);
        return (info != null);
    }
}
