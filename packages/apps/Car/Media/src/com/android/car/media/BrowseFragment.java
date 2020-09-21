/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.car.media;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.car.media.browse.BrowseAdapter;
import com.android.car.media.browse.ContentForwardStrategy;
import com.android.car.media.common.GridSpacingItemDecoration;
import com.android.car.media.common.MediaItemMetadata;
import com.android.car.media.common.MediaSource;
import com.android.car.media.widgets.ViewUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Stack;

import androidx.car.widget.PagedListView;

/**
 * A {@link Fragment} that implements the content forward browsing experience.
 */
public class BrowseFragment extends Fragment {
    private static final String TAG = "BrowseFragment";
    private static final String TOP_MEDIA_ITEM_KEY = "top_media_item";
    private static final String MEDIA_SOURCE_PACKAGE_NAME_KEY = "media_source";
    private static final String BROWSE_STACK_KEY = "browse_stack";

    private PagedListView mBrowseList;
    private ProgressBar mProgressBar;
    private ImageView mErrorIcon;
    private TextView mErrorMessage;
    private MediaSource mMediaSource;
    private BrowseAdapter mBrowseAdapter;
    private String mMediaSourcePackageName;
    private MediaItemMetadata mTopMediaItem;
    private Callbacks mCallbacks;
    private int mFadeDuration;
    private int mProgressBarDelay;
    private Handler mHandler = new Handler();
    private Stack<MediaItemMetadata> mBrowseStack = new Stack<>();
    private MediaSource.Observer mBrowseObserver = new MediaSource.Observer() {
        @Override
        protected void onBrowseConnected(boolean success) {
            BrowseFragment.this.onBrowseConnected(success);
        }

        @Override
        protected void onBrowseDisconnected() {
            BrowseFragment.this.onBrowseDisconnected();
        }
    };
    private BrowseAdapter.Observer mBrowseAdapterObserver = new BrowseAdapter.Observer() {
        @Override
        protected void onDirty() {
            switch (mBrowseAdapter.getState()) {
                case LOADING:
                case IDLE:
                    // Still loading... nothing to do.
                    break;
                case LOADED:
                    stopLoadingIndicator();
                    mBrowseAdapter.update();
                    if (mBrowseAdapter.getItemCount() > 0) {
                        ViewUtils.showViewAnimated(mBrowseList, mFadeDuration);
                        ViewUtils.hideViewAnimated(mErrorIcon, mFadeDuration);
                        ViewUtils.hideViewAnimated(mErrorMessage, mFadeDuration);
                    } else {
                        mErrorMessage.setText(R.string.nothing_to_play);
                        ViewUtils.hideViewAnimated(mBrowseList, mFadeDuration);
                        ViewUtils.hideViewAnimated(mErrorIcon, mFadeDuration);
                        ViewUtils.showViewAnimated(mErrorMessage, mFadeDuration);
                    }
                    break;
                case ERROR:
                    stopLoadingIndicator();
                    mErrorMessage.setText(R.string.unknown_error);
                    ViewUtils.hideViewAnimated(mBrowseList, mFadeDuration);
                    ViewUtils.showViewAnimated(mErrorMessage, mFadeDuration);
                    ViewUtils.showViewAnimated(mErrorIcon, mFadeDuration);
                    break;
            }
        }

        @Override
        protected void onPlayableItemClicked(MediaItemMetadata item) {
            mCallbacks.onPlayableItemClicked(mMediaSource, item);
        }

        @Override
        protected void onBrowseableItemClicked(MediaItemMetadata item) {
            navigateInto(item);
        }

        @Override
        protected void onMoreButtonClicked(MediaItemMetadata item) {
            navigateInto(item);
        }
    };

    /**
     * Fragment callbacks (implemented by the hosting Activity)
     */
    public interface Callbacks {
        /**
         * @return a {@link MediaSource} corresponding to the given package name
         */
        MediaSource getMediaSource(String packageName);

        /**
         * Method invoked when the back stack changes (for example, when the user moves up or down
         * the media tree)
         */
        void onBackStackChanged();

        /**
         * Method invoked when the user clicks on a playable item
         *
         * @param mediaSource {@link MediaSource} the playable item belongs to
         * @param item item to be played.
         */
        void onPlayableItemClicked(MediaSource mediaSource, MediaItemMetadata item);
    }

    /**
     * Moves the user one level up in the browse tree, if possible.
     */
    public void navigateBack() {
        mBrowseStack.pop();
        if (mBrowseAdapter != null) {
            mBrowseAdapter.setParentMediaItemId(getCurrentMediaItem());
        }
        if (mCallbacks != null) {
            mCallbacks.onBackStackChanged();
        }
    }

    /**
     * @return whether the user is in a level other than the top.
     */
    public boolean isBackEnabled() {
        return !mBrowseStack.isEmpty();
    }

    /**
     * Creates a new instance of this fragment.
     *
     * @param mediaSource media source being displayed
     * @param item media tree node to display on this fragment.
     * @return a fully initialized {@link BrowseFragment}
     */
    public static BrowseFragment newInstance(MediaSource mediaSource, MediaItemMetadata item) {
        BrowseFragment fragment = new BrowseFragment();
        Bundle args = new Bundle();
        args.putParcelable(TOP_MEDIA_ITEM_KEY, item);
        args.putString(MEDIA_SOURCE_PACKAGE_NAME_KEY, mediaSource.getPackageName());
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Bundle arguments = getArguments();
        if (arguments != null) {
            mTopMediaItem = arguments.getParcelable(TOP_MEDIA_ITEM_KEY);
            mMediaSourcePackageName = arguments.getString(MEDIA_SOURCE_PACKAGE_NAME_KEY);
        }
        if (savedInstanceState != null) {
            List<MediaItemMetadata> savedStack =
                    savedInstanceState.getParcelableArrayList(BROWSE_STACK_KEY);
            mBrowseStack.clear();
            if (savedStack != null) {
                mBrowseStack.addAll(savedStack);
            }
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, final ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_browse, container, false);
        mProgressBar = view.findViewById(R.id.loading_spinner);
        mProgressBarDelay = getContext().getResources()
                .getInteger(R.integer.progress_indicator_delay);
        mBrowseList = view.findViewById(R.id.browse_list);
        mErrorIcon = view.findViewById(R.id.error_icon);
        mErrorMessage = view.findViewById(R.id.error_message);
        mFadeDuration = getContext().getResources().getInteger(
                R.integer.new_album_art_fade_in_duration);
        int numColumns = getContext().getResources().getInteger(R.integer.num_browse_columns);
        GridLayoutManager gridLayoutManager = new GridLayoutManager(getContext(), numColumns);
        RecyclerView recyclerView = mBrowseList.getRecyclerView();
        recyclerView.setVerticalFadingEdgeEnabled(true);
        recyclerView.setFadingEdgeLength(getResources()
                .getDimensionPixelSize(R.dimen.car_padding_5));
        recyclerView.setLayoutManager(gridLayoutManager);
        recyclerView.addItemDecoration(new GridSpacingItemDecoration(
                getResources().getDimensionPixelSize(R.dimen.car_padding_4),
                getResources().getDimensionPixelSize(R.dimen.car_keyline_1),
                getResources().getDimensionPixelSize(R.dimen.car_keyline_1)
        ));
        return view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mCallbacks = (Callbacks) context;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mCallbacks = null;
    }

    @Override
    public void onStart() {
        super.onStart();
        startLoadingIndicator();
        mMediaSource = mCallbacks.getMediaSource(mMediaSourcePackageName);
        if (mMediaSource != null) {
            mMediaSource.subscribe(mBrowseObserver);
        }
    }

    private Runnable mProgressIndicatorRunnable = new Runnable() {
        @Override
        public void run() {
            ViewUtils.showViewAnimated(mProgressBar, mFadeDuration);
        }
    };

    private void startLoadingIndicator() {
        // Display the indicator after a certain time, to avoid flashing the indicator constantly,
        // even when performance is acceptable.
        mHandler.postDelayed(mProgressIndicatorRunnable, mProgressBarDelay);
    }

    private void stopLoadingIndicator() {
        mHandler.removeCallbacks(mProgressIndicatorRunnable);
        ViewUtils.hideViewAnimated(mProgressBar, mFadeDuration);
    }

    @Override
    public void onStop() {
        super.onStop();
        stopLoadingIndicator();
        if (mMediaSource != null) {
            mMediaSource.unsubscribe(mBrowseObserver);
        }
        if (mBrowseAdapter != null) {
            mBrowseAdapter.stop();
            mBrowseAdapter = null;
        }
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        ArrayList<MediaItemMetadata> stack = new ArrayList<>(mBrowseStack);
        outState.putParcelableArrayList(BROWSE_STACK_KEY, stack);
    }

    private void onBrowseConnected(boolean success) {
        if (mBrowseAdapter != null) {
            mBrowseAdapter.stop();
            mBrowseAdapter = null;
        }
        if (!success) {
            ViewUtils.hideViewAnimated(mBrowseList, mFadeDuration);
            stopLoadingIndicator();
            mErrorMessage.setText(R.string.cannot_connect_to_app);
            ViewUtils.showViewAnimated(mErrorIcon, mFadeDuration);
            ViewUtils.showViewAnimated(mErrorMessage, mFadeDuration);
            return;
        }
        mBrowseAdapter = new BrowseAdapter(getContext(), mMediaSource, getCurrentMediaItem(),
                ContentForwardStrategy.DEFAULT_STRATEGY);
        mBrowseList.setAdapter(mBrowseAdapter);
        mBrowseList.setDividerVisibilityManager(mBrowseAdapter);
        mBrowseAdapter.registerObserver(mBrowseAdapterObserver);
        mBrowseAdapter.start();
    }

    private void onBrowseDisconnected() {
        if (mBrowseAdapter != null) {
            mBrowseAdapter.stop();
            mBrowseAdapter = null;
        }
    }

    private void navigateInto(MediaItemMetadata item) {
        mBrowseStack.push(item);
        mBrowseAdapter.setParentMediaItemId(item);
        mCallbacks.onBackStackChanged();
    }

    /**
     * @return the current item being displayed
     */
    public MediaItemMetadata getCurrentMediaItem() {
        if (mBrowseStack.isEmpty()) {
            return mTopMediaItem;
        } else {
            return mBrowseStack.lastElement();
        }
    }
}
