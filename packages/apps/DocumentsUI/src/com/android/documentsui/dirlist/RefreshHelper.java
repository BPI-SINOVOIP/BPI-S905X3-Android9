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
package com.android.documentsui.dirlist;

import static android.support.v4.util.Preconditions.checkState;

import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.OnItemTouchListener;
import android.view.MotionEvent;

import com.android.documentsui.base.BooleanConsumer;
import com.android.documentsui.base.Events;

/**
 * Class providing support for gluing gesture scaling of the ui into RecyclerView + DocsUI.
 */
final class RefreshHelper {

    private final BooleanConsumer mRefreshLayoutEnabler;

    private boolean mAttached;

    public RefreshHelper(BooleanConsumer refreshLayoutEnabler) {
        mRefreshLayoutEnabler = refreshLayoutEnabler;
    }

    private boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
        if (Events.isMouseEvent(e)) {
            if (Events.isActionDown(e)) {
                mRefreshLayoutEnabler.accept(false);
            }
        } else {
            // If user has started some gesture while RecyclerView is not at the top, disable
            // refresh
            if (Events.isActionDown(e) && rv.computeVerticalScrollOffset() != 0) {
                mRefreshLayoutEnabler.accept(false);
            }
        }

        if (Events.isActionUp(e)) {
            mRefreshLayoutEnabler.accept(true);
        }

        return false;
    }

    private void onTouchEvent(RecyclerView rv, MotionEvent e) {
        if (Events.isActionUp(e)) {
            mRefreshLayoutEnabler.accept(true);
        }
    }

    void attach(RecyclerView view) {
        checkState(!mAttached);

        view.addOnItemTouchListener(
                new OnItemTouchListener() {
                    @Override
                    public void onTouchEvent(RecyclerView rv, MotionEvent e) {
                        RefreshHelper.this.onTouchEvent(rv, e);
                    }

                    @Override
                    public boolean onInterceptTouchEvent(RecyclerView rv, MotionEvent e) {
                        return RefreshHelper.this.onInterceptTouchEvent(rv, e);
                    }

                    @Override
                    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}
        });

        mAttached = true;
    }
}
