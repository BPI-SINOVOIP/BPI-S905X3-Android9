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

package com.android.documentsui;

import static com.android.documentsui.StubProvider.EXTRA_SIZE;
import static com.android.documentsui.StubProvider.ROOT_0_ID;
import static com.android.documentsui.StubProvider.ROOT_1_ID;

import static android.provider.DocumentsContract.buildDocumentUri;
import android.provider.DocumentsContract;
import com.android.documentsui.archives.ResourcesProvider;

import android.content.ComponentName;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.net.Uri;
import android.os.Bundle;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.test.filters.LargeTest;
import android.support.test.filters.Suppress;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.util.Log;

import com.android.documentsui.files.FilesActivity;
import com.android.documentsui.services.TestNotificationService;
import com.android.documentsui.R;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
* This class tests the below points.
* - Cancel copying or moving file before starting it.
* - Cancel during copying or moving file.
*/
@LargeTest
public class CancelFromNotificationUiTest extends ActivityTest<FilesActivity> {
    private static final String PACKAGE_NAME = "com.android.documentsui.tests";

    private static final String TARGET_FILE = "dummy.data";

    private static final int BUFFER_SIZE = 10 * 1024 * 1024;

    private static final String ACCESS_APP_NAME = "DocumentsUI Tests";

    private static final String ALLOW = "ALLOW";

    private static final String TURN_OFF = "TURN OFF";

    private static final String COPY = "Copy to…";

    private static final String MOVE = "Move to…";

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (TestNotificationService.ACTION_OPERATION_RESULT.equals(action)) {
                mOperationExecuted = intent.getBooleanExtra(
                        TestNotificationService.EXTRA_RESULT, false);
                if (!mOperationExecuted) {
                    mErrorReason = intent.getStringExtra(
                            TestNotificationService.EXTRA_ERROR_REASON);
                }
                mCountDownLatch.countDown();
            }
        }
    };

    private CountDownLatch mCountDownLatch;

    private boolean mOperationExecuted;

    private String mErrorReason;

    public CancelFromNotificationUiTest() {
        super(FilesActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        // super.setUp() method will change the storage size to 100MB.
        // So, reset the storage size again to 500MB.
        Bundle bundle = new Bundle();
        bundle.putLong(EXTRA_SIZE, 500L);
        mDocsHelper.configure(null, bundle);

        initTestFiles();

        IntentFilter filter = new IntentFilter();
        filter.addAction(TestNotificationService.ACTION_OPERATION_RESULT);
        context.registerReceiver(mReceiver, filter);
        context.sendBroadcast(new Intent(
                TestNotificationService.ACTION_CHANGE_CANCEL_MODE));

        mOperationExecuted = false;
        mErrorReason = "No response from Notification";
        mCountDownLatch = new CountDownLatch(1);
    }

    @Override
    public void tearDown() throws Exception {
        mCountDownLatch.countDown();
        mCountDownLatch = null;

        context.unregisterReceiver(mReceiver);
        try {
            if (isEnableAccessNotification()) {
                disallowNotificationAccess();
            }
        } catch (Exception e) {
            // ignore
        }
        super.tearDown();
    }

    @Override
    public void initTestFiles() throws RemoteException {
        try {
            if (!isEnableAccessNotification()) {
               allowNotificationAccess();
            }
            createDummyFile();
        } catch (Exception e) {
            fail("Initialization failed. " + e.toString());
        }
    }

    private void createDummyFile() throws Exception {
        Uri uri = mDocsHelper.createDocument(rootDir0, "*/*", TARGET_FILE);
        byte[] dummyByte = new byte[BUFFER_SIZE];
        mDocsHelper.writeDocument(uri, dummyByte);
        for (int i = 0; i < 49; i++) {
            dummyByte = null;
            dummyByte = new byte[BUFFER_SIZE];
            mDocsHelper.writeAppendDocument(uri, dummyByte);
        }
    }

    private void allowNotificationAccess() throws Exception {
        Intent intent = new Intent();
        intent.setAction(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS);
        getActivity().startActivity(intent);
        device.waitForIdle();

        bots.main.findMenuLabelWithName(ACCESS_APP_NAME).click();
        device.waitForIdle();

        bots.main.findMenuLabelWithName(ALLOW).click();
        bots.keyboard.pressKey(KeyEvent.KEYCODE_BACK);
    }

    private void disallowNotificationAccess() throws Exception {
        Intent intent = new Intent();
        intent.setAction(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS);
        getActivity().startActivity(intent);
        device.waitForIdle();

        bots.main.findMenuLabelWithName(ACCESS_APP_NAME).click();
        device.waitForIdle();

        bots.main.findMenuLabelWithName(TURN_OFF).click();
        bots.keyboard.pressKey(KeyEvent.KEYCODE_BACK);
    }

    private boolean isEnableAccessNotification() {
        ContentResolver resolver = getActivity().getContentResolver();
        String listeners = Settings.Secure.getString(resolver,"enabled_notification_listeners");

        if (!TextUtils.isEmpty(listeners)) {
            String[] list = listeners.split(":");
            for(String item : list) {
                if(item.startsWith(PACKAGE_NAME)) {
                    return true;
                }
            }
        }
        return false;
    }

    public void testCopyDocument_Cancel() throws Exception {
        bots.roots.openRoot(ROOT_0_ID);

        bots.directory.findDocument(TARGET_FILE);
        device.waitForIdle();

        bots.directory.selectDocument(TARGET_FILE);
        device.waitForIdle();

        bots.main.clickToolbarOverflowItem(COPY);
        device.waitForIdle();

        bots.main.clickDialogCancelButton();
        device.waitForIdle();

        bots.directory.waitForDocument(TARGET_FILE);
    }

    public void testCopyDocument_CancelFromNotification() throws Exception {
        bots.roots.openRoot(ROOT_0_ID);
        bots.directory.findDocument(TARGET_FILE);
        device.waitForIdle();

        bots.directory.selectDocument(TARGET_FILE);
        device.waitForIdle();

        bots.main.clickToolbarOverflowItem(COPY);
        device.waitForIdle();

        bots.roots.openRoot(ROOT_1_ID);
        bots.main.clickDialogOkButton();
        device.waitForIdle();

        try {
            mCountDownLatch.await(60, TimeUnit.SECONDS);
        } catch (Exception e) {
            fail("Cannot wait because of error." + e.toString());
        }

        assertTrue(mErrorReason, mOperationExecuted);

        bots.roots.openRoot(ROOT_1_ID);
        device.waitForIdle();
        assertFalse(bots.directory.hasDocuments(TARGET_FILE));

        bots.roots.openRoot(ROOT_0_ID);
        device.waitForIdle();
        assertTrue(bots.directory.hasDocuments(TARGET_FILE));
    }

    public void testMoveDocument_Cancel() throws Exception {
        bots.roots.openRoot(ROOT_0_ID);

        bots.directory.findDocument(TARGET_FILE);
        device.waitForIdle();

        bots.directory.selectDocument(TARGET_FILE);
        device.waitForIdle();

        bots.main.clickToolbarOverflowItem(MOVE);
        device.waitForIdle();

        bots.main.clickDialogCancelButton();
        device.waitForIdle();

        bots.directory.waitForDocument(TARGET_FILE);
    }

    public void testMoveDocument_CancelFromNotification() throws Exception {
        bots.roots.openRoot(ROOT_0_ID);
        bots.directory.findDocument(TARGET_FILE);
        device.waitForIdle();

        bots.directory.selectDocument(TARGET_FILE);
        device.waitForIdle();

        bots.main.clickToolbarOverflowItem(MOVE);
        device.waitForIdle();

        bots.roots.openRoot(ROOT_1_ID);
        bots.main.clickDialogOkButton();
        device.waitForIdle();

        try {
            mCountDownLatch.await(60, TimeUnit.SECONDS);
        } catch (Exception e) {
            fail("Cannot wait because of error." + e.toString());
        }

        assertTrue(mErrorReason, mOperationExecuted);

        bots.roots.openRoot(ROOT_1_ID);
        device.waitForIdle();
        assertFalse(bots.directory.hasDocuments(TARGET_FILE));

        bots.roots.openRoot(ROOT_0_ID);
        device.waitForIdle();
        assertTrue(bots.directory.hasDocuments(TARGET_FILE));
    }
}
