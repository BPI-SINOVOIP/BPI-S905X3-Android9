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

package com.android.managedprovisioning.task;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.content.pm.PackageManager.INSTALL_ALLOW_TEST;
import static android.content.pm.PackageManager.INSTALL_REPLACE_EXISTING;

import static com.android.managedprovisioning.task.InstallPackageTask.ERROR_INSTALLATION_FAILED;
import static com.android.managedprovisioning.task.InstallPackageTask.ERROR_PACKAGE_INVALID;

import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.UserHandle;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.model.ProvisioningParams;

import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.stubbing.Answer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;

public class InstallPackageTaskTest extends AndroidTestCase {
    private static final String TEST_PACKAGE_NAME = "com.android.test";
    private static final String OTHER_PACKAGE_NAME = "com.android.other";
    private static final ProvisioningParams TEST_PARAMS = new ProvisioningParams.Builder()
            .setDeviceAdminPackageName(TEST_PACKAGE_NAME)
            .setProvisioningAction(ACTION_PROVISION_MANAGED_DEVICE)
            .build();
    private static final int TEST_USER_ID = 123;
    private static final byte[] APK_CONTENT = new byte[]{'t', 'e', 's', 't'};
    private static final long TIMEOUT = 10000;

    private static int sSessionId = 0;

    @Mock private Context mMockContext;
    @Mock private PackageManager mPackageManager;
    @Mock private PackageInstaller mPackageInstaller;
    @Mock private PackageInstaller.Session mSession;
    @Mock private OutputStream mSessionWriteStream;
    @Mock private DevicePolicyManager mDpm;
    @Mock private AbstractProvisioningTask.Callback mCallback;
    @Mock private DownloadPackageTask mDownloadPackageTask;
    private InstallPackageTask mTask;
    private final SettingsFacade mSettingsFacade = new SettingsFacadeStub();
    private String mTestPackageLocation;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());
        MockitoAnnotations.initMocks(this);

        when(mMockContext.getPackageManager()).thenReturn(mPackageManager);
        when(mMockContext.getPackageName()).thenReturn(getContext().getPackageName());
        when(mPackageManager.getPackageInstaller()).thenReturn(mPackageInstaller);
        when(mPackageInstaller.createSession(any(PackageInstaller.SessionParams.class))).thenAnswer(
                (Answer<Integer>) invocation -> sSessionId++);
        when(mPackageInstaller.openSession(anyInt())).thenReturn(mSession);
        when(mSession.openWrite(anyString(), anyLong(), anyLong())).thenReturn(mSessionWriteStream);
        when(mMockContext.registerReceiver(any(BroadcastReceiver.class),
                any(IntentFilter.class))).thenAnswer(
                (Answer<Intent>) invocation -> (Intent) getContext().registerReceiver(
                        invocation.getArgument(0), invocation.getArgument(1)));
        when(mMockContext.getSystemServiceName(eq(DevicePolicyManager.class)))
                .thenReturn(Context.DEVICE_POLICY_SERVICE);
        when(mMockContext.getSystemService(eq(Context.DEVICE_POLICY_SERVICE))).thenReturn(mDpm);
        when(mMockContext.getUser()).thenReturn(Process.myUserHandle());
        when(mMockContext.getUserId()).thenReturn(UserHandle.myUserId());

        mTestPackageLocation = File.createTempFile("test", "apk").getPath();
        try (FileOutputStream out = new FileOutputStream(mTestPackageLocation)) {
            out.write(APK_CONTENT);
        }

        mTask = new InstallPackageTask(mSettingsFacade, mDownloadPackageTask, mMockContext,
                TEST_PARAMS, mCallback);
    }

    @SmallTest
    public void testNoDownloadLocation() {
        // GIVEN no package was downloaded
        when(mDownloadPackageTask.getDownloadedPackageLocation()).thenReturn(null);

        // WHEN running the InstallPackageTask without specifying an install location
        mTask.run(TEST_USER_ID);
        // THEN no package is installed, but we get a success callback
        verify(mPackageManager, never()).getPackageInstaller();
        verify(mCallback).onSuccess(mTask);
        verifyNoMoreInteractions(mCallback);
        assertTrue(mSettingsFacade.isPackageVerifierEnabled(mMockContext));
    }

    @SmallTest
    public void testSuccess() throws Exception {
        // GIVEN a package was downloaded to TEST_LOCATION
        when(mDownloadPackageTask.getDownloadedPackageLocation()).thenReturn(mTestPackageLocation);

        // WHEN running the InstallPackageTask specifying an install location
        mTask.run(TEST_USER_ID);

        // THEN package installed is invoked with an install observer
        IntentSender observer = verifyPackageInstalled(INSTALL_REPLACE_EXISTING);

        // WHEN the package installed callback is invoked with success
        Intent fillIn = new Intent();
        fillIn.putExtra(PackageInstaller.EXTRA_PACKAGE_NAME, TEST_PACKAGE_NAME);
        fillIn.putExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_SUCCESS);
        observer.sendIntent(getContext(), 0, fillIn, null, null);

        // THEN we receive a success callback
        verify(mCallback, timeout(TIMEOUT)).onSuccess(mTask);
        verifyNoMoreInteractions(mCallback);
        assertTrue(mSettingsFacade.isPackageVerifierEnabled(mMockContext));
    }

    @SmallTest
    public void testSuccess_allowTestOnly() throws Exception {
        // GIVEN a package was downloaded to TEST_LOCATION
        when(mDownloadPackageTask.getDownloadedPackageLocation()).thenReturn(mTestPackageLocation);
        // WHEN package to be installed is the current device owner.
        when(mDpm.isDeviceOwnerApp(eq(TEST_PACKAGE_NAME))).thenReturn(true);

        // WHEN running the InstallPackageTask specifying an install location
        mTask.run(TEST_USER_ID);

        // THEN package installed is invoked with an install observer
        IntentSender observer = verifyPackageInstalled(
                INSTALL_REPLACE_EXISTING | INSTALL_ALLOW_TEST);

        // WHEN the package installed callback is invoked with success
        Intent fillIn = new Intent();
        fillIn.putExtra(PackageInstaller.EXTRA_PACKAGE_NAME, TEST_PACKAGE_NAME);
        fillIn.putExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_SUCCESS);
        observer.sendIntent(getContext(), 0, fillIn, null, null);

        // THEN we receive a success callback
        verify(mCallback, timeout(TIMEOUT)).onSuccess(mTask);
        verifyNoMoreInteractions(mCallback);
        assertTrue(mSettingsFacade.isPackageVerifierEnabled(mMockContext));
    }


    @SmallTest
    public void testInstallFailedVersionDowngrade() throws Exception {
        // GIVEN a package was downloaded to TEST_LOCATION
        when(mDownloadPackageTask.getDownloadedPackageLocation()).thenReturn(mTestPackageLocation);

        // WHEN running the InstallPackageTask with a package already at a higher version
        mTask.run(TEST_USER_ID);

        // THEN package installed is invoked with an install observer
        IntentSender observer = verifyPackageInstalled(INSTALL_REPLACE_EXISTING);

        // WHEN the package installed callback is invoked with version downgrade error
        Intent fillIn = new Intent();
        fillIn.putExtra(PackageInstaller.EXTRA_PACKAGE_NAME, TEST_PACKAGE_NAME);
        fillIn.putExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
        fillIn.putExtra(PackageInstaller.EXTRA_LEGACY_STATUS,
                PackageManager.INSTALL_FAILED_VERSION_DOWNGRADE);
        observer.sendIntent(getContext(), 0, fillIn, null, null);

        // THEN we get a success callback, because an existing version of the DPC is present
        verify(mCallback, timeout(TIMEOUT)).onSuccess(mTask);
        verifyNoMoreInteractions(mCallback);
        assertTrue(mSettingsFacade.isPackageVerifierEnabled(mMockContext));
    }

    @SmallTest
    public void testInstallFailedOtherError() throws Exception {
        // GIVEN a package was downloaded to TEST_LOCATION
        when(mDownloadPackageTask.getDownloadedPackageLocation()).thenReturn(mTestPackageLocation);

        // WHEN running the InstallPackageTask with a package already at a higher version
        mTask.run(TEST_USER_ID);

        // THEN package installed is invoked with an install observer
        IntentSender observer = verifyPackageInstalled(INSTALL_REPLACE_EXISTING);

        // WHEN the package installed callback is invoked with version invalid apk error
        Intent fillIn = new Intent();
        fillIn.putExtra(PackageInstaller.EXTRA_PACKAGE_NAME, TEST_PACKAGE_NAME);
        fillIn.putExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
        fillIn.putExtra(PackageInstaller.EXTRA_LEGACY_STATUS,
                PackageManager.INSTALL_FAILED_INVALID_APK);
        observer.sendIntent(getContext(), 0, fillIn, null, null);

        // THEN we get a success callback, because an existing version of the DPC is present
        verify(mCallback, timeout(TIMEOUT)).onError(mTask, ERROR_INSTALLATION_FAILED);
        verifyNoMoreInteractions(mCallback);
        assertTrue(mSettingsFacade.isPackageVerifierEnabled(mMockContext));
    }

    @SmallTest
    public void testDifferentPackageName() throws Exception {
        // GIVEN a package was downloaded to TEST_LOCATION
        when(mDownloadPackageTask.getDownloadedPackageLocation()).thenReturn(mTestPackageLocation);

        // WHEN running the InstallPackageTask with a package already at a higher version
        mTask.run(TEST_USER_ID);

        // THEN package installed is invoked with an install observer
        IntentSender observer = verifyPackageInstalled(INSTALL_REPLACE_EXISTING);

        // WHEN the package installed callback is invoked with the wrong package
        Intent fillIn = new Intent();
        fillIn.putExtra(PackageInstaller.EXTRA_PACKAGE_NAME, OTHER_PACKAGE_NAME);
        observer.sendIntent(getContext(), 0, fillIn, null, null);

        // THEN we get a success callback, because the wrong package name
        verify(mCallback, timeout(TIMEOUT)).onError(mTask, ERROR_PACKAGE_INVALID);
        verifyNoMoreInteractions(mCallback);
        assertTrue(mSettingsFacade.isPackageVerifierEnabled(mContext));
    }

    private IntentSender verifyPackageInstalled(int installFlags) throws IOException {
        ArgumentCaptor<PackageInstaller.SessionParams> paramsCaptor
                = ArgumentCaptor.forClass(PackageInstaller.SessionParams.class);
        ArgumentCaptor<byte[]> fileContentCaptor = ArgumentCaptor.forClass(byte[].class);

        verify(mPackageInstaller).createSession(paramsCaptor.capture());
        assertEquals(installFlags, paramsCaptor.getValue().installFlags);
        verify(mSessionWriteStream).write(fileContentCaptor.capture(), eq(0),
                eq(APK_CONTENT.length));
        assertTrue(Arrays.equals(APK_CONTENT,
                Arrays.copyOf(fileContentCaptor.getValue(), APK_CONTENT.length)));

        ArgumentCaptor<IntentSender> intentSenderCaptor
                = ArgumentCaptor.forClass(IntentSender.class);

        // THEN package installation was started and we will receive a status callback
        verify(mSession).commit(intentSenderCaptor.capture());
        return intentSenderCaptor.getValue();
    }

    private static class SettingsFacadeStub extends SettingsFacade {
        private boolean mPackageVerifierEnabled = true;

        @Override
        public boolean isPackageVerifierEnabled(Context c) {
            return mPackageVerifierEnabled;
        }

        @Override
        public void setPackageVerifierEnabled(Context c, boolean packageVerifierEnabled) {
            mPackageVerifierEnabled = packageVerifierEnabled;
        }
    }
}
