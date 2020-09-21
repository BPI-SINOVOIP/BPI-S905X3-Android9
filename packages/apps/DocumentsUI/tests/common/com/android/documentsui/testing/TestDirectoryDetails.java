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

import com.android.documentsui.MenuManager.DirectoryDetails;

/**
 * Test copy of DirectoryDetails, everything default to false
 */
public class TestDirectoryDetails extends DirectoryDetails {

    public boolean isInRecents;
    public boolean hasRootSettings;
    public boolean hasItemsToPaste;
    public boolean canCreateDoc;
    public boolean canCreateDirectory;

    public TestDirectoryDetails() {
        super(null);
    }

    @Override
    public boolean hasRootSettings() {
        return hasRootSettings;
    }

    @Override
    public boolean hasItemsToPaste() {
        return hasItemsToPaste;
    }

    @Override
    public boolean isInRecents() {
        return isInRecents;
    }

    @Override
    public boolean canCreateDoc() {
        return canCreateDoc;
    }

    @Override
    public boolean canCreateDirectory() {
        return canCreateDirectory;
    }
}