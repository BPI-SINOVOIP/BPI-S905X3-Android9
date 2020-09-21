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

package com.android.documentsui.queries;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertNull;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.fail;

import android.annotation.Nullable;
import android.os.Bundle;
import android.os.Handler;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.documentsui.R;
import com.android.documentsui.base.EventHandler;
import com.android.documentsui.queries.SearchViewManager.SearchManagerListener;
import com.android.documentsui.queries.SearchViewManager;
import com.android.documentsui.testing.TestEventHandler;
import com.android.documentsui.testing.TestHandler;
import com.android.documentsui.testing.TestMenu;
import com.android.documentsui.testing.TestTimer;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Timer;
import java.util.TimerTask;

@RunWith(AndroidJUnit4.class)
@SmallTest
public final class SearchViewManagerTest {

    private TestEventHandler<String> mTestEventHandler;
    private TestTimer mTestTimer;
    private TestHandler mTestHandler;
    private SearchViewManager mSearchViewManager;

    private boolean mListenerOnSearchChangedCalled;

    @Before
    public void setUp() {
        mTestEventHandler = new TestEventHandler<>();
        mTestTimer = new TestTimer();
        mTestHandler = new TestHandler();

        final SearchManagerListener searchListener = new SearchManagerListener() {
            @Override
            public void onSearchChanged(@Nullable String query) {
                mListenerOnSearchChangedCalled = true;
            }
            @Override
            public void onSearchFinished() {}
            @Override
            public void onSearchViewChanged(boolean opened) {}
        };

        mSearchViewManager = new TestableSearchViewManager(
                searchListener, mTestEventHandler, null, mTestTimer, mTestHandler);

        final TestMenu testMenu = TestMenu.create();
        mSearchViewManager.install(testMenu, true);
    }

    private static class TestableSearchViewManager extends SearchViewManager {
        public TestableSearchViewManager(
                SearchManagerListener listener,
                EventHandler<String> commandProcessor,
                @Nullable Bundle savedState,
                Timer timer,
                Handler handler) {
            super(listener, commandProcessor, savedState, timer, handler);
        }

        @Override
        public TimerTask createSearchTask(String newText) {
            TimerTask task = super.createSearchTask(newText);
            TestTimer.Task testTask = new TestTimer.Task(task);
            return testTask;
        }
    }

    private void fastForwardTo(long timeMs) {
        mTestTimer.fastForwardTo(timeMs);
        mTestHandler.dispatchAllMessages();
    }

    @Test
    public void testIsExpanded_ExpandsOnClick() {
        mSearchViewManager.onClick(null);
        assertTrue(mSearchViewManager.isExpanded());
    }

    @Test
    public void testIsExpanded_CollapsesOnMenuItemActionCollapse() {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onMenuItemActionCollapse(null);
        assertFalse(mSearchViewManager.isExpanded());
    }

    @Test
    public void testIsSearching_FalseOnClick() throws Exception {
        mSearchViewManager.onClick(null);
        assertFalse(mSearchViewManager.isSearching());
    }

    @Test
    public void testIsSearching_TrueOnQueryTextSubmit() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextSubmit("query");
        assertTrue(mSearchViewManager.isSearching());
    }

    @Test
    public void testIsSearching_FalseImmediatelyAfterOnQueryTextChange() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        assertFalse(mSearchViewManager.isSearching());
    }

    @Test
    public void testIsSearching_TrueAfterOnQueryTextChangeAndWait() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS);
        assertTrue(mSearchViewManager.isSearching());
    }

    @Test
    public void testIsSearching_FalseWhenSecondOnQueryTextChangeResetsTimer() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS - 1);
        mSearchViewManager.onQueryTextChange("qu");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS);
        assertFalse(mSearchViewManager.isSearching());
    }

    @Test
    public void testIsSearching_TrueAfterSecondOnQueryTextChangeResetsTimer() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS - 1);
        mSearchViewManager.onQueryTextChange("qu");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS * 2);
        assertTrue(mSearchViewManager.isSearching());
    }

    @Test
    public void testIsSearching_FalseIfSearchCanceled() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        mSearchViewManager.cancelSearch();
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS);
        assertFalse(mSearchViewManager.isSearching());
    }

    @Test
    public void testOnSearchChanged_CalledAfterOnQueryTextSubmit() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextSubmit("q");
        assertTrue(mListenerOnSearchChangedCalled);
    }

    @Test
    public void testOnSearchChanged_NotCalledImmediatelyAfterOnQueryTextChanged() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        assertFalse(mListenerOnSearchChangedCalled);
    }

    @Test
    public void testOnSearchChanged_CalledAfterOnQueryTextChangedAndWait() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS);
        assertTrue(mListenerOnSearchChangedCalled);
    }

    @Test
    public void testOnSearchChanged_CalledOnlyOnceAfterOnQueryTextSubmit() throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        mSearchViewManager.onQueryTextSubmit("q");

        // Clear the flag to check if it gets set again.
        mListenerOnSearchChangedCalled = false;
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS);
        assertFalse(mListenerOnSearchChangedCalled);
    }

    @Test
    public void testOnSearchChanged_NotCalledForOnQueryTextSubmitIfSearchAlreadyFinished()
            throws Exception {
        mSearchViewManager.onClick(null);
        mSearchViewManager.onQueryTextChange("q");
        fastForwardTo(SearchViewManager.SEARCH_DELAY_MS);
        // Clear the flag to check if it gets set again.
        mListenerOnSearchChangedCalled = false;
        mSearchViewManager.onQueryTextSubmit("q");
        assertFalse(mListenerOnSearchChangedCalled);
    }
}
