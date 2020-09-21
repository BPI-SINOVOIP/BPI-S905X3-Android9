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

package android.security;

import android.test.AndroidTestCase;
import java.io.IOException;

/**
 * Verify the selinux domain for apps running with 25<targetSdkVersion<=27
 */
public class SELinuxTargetSdkTest extends SELinuxTargetSdkTestBase
{
    /**
     * Verify that net.dns properties may not be read
     */
    public void testNoDns() throws IOException {
        noDns();
    }

    /**
     * Verify that selinux context is the expected domain based on
     * targetSdkVersion = 27
     */
    public void testAppDomainContext() throws IOException {
        String context = "u:r:untrusted_app_27:s0:c[0-9]+,c[0-9]+";
        String msg = "Untrusted apps with targetSdkVersion in range 26-27 " +
            "must run in the untrusted_app_27 selinux domain and use the levelFrom=user " +
            "selector in SELinux seapp_contexts which adds two category types " +
            "to the app's selinux context.\n" +
            "Example expected value: u:r:untrusted_app_27:s0:c512,c768\n" +
            "Actual value: ";
        appDomainContext(context, msg);
    }

    /**
     * Verify that selinux context is the expected type based on
     * targetSdkVersion = current
     */
    public void testAppDataContext() throws Exception {
        String context = "u:object_r:app_data_file:s0:c[0-9]+,c[0-9]+";
        String msg = "Untrusted apps with targetSdkVersion in range 26-27 " +
            "must use the app_data_file selinux context and use the levelFrom=all " +
            "selector in SELinux seapp_contexts which adds four category types " +
            "to the app_data_file context.\n" +
            "Example expected value: u:object_r:app_data_file:s0:c512,c768\n" +
            "Actual value: ";
        appDataContext(context, msg);
    }
}
