/*
 * Copyright (C) 2016 The Android Open Source Project
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

import android.app.Activity;
import android.app.UiAutomation;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.RemoteException;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.support.test.uiautomator.Configurator;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.test.ActivityInstrumentationTestCase2;
import android.view.MotionEvent;

import com.android.documentsui.base.Features;
import com.android.documentsui.base.RootInfo;
import com.android.documentsui.bots.Bots;
import com.android.documentsui.bots.UiBot;

import javax.annotation.Nullable;

/**
 * Provides basic test environment for UI tests:
 * - Launches activity
 * - Creates and gives access to test root directories and test files
 * - Cleans up the test environment
 */
public abstract class ActivityTest<T extends Activity> extends ActivityInstrumentationTestCase2<T> {

    static final int TIMEOUT = 5000;

    // Testing files. For custom ones, override initTestFiles().
    public static final String dirName1 = "Dir1";
    public static final String childDir1 = "ChildDir1";
    public static final String fileName1 = "file1.log";
    public static final String fileName2 = "file12.png";
    public static final String fileName3 = "anotherFile0.log";
    public static final String fileName4 = "poodles.text";
    public static final String fileNameNoRename = "NO_RENAMEfile.txt";

    public Bots bots;
    public UiDevice device;
    public Context context;
    public UiAutomation automation;

    public Features features;
    public RootInfo rootDir0;
    public RootInfo rootDir1;
    protected ContentResolver mResolver;
    protected DocumentsProviderHelper mDocsHelper;
    protected ContentProviderClient mClient;

    public ActivityTest(Class<T> activityClass) {
        super(activityClass);
    }

    /*
     * Returns the root that will be opened within the activity.
     * By default tests are started with one of the test roots.
     * Override the method if you want to open different root on start.
     * @return Root that will be opened. Return null if you want to open activity's default root.
     */
    protected @Nullable RootInfo getInitialRoot() {
        return rootDir0;
    }

    /**
     * Returns the authority of the testing provider begin used.
     * By default it's StubProvider's authority.
     * @return Authority of the provider.
     */
    protected String getTestingProviderAuthority() {
        return StubProvider.DEFAULT_AUTHORITY;
    }

    /**
     * Resolves testing roots.
     */
    protected void setupTestingRoots() throws RemoteException {
        rootDir0 = mDocsHelper.getRoot(StubProvider.ROOT_0_ID);
        rootDir1 = mDocsHelper.getRoot(StubProvider.ROOT_1_ID);
    }

    @Override
    public void setUp() throws Exception {
        device = UiDevice.getInstance(getInstrumentation());
        // NOTE: Must be the "target" context, else security checks in content provider will fail.
        context = getInstrumentation().getTargetContext();
        automation = getInstrumentation().getUiAutomation();
        features = new Features.RuntimeFeatures(context.getResources(), null);

        bots = new Bots(device, automation, context, TIMEOUT);

        Configurator.getInstance().setToolType(MotionEvent.TOOL_TYPE_MOUSE);

        mResolver = context.getContentResolver();
        mClient = mResolver.acquireUnstableContentProviderClient(getTestingProviderAuthority());
        mDocsHelper = new DocumentsProviderHelper(getTestingProviderAuthority(), mClient);

        device.setOrientationNatural();
        setupTestingRoots();

        launchActivity();
        resetStorage();

        // Since at the launch of activity, ROOT_0 and ROOT_1 have no files, drawer will
        // automatically open for phone devices. Espresso register click() as (x, y) MotionEvents,
        // so if a drawer is on top of a file we want to select, it will actually click the drawer.
        // Thus to start a clean state, we always try to close first.
        bots.roots.closeDrawer();

        // Configure the provider back to default.
        mDocsHelper.configure(null, Bundle.EMPTY);
    }

    @Override
    public void tearDown() throws Exception {
        device.unfreezeRotation();
        mClient.release();
        super.tearDown();
    }

    protected void launchActivity() {
        final Intent intent = context.getPackageManager().getLaunchIntentForPackage(
                UiBot.TARGET_PKG);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK | Intent.FLAG_ACTIVITY_NEW_TASK);
        if (getInitialRoot() != null) {
            intent.setAction(Intent.ACTION_VIEW);
            intent.setDataAndType(getInitialRoot().getUri(), DocumentsContract.Root.MIME_TYPE_ITEM);
        }
        setActivityIntent(intent);
        getActivity();  // Launch the activity.
    }

    protected void resetStorage() throws RemoteException {
        mClient.call("clear", null, null);
        device.waitForIdle();
    }

    protected void initTestFiles() throws RemoteException {
        mDocsHelper.createFolder(rootDir0, dirName1);
        mDocsHelper.createDocument(rootDir0, "text/plain", fileName1);
        mDocsHelper.createDocument(rootDir0, "image/png", fileName2);
        mDocsHelper.createDocumentWithFlags(rootDir0.documentId, "text/plain", fileNameNoRename,
                Document.FLAG_SUPPORTS_WRITE);

        mDocsHelper.createDocument(rootDir1, "text/plain", fileName3);
        mDocsHelper.createDocument(rootDir1, "text/plain", fileName4);
    }

    void assertDefaultContentOfTestDir0() throws UiObjectNotFoundException {
        bots.directory.waitForDocument(fileName1);
        bots.directory.waitForDocument(fileName2);
        bots.directory.waitForDocument(dirName1);
        bots.directory.waitForDocument(fileNameNoRename);
        bots.directory.assertDocumentsCount(4);
    }

    void assertDefaultContentOfTestDir1() throws UiObjectNotFoundException {
        bots.directory.waitForDocument(fileName3);
        bots.directory.waitForDocument(fileName4);
        bots.directory.assertDocumentsCount(2);
    }
}
