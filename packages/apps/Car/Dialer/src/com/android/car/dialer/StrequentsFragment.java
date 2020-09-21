/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.car.dialer;

import android.content.ContentResolver;
import android.content.Context;
import android.content.CursorLoader;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.car.widget.DayNightStyle;
import androidx.car.widget.PagedListView;

import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.telecom.UiCallManager;

/**
 * Contains a list of contacts. The call types can be any of the CALL_TYPE_* fields from
 * {@link PhoneLoader}.
 */
public class StrequentsFragment extends Fragment {
    private static final String TAG = "Em.StrequentsFrag";

    private static final String KEY_MAX_CLICKS = "max_clicks";
    private static final int DEFAULT_MAX_CLICKS = 6;

    private UiCallManager mUiCallManager;
    private StrequentsAdapter mAdapter;
    private CursorLoader mSpeedialCursorLoader;
    private CursorLoader mCallLogCursorLoader;
    private Context mContext;
    private PagedListView mListView;
    private Cursor mStrequentCursor;
    private Cursor mCallLogCursor;
    private boolean mHasLoadedData;

    public static StrequentsFragment newInstance() {
        return new StrequentsFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "onCreate");
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "onCreateView");
        }

        mContext = getContext();
        mUiCallManager = UiCallManager.get();

        View view = inflater.inflate(R.layout.strequents_fragment, container, false);
        mListView = view.findViewById(R.id.list_view);
        int numOfColumn = getContext().getResources().getInteger(
                R.integer.favorite_fragment_grid_column);
        mListView.getRecyclerView().setLayoutManager(
                new GridLayoutManager(getContext(), numOfColumn));
        mListView.getRecyclerView().addItemDecoration(new ItemSpacingDecoration());

        mSpeedialCursorLoader = PhoneLoader.registerCallObserver(PhoneLoader.CALL_TYPE_SPEED_DIAL,
                mContext, (loader, cursor) -> {
                    if (Log.isLoggable(TAG, Log.DEBUG)) {
                        Log.d(TAG, "PhoneLoader: onLoadComplete (CALL_TYPE_SPEED_DIAL)");
                    }

                    onLoadStrequentCursor(cursor);
                });

        // Get the latest call log from the call logs history.
        mCallLogCursorLoader = PhoneLoader.registerCallObserver(PhoneLoader.CALL_TYPE_ALL, mContext,
                (loader, cursor) -> {
                    if (Log.isLoggable(TAG, Log.DEBUG)) {
                        Log.d(TAG, "PhoneLoader: onLoadComplete (CALL_TYPE_ALL)");
                    }
                    onLoadCallLogCursor(cursor);
                });

        ContentResolver contentResolver = mContext.getContentResolver();
        contentResolver.registerContentObserver(mSpeedialCursorLoader.getUri(),
                false, new SpeedDialContentObserver(new Handler()));
        contentResolver.registerContentObserver(mCallLogCursorLoader.getUri(),
                false, new CallLogContentObserver(new Handler()));

        // Maximum number of forward acting clicks the user can perform
        Bundle args = getArguments();
        int maxClicks = args == null
                ? DEFAULT_MAX_CLICKS
                : args.getInt(KEY_MAX_CLICKS, DEFAULT_MAX_CLICKS /* G.maxForwardClicks.get() */);
        // We want to show one fewer page than max clicks to allow clicking on an item,
        // but, the first page is "free" since it doesn't take any clicks to show
        final int maxPages = maxClicks < 0 ? -1 : maxClicks;
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "Max clicks: " + maxClicks + ", Max pages: " + maxPages);
        }

        mAdapter = new StrequentsAdapter(mContext, mUiCallManager);
        mAdapter.setStrequentsListener(viewHolder -> {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "onContactedClicked");
            }

            mUiCallManager.safePlaceCall((String) viewHolder.itemView.getTag(), false);
        });
        mListView.setMaxPages(maxPages);
        mListView.setAdapter(mAdapter);

        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "setItemAnimator");
        }

        mListView.getRecyclerView().setItemAnimator(null);
        return view;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "onDestroyView");
        }

        mAdapter.setStrequentCursor(null);
        mAdapter.setLastCallCursor(null);
        mCallLogCursorLoader.reset();
        mSpeedialCursorLoader.reset();
        mCallLogCursor = null;
        mStrequentCursor = null;
        mHasLoadedData = false;
        mContext = null;
    }

    private void loadDataIntoAdapter() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "loadDataIntoAdapter");
        }

        mHasLoadedData = true;
        mAdapter.setLastCallCursor(mCallLogCursor);
        mAdapter.setStrequentCursor(mStrequentCursor);
    }

    private void onLoadStrequentCursor(Cursor cursor) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "onLoadStrequentCursor");
        }

        if (cursor == null) {
            throw new IllegalArgumentException(
                    "cursor was null in on speed dial fetched");
        }

        mStrequentCursor = cursor;
        if (mCallLogCursor != null) {
            if (mHasLoadedData) {
                mAdapter.setStrequentCursor(cursor);
            } else {
                loadDataIntoAdapter();
            }
        }
    }

    private void onLoadCallLogCursor(Cursor cursor) {
        if (cursor == null) {
            throw new IllegalArgumentException(
                    "cursor was null in on calls fetched");
        }

        mCallLogCursor = cursor;
        if (mStrequentCursor != null) {
            if (mHasLoadedData) {
                mAdapter.setLastCallCursor(cursor);
            } else {
                loadDataIntoAdapter();
            }
        }
    }

    /**
     * A {@link ContentResolver} that is responsible for reloading the user's starred and frequent
     * contacts.
     */
    private class SpeedDialContentObserver extends ContentObserver {
        public SpeedDialContentObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange) {
            onChange(selfChange, null);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "SpeedDialContentObserver onChange() called. Reloading strequents.");
            }
            mSpeedialCursorLoader.startLoading();
        }
    }

    /**
     * A {@link ContentResolver} that is responsible for reloading the user's recent calls.
     */
    private class CallLogContentObserver extends ContentObserver {
        public CallLogContentObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange) {
            onChange(selfChange, null);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "CallLogContentObserver onChange() called. Reloading call log.");
            }
            mCallLogCursorLoader.startLoading();
        }
    }

    private class ItemSpacingDecoration extends RecyclerView.ItemDecoration {

        @Override
        public void getItemOffsets(@NonNull Rect outRect, @NonNull View view,
                @NonNull RecyclerView parent, @NonNull RecyclerView.State state) {
            super.getItemOffsets(outRect, view, parent, state);
            int carPadding1 = mContext.getResources().getDimensionPixelOffset(
                    R.dimen.car_padding_1);

            int leftPadding = 0;
            int rightPadding = 0;
            if (parent.getChildAdapterPosition(view) % 2 == 0) {
                rightPadding = carPadding1;
            } else {
                leftPadding = carPadding1;
            }

            outRect.set(leftPadding, carPadding1, rightPadding, carPadding1);
        }
    }
}
