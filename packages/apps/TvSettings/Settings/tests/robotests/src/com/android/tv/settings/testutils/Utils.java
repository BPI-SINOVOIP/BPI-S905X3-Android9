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

package com.android.tv.settings.testutils;

import static org.robolectric.Shadows.shadowOf;

import android.Manifest;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;

import org.robolectric.RuntimeEnvironment;
import org.robolectric.shadows.ShadowPackageManager;

public class Utils {

    public static void addAutofill(String packageName, String className) {
        final ShadowPackageManager shadowPackageManager = shadowOf(
                RuntimeEnvironment.application.getPackageManager());
        final Intent intent = new Intent("android.service.autofill.AutofillService");
        final ResolveInfo resolveInfo = new ResolveInfo();
        resolveInfo.resolvePackageName = packageName;
        final ServiceInfo serviceInfo = new ServiceInfo();
        serviceInfo.permission = Manifest.permission.BIND_AUTOFILL_SERVICE;
        serviceInfo.packageName = packageName;
        serviceInfo.name = className;
        resolveInfo.serviceInfo = serviceInfo;
        final ApplicationInfo applicationInfo = new ApplicationInfo();
        applicationInfo.flags = ApplicationInfo.FLAG_SYSTEM;
        serviceInfo.applicationInfo = applicationInfo;
        final PackageInfo autofillPackageInfo = new PackageInfo();
        autofillPackageInfo.packageName = packageName;
        autofillPackageInfo.applicationInfo = applicationInfo;
        shadowPackageManager.addPackage(autofillPackageInfo);
        shadowPackageManager.addResolveInfoForIntent(intent, resolveInfo);
    }

}
