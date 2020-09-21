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

package com.android.car.media.common;

import android.annotation.IntDef;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.media.MediaMetadata;
import android.media.Rating;
import android.media.session.MediaController;
import android.media.session.MediaController.TransportControls;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.media.session.PlaybackState.Actions;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Collectors;

/**
 * Wrapper of {@link MediaSession}. It provides access to media session events and extended
 * information on the currently playing item metadata.
 */
public class PlaybackModel {
    private static final String TAG = "PlaybackModel";

    private static final String ACTION_SET_RATING =
            "com.android.car.media.common.ACTION_SET_RATING";
    private static final String EXTRA_SET_HEART = "com.android.car.media.common.EXTRA_SET_HEART";

    private final Handler mHandler = new Handler();
    @Nullable
    private final Context mContext;
    private final List<PlaybackObserver> mObservers = new ArrayList<>();
    private MediaController mMediaController;
    private MediaSource mMediaSource;
    private boolean mIsStarted;

    /**
     * An observer of this model
     */
    public abstract static class PlaybackObserver {
        /**
         * Called whenever the playback state of the current media item changes.
         */
        protected void onPlaybackStateChanged() {};

        /**
         * Called when the top source media app changes.
         */
        protected void onSourceChanged() {};

        /**
         * Called when the media item being played changes.
         */
        protected void onMetadataChanged() {};
    }

    private MediaController.Callback mCallback = new MediaController.Callback() {
        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "onPlaybackStateChanged: " + state);
            }
            PlaybackModel.this.notify(PlaybackObserver::onPlaybackStateChanged);
        }

        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "onMetadataChanged: " + metadata);
            }
            PlaybackModel.this.notify(PlaybackObserver::onMetadataChanged);
        }
    };

    /**
     * Creates a {@link PlaybackModel}
     */
    public PlaybackModel(@NonNull Context context) {
       this(context, null);
    }

    /**
     * Creates a {@link PlaybackModel} wrapping to the given media controller
     */
    public PlaybackModel(@NonNull Context context, @Nullable MediaController controller) {
        mContext = context;
        changeMediaController(controller);
    }

    /**
     * Sets the {@link MediaController} wrapped by this model.
     */
    public void setMediaController(@Nullable MediaController mediaController) {
        changeMediaController(mediaController);
    }

    private void changeMediaController(@Nullable MediaController mediaController) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "New media controller: " + (mediaController != null
                    ? mediaController.getPackageName() : null));
        }
        if ((mediaController == null && mMediaController == null)
                || (mediaController != null && mMediaController != null
                && mediaController.getPackageName().equals(mMediaController.getPackageName()))) {
            // If no change, do nothing.
            return;
        }
        if (mMediaController != null) {
            mMediaController.unregisterCallback(mCallback);
        }
        mMediaController = mediaController;
        mMediaSource = mMediaController != null
            ? new MediaSource(mContext, mMediaController.getPackageName()) : null;
        if (mMediaController != null && mIsStarted) {
            mMediaController.registerCallback(mCallback);
        }
        if (mIsStarted) {
            notify(PlaybackObserver::onSourceChanged);
        }
    }

    /**
     * Starts following changes on the playback state of the given source. If any changes happen,
     * all observers registered through {@link #registerObserver(PlaybackObserver)} will be
     * notified.
     */
    private void start() {
        if (mMediaController != null) {
            mMediaController.registerCallback(mCallback);
        }
        mIsStarted = true;
    }

    /**
     * Stops following changes on the list of active media sources.
     */
    private void stop() {
        if (mMediaController != null) {
            mMediaController.unregisterCallback(mCallback);
        }
        mIsStarted = false;
    }

    private void notify(Consumer<PlaybackObserver> notification) {
        mHandler.post(() -> {
            List<PlaybackObserver> observers = new ArrayList<>(mObservers);
            for (PlaybackObserver observer : observers) {
                notification.accept(observer);
            }
        });
    }

    /**
     * @return a {@link MediaSource} providing access to metadata of the currently playing media
     * source, or NULL if the media source has no active session.
     */
    @Nullable
    public MediaSource getMediaSource() {
        return mMediaSource;
    }

    /**
     * @return a {@link MediaController} that can be used to control this media source, or NULL
     * if the media source has no active session.
     */
    @Nullable
    public MediaController getMediaController() {
        return mMediaController;
    }

    /**
     * @return {@link Action} selected as the main action for the current media item, based on the
     * current playback state and the available actions reported by the media source.
     * Changes on this value will be notified through
     * {@link PlaybackObserver#onPlaybackStateChanged()}
     */
    @Action
    public int getMainAction() {
        return getMainAction(mMediaController != null ? mMediaController.getPlaybackState() : null);
    }

    /**
     * @return {@link MediaItemMetadata} of the currently selected media item in the media source.
     * Changes on this value will be notified through {@link PlaybackObserver#onMetadataChanged()}
     */
    @Nullable
    public MediaItemMetadata getMetadata() {
        if (mMediaController == null) {
            return null;
        }
        MediaMetadata metadata = mMediaController.getMetadata();
        if (metadata == null) {
            return null;
        }
        return new MediaItemMetadata(metadata);
    }

    /**
     * @return duration of the media item, in milliseconds. The current position in this duration
     * can be obtained by calling {@link #getProgress()}.
     * Changes on this value will be notified through {@link PlaybackObserver#onMetadataChanged()}
     */
    public long getMaxProgress() {
        if (mMediaController == null || mMediaController.getMetadata() == null) {
            return 0;
        } else {
            return mMediaController.getMetadata()
                    .getLong(MediaMetadata.METADATA_KEY_DURATION);
        }
    }

    /**
     * Sends a 'play' command to the media source
     */
    public void onPlay() {
        if (mMediaController != null) {
            mMediaController.getTransportControls().play();
        }
    }

    /**
     * Sends a 'skip previews' command to the media source
     */
    public void onSkipPreviews() {
        if (mMediaController != null) {
            mMediaController.getTransportControls().skipToPrevious();
        }
    }

    /**
     * Sends a 'skip next' command to the media source
     */
    public void onSkipNext() {
        if (mMediaController != null) {
            mMediaController.getTransportControls().skipToNext();
        }
    }

    /**
     * Sends a 'pause' command to the media source
     */
    public void onPause() {
        if (mMediaController != null) {
            mMediaController.getTransportControls().pause();
        }
    }

    /**
     * Sends a 'stop' command to the media source
     */
    public void onStop() {
        if (mMediaController != null) {
            mMediaController.getTransportControls().stop();
        }
    }

    /**
     * Sends a custom action to the media source
     * @param action identifier of the custom action
     * @param extras additional data to send to the media source.
     */
    public void onCustomAction(String action, Bundle extras) {
        if (mMediaController == null) return;
        TransportControls cntrl = mMediaController.getTransportControls();

        if (ACTION_SET_RATING.equals(action)) {
            boolean setHeart = extras != null && extras.getBoolean(EXTRA_SET_HEART, false);
            cntrl.setRating(Rating.newHeartRating(setHeart));
        } else {
            cntrl.sendCustomAction(action, extras);
        }

        mMediaController.getTransportControls().sendCustomAction(action, extras);
    }

    /**
     * Starts playing a given media item. This id corresponds to {@link MediaItemMetadata#getId()}.
     */
    public void onPlayItem(String mediaItemId) {
        if (mMediaController != null) {
            mMediaController.getTransportControls().playFromMediaId(mediaItemId, null);
        }
    }

    /**
     * Skips to a particular item in the media queue. This id is {@link MediaItemMetadata#mQueueId}
     * of the items obtained through {@link #getQueue()}.
     */
    public void onSkipToQueueItem(long queueId) {
        if (mMediaController != null) {
            mMediaController.getTransportControls().skipToQueueItem(queueId);
        }
    }

    /**
     * Prepares the current media source for playback.
     */
    public void onPrepare() {
        if (mMediaController != null) {
            mMediaController.getTransportControls().prepare();
        }
    }

    /**
     * Possible main actions.
     */
    @IntDef({ACTION_PLAY, ACTION_STOP, ACTION_PAUSE, ACTION_DISABLED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Action {}

    /** Main action is disabled. The source can't play media at this time */
    public static final int ACTION_DISABLED = 0;
    /** Start playing */
    public static final int ACTION_PLAY = 1;
    /** Stop playing */
    public static final int ACTION_STOP = 2;
    /** Pause playing */
    public static final int ACTION_PAUSE = 3;

    @Action
    private static int getMainAction(PlaybackState state) {
        if (state == null) {
            return ACTION_DISABLED;
        }

        @Actions long actions = state.getActions();
        int stopAction = ACTION_DISABLED;
        if ((actions & (PlaybackState.ACTION_PAUSE | PlaybackState.ACTION_PLAY_PAUSE)) != 0) {
            stopAction = ACTION_PAUSE;
        } else if ((actions & PlaybackState.ACTION_STOP) != 0) {
            stopAction = ACTION_STOP;
        }

        switch (state.getState()) {
            case PlaybackState.STATE_PLAYING:
            case PlaybackState.STATE_BUFFERING:
            case PlaybackState.STATE_CONNECTING:
            case PlaybackState.STATE_FAST_FORWARDING:
            case PlaybackState.STATE_REWINDING:
            case PlaybackState.STATE_SKIPPING_TO_NEXT:
            case PlaybackState.STATE_SKIPPING_TO_PREVIOUS:
            case PlaybackState.STATE_SKIPPING_TO_QUEUE_ITEM:
                return stopAction;
            case PlaybackState.STATE_STOPPED:
            case PlaybackState.STATE_PAUSED:
            case PlaybackState.STATE_NONE:
                return ACTION_PLAY;
            case PlaybackState.STATE_ERROR:
                return ACTION_DISABLED;
            default:
                Log.w(TAG, String.format("Unknown PlaybackState: %d", state.getState()));
                return ACTION_DISABLED;
        }
    }

    /**
     * @return the current playback progress, in milliseconds. This is a value between 0 and
     * {@link #getMaxProgress()} or PROGRESS_UNKNOWN of the current position is unknown.
     */
    public long getProgress() {
        if (mMediaController == null) {
            return 0;
        }
        PlaybackState state = mMediaController.getPlaybackState();
        if (state == null) {
            return 0;
        }
        if (state.getPosition() == PlaybackState.PLAYBACK_POSITION_UNKNOWN) {
            return PlaybackState.PLAYBACK_POSITION_UNKNOWN;
        }
        long timeDiff = SystemClock.elapsedRealtime() - state.getLastPositionUpdateTime();
        float speed = state.getPlaybackSpeed();
        if (state.getState() == PlaybackState.STATE_PAUSED
                || state.getState() == PlaybackState.STATE_STOPPED) {
            // This guards against apps who don't keep their playbackSpeed to spec (b/62375164)
            speed = 0f;
        }
        long posDiff = (long) (timeDiff * speed);
        return Math.min(posDiff + state.getPosition(), getMaxProgress());
    }

    /**
     * @return true if the current media source is playing a media item. Changes on this value
     * would be notified through {@link PlaybackObserver#onPlaybackStateChanged()}
     */
    public boolean isPlaying() {
        return mMediaController != null
                && mMediaController.getPlaybackState() != null
                && mMediaController.getPlaybackState().getState() == PlaybackState.STATE_PLAYING;
    }

    /**
     * Registers an observer to be notified of media events. If the model is not started yet it
     * will start right away. If the model was already started, the observer will receive an
     * immediate {@link PlaybackObserver#onSourceChanged()} event.
     */
    public void registerObserver(PlaybackObserver observer) {
        mObservers.add(observer);
        if (!mIsStarted) {
            start();
        } else {
            observer.onSourceChanged();
        }
    }

    /**
     * Unregisters an observer previously registered using
     * {@link #registerObserver(PlaybackObserver)}. There are no other observers the model will
     * stop tracking changes right away.
     */
    public void unregisterObserver(PlaybackObserver observer) {
        mObservers.remove(observer);
        if (mObservers.isEmpty() && mIsStarted) {
            stop();
        }
    }

    /**
     * @return true if the media source supports skipping to next item. Changes on this value
     * will be notified through {@link PlaybackObserver#onPlaybackStateChanged()}
     */
    public boolean isSkipNextEnabled() {
        return mMediaController != null
                && mMediaController.getPlaybackState() != null
                && (mMediaController.getPlaybackState().getActions()
                    & PlaybackState.ACTION_SKIP_TO_NEXT) != 0;
    }

    /**
     * @return true if the media source supports skipping to previous item. Changes on this value
     * will be notified through {@link PlaybackObserver#onPlaybackStateChanged()}
     */
    public boolean isSkipPreviewsEnabled() {
        return mMediaController != null
                && mMediaController.getPlaybackState() != null
                && (mMediaController.getPlaybackState().getActions()
                    & PlaybackState.ACTION_SKIP_TO_PREVIOUS) != 0;
    }

    /**
     * @return true if the media source is buffering. Changes on this value would be notified
     * through {@link PlaybackObserver#onPlaybackStateChanged()}
     */
    public boolean isBuffering() {
        return mMediaController != null
                && mMediaController.getPlaybackState() != null
                && mMediaController.getPlaybackState().getState() == PlaybackState.STATE_BUFFERING;
    }

    /**
     * @return a human readable description of the error that cause the media source to be in a
     * non-playable state, or null if there is no error. Changes on this value will be notified
     * through {@link PlaybackObserver#onPlaybackStateChanged()}
     */
    @Nullable
    public CharSequence getErrorMessage() {
        return mMediaController != null && mMediaController.getPlaybackState() != null
                ? mMediaController.getPlaybackState().getErrorMessage()
                : null;
    }

    /**
     * @return a sorted list of {@link MediaItemMetadata} corresponding to the queue of media items
     * as reported by the media source. Changes on this value will be notified through
     * {@link PlaybackObserver#onPlaybackStateChanged()}.
     */
    @NonNull
    public List<MediaItemMetadata> getQueue() {
        if (mMediaController == null) {
            return new ArrayList<>();
        }
        List<MediaSession.QueueItem> items = mMediaController.getQueue();
        if (items != null) {
            return items.stream()
                    .filter(item -> item.getDescription() != null
                        && item.getDescription().getTitle() != null)
                    .map(MediaItemMetadata::new)
                    .collect(Collectors.toList());
        } else {
            return new ArrayList<>();
        }
    }

    /**
     * @return the title of the queue or NULL if not available.
     */
    @Nullable
    public CharSequence getQueueTitle() {
        if (mMediaController == null) {
            return null;
        }
        return mMediaController.getQueueTitle();
    }

    /**
     * @return queue id of the currently playing queue item, or
     * {@link MediaSession.QueueItem#UNKNOWN_ID} if none of the items is currently playing.
     */
    public long getActiveQueueItemId() {
        PlaybackState playbackState = mMediaController.getPlaybackState();
        if (playbackState == null) return MediaSession.QueueItem.UNKNOWN_ID;
        return playbackState.getActiveQueueItemId();
    }

    /**
     * @return true if the media queue is not empty. Detailed information can be obtained by
     * calling to {@link #getQueue()}. Changes on this value will be notified through
     * {@link PlaybackObserver#onPlaybackStateChanged()}.
     */
    public boolean hasQueue() {
        if (mMediaController == null) {
            return false;
        }
        List<MediaSession.QueueItem> items = mMediaController.getQueue();
        return items != null && !items.isEmpty();
    }

    private @Nullable CustomPlaybackAction getRatingAction() {
        PlaybackState playbackState = mMediaController.getPlaybackState();
        if (playbackState == null) return null;

        long stdActions = playbackState.getActions();
        if ((stdActions & PlaybackState.ACTION_SET_RATING) == 0) return null;

        int ratingType = mMediaController.getRatingType();
        if (ratingType != Rating.RATING_HEART) return null;

        MediaMetadata metadata = mMediaController.getMetadata();
        boolean hasHeart = false;
        if (metadata != null) {
            Rating rating = metadata.getRating(MediaMetadata.METADATA_KEY_USER_RATING);
            hasHeart = rating != null && rating.hasHeart();
        }

        int iconResource = hasHeart ? R.drawable.ic_star_filled : R.drawable.ic_star_empty;
        Drawable icon = mContext.getResources().getDrawable(iconResource, null);
        Bundle extras = new Bundle();
        extras.putBoolean(EXTRA_SET_HEART, !hasHeart);
        return new CustomPlaybackAction(icon, ACTION_SET_RATING, extras);
    }

    /**
     * @return a sorted list of custom actions, as reported by the media source. Changes on this
     * value will be notified through
     * {@link PlaybackObserver#onPlaybackStateChanged()}.
     */
    public List<CustomPlaybackAction> getCustomActions() {
        List<CustomPlaybackAction> actions = new ArrayList<>();
        if (mMediaController == null) return actions;
        PlaybackState playbackState = mMediaController.getPlaybackState();
        if (playbackState == null) return actions;

        CustomPlaybackAction ratingAction = getRatingAction();
        if (ratingAction != null) actions.add(ratingAction);

        for (PlaybackState.CustomAction action : playbackState.getCustomActions()) {
            Resources resources = getResourcesForPackage(mMediaController.getPackageName());
            if (resources == null) {
                actions.add(null);
            } else {
                // the resources may be from another package. we need to update the configuration
                // using the context from the activity so we get the drawable from the correct DPI
                // bucket.
                resources.updateConfiguration(mContext.getResources().getConfiguration(),
                        mContext.getResources().getDisplayMetrics());
                Drawable icon = resources.getDrawable(action.getIcon(), null);
                actions.add(new CustomPlaybackAction(icon, action.getAction(), action.getExtras()));
            }
        }
        return actions;
    }

    private Resources getResourcesForPackage(String packageName) {
        try {
            return mContext.getPackageManager().getResourcesForApplication(packageName);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Unable to get resources for " + packageName);
            return null;
        }
    }
}
