/*
 * Copyright (C) 2016 Google Inc.
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

package android.os.cts;

import com.android.compatibility.common.util.ApiLevelUtil;

import android.os.Build;
import android.os.SystemProperties;
import android.test.InstrumentationTestCase;
import android.util.Log;

/**
 * Tests for Security Patch String settings
 */
public class SecurityPatchTest extends InstrumentationTestCase {

    private static final String TAG = SecurityPatchTest.class.getSimpleName();
    private static final String SECURITY_PATCH_ERROR =
            "security_patch should be in the format \"YYYY-MM-DD\". Found \"%s\"";
    private static final String SECURITY_PATCH_DATE_ERROR =
            "security_patch should be \"%d-%02d\" or later. Found \"%s\"";
    private static final int SECURITY_PATCH_YEAR = 2016;
    private static final int SECURITY_PATCH_MONTH = 12;

    private boolean mSkipTests = false;
    private String mVendorSecurityPatch;
    private String mBuildSecurityPatch;

    @Override
    protected void setUp() {
        mSkipTests = (ApiLevelUtil.isBefore(Build.VERSION_CODES.M));
        mVendorSecurityPatch = SystemProperties.get("ro.vendor.build.security_patch", "");
        mBuildSecurityPatch = Build.VERSION.SECURITY_PATCH;
    }

    /** Security patch string must exist in M or higher **/
    public void testSecurityPatchFound() {
        if (mSkipTests) {
            Log.w(TAG, "Skipping M+ Test.");
            return;
        }
        String error = String.format(SECURITY_PATCH_ERROR, mBuildSecurityPatch);
        assertTrue(error, !mBuildSecurityPatch.isEmpty());
    }

    public void testVendorSecurityPatchFound() {
        if (Build.VERSION.FIRST_SDK_INT <= Build.VERSION_CODES.O) {
            Log.w(TAG, "Skipping P+ Test");
            return;
        }
        assertTrue(!mVendorSecurityPatch.isEmpty());
    }

    public void testSecurityPatchesFormat() {
        if (mSkipTests) {
            Log.w(TAG, "Skipping M+ Test.");
            return;
        }
        String error = String.format(SECURITY_PATCH_ERROR, mBuildSecurityPatch);
        testSecurityPatchFormat(mBuildSecurityPatch, error);

        if (Build.VERSION.FIRST_SDK_INT <= Build.VERSION_CODES.O) {
            Log.w(TAG, "Skipping P+ Test");
            return;
        }
        error = String.format(SECURITY_PATCH_ERROR, mVendorSecurityPatch);
        testSecurityPatchFormat(mVendorSecurityPatch, error);
    }

    /** Security patch should be of the form YYYY-MM-DD in M or higher */
    private void testSecurityPatchFormat(String patch, String error) {
        assertEquals(error, 10, patch.length());
        assertTrue(error, Character.isDigit(patch.charAt(0)));
        assertTrue(error, Character.isDigit(patch.charAt(1)));
        assertTrue(error, Character.isDigit(patch.charAt(2)));
        assertTrue(error, Character.isDigit(patch.charAt(3)));
        assertEquals(error, '-', patch.charAt(4));
        assertTrue(error, Character.isDigit(patch.charAt(5)));
        assertTrue(error, Character.isDigit(patch.charAt(6)));
        assertEquals(error, '-', patch.charAt(7));
        assertTrue(error, Character.isDigit(patch.charAt(8)));
        assertTrue(error, Character.isDigit(patch.charAt(9)));
    }

    public void testSecurityPatchDates() {
        if (mSkipTests) {
            Log.w(TAG, "Skipping M+ Test.");
            return;
        }

        String error = String.format(SECURITY_PATCH_DATE_ERROR,
                                     SECURITY_PATCH_YEAR,
                                     SECURITY_PATCH_MONTH,
                                     mBuildSecurityPatch);
        testSecurityPatchDate(mBuildSecurityPatch, error);

        if (Build.VERSION.FIRST_SDK_INT <= Build.VERSION_CODES.O) {
            Log.w(TAG, "Skipping P+ Test");
            return;
        }
        error = String.format(SECURITY_PATCH_DATE_ERROR,
                                     SECURITY_PATCH_YEAR,
                                     SECURITY_PATCH_MONTH,
                                     mVendorSecurityPatch);
        testSecurityPatchDate(mVendorSecurityPatch, error);
    }
    /** Security patch should no older than the month this test was updated in M or higher **/
    private void testSecurityPatchDate(String patch, String error) {
        int declaredYear = 0;
        int declaredMonth = 0;

        try {
            declaredYear = Integer.parseInt(patch.substring(0,4));
            declaredMonth = Integer.parseInt(patch.substring(5,7));
        } catch (Exception e) {
            assertTrue(error, false);
        }

        assertTrue(error, declaredYear >= SECURITY_PATCH_YEAR);
        assertTrue(error, (declaredYear > SECURITY_PATCH_YEAR) ||
                          (declaredMonth >= SECURITY_PATCH_MONTH));
    }
}
