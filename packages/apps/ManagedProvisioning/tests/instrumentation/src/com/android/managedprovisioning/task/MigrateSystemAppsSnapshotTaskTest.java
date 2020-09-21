/*
 * Copyright 2018, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning.task;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.FileUtils;
import android.os.UserHandle;
import android.os.UserManager;
import android.support.test.InstrumentationRegistry;

import com.android.managedprovisioning.task.nonrequiredapps.SystemAppsSnapshot;
import com.android.managedprovisioning.tests.R;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

@RunWith(MockitoJUnitRunner.class)
public class MigrateSystemAppsSnapshotTaskTest {
    private static final int USER_A_ID = 10;
    private static final int USER_A_SERIAL_NUMBER = 20;
    private static final int USER_A_SNAPSHOT_FILE = R.raw.snapshot;

    private static final int USER_B_ID = 11;
    private static final int USER_B_SERIAL_NUMBER = 21;
    private static final int USER_B_SNAPSHOT_FILE = R.raw.snapshot2;

    private static final int NOT_EXIST_USER_ID = 99;
    private static final int INVALID_SERIAL_NUMBER = -1;

    private static final Set<String> SNAPSHOT_A = new HashSet<>();
    static {
        SNAPSHOT_A.add("com.app.a");
        SNAPSHOT_A.add("com.app.b");
    }

    private static final Set<String> SNAPSHOT_B = new HashSet<>();
    static {
        SNAPSHOT_B.add("com.app.a");
        SNAPSHOT_B.add("com.app.c");
    }

    private MigrateSystemAppsSnapshotTask mMigrateSystemAppsSnapshotTask;
    private SystemAppsSnapshot mSystemAppsSnapshot;
    @Mock
    private Context mContext;
    @Mock
    private AbstractProvisioningTask.Callback mCallback;
    @Mock
    private UserManager mUserManager;

    @Before
    public void setup() {
        when(mContext.getFilesDir())
                .thenReturn(
                        new File(InstrumentationRegistry.getTargetContext().getFilesDir(), "test"));
        mMigrateSystemAppsSnapshotTask = new MigrateSystemAppsSnapshotTask(mContext, mCallback);
        mSystemAppsSnapshot = new SystemAppsSnapshot(mContext);
    }

    @Before
    public void setupUserManager() {
        when(mContext.getSystemService(Context.USER_SERVICE)).thenReturn(mUserManager);
        when(mUserManager.getUserSerialNumber(USER_A_ID)).thenReturn(USER_A_SERIAL_NUMBER);
        when(mUserManager.getUserSerialNumber(USER_B_ID)).thenReturn(USER_B_SERIAL_NUMBER);
        when(mUserManager.getUserSerialNumber(NOT_EXIST_USER_ID)).thenReturn(INVALID_SERIAL_NUMBER);
    }

    @After
    public void tearDown() {
        FileUtils.deleteContentsAndDir(SystemAppsSnapshot.getFolder(mContext));
        FileUtils.deleteContentsAndDir(SystemAppsSnapshot.getLegacyFolder(mContext));
    }

    @Test
    public void testRun_nothingToMigrate() {
        mMigrateSystemAppsSnapshotTask.run(UserHandle.USER_SYSTEM);

        assertFalse(SystemAppsSnapshot.getFolder(mContext).exists());
    }

    @Test
    public void testRun_alreadyMigrated() throws Exception {
        copyTestFileTo(USER_A_SNAPSHOT_FILE, SystemAppsSnapshot.getFolder(mContext), "20.xml");

        mMigrateSystemAppsSnapshotTask.run(UserHandle.USER_SYSTEM);

        assertFilesAreMigrated(new String[]{"20.xml"});
    }

    @Test
    public void testRun_migrate_userExists() throws Exception {
        File legacyFolder = SystemAppsSnapshot.getLegacyFolder(mContext);
        copyTestFileTo(USER_A_SNAPSHOT_FILE, legacyFolder, "10.xml");
        copyTestFileTo(USER_B_SNAPSHOT_FILE, legacyFolder, "11.xml");

        mMigrateSystemAppsSnapshotTask.run(UserHandle.USER_SYSTEM);

        assertFilesAreMigrated(new String[] {"20.xml", "21.xml"});
        assertSnapshotFileContent(10, SNAPSHOT_A);
        assertSnapshotFileContent(11, SNAPSHOT_B);
    }

    @Test
    public void testRun_migrate_userDoesNotExists() throws Exception {
        File legacyFolder = SystemAppsSnapshot.getLegacyFolder(mContext);
        copyTestFileTo(USER_A_SNAPSHOT_FILE, legacyFolder, "99.xml");

        mMigrateSystemAppsSnapshotTask.run(UserHandle.USER_SYSTEM);

        assertFilesAreMigrated(new String[] {});
    }

    @Test
    public void testRun_migrate_invalidFileName() throws Exception {
        File legacyFolder = SystemAppsSnapshot.getLegacyFolder(mContext);
        copyTestFileTo(USER_A_SNAPSHOT_FILE, legacyFolder, "random.xml");

        mMigrateSystemAppsSnapshotTask.run(UserHandle.USER_SYSTEM);

        assertFilesAreMigrated(new String[] {});
    }

    private void copyTestFileTo(
            int fileToBeCopied, File destinationFolder, String fileName) throws Exception {
        File destination = new File(destinationFolder, fileName);
        destination.getParentFile().mkdirs();
        destination.createNewFile();
        FileUtils.copy(getTestFile(fileToBeCopied), new FileOutputStream(destination));
    }

    private void assertSnapshotFileContent(int userId, Set<String> expected) {
        Set<String> actual = mSystemAppsSnapshot.getSnapshot(userId);
        assertEquals(expected, actual);
    }

    private InputStream getTestFile(int resId) {
        return InstrumentationRegistry.getContext().getResources().openRawResource(resId);
    }

    private void assertFilesAreMigrated(String[] expectedFileNames) {
        File legacyFolder = SystemAppsSnapshot.getLegacyFolder(mContext);
        File newFolder = SystemAppsSnapshot.getFolder(mContext);

        assertFalse(legacyFolder.exists());
        assertTrue(newFolder.exists());

        File[] files = newFolder.listFiles();
        assertEquals(expectedFileNames.length, files.length);

        String[] actualFilesNames =
                Arrays.stream(files).map(file -> file.getName()).toArray(String[]::new);

        Arrays.sort(expectedFileNames);
        Arrays.sort(actualFilesNames);
        Assert.assertArrayEquals(expectedFileNames, actualFilesNames);
    }
}
