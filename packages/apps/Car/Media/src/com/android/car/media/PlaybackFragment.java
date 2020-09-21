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

import android.content.Context;
import android.media.session.MediaController;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.constraint.ConstraintLayout;
import android.support.constraint.ConstraintSet;
import android.support.v4.app.Fragment;
import android.support.v7.widget.RecyclerView;
import android.transition.Transition;
import android.transition.TransitionInflater;
import android.transition.TransitionListenerAdapter;
import android.transition.TransitionManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

import com.android.car.media.common.MediaItemMetadata;
import com.android.car.media.common.MediaSource;
import com.android.car.media.common.PlaybackControls;
import com.android.car.media.common.PlaybackModel;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemAdapter;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.PagedListView;
import androidx.car.widget.TextListItem;

/**
 * A {@link Fragment} that implements both the playback and the content forward browsing experience.
 * It observes a {@link PlaybackModel} and updates its information depending on the currently
 * playing media source through the {@link android.media.session.MediaSession} API.
 */
public class PlaybackFragment extends Fragment {
    private static final String TAG = "PlaybackFragment";

    private PlaybackModel mModel;
    private PlaybackControls mPlaybackControls;
    private QueueItemsAdapter mQueueAdapter;
    private PagedListView mQueue;
    private Callbacks mCallbacks;

    private MetadataController mMetadataController;
    private ConstraintLayout mRootView;

    private boolean mNeedsStateUpdate;
    private boolean mUpdatesPaused;
    private boolean mQueueIsVisible;
    private List<MediaItemMetadata> mQueueItems = new ArrayList<>();
    private PlaybackModel.PlaybackObserver mPlaybackObserver =
            new PlaybackModel.PlaybackObserver() {
                @Override
                public void onPlaybackStateChanged() {
                    updateState();
                }

                @Override
                public void onSourceChanged() {
                    updateAccentColor();
                    updateState();
                }

                @Override
                public void onMetadataChanged() {
                }
            };
    private ListItemProvider mQueueItemsProvider = new ListItemProvider() {
        @Override
        public ListItem get(int position) {
            if (position < 0 || position >= mQueueItems.size()) {
                return null;
            }
            MediaItemMetadata item = mQueueItems.get(position);
            TextListItem textListItem = new TextListItem(getContext());
            textListItem.setTitle(item.getTitle() != null ? item.getTitle().toString() : null);
            textListItem.setBody(item.getSubtitle() != null ? item.getSubtitle().toString() : null);
            textListItem.setOnClickListener(v -> onQueueItemClicked(item));

            return textListItem;
        }
        @Override
        public int size() {
            return mQueueItems.size();
        }
    };
    private class QueueItemsAdapter extends ListItemAdapter {
        QueueItemsAdapter(Context context, ListItemProvider itemProvider) {
            super(context, itemProvider, BackgroundStyle.SOLID);
            setHasStableIds(true);
        }

        void refresh() {
            // TODO: Perform a diff between current and new content and trigger the proper
            // RecyclerView updates.
            this.notifyDataSetChanged();
        }

        @Override
        public long getItemId(int position) {
            return mQueueItems.get(position).getQueueId();
        }
    }

    /**
     * Callbacks this fragment can trigger
     */
    public interface Callbacks {
        /**
         * Returns the playback model to use.
         */
        PlaybackModel getPlaybackModel();

        /**
         * Indicates that the "show queue" button has been clicked
         */
        void onQueueButtonClicked();
    }

    private PlaybackControls.Listener mPlaybackControlsListener = new PlaybackControls.Listener() {
        @Override
        public void onToggleQueue() {
            if (mCallbacks != null) {
                mCallbacks.onQueueButtonClicked();
            }
        }
    };

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, final ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_playback, container, false);
        mRootView = view.findViewById(R.id.playback_container);
        mQueue = view.findViewById(R.id.queue_list);

        initPlaybackControls(view.findViewById(R.id.playback_controls));
        initQueue(mQueue);
        initMetadataController(view);
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

    private void initPlaybackControls(PlaybackControls playbackControls) {
        mPlaybackControls = playbackControls;
        mPlaybackControls.setListener(mPlaybackControlsListener);
        mPlaybackControls.setAnimationViewGroup(mRootView);
    }

    private void initQueue(PagedListView queueList) {
        RecyclerView recyclerView = queueList.getRecyclerView();
        recyclerView.setVerticalFadingEdgeEnabled(true);
        recyclerView.setFadingEdgeLength(getResources()
                .getDimensionPixelSize(R.dimen.car_padding_4));
        mQueueAdapter = new QueueItemsAdapter(getContext(), mQueueItemsProvider);
        queueList.setAdapter(mQueueAdapter);
    }

    private void initMetadataController(View view) {
        ImageView albumArt = view.findViewById(R.id.album_art);
        TextView title = view.findViewById(R.id.title);
        TextView subtitle = view.findViewById(R.id.subtitle);
        SeekBar seekbar = view.findViewById(R.id.seek_bar);
        TextView time = view.findViewById(R.id.time);
        mMetadataController = new MetadataController(title, subtitle, time, seekbar, albumArt);
    }

    @Override
    public void onStart() {
        super.onStart();
        mModel = mCallbacks.getPlaybackModel();
        mMetadataController.setModel(mModel);
        mPlaybackControls.setModel(mModel);
        mModel.registerObserver(mPlaybackObserver);
    }

    @Override
    public void onStop() {
        super.onStop();
        mModel.unregisterObserver(mPlaybackObserver);
        mMetadataController.setModel(null);
        mPlaybackControls.setModel(null);
        mModel = null;
    }

    /**
     * Hides or shows the playback queue
     */
    public void toggleQueueVisibility() {
        mQueueIsVisible = !mQueueIsVisible;
        mPlaybackControls.setQueueVisible(mQueueIsVisible);

        Transition transition = TransitionInflater.from(getContext()).inflateTransition(
                mQueueIsVisible ? R.transition.queue_in : R.transition.queue_out);
        transition.addListener(new TransitionListenerAdapter() {

            @Override
            public void onTransitionStart(Transition transition) {
                super.onTransitionStart(transition);
                mUpdatesPaused = true;
                mMetadataController.pauseUpdates();
            }

            @Override
            public void onTransitionEnd(Transition transition) {
                mUpdatesPaused = false;
                if (mNeedsStateUpdate) {
                    updateState();
                }
                mMetadataController.resumeUpdates();
                mQueue.getRecyclerView().scrollToPosition(0);
            }
        });
        TransitionManager.beginDelayedTransition(mRootView, transition);
        ConstraintSet constraintSet = new ConstraintSet();
        constraintSet.clone(mRootView.getContext(),
                mQueueIsVisible ? R.layout.fragment_playback_with_queue : R.layout.fragment_playback);
        constraintSet.applyTo(mRootView);
    }

    private void updateState() {
        if (mUpdatesPaused) {
            mNeedsStateUpdate = true;
            return;
        }
        mNeedsStateUpdate = false;
        mQueueItems = mModel.getQueue().stream()
                .filter(item -> item.getTitle() != null)
                .collect(Collectors.toList());
        mQueueAdapter.refresh();
    }

    private void updateAccentColor() {
        int defaultColor = getResources().getColor(android.R.color.background_dark, null);
        MediaSource mediaSource = mModel.getMediaSource();
        int color = mediaSource == null ? defaultColor : mediaSource.getAccentColor(defaultColor);
        // TODO: Update queue color
    }

    private void onQueueItemClicked(MediaItemMetadata item) {
        mModel.onSkipToQueueItem(item.getQueueId());
    }

    /**
     * Collapses the playback controls.
     */
    public void closeOverflowMenu() {
        mPlaybackControls.close();
    }
}
