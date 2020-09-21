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
package com.android.documentsui.testing;

import android.content.Context;
import android.provider.DocumentsContract.Document;
import android.support.test.InstrumentationRegistry;
import android.test.mock.MockContentResolver;

import com.android.documentsui.DocsSelectionHelper;
import com.android.documentsui.FocusManager;
import com.android.documentsui.Injector;
import com.android.documentsui.archives.ArchivesProvider;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.Features;
import com.android.documentsui.base.RootInfo;
import com.android.documentsui.base.State;
import com.android.documentsui.dirlist.TestFocusHandler;
import com.android.documentsui.sorting.SortModel;
import com.android.documentsui.ui.TestDialogController;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executor;

public class TestEnv {

    public static DocumentInfo FOLDER_0;
    public static DocumentInfo FOLDER_1;
    public static DocumentInfo FOLDER_2;
    public static DocumentInfo FILE_TXT;
    public static DocumentInfo FILE_PNG;
    public static DocumentInfo FILE_JPG;
    public static DocumentInfo FILE_GIF;
    public static DocumentInfo FILE_PDF;
    public static DocumentInfo FILE_MP4;
    public static DocumentInfo FILE_APK;
    public static DocumentInfo FILE_PARTIAL;
    public static DocumentInfo FILE_ARCHIVE;
    public static DocumentInfo FILE_IN_ARCHIVE;
    public static DocumentInfo FILE_VIRTUAL;
    public static DocumentInfo FILE_READ_ONLY;

    public final TestScheduledExecutorService mExecutor;
    public final State state = new State();
    public final TestProvidersAccess providers = new TestProvidersAccess();
    public final TestDocumentsAccess docs = new TestDocumentsAccess();
    public final TestFocusHandler focusHandler = new TestFocusHandler();
    public final TestDialogController dialogs = new TestDialogController();
    public final TestModel model;
    public final TestModel archiveModel;
    public final DocsSelectionHelper selectionMgr;
    public final TestSearchViewManager searchViewManager;
    public final Injector<?> injector;
    public final Features features;

    public final MockContentResolver contentResolver;
    public final Map<String, TestDocumentsProvider> mockProviders;

    private TestEnv(Context context, Features features, String authority) {
        this.features = features;
        state.sortModel = SortModel.createModel();
        mExecutor = new TestScheduledExecutorService();
        model = new TestModel(authority, features);
        archiveModel = new TestModel(ArchivesProvider.AUTHORITY, features);
        selectionMgr = SelectionHelpers.createTestInstance();
        searchViewManager = new TestSearchViewManager();
        injector = new Injector(
                features,
                new TestActivityConfig(),
                null,       // ScopedPreferences are not currently required for tests
                null,       // MessageBuilder is not currently required for tests
                dialogs,
                new TestFileTypeLookup(),
                (roots) -> {},  // not sure why, but java gets angry when I declare roots type.
                model);

        injector.selectionMgr = selectionMgr;
        injector.focusManager = new FocusManager(features, selectionMgr, null, null, 0);
        injector.searchManager = searchViewManager;

        contentResolver = new MockContentResolver();
        mockProviders = new HashMap<>(providers.getRootsBlocking().size());
        registerProviders();
    }

    private void registerProviders() {
        for (RootInfo root : providers.getRootsBlocking()) {
            if (!mockProviders.containsKey(root.authority)) {
                TestDocumentsProvider provider = new TestDocumentsProvider(root.authority);
                contentResolver.addProvider(root.authority, provider);
                mockProviders.put(root.authority, provider);
            }
        }
    }

    // Many terrible creational permutations == easy to user for test authors!
    public static TestEnv create(Features features) {
        return create(features, TestProvidersAccess.HOME.authority);
    }

    public static TestEnv create() {
        return create(TestProvidersAccess.HOME.authority);
    }

    public static TestEnv create(Features features, String authority) {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        return create(context, features, authority);
    }

    public static TestEnv create(String authority) {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        Features features = new Features.RuntimeFeatures(context.getResources(), null);
        return create(context, features, authority);
    }

    private static TestEnv create(Context context, Features features, String authority) {
        TestEnv env = new TestEnv(context, features, authority);
        env.reset();
        return env;
    }

    public void clear() {
        model.reset();
        model.update();
    }

    public void reset() {
        model.reset();
        FOLDER_0 = model.createFolder("folder 0");
        FOLDER_1 = model.createFolder("folder 1");
        FOLDER_2 = model.createFolder("folder 2");
        FILE_TXT = model.createFile("woowoo.txt");
        FILE_PNG = model.createFile("peppey.png");
        FILE_JPG = model.createFile("jiffy.jpg");
        FILE_GIF = model.createFile("glibby.gif");
        FILE_PDF = model.createFile("busy.pdf");
        FILE_MP4 = model.createFile("cameravideo.mp4");
        FILE_APK = model.createFile("becareful.apk");
        FILE_PARTIAL = model.createFile(
                "UbuntuFlappyBird.iso",
                Document.FLAG_SUPPORTS_DELETE
                        | Document.FLAG_PARTIAL);
        FILE_READ_ONLY = model.createFile("topsecretsystemfile.bin", 0);
        FILE_ARCHIVE = model.createFile("whatsinthere.zip");
        FILE_IN_ARCHIVE = archiveModel.createFile("whatsinthere.png", 0);
        FILE_VIRTUAL = model.createDocument(
                "virtualdoc.vnd",
                "application/vnd.google-apps.document",
                Document.FLAG_VIRTUAL_DOCUMENT
                        | Document.FLAG_SUPPORTS_DELETE
                        | Document.FLAG_SUPPORTS_RENAME);

        archiveModel.update();
        model.update();
    }

    public void populateStack() {
        DocumentInfo rootDoc = model.getDocument("1");

        // These are test setup sanity checks, not test assertions.
        assert rootDoc != null;
        assert rootDoc.isDirectory();
        assert FOLDER_0.equals(rootDoc);

        state.stack.changeRoot(TestProvidersAccess.HOME);
        state.stack.push(rootDoc);
    }

    public void beforeAsserts() throws Exception {
        mExecutor.waitForTasks(30000); // 30 secs
    }

    public Executor lookupExecutor(String authority) {
        return mExecutor;
    }

    public void selectDocument(DocumentInfo info) {
        List<String> ids = new ArrayList<>(1);
        ids.add(info.documentId);
        selectionMgr.setItemsSelected(ids, true);
    }

    // Easily copy docs, so we don't pollute static data across tests.
    public static DocumentInfo clone(DocumentInfo a) {
        DocumentInfo b = new DocumentInfo();
        b.authority = a.authority;
        b.documentId = a.documentId;
        b.mimeType = a.mimeType;
        b.displayName = a.displayName;
        b.lastModified = a.lastModified;
        b.flags = a.flags;
        b.summary = a.summary;
        b.size = a.size;
        b.icon = a.icon;
        b.derivedUri = a.derivedUri;

        return b;
    }
}
