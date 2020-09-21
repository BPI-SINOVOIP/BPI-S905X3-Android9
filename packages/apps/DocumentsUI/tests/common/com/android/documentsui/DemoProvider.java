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

import android.app.AuthenticationRequiredException;
import android.app.PendingIntent;
import android.content.Intent;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.os.Bundle;
import android.provider.DocumentsContract;

import java.io.FileNotFoundException;

/**
 * Provides data view that exercises some of the more esoteric functionality...like display of INFO
 * and ERROR messages.
 * <p>
 * Do not use this provider for automated testing.
 */
public class DemoProvider extends TestRootProvider {

    public static final String DIR_INFO = "show info";
    public static final String DIR_ERROR = "show error";
    public static final String DIR_ERROR_AND_INFO = "show both error and info";
    public static final String DIR_THROW = "throw a nice exception";
    public static final String DIR_AUTH = "throw a authentication exception";

    public static final String MSG_INFO = "All files in this root support settings.";
    public static final String MSG_ERROR = "I'm an error. Don't judge me.";
    public static final String MSG_ERROR_AND_INFO = "ERROR: Both ERROR and INFO returned.";

    private static final String ROOT_ID = "demo-root";
    private static final String ROOT_DOC_ID = "root0";

    public DemoProvider() {
        super("Demo Root", ROOT_ID, 0, ROOT_DOC_ID);
    }

    @Override
    public Cursor queryDocument(String documentId, String[] projection)
            throws FileNotFoundException {
        MatrixCursor c = createDocCursor(projection);
        Bundle extras = c.getExtras();
        extras.putString(
                DocumentsContract.EXTRA_INFO,
                "This provider is for feature demos only. Do not use from automated tests.");
        addFolder(c, documentId);
        return c;
    }

    @Override
    public Cursor queryChildDocuments(
            String parentDocumentId, String[] projection, String sortOrder)
            throws FileNotFoundException {
        MatrixCursor c = createDocCursor(projection);
        Bundle extras = c.getExtras();

        switch (parentDocumentId) {
            case DIR_INFO:
                extras.putString(DocumentsContract.EXTRA_INFO, MSG_INFO);
                addFolder(c, "folder");
                addFile(c, "zzz");
                for (int i = 0; i < 100; i++) {
                    addFile(c, "" + i, DocumentsContract.Document.FLAG_SUPPORTS_SETTINGS);
                }
                break;

            case DIR_ERROR:
                extras.putString(DocumentsContract.EXTRA_ERROR, MSG_ERROR);
                break;

            case DIR_ERROR_AND_INFO:
                extras.putString(DocumentsContract.EXTRA_INFO, MSG_INFO);
                extras.putString(DocumentsContract.EXTRA_ERROR, MSG_ERROR_AND_INFO);
                break;

            case DIR_THROW:
                throw new RuntimeException();

            case DIR_AUTH:
                Intent intent = new Intent("com.android.documentsui.test.action.AUTHENTICATE");
                PendingIntent pIntent = PendingIntent.getActivity(getContext(),
                        AbstractActionHandler.CODE_AUTHENTICATION, intent, 0);
                throw new AuthenticationRequiredException(new UnsupportedOperationException(),
                        pIntent);

            default:
                addFolder(c, DIR_INFO);
                addFolder(c, DIR_ERROR);
                addFolder(c, DIR_ERROR_AND_INFO);
                addFolder(c, DIR_THROW);
                addFolder(c, DIR_AUTH);
                break;
        }

        return c;
    }
}

