/*
 * Copyright (C) 2013 The Android Open Source Project
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

import static com.android.documentsui.base.SharedMinimal.DEBUG;

import android.annotation.Nullable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.DocumentsContract.Root;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnActionExpandListener;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.widget.SearchView;
import android.widget.SearchView.OnQueryTextListener;

import com.android.documentsui.R;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.DocumentStack;
import com.android.documentsui.base.EventHandler;
import com.android.documentsui.base.RootInfo;
import com.android.documentsui.base.Shared;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;

import java.util.Timer;
import java.util.TimerTask;

/**
 * Manages searching UI behavior.
 */
public class SearchViewManager implements
        SearchView.OnCloseListener, OnQueryTextListener, OnClickListener, OnFocusChangeListener,
        OnActionExpandListener {

    private static final String TAG = "SearchManager";

    // How long we wait after the user finishes typing before kicking off a search.
    public static final int SEARCH_DELAY_MS = 750;

    private final SearchManagerListener mListener;
    private final EventHandler<String> mCommandProcessor;
    private final Timer mTimer;
    private final Handler mUiHandler;

    private final Object mSearchLock;
    @GuardedBy("mSearchLock")
    private @Nullable Runnable mQueuedSearchRunnable;
    @GuardedBy("mSearchLock")
    private @Nullable TimerTask mQueuedSearchTask;
    private @Nullable String mCurrentSearch;
    private boolean mSearchExpanded;
    private boolean mIgnoreNextClose;
    private boolean mFullBar;

    private Menu mMenu;
    private MenuItem mMenuItem;
    private SearchView mSearchView;

    public SearchViewManager(
            SearchManagerListener listener,
            EventHandler<String> commandProcessor,
            @Nullable Bundle savedState) {
        this(listener, commandProcessor, savedState, new Timer(),
                new Handler(Looper.getMainLooper()));
    }

    @VisibleForTesting
    protected SearchViewManager(
            SearchManagerListener listener,
            EventHandler<String> commandProcessor,
            @Nullable Bundle savedState,
            Timer timer,
            Handler handler) {
        assert (listener != null);
        assert (commandProcessor != null);

        mSearchLock = new Object();
        mListener = listener;
        mCommandProcessor = commandProcessor;
        mTimer = timer;
        mUiHandler = handler;
        mCurrentSearch = savedState != null ? savedState.getString(Shared.EXTRA_QUERY) : null;
    }

    public void install(Menu menu, boolean isFullBarSearch) {
        mMenu = menu;
        mMenuItem = mMenu.findItem(R.id.option_menu_search);
        mSearchView = (SearchView) mMenuItem.getActionView();

        mSearchView.setOnQueryTextListener(this);
        mSearchView.setOnCloseListener(this);
        mSearchView.setOnSearchClickListener(this);
        mSearchView.setOnQueryTextFocusChangeListener(this);

        mFullBar = isFullBarSearch;
        if (mFullBar) {
            mMenuItem.setShowAsActionFlags(MenuItem.SHOW_AS_ACTION_COLLAPSE_ACTION_VIEW
                    | MenuItem.SHOW_AS_ACTION_ALWAYS);
            mMenuItem.setOnActionExpandListener(this);
            mSearchView.setMaxWidth(Integer.MAX_VALUE);
        }

        restoreSearch();
    }

    /**
     * Used to hide menu icons, when the search is being restored. Needed because search restoration
     * is done before onPrepareOptionsMenu(Menu menu) that is overriding the icons visibility.
     */
    public void updateMenu() {
        if (isSearching() && mFullBar) {
            mMenu.setGroupVisible(R.id.group_hide_when_searching, false);
        }
    }

    /**
     * @param stack New stack.
     */
    public void update(DocumentStack stack) {
        if (mMenuItem == null) {
            if (DEBUG) Log.d(TAG, "update called before Search MenuItem installed.");
            return;
        }

        if (mCurrentSearch != null) {
            mMenuItem.expandActionView();

            mSearchView.setIconified(false);
            mSearchView.clearFocus();
            mSearchView.setQuery(mCurrentSearch, false);
        } else {
            mSearchView.clearFocus();
            if (!mSearchView.isIconified()) {
                mIgnoreNextClose = true;
                mSearchView.setIconified(true);
            }

            if (mMenuItem.isActionViewExpanded()) {
                mMenuItem.collapseActionView();
            }
        }

        showMenu(stack);
    }

    public void showMenu(@Nullable DocumentStack stack) {
        final DocumentInfo cwd = stack != null ? stack.peek() : null;

        boolean supportsSearch = true;

        // Searching in archives is not enabled, as archives are backed by
        // a different provider than the root provider.
        if (cwd != null && cwd.isInArchive()) {
            supportsSearch = false;
        }

        final RootInfo root = stack != null ? stack.getRoot() : null;
        if (root == null || (root.flags & Root.FLAG_SUPPORTS_SEARCH) == 0) {
            supportsSearch = false;
        }

        if (mMenuItem == null) {
            if (DEBUG) Log.d(TAG, "showMenu called before Search MenuItem installed.");
            return;
        }

        if (!supportsSearch) {
            mCurrentSearch = null;
        }

        mMenuItem.setVisible(supportsSearch);
    }

    /**
     * Cancels current search operation. Triggers clearing and collapsing the SearchView.
     *
     * @return True if it cancels search. False if it does not operate search currently.
     */
    public boolean cancelSearch() {
        if (isExpanded() || isSearching()) {
            cancelQueuedSearch();
            // If the query string is not empty search view won't get iconified
            mSearchView.setQuery("", false);

            if (mFullBar) {
               onClose();
            } else {
                // Causes calling onClose(). onClose() is triggering directory content update.
                mSearchView.setIconified(true);
            }
            return true;
        }
        return false;
    }

    private void cancelQueuedSearch() {
        synchronized (mSearchLock) {
            if (mQueuedSearchTask != null) {
                mQueuedSearchTask.cancel();
            }
            mQueuedSearchTask = null;
            mUiHandler.removeCallbacks(mQueuedSearchRunnable);
            mQueuedSearchRunnable = null;
        }
    }

    /**
     * Sets search view into the searching state. Used to restore state after device orientation
     * change.
     */
    private void restoreSearch() {
        if (isSearching()) {
            if (mFullBar) {
                mMenuItem.expandActionView();
            } else {
                mSearchView.setIconified(false);
            }
            onSearchExpanded();
            mSearchView.setQuery(mCurrentSearch, false);
            mSearchView.clearFocus();
        }
    }

    private void onSearchExpanded() {
        mSearchExpanded = true;
        if (mFullBar) {
            mMenu.setGroupVisible(R.id.group_hide_when_searching, false);
        }

        mListener.onSearchViewChanged(true);
    }

    /**
     * Clears the search. Triggers refreshing of the directory content.
     * @return True if the default behavior of clearing/dismissing SearchView should be overridden.
     *         False otherwise.
     */
    @Override
    public boolean onClose() {
        mSearchExpanded = false;
        if (mIgnoreNextClose) {
            mIgnoreNextClose = false;
            return false;
        }

        // Refresh the directory if a search was done
        if (mCurrentSearch != null) {
            mCurrentSearch = null;
            mListener.onSearchChanged(mCurrentSearch);
        }

        if (mFullBar) {
            mMenuItem.collapseActionView();
        }
        mListener.onSearchFinished();

        mListener.onSearchViewChanged(false);

        return false;
    }

    /**
     * Called when owning activity is saving state to be used to restore state during creation.
     * @param state Bundle to save state too
     */
    public void onSaveInstanceState(Bundle state) {
        state.putString(Shared.EXTRA_QUERY, mCurrentSearch);
    }

    /**
     * Sets mSearchExpanded. Called when search icon is clicked to start search for both search view
     * modes.
     */
    @Override
    public void onClick(View v) {
        onSearchExpanded();
    }

    @Override
    public boolean onQueryTextSubmit(String query) {

        if (mCommandProcessor.accept(query)) {
            mSearchView.setQuery("", false);
        } else {
            cancelQueuedSearch();
            // Don't kick off a search if we've already finished it.
            if (mCurrentSearch != query) {
                mCurrentSearch = query;
                mListener.onSearchChanged(mCurrentSearch);
            }
            mSearchView.clearFocus();
        }

        return true;
    }

    /**
     * Used to detect and handle back button pressed event when search is expanded.
     */
    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (!hasFocus) {
            if (mCurrentSearch == null) {
                mSearchView.setIconified(true);
            } else if (TextUtils.isEmpty(mSearchView.getQuery())) {
                cancelSearch();
            }
        }
    }

    @VisibleForTesting
    protected TimerTask createSearchTask(String newText) {
        return new TimerTask() {
            @Override
            public void run() {
                // Do the actual work on the main looper.
                synchronized (mSearchLock) {
                    mQueuedSearchRunnable = () -> {
                        mCurrentSearch = newText;
                        if (mCurrentSearch != null && mCurrentSearch.isEmpty()) {
                            mCurrentSearch = null;
                        }
                        mListener.onSearchChanged(mCurrentSearch);
                    };
                    mUiHandler.post(mQueuedSearchRunnable);
                }
            }
        };
    }

    @Override
    public boolean onQueryTextChange(String newText) {
        cancelQueuedSearch();
        synchronized (mSearchLock) {
            mQueuedSearchTask = createSearchTask(newText);

            mTimer.schedule(mQueuedSearchTask, SEARCH_DELAY_MS);
        }
        return true;
    }

    @Override
    public boolean onMenuItemActionCollapse(MenuItem item) {
        mMenu.setGroupVisible(R.id.group_hide_when_searching, true);

        // Handles case when search view is collapsed by using the arrow on the left of the bar
        if (isExpanded() || isSearching()) {
            cancelSearch();
            return false;
        }
        return true;
    }

    @Override
    public boolean onMenuItemActionExpand(MenuItem item) {
        return true;
    }

    public String getCurrentSearch() {
        return mCurrentSearch;
    }

    public boolean isSearching() {
        return mCurrentSearch != null;
    }

    public boolean isExpanded() {
        return mSearchExpanded;
    }

    public interface SearchManagerListener {
        void onSearchChanged(@Nullable String query);
        void onSearchFinished();
        void onSearchViewChanged(boolean opened);
    }
}
