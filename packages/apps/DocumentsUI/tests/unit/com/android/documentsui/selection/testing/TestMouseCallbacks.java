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
import static org.junit.Assert.assertTrue;

import android.support.v7.widget.RecyclerView;
import android.view.MotionEvent;

import com.android.documentsui.selection.MouseInputHandler;
import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

public final class TestMouseCallbacks extends MouseInputHandler.Callbacks {

    private String mFocusItemId;
    private int mFocusPosition;
    private MotionEvent mLastContextEvent;
    private ItemDetails mActivated;

    @Override
    public boolean onItemActivated(ItemDetails item, MotionEvent e) {
        mActivated = item;
        return true;
    }

    @Override
    public boolean onContextClick(MotionEvent e) {
        mLastContextEvent = e;
        return false;
    }

    @Override
    public void onPerformHapticFeedback() {
    }

    @Override
    public void clearFocus() {
        mFocusPosition = RecyclerView.NO_POSITION;
        mFocusItemId = null;
    }

    @Override
    public void focusItem(ItemDetails item) {
        mFocusItemId = item.getStableId();
        mFocusPosition = item.getPosition();
    }

    @Override
    public int getFocusedPosition() {
        return mFocusPosition;
    }

    @Override
    public boolean hasFocusedItem() {
        return mFocusItemId != null;
    }

    public void assertLastEvent(MotionEvent expected) {
        // sadly, MotionEvent doesn't implement equals, so we compare references.
        assertTrue(expected == mLastContextEvent);
    }

    public void assertActivated(ItemDetails expected) {
        assertEquals(expected, mActivated);
    }

    public void assertHasFocus(boolean focused) {
        assertEquals(focused, hasFocusedItem());
    }

    public void assertFocused(String expectedId) {
        assertEquals(expectedId, mFocusItemId);
    }
}