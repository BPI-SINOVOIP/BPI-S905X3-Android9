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
package com.android.documentsui.inspector;

import static junit.framework.Assert.fail;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertTrue;

import android.app.Activity;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.support.annotation.Nullable;
import android.support.test.runner.AndroidJUnit4;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.View.OnClickListener;

import com.android.documentsui.InspectorProvider;
import com.android.documentsui.R;
import com.android.documentsui.TestProviderActivity;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.Shared;
import com.android.documentsui.inspector.InspectorController.ActionDisplay;
import com.android.documentsui.inspector.InspectorController.DataSupplier;
import com.android.documentsui.inspector.InspectorController.DebugDisplay;
import com.android.documentsui.inspector.InspectorController.DetailsDisplay;
import com.android.documentsui.inspector.InspectorController.HeaderDisplay;
import com.android.documentsui.inspector.InspectorController.MediaDisplay;
import com.android.documentsui.inspector.actions.Action;
import com.android.documentsui.testing.TestConsumer;
import com.android.documentsui.testing.TestEnv;
import com.android.documentsui.testing.TestLoaderManager;
import com.android.documentsui.testing.TestPackageManager;
import com.android.documentsui.testing.TestPackageManager.TestResolveInfo;
import com.android.documentsui.testing.TestProvidersAccess;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.function.Consumer;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class InspectorControllerTest  {

    private static final String OPEN_IN_PROVIDER_DOC = "OpenInProviderTest";

    private TestActivity mActivity;
    private TestLoaderManager mLoaderManager;
    private TestDataSupplier mDataSupplier;
    private TestPackageManager mPm;
    private InspectorController mController;
    private TestEnv mEnv;
    private TestHeader mHeaderTestDouble;
    private TestDetails mDetailsTestDouble;
    private TestMedia mMedia;
    private TestAction mShowInProvider;
    private TestAction mDefaultsTestDouble;
    private TestDebug mDebugTestDouble;
    private TestRunnable mErrCallback;
    private Bundle mTestArgs;

    @Before
    public void setUp() throws Exception {

        mEnv = TestEnv.create();
        mPm = TestPackageManager.create();
        mLoaderManager = new TestLoaderManager();
        mDataSupplier = new TestDataSupplier();
        mHeaderTestDouble = new TestHeader();
        mDetailsTestDouble = new TestDetails();
        mMedia = new TestMedia();
        mShowInProvider = new TestAction();
        mDefaultsTestDouble = new TestAction();
        mDebugTestDouble = new TestDebug();
        mErrCallback = new TestRunnable();
        mTestArgs = new Bundle();

        // Add some fake data.
        mDataSupplier.mDoc = TestEnv.FILE_JPG;
        mDataSupplier.mMetadata = new Bundle();
        TestMetadata.populateExifData(mDataSupplier.mMetadata);

        // Crashes if not called before "new TestActivity".
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
        mActivity = new TestActivity();
        recreateController();
    }

    private void recreateController() {
        mController = new InspectorController(
                mActivity,
                mDataSupplier,
                mPm,
                new TestProvidersAccess(),
                mHeaderTestDouble,
                mDetailsTestDouble,
                mMedia,
                mShowInProvider,
                mDefaultsTestDouble,
                mDebugTestDouble,
                mTestArgs,
                mErrCallback);
    }

    /**
     * Tests Debug view should be hidden and therefore not updated by default.
     */
    @Test
    public void testHideDebugByDefault() throws Exception {
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)
        mDebugTestDouble.assertVisible(false);
        mDebugTestDouble.assertEmpty();
    }

    /**
     * Tests Debug view should be updated when visible.
     */
    @Test
    public void testShowDebugUpdatesView() throws Exception {
        mTestArgs.putBoolean(Shared.EXTRA_SHOW_DEBUG, true);
        recreateController();
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)
        mDebugTestDouble.assertVisible(true);
        mDebugTestDouble.assertNotEmpty();
    }

    /**
     * Tests Debug view should be updated when visible.
     */
    @Test
    public void testExtraTitleOverridesDisplayName() throws Exception {
        mTestArgs.putString(Intent.EXTRA_TITLE, "hammy!");
        recreateController();
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)
        mHeaderTestDouble.assertTitle("hammy!");
    }

    /**
     * Tests show in provider feature of the controller. This test loads a documentInfo from a uri.
     *  calls showInProvider on the documentInfo and verifies that the TestProvider activity has
     *  started.
     *
     *  @see InspectorProvider
     *  @see TestProviderActivity
     *
     * @throws Exception
     */
    @Test
    public void testShowInProvider() throws Exception {

        Uri uri = DocumentsContract.buildDocumentUri(InspectorProvider.AUTHORITY,
            OPEN_IN_PROVIDER_DOC);
        mController.showInProvider(uri);

        assertNotNull(mActivity.started);
        assertEquals("com.android.documentsui", mActivity.started.getPackage());
        assertEquals(uri, mActivity.started.getData());
    }
    /**
     * Test that valid input will update the view properly. The test uses a test double for header
     * and details view and asserts that .accept was called on both.
     *
     * @throws Exception
     */
    @Test
    public void testUpdateViewWithValidInput() throws Exception {
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)
        mHeaderTestDouble.assertCalled();
        mDetailsTestDouble.assertCalled();
    }

    /**
     * Test that when a document has the FLAG_SUPPORTS_SETTINGS set the showInProvider view will
     * be visible.
     *
     * @throws Exception
     */
    @Test
    public void testShowInProvider_visible() throws Exception {
        DocumentInfo doc = new DocumentInfo();
        doc.derivedUri =
            DocumentsContract.buildDocumentUri(InspectorProvider.AUTHORITY, OPEN_IN_PROVIDER_DOC);
        doc.flags = doc.flags | Document.FLAG_SUPPORTS_SETTINGS;
        mDataSupplier.mDoc = doc;
        mController.loadInfo(doc.derivedUri);  // actual URI doesn't matter :)
        assertTrue(mShowInProvider.becameVisible);
    }

    /**
     * Test that when a document does not have the FLAG_SUPPORTS_SETTINGS set the view will be
     * invisible.
     * @throws Exception
     */
    @Test
    public void testShowInProvider_invisible() throws Exception {
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)
        assertFalse(mShowInProvider.becameVisible);
    }

    /**
     * Test that the action clear app defaults is visible when conditions are met.
     * @throws Exception
     */
    @Test
    public void testAppDefaults_visible() throws Exception {
        mPm.queryIntentProvidersResults = new ArrayList<>();
        mPm.queryIntentProvidersResults.add(new TestResolveInfo());
        mPm.queryIntentProvidersResults.add(new TestResolveInfo());
        DocumentInfo doc = new DocumentInfo();
        doc.derivedUri =
            DocumentsContract.buildDocumentUri(InspectorProvider.AUTHORITY, OPEN_IN_PROVIDER_DOC);

        mDataSupplier.mDoc = doc;
        mController.loadInfo(doc.derivedUri);  // actual URI doesn't matter :)
        assertTrue(mDefaultsTestDouble.becameVisible);
    }

    /**
     * Test that action clear app defaults is invisible when conditions have not been met.
     * @throws Exception
     */
    @Test
    public void testAppDefaults_invisible() throws Exception {
        mPm.queryIntentProvidersResults = new ArrayList<>();
        mPm.queryIntentProvidersResults.add(new TestResolveInfo());
        DocumentInfo doc = new DocumentInfo();
        doc.derivedUri =
            DocumentsContract.buildDocumentUri(InspectorProvider.AUTHORITY, OPEN_IN_PROVIDER_DOC);

        mDataSupplier.mDoc = doc;
        mController.loadInfo(doc.derivedUri);  // actual URI doesn't matter :)
        assertFalse(mDefaultsTestDouble.becameVisible);
    }

    /**
     * Test that update view will handle a null value properly. It uses a runnable to verify that
     * the static method Snackbars.showInspectorError(Activity activity) is called.
     *
     * @throws Exception
     */
    @Test
    public void testUpdateView_withNullValue() throws Exception {
        mDataSupplier.mDoc = null;
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)

        mErrCallback.assertCalled();
        mHeaderTestDouble.assertNotCalled();
        mDetailsTestDouble.assertNotCalled();
    }

    @Test
    public void testMetadata_GeoHandlerInstalled() {
        DocumentInfo doc = TestEnv.clone(TestEnv.FILE_JPG);
        doc.flags |= DocumentsContract.Document.FLAG_SUPPORTS_METADATA;
        mDataSupplier.mDoc = doc;
        mController.loadInfo(doc.derivedUri);  // actual URI doesn't matter :)
        mMedia.assertGeoCallbackInstalled();
    }

    @Test
    public void testMetadata_notDisplayedWhenNotReturned() {
        DocumentInfo doc = TestEnv.clone(TestEnv.FILE_JPG);
        doc.flags |= DocumentsContract.Document.FLAG_SUPPORTS_METADATA;
        mDataSupplier.mDoc = doc;
        mDataSupplier.mMetadata = null;  // sorry, no results sucka!
        mController.loadInfo(doc.derivedUri);  // actual URI doesn't matter :)
    }

    @Test
    public void testMetadata_notDisplayedDocWithoutSupportFlag() {
        assert !TestEnv.FILE_JPG.isMetadataSupported();
        mDataSupplier.mDoc = TestEnv.FILE_JPG;  // this is the default value. For "good measure".
        mController.loadInfo(TestEnv.FILE_JPG.derivedUri);  // actual URI doesn't matter :)
        mMedia.assertVisible(false);
        mMedia.assertEmpty();
    }

    @Test
    public void testMetadata_GeoHandlerStartsAction() {
        DocumentInfo doc = TestEnv.clone(TestEnv.FILE_JPG);
        doc.flags |= DocumentsContract.Document.FLAG_SUPPORTS_METADATA;
        mDataSupplier.mDoc = doc;
        mController.loadInfo(doc.derivedUri);  // actual URI doesn't matter :)
        mMedia.mGeoClickCallback.run();
        Intent geoIntent = mActivity.started;
        assertEquals(Intent.ACTION_VIEW, geoIntent.getAction());
        assertNotNull(geoIntent);
        Uri uri = geoIntent.getData();
        assertEquals("geo", uri.getScheme());
        String strUri = uri.toSafeString();
        assertTrue(strUri.contains("33."));
        assertTrue(strUri.contains("-118."));
        assertTrue(strUri.contains(TestEnv.FILE_JPG.displayName));
    }

    private static class TestActivity extends Activity {

        private @Nullable Intent started;

        @Override
        public void startActivity(Intent intent) {
            started = intent;
        }
    }

    private static class TestAction implements ActionDisplay {

        private TestAction() {
            becameVisible = false;
        }

        private boolean becameVisible;

        @Override
        public void init(Action action, OnClickListener listener) {

        }

        @Override
        public void setVisible(boolean visible) {
            becameVisible = visible;
        }

        @Override
        public void setActionHeader(String header) {

        }

        @Override
        public void setAppIcon(Drawable icon) {

        }

        @Override
        public void setAppName(String name) {

        }

        @Override
        public void showAction(boolean visible) {

        }
    }


    private static class TestHeader implements HeaderDisplay {

        private boolean mCalled = false;
        private @Nullable String mTitle;

        @Override
        public void accept(DocumentInfo info, String displayName) {
            mCalled = true;
            mTitle = displayName;
        }

        public void assertTitle(String expected) {
            Assert.assertEquals(expected, mTitle);
        }

        public void assertCalled() {
            Assert.assertTrue(mCalled);
        }

        public void assertNotCalled() {
            Assert.assertFalse(mCalled);
        }
    }

    private static class TestDetails implements DetailsDisplay {

        private boolean mCalled = false;

        @Override
        public void accept(DocumentInfo info) {
            mCalled = true;
        }

        @Override
        public void setChildrenCount(int count) {
        }

        public void assertCalled() {
            Assert.assertTrue(mCalled);
        }

        public void assertNotCalled() {
            Assert.assertFalse(mCalled);
        }
    }

    private static class TestMedia extends TestTable implements MediaDisplay {

        private @Nullable Runnable mGeoClickCallback;

        @Override
        public void accept(DocumentInfo info, Bundle metadata, Runnable geoClickCallback) {
            mGeoClickCallback = geoClickCallback;
        }

        void assertGeoCallbackInstalled() {
            assertNotNull(mGeoClickCallback);
        }
    }

    private static class TestDebug extends TestTable implements DebugDisplay {

        @Override
        public void accept(DocumentInfo info) {
            // We have to emulate some of the real DebugView behavior
            // because the controller makes us visible if we're not-empty.
            put(R.string.debug_content_uri, info.derivedUri.toString());
        }

        @Override
        public void accept(Bundle metadata) {
        }
    }

    private static final class TestDataSupplier implements DataSupplier {

        private @Nullable DocumentInfo mDoc;
        private @Nullable Bundle mMetadata;

        @Override
        public void loadDocInfo(Uri uri, Consumer<DocumentInfo> callback) {
            callback.accept(mDoc);
        }

        @Override
        public void getDocumentMetadata(Uri uri, Consumer<Bundle> callback) {
            callback.accept(mMetadata);
        }

        @Override
        public void loadDirCount(DocumentInfo directory, Consumer<Integer> callback) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void reset() {
            throw new UnsupportedOperationException();
        }
    }

    private static final class TestRunnable implements Runnable {
        private boolean mCalled;

        @Override
        public void run() {
            mCalled = true;
        }

        void assertCalled() {
            assertTrue(mCalled);
        }
    }
}