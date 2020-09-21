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

package com.android.documentsui.dirlist;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.fail;

import android.provider.DocumentsContract;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.MotionEvent;
import android.view.View;

import com.android.documentsui.DocsSelectionHelper;
import com.android.documentsui.MenuManager.SelectionDetails;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.Events;
import com.android.documentsui.base.Providers;
import com.android.documentsui.base.State;
import com.android.documentsui.dirlist.DragStartListener.RuntimeDragStartListener;
import com.android.documentsui.selection.MutableSelection;
import com.android.documentsui.selection.Selection;
import com.android.documentsui.selection.TestItemDetailsLookup;
import com.android.documentsui.testing.SelectionHelpers;
import com.android.documentsui.testing.TestDragAndDropManager;
import com.android.documentsui.testing.TestEvents;
import com.android.documentsui.testing.TestSelectionDetails;
import com.android.documentsui.testing.Views;
import com.android.internal.widget.RecyclerView;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class DragStartListenerTest {

    private RuntimeDragStartListener mListener;
    private TestEvents.Builder mEvent;
    private DocsSelectionHelper mSelectionMgr;
    private TestItemDetailsLookup mDocLookup;
    private SelectionDetails mSelectionDetails;
    private String mViewModelId;
    private TestDragAndDropManager mManager;

    @Before
    public void setUp() throws Exception {
        mSelectionMgr = SelectionHelpers.createTestInstance();
        mManager = new TestDragAndDropManager();
        mSelectionDetails = new TestSelectionDetails();
        mDocLookup = new TestItemDetailsLookup();

        DocumentInfo doc = new DocumentInfo();
        doc.authority = Providers.AUTHORITY_STORAGE;
        doc.documentId = "id";
        doc.derivedUri = DocumentsContract.buildDocumentUri(doc.authority, doc.documentId);

        State state = new State();
        state.stack.push(doc);

        mListener = new DragStartListener.RuntimeDragStartListener(
                null, // icon helper
                state,
                mDocLookup,
                mSelectionMgr,
                mSelectionDetails,
                // view finder
                (float x, float y) -> {
                    return Views.createTestView(x, y);
                },
                // model id finder
                (View view) -> {
                    return mViewModelId;
                },
                // docInfo Converter
                (Selection selection) -> {
                    return new ArrayList<>();
                },
                mManager);

        mViewModelId = "1234";

        mDocLookup.initAt(1).setInItemDragRegion(true);
        mEvent = TestEvents.builder()
                .action(MotionEvent.ACTION_MOVE)
                .mouse()
                .primary();
    }

    @Test
    public void testMouseEvent() {
        assertTrue(Events.isMouseDragEvent(mEvent.build()));
    }

    @Test
    public void testDragStarted_OnMouseMove() {
        assertTrue(mListener.onMouseDragEvent(mEvent.build()));
        mManager.startDragHandler.assertCalled();
    }

    @Test
    public void testDragNotStarted_NonModelBackedView() {
        mViewModelId = null;
        assertFalse(mListener.onMouseDragEvent(mEvent.build()));
        mManager.startDragHandler.assertNotCalled();
    }

    @Test
    public void testThrows_OnNonMouseMove() {
        assertThrows(mEvent.touch().build());
    }

    @Test
    public void testThrows_OnNonPrimaryMove() {
        mEvent.releaseButton(MotionEvent.BUTTON_PRIMARY);
        assertThrows(mEvent.pressButton(MotionEvent.BUTTON_SECONDARY).build());
    }

    @Test
    public void testThrows_OnNonMove() {
        assertThrows(mEvent.action(MotionEvent.ACTION_UP).build());
    }

    @Test
    public void testThrows_WhenNotOnItem() {
        mDocLookup.initAt(RecyclerView.NO_POSITION);
        assertThrows(mEvent.build());
    }

    @Test
    public void testDragStart_nonSelectedItem() {
        Selection selection = mListener.getSelectionToBeCopied("1234",
                mEvent.action(MotionEvent.ACTION_MOVE).build());
        assertTrue(selection.size() == 1);
        assertTrue(selection.contains("1234"));
    }

    @Test
    public void testDragStart_selectedItem() {
        MutableSelection selection = new MutableSelection();
        selection.add("1234");
        selection.add("5678");
        mSelectionMgr.replaceSelection(selection);

        selection = mListener.getSelectionToBeCopied("1234",
                mEvent.action(MotionEvent.ACTION_MOVE).build());
        assertTrue(selection.size() == 2);
        assertTrue(selection.contains("1234"));
        assertTrue(selection.contains("5678"));
    }

    @Test
    public void testDragStart_newNonSelectedItem() {
        MutableSelection selection = new MutableSelection();
        selection.add("5678");
        mSelectionMgr.replaceSelection(selection);

        selection = mListener.getSelectionToBeCopied("1234",
                mEvent.action(MotionEvent.ACTION_MOVE).build());
        assertTrue(selection.size() == 1);
        assertTrue(selection.contains("1234"));
        // After this, selection should be cleared
        assertFalse(mSelectionMgr.hasSelection());
    }

    @Test
    public void testCtrlDragStart_newNonSelectedItem() {
        MutableSelection selection = new MutableSelection();
        selection.add("5678");
        mSelectionMgr.replaceSelection(selection);

        selection = mListener.getSelectionToBeCopied("1234",
                mEvent.action(MotionEvent.ACTION_MOVE).ctrl().build());
        assertTrue(selection.size() == 2);
        assertTrue(selection.contains("1234"));
        assertTrue(selection.contains("5678"));
    }

    private void assertThrows(MotionEvent e) {
        try {
            mListener.onMouseDragEvent(e);
            fail();
        } catch (IllegalArgumentException expected) {}
    }
}
