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

package com.google.android.tv.partner.support;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import com.android.tv.testing.TestSingletonApp;
import com.android.tv.testing.constants.ConfigConstants;
import com.google.thirdparty.robolectric.GoogleRobolectricTestRunner;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

/** Tests for {@link BaseCustomization}. */

// TODO: move to partner-support

@RunWith(GoogleRobolectricTestRunner.class)
@Config(
    manifest =
            "//third_party/java_src/android_app/live_channels/common/src"
                    + "/com/android/tv/common"
                    + ":common/AndroidManifest.xml",
    sdk = ConfigConstants.SDK,
    application = TestSingletonApp.class
)
public class BaseCustomizationTest {

    private static final String[] PERMISSIONS = {"com.example.permission"};

    private Context context;
    private PackageManager packageManager;

    @Before
    public void setUp() {
        context = mock(Context.class);
        // TODO(b/68809029): Don't mock PackageManager
        packageManager = mock(PackageManager.class);
        when(context.getPackageManager()).thenReturn(packageManager);
        List<PackageInfo> packageInfos = new ArrayList<>();
        PackageInfo packageInfo = new PackageInfo();
        packageInfo.packageName = "com.example.custom";
        packageInfos.add(packageInfo);
        when(packageManager.getPackagesHoldingPermissions(PERMISSIONS, 0)).thenReturn(packageInfos);
    }

    @Test
    public void getPackageName_example() {
        TestCustomizations testCustomizations = new TestCustomizations(context);
        assertThat(testCustomizations.getPackageName()).isEqualTo("com.example.custom");
    }

    @Test
    public void getPackageName_none() {
        TestCustomizations testCustomizations =
                new TestCustomizations(RuntimeEnvironment.application);
        assertThat(testCustomizations.getPackageName()).isEmpty();
    }

    static class TestCustomizations extends BaseCustomization {
        protected TestCustomizations(Context context) {
            super(context, PERMISSIONS);
        }
    }
}
