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
package com.android.server;

import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.when;

import android.content.pm.PackageManagerInternal;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.testing.TestableContext;

import org.junit.Before;
import org.junit.Rule;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

public class UiServiceTestCase {
    @Mock protected PackageManagerInternal mPmi;

    protected static final String PKG_N_MR1 = "com.example.n_mr1";
    protected static final String PKG_O = "com.example.o";
    protected static final String PKG_P = "com.example.p";

    @Rule
    public final TestableContext mContext =
            new TestableContext(InstrumentationRegistry.getContext(), null);

    protected TestableContext getContext() {
        return mContext;
    }

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);

        // Share classloader to allow package access.
        System.setProperty("dexmaker.share_classloader", "true");

        // Assume some default packages
        LocalServices.removeServiceForTest(PackageManagerInternal.class);
        LocalServices.addService(PackageManagerInternal.class, mPmi);
        when(mPmi.getPackageTargetSdkVersion(anyString()))
                .thenAnswer((iom) -> {
                    switch ((String) iom.getArgument(0)) {
                        case PKG_N_MR1:
                            return Build.VERSION_CODES.N_MR1;
                        case PKG_O:
                            return Build.VERSION_CODES.O;
                        case PKG_P:
                            return Build.VERSION_CODES.P;
                        default:
                            return Build.VERSION_CODES.CUR_DEVELOPMENT;
                    }
                });
    }
}
