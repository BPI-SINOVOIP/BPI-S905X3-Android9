/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.content.pm.cts;

import static android.content.pm.ApplicationInfo.CATEGORY_MAPS;
import static android.content.pm.ApplicationInfo.CATEGORY_PRODUCTIVITY;
import static android.content.pm.ApplicationInfo.CATEGORY_UNDEFINED;
import static android.content.pm.ApplicationInfo.FLAG_MULTIARCH;
import static android.content.pm.ApplicationInfo.FLAG_SUPPORTS_RTL;
import static android.os.Process.FIRST_APPLICATION_UID;
import static android.os.Process.LAST_APPLICATION_UID;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.cts.R;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Parcel;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.StringBuilderPrinter;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test {@link ApplicationInfo}.
 */
@RunWith(AndroidJUnit4.class)
public class ApplicationInfoTest {
    private static final String SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME = "com.android.cts.stub";

    private ApplicationInfo mApplicationInfo;
    private String mPackageName;

    @Before
    public void setUp() throws Exception {
        mPackageName = getContext().getPackageName();
    }

    private Context getContext() {
        return InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    @Test
    public void testConstructor() {
        ApplicationInfo info = new ApplicationInfo();
        // simple test to ensure packageName is copied by copy constructor
        // TODO: consider expanding to check all member variables
        info.packageName = mPackageName;
        ApplicationInfo copy = new ApplicationInfo(info);
        assertEquals(info.packageName, copy.packageName);
    }

    @Test
    public void testWriteToParcel() throws NameNotFoundException {
        mApplicationInfo = getContext().getPackageManager().getApplicationInfo(mPackageName, 0);

        Parcel p = Parcel.obtain();
        mApplicationInfo.writeToParcel(p, 0);

        p.setDataPosition(0);
        ApplicationInfo info = ApplicationInfo.CREATOR.createFromParcel(p);
        assertEquals(mApplicationInfo.taskAffinity, info.taskAffinity);
        assertEquals(mApplicationInfo.permission, info.permission);
        assertEquals(mApplicationInfo.processName, info.processName);
        assertEquals(mApplicationInfo.className, info.className);
        assertEquals(mApplicationInfo.theme, info.theme);
        assertEquals(mApplicationInfo.flags, info.flags);
        assertEquals(mApplicationInfo.sourceDir, info.sourceDir);
        assertEquals(mApplicationInfo.publicSourceDir, info.publicSourceDir);
        assertEquals(mApplicationInfo.sharedLibraryFiles, info.sharedLibraryFiles);
        assertEquals(mApplicationInfo.dataDir, info.dataDir);
        assertEquals(mApplicationInfo.uid, info.uid);
        assertEquals(mApplicationInfo.enabled, info.enabled);
        assertEquals(mApplicationInfo.manageSpaceActivityName, info.manageSpaceActivityName);
        assertEquals(mApplicationInfo.descriptionRes, info.descriptionRes);
    }

    @Test
    public void testToString() {
        mApplicationInfo = new ApplicationInfo();
        assertNotNull(mApplicationInfo.toString());
    }

    @Test
    public void testDescribeContents() throws NameNotFoundException {
       mApplicationInfo = getContext().getPackageManager().getApplicationInfo(mPackageName, 0);

        assertEquals(0, mApplicationInfo.describeContents());
    }

    @Test
    public void testDump() {
        mApplicationInfo = new ApplicationInfo();

        StringBuilder sb = new StringBuilder();
        assertEquals(0, sb.length());
        StringBuilderPrinter p = new StringBuilderPrinter(sb);

        String prefix = "";
        mApplicationInfo.dump(p, prefix);
        assertNotNull(sb.toString());
        assertTrue(sb.length() > 0);
    }

    @Test
    public void testLoadDescription() throws NameNotFoundException {
        mApplicationInfo = getContext().getPackageManager().getApplicationInfo(mPackageName, 0);

        assertNull(mApplicationInfo.loadDescription(getContext().getPackageManager()));

        mApplicationInfo.descriptionRes = R.string.hello_world;
        assertEquals(getContext().getResources().getString(R.string.hello_world),
                mApplicationInfo.loadDescription(getContext().getPackageManager()));
    }

    @Test
    public void verifyOwnInfo() throws NameNotFoundException {
        mApplicationInfo = getContext().getPackageManager().getApplicationInfo(mPackageName, 0);

        assertEquals("Android TestCase", mApplicationInfo.nonLocalizedLabel);
        assertEquals(R.drawable.size_48x48, mApplicationInfo.icon);
        assertEquals("android.content.cts.MockApplication", mApplicationInfo.name);
        int flags = FLAG_MULTIARCH | FLAG_SUPPORTS_RTL;
        assertEquals(flags, mApplicationInfo.flags & flags);
        assertEquals(CATEGORY_PRODUCTIVITY, mApplicationInfo.category);
    }

    @Test
    public void verifyDefaultValues() throws NameNotFoundException {
        // The application "com.android.cts.stub" does not have any attributes set
        mApplicationInfo = getContext().getPackageManager().getApplicationInfo(
                SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME, 0);

        assertNull(mApplicationInfo.className);
        assertNull(mApplicationInfo.permission);
        assertEquals(SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME, mApplicationInfo.packageName);
        assertEquals(SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME, mApplicationInfo.processName);
        assertEquals(SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME, mApplicationInfo.taskAffinity);
        assertTrue(FIRST_APPLICATION_UID <= mApplicationInfo.uid
                && LAST_APPLICATION_UID >= mApplicationInfo.uid);
        assertEquals(0, mApplicationInfo.theme);
        assertEquals(0, mApplicationInfo.requiresSmallestWidthDp);
        assertEquals(0, mApplicationInfo.compatibleWidthLimitDp);
        assertEquals(0, mApplicationInfo.largestWidthLimitDp);
        assertNotNull(mApplicationInfo.sourceDir);
        assertEquals(mApplicationInfo.sourceDir, mApplicationInfo.publicSourceDir);
        assertNull(mApplicationInfo.splitSourceDirs);
        assertArrayEquals(mApplicationInfo.splitSourceDirs, mApplicationInfo.splitPublicSourceDirs);
        assertEquals("/data/user/0/" + SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME,
                mApplicationInfo.dataDir);
        assertEquals("/data/user_de/0/" + SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME,
                mApplicationInfo.deviceProtectedDataDir);
        assertEquals("/data/user/0/" + SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME,
                mApplicationInfo.credentialProtectedDataDir);
        assertNull(mApplicationInfo.sharedLibraryFiles);
        assertTrue(mApplicationInfo.enabled);
        assertNull(mApplicationInfo.manageSpaceActivityName);
        assertEquals(0, mApplicationInfo.descriptionRes);
        assertEquals(0, mApplicationInfo.uiOptions);
        assertEquals(CATEGORY_UNDEFINED, mApplicationInfo.category);
    }

    @Test(expected=IllegalArgumentException.class)
    public void setOwnAppCategory() throws Exception {
        getContext().getPackageManager().setApplicationCategoryHint(getContext().getPackageName(),
                CATEGORY_MAPS);
    }

    @Test(expected=IllegalArgumentException.class)
    public void setAppCategoryByNotInstaller() throws Exception {
        getContext().getPackageManager().setApplicationCategoryHint(
                SYNC_ACCOUNT_ACCESS_STUB_PACKAGE_NAME, CATEGORY_MAPS);
    }
}
