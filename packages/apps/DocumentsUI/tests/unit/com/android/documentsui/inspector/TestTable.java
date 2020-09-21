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

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNull;
import static junit.framework.Assert.assertTrue;

import android.view.View.OnClickListener;

import com.android.documentsui.inspector.InspectorController.TableDisplay;

import java.util.HashMap;
import java.util.Map;

/**
 * Friendly testable table implementation.
 */
class TestTable implements TableDisplay {

    private Map<Integer, CharSequence> mRows;
    private boolean mVisible;

    public TestTable() {
        mRows = new HashMap<>();
    }

    public void assertHasRow(int keyId, CharSequence expected) {
        assertEquals(expected, mRows.get(keyId));
    }

    public void assertNotInTable (int keyId) {
        assertNull(mRows.get(keyId));
    }

    @Override
    public void put(int keyId, CharSequence value) {
        mRows.put(keyId, value);
    }

    @Override
    public void put(int keyId, CharSequence value, OnClickListener callback) {
        mRows.put(keyId, value);
    }

    @Override
    public void setVisible(boolean visible) {
        mVisible = visible;
    }

    @Override
    public boolean isEmpty() {
        return mRows.isEmpty();
    }

    void assertEmpty() {
        assertTrue(mRows.isEmpty());
    }

    void assertNotEmpty() {
        assertFalse(mRows.isEmpty());
    }

    void assertVisible(boolean expected) {
        assertEquals(expected, mVisible);
    }
}
