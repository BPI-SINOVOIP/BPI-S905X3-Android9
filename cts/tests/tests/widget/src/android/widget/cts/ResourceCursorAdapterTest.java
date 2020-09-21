/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.widget.cts;

import static junit.framework.Assert.assertTrue;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;

import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ResourceCursorAdapter;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test {@link ResourceCursorAdapter}.
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class ResourceCursorAdapterTest {
    private ResourceCursorAdapter mResourceCursorAdapter;

    private Context mContext;

    private ViewGroup mParent;

    private Cursor mCursor;

    @Before
    public void setup() {
        mContext = InstrumentationRegistry.getTargetContext();
        LayoutInflater layoutInflater = (LayoutInflater) mContext.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mParent = (ViewGroup) layoutInflater.inflate(R.layout.cursoradapter_host, null);
        mCursor = createTestCursor(3, 3);
    }

    @UiThreadTest
    @Test
    public void testConstructor() {
        MockResourceCursorAdapter adapter = new MockResourceCursorAdapter(mContext, -1, null);
        // the default is true
        assertTrue(adapter.isAutoRequery());
        assertNull(adapter.getCursor());

        adapter = new MockResourceCursorAdapter(mContext, R.layout.cursoradapter_item0, mCursor);
        // the default is true
        assertTrue(adapter.isAutoRequery());
        assertSame(mCursor, adapter.getCursor());

        adapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item0, mCursor, false);
        assertFalse(adapter.isAutoRequery());
        assertSame(mCursor, adapter.getCursor());
    }

    @UiThreadTest
    @Test
    public void testSetViewResource() {
        mResourceCursorAdapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item0, mCursor);

        View result = mResourceCursorAdapter.newView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item0, result.getId());

        // set the new view resource
        mResourceCursorAdapter.setViewResource(R.layout.cursoradapter_item1);
        result = mResourceCursorAdapter.newView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item1, result.getId());
    }

    @UiThreadTest
    @Test
    public void testSetDropDownViewResource() {
        mResourceCursorAdapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item0, mCursor);

        View result = mResourceCursorAdapter.newDropDownView(null, null, mParent);
        assertNotNull(result);
        // the original dropdown'layout is set in constructor
        assertEquals(R.id.cursorAdapter_item0, result.getId());

        // set the dropdown to new layout
        mResourceCursorAdapter.setDropDownViewResource(R.layout.cursoradapter_item1);

        // the dropdown changes
        result = mResourceCursorAdapter.newDropDownView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item1, result.getId());

        // the view does not change
        result = mResourceCursorAdapter.newView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item0, result.getId());
    }

    // parameters Context and Cursor are never read in the method
    @UiThreadTest
    @Test
    public void testNewDropDownView() {
        mResourceCursorAdapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item0, mCursor);
        View result = mResourceCursorAdapter.newDropDownView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item0, result.getId());

        mResourceCursorAdapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item1, mCursor);
        result = mResourceCursorAdapter.newDropDownView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item1, result.getId());
    }

    // The parameters Context and Cursor are never read in the method
    @UiThreadTest
    @Test
    public void testNewView() {
        mResourceCursorAdapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item0, mCursor);
        View result = mResourceCursorAdapter.newView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item0, result.getId());

        mResourceCursorAdapter = new MockResourceCursorAdapter(mContext,
                R.layout.cursoradapter_item1, mCursor);
        result = mResourceCursorAdapter.newView(null, null, mParent);
        assertNotNull(result);
        assertEquals(R.id.cursorAdapter_item1, result.getId());
    }

    /**
     * Creates the test cursor.
     *
     * @param colCount the column count
     * @param rowCount the row count
     * @return the cursor
     */
    @SuppressWarnings("unchecked")
    private Cursor createTestCursor(int colCount, int rowCount) {
        String[] columns = new String[colCount + 1];
        for (int i = 0; i < colCount; i++) {
            columns[i] = "column" + i;
        }
        columns[colCount] = "_id";

        MatrixCursor cursor = new MatrixCursor(columns, rowCount);
        Object[] row = new Object[colCount + 1];
        for (int i = 0; i < rowCount; i++) {
            for (int j = 0; j < colCount; j++) {
                row[j] = "" + i + "" + j;
            }
            row[colCount] = i;
            cursor.addRow(row);
        }
        return cursor;
    }

    private static class MockResourceCursorAdapter extends ResourceCursorAdapter {
        private final boolean mAutoRequery;

        public MockResourceCursorAdapter(Context context, int layout, Cursor c) {
            super(context, layout, c);

            // Default is true.
            mAutoRequery = true;
        }

        public MockResourceCursorAdapter(Context context, int layout,
                Cursor c, boolean autoRequery) {
            super(context, layout, c, autoRequery);

            mAutoRequery = autoRequery;
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
        }

        public boolean isAutoRequery() {
            return mAutoRequery;
        }
    }
}
