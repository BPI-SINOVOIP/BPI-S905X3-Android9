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

package com.android.documentsui.selection.testing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.android.documentsui.selection.SelectionHelper.SelectionObserver;

import java.util.HashSet;
import java.util.Set;

public class TestSelectionObserver extends SelectionObserver {

    private final Set<String> mSelected = new HashSet<>();
    private boolean mSelectionChanged = false;
    private boolean mSelectionReset = false;
    private boolean mSelectionRestored = false;

    public void reset() {
        mSelected.clear();
        mSelectionChanged = false;
        mSelectionReset = false;
    }

    @Override
    public void onItemStateChanged(String id, boolean selected) {
        if (selected) {
            assertNotSelected(id);
            mSelected.add(id);
        } else {
            assertSelected(id);
            mSelected.remove(id);
        }
    }

    @Override
    public void onSelectionReset() {
        mSelectionReset = true;
        mSelected.clear();
    }

    @Override
    public void onSelectionChanged() {
        mSelectionChanged = true;
    }

    @Override
    public void onSelectionRestored() {
        mSelectionRestored = true;
    }

    void assertNoSelection() {
        assertTrue(mSelected.isEmpty());
    }

    void assertSelectionSize(int expected) {
        assertEquals(expected, mSelected.size());
    }

    void assertSelected(String id) {
        assertTrue(id + " is not selected.", mSelected.contains(id));
    }

    void assertNotSelected(String id) {
        assertFalse(id + " is already selected", mSelected.contains(id));
    }

    public void assertSelectionChanged() {
        assertTrue(mSelectionChanged);
    }

    public void assertSelectionUnchanged() {
        assertFalse(mSelectionChanged);
    }

    public void assertSelectionReset() {
        assertTrue(mSelectionReset);
    }

    public void assertSelectionRestored() {
        assertTrue(mSelectionRestored);
    }
}
