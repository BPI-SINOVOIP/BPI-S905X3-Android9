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

package com.android.documentsui;

import android.support.test.filters.LargeTest;

import com.android.documentsui.files.FilesActivity;

@LargeTest
public class DirectoryMessagesUiTest extends ActivityTest<FilesActivity> {

    public DirectoryMessagesUiTest() {
        super(FilesActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        bots.roots.openRoot("Demo Root");
        bots.main.switchToListMode();
    }

    public void testAuthenticationMessage_visible() throws Exception {
        // If feature is disabled, this test is a no-op.
        if (!features.isRemoteActionsEnabled()) {
            return;
        }
        bots.directory.openDocument(DemoProvider.DIR_AUTH);
        bots.directory.assertHasMessage(
                "To view this directory, sign in to DocumentsUI Tests");
        bots.directory.assertHasMessageButtonText("SIGN IN");
    }

    public void testInfoMessage_visible() throws Exception {
        bots.directory.openDocument(DemoProvider.DIR_INFO);
        bots.directory.assertHasMessage(DemoProvider.MSG_INFO);
        bots.directory.assertHasMessageButtonText("DISMISS");
    }

    public void testInfoMessage_dismissable() throws Exception {
        bots.directory.openDocument(DemoProvider.DIR_INFO);
        bots.directory.assertHasMessage(true);
        bots.directory.clickMessageButton();
        bots.directory.assertHasMessage(false);
    }

    public void testErrorMessage_visible() throws Exception {
        bots.directory.openDocument(DemoProvider.DIR_ERROR);
        bots.directory.assertHasMessage(DemoProvider.MSG_ERROR);
        bots.directory.assertHasMessageButtonText("DISMISS");
    }

    public void testErrorMessage_dismissable() throws Exception {
        bots.directory.openDocument(DemoProvider.DIR_ERROR);
        bots.directory.assertHasMessage(true);
        bots.directory.clickMessageButton();
        bots.directory.assertHasMessage(false);
    }

    public void testErrorMessage_supercedesInfoMessage() throws Exception {
        // When both error and info are returned in Directory, only show the error.
        bots.directory.openDocument(DemoProvider.DIR_ERROR_AND_INFO);
        bots.directory.assertHasMessage(DemoProvider.MSG_ERROR_AND_INFO);
    }
}
