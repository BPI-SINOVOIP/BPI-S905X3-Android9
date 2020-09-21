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

import com.android.documentsui.DocsSelectionHelper;
import com.android.documentsui.dirlist.DocsStableIdProvider;
import com.android.documentsui.dirlist.DocumentsAdapter;
import com.android.documentsui.dirlist.TestDocumentsAdapter;
import com.android.documentsui.selection.DefaultSelectionHelper;
import com.android.documentsui.selection.DefaultSelectionHelper.SelectionMode;
import com.android.documentsui.selection.SelectionHelper.SelectionPredicate;

import java.util.Collections;
import java.util.List;

public class SelectionHelpers {

    public static final SelectionPredicate CAN_SET_ANYTHING = new SelectionPredicate() {
        @Override
        public boolean canSetStateForId(String id, boolean nextState) {
            return true;
        }

        @Override
        public boolean canSetStateAtPosition(int position, boolean nextState) {
            return true;
        }
    };

    private SelectionHelpers() {}

    public static DocsSelectionHelper createTestInstance() {
        return createTestInstance(Collections.emptyList());
    }

    public static DocsSelectionHelper createTestInstance(List<String> docs) {
        return createTestInstance(docs, DefaultSelectionHelper.MODE_MULTIPLE);
    }

    public static DocsSelectionHelper createTestInstance(
            List<String> docs, @SelectionMode int mode) {
        return createTestInstance(new TestDocumentsAdapter(docs), mode, CAN_SET_ANYTHING);
    }

    public static DocsSelectionHelper createTestInstance(
            DocumentsAdapter adapter, @SelectionMode int mode, SelectionPredicate canSetState) {
        DocsSelectionHelper manager = mode == DefaultSelectionHelper.MODE_SINGLE
                ? DocsSelectionHelper.createSingleSelect()
                : DocsSelectionHelper.createMultiSelect();

        manager.reset(adapter, new DocsStableIdProvider(adapter), canSetState);

        return manager;
    }
}
