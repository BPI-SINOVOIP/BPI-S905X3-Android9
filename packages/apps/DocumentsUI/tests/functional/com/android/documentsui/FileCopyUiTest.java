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

import static com.android.documentsui.StubProvider.ROOT_0_ID;
import static com.android.documentsui.StubProvider.ROOT_1_ID;

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
import android.support.test.uiautomator.Configurator;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.files.FilesActivity;
import com.android.documentsui.services.TestNotificationService;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.List;
import java.util.ArrayList;

/**
* This class test the below points
* - Copy large number of files
*/
@LargeTest
public class FileCopyUiTest extends ActivityTest<FilesActivity> {
    private static final String PACKAGE_NAME = "com.android.documentsui.tests";

    private static final String ACCESS_APP_NAME = "DocumentsUI Tests";

    private static final String ALLOW = "ALLOW";

    private static final String TURN_OFF = "TURN OFF";

    private static final String COPY = "Copy to…";

    private static final String MOVE = "Move to…";

    private static final String SELECT_ALL = "Select all";

    private static final int DUMMY_FILE_COUNT = 1000;

    private final List<String> mCopyFileList = new ArrayList<String>();

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

    public FileCopyUiTest() {
        super(FilesActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        // Set a flag to prevent many refreshes.
        Bundle bundle = new Bundle();
        bundle.putBoolean(StubProvider.EXTRA_ENABLE_ROOT_NOTIFICATION, false);
        mDocsHelper.configure(null, bundle);

        initTestFiles();

        IntentFilter filter = new IntentFilter();
        filter.addAction(TestNotificationService.ACTION_OPERATION_RESULT);
        context.registerReceiver(mReceiver, filter);
        context.sendBroadcast(new Intent(
                TestNotificationService.ACTION_CHANGE_EXECUTION_MODE));

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
            createDummyFiles();
        } catch (Exception e) {
            fail("Initialization failed");
        }
    }

    private void createDummyFiles() throws Exception {
        final ThreadPoolExecutor exec = new ThreadPoolExecutor(
                5, 5, 1000L, TimeUnit.MILLISECONDS,
                        new ArrayBlockingQueue<Runnable>(100, true));
        for (int i = 0; i < DUMMY_FILE_COUNT; i++) {
            final String fileName = "file" + String.format("%04d", i) + ".log";
            if (exec.getQueue().size() >= 80) {
                Thread.sleep(50);
            }
            exec.submit(new Runnable() {
                @Override
                public void run() {
                    Uri uri = mDocsHelper.createDocument(rootDir0, "text/plain", fileName);
                    try {
                        mDocsHelper.writeDocument(uri, new byte[1]);
                    } catch (Exception e) {
                        // ignore
                    }
                }
            });
            mCopyFileList.add(fileName);
        }
        exec.shutdown();
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
        String listeners = Settings.Secure.getString(
                resolver,"enabled_notification_listeners");
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

    public void testCopyAllDocument() throws Exception {
        bots.roots.openRoot(ROOT_0_ID);
        bots.main.clickToolbarOverflowItem(SELECT_ALL);
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

        List<DocumentInfo> root1 = mDocsHelper.listChildren(rootDir1.documentId, 1000);
        List<String> copiedFileList = new ArrayList<String>();
        for (DocumentInfo info : root1) {
            copiedFileList.add(info.displayName);
        }

        for (String name : mCopyFileList) {
            assertTrue("Not found " + name, copiedFileList.contains(name));
        }
    }
}
