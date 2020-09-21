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

import android.annotation.Nullable;
import android.content.Context;
import android.content.SharedPreferences;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.Handler;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * This is an abstractions over {@link MediaSessionManager} that provides information about the
 * currently "active" media session.
 * <p>
 * It automatically determines the foreground media app (the one that would normally
 * receive playback events) and exposes metadata and events from such app, or when a different app
 * becomes foreground.
 * <p>
 * This requires the android.Manifest.permission.MEDIA_CONTENT_CONTROL permission to be held by the
 * calling app.
 */
public class ActiveMediaSourceManager {
    private static final String TAG = "ActiveSourceManager";

    private static final String PLAYBACK_MODEL_SHARED_PREFS =
            "com.android.car.media.PLAYBACK_MODEL";
    private static final String PLAYBACK_MODEL_ACTIVE_PACKAGE_NAME_KEY =
            "active_packagename";

    private final MediaSessionManager mMediaSessionManager;
    private final Handler mHandler = new Handler();
    private final Context mContext;
    private final List<Observer> mObservers = new ArrayList<>();
    private final MediaSessionUpdater mMediaSessionUpdater = new MediaSessionUpdater();
    private final SharedPreferences mSharedPreferences;
    @Nullable
    private MediaController mMediaController;
    private boolean mIsStarted;

    /**
     * Temporary work-around to bug b/76017849.
     * MediaSessionManager is not notifying media session priority changes.
     * As a work-around we subscribe to playback state changes on all controllers to detect
     * potential priority changes.
     * This might cause a few unnecessary checks, but selecting the top-most controller is a
     * cheap operation.
     */
    private class MediaSessionUpdater {
        private List<MediaController> mControllers = new ArrayList<>();

        private MediaController.Callback mCallback = new MediaController.Callback() {
            @Override
            public void onPlaybackStateChanged(PlaybackState state) {
                selectMediaController(mMediaSessionManager.getActiveSessions(null));
            }

            @Override
            public void onSessionDestroyed() {
                selectMediaController(mMediaSessionManager.getActiveSessions(null));
            }
        };

        void setControllersByPackageName(List<MediaController> newControllers) {
            for (MediaController oldController : mControllers) {
                oldController.unregisterCallback(mCallback);
            }
            for (MediaController newController : newControllers) {
                newController.registerCallback(mCallback);
            }
            mControllers.clear();
            mControllers.addAll(newControllers);
        }
    }

    /**
     * An observer of this model
     */
    public interface Observer {
        /**
         * Called when the top source media app changes.
         */
        void onActiveSourceChanged();
    }

    private MediaSessionManager.OnActiveSessionsChangedListener mSessionChangeListener =
            this::selectMediaController;

    /**
     * Creates a {@link ActiveMediaSourceManager}. This instance is going to be inactive until
     * {@link #start()} method is invoked.
     */
    public ActiveMediaSourceManager(Context context) {
        mContext = context;
        mMediaSessionManager = mContext.getSystemService(MediaSessionManager.class);
        mSharedPreferences = mContext.getSharedPreferences(PLAYBACK_MODEL_SHARED_PREFS,
                Context.MODE_PRIVATE);
    }

    /**
     * Selects one of the provided controllers as the "currently playing" one.
     */
    private void selectMediaController(List<MediaController> controllers) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            dump("Selecting a media controller from: ", controllers);
        }
        changeMediaController(getTopMostController(controllers));
        mMediaSessionUpdater.setControllersByPackageName(controllers);
    }

    private void dump(String title, List<MediaController> controllers) {
        Log.d(TAG, title + " (total: " + controllers.size() + ")");
        for (MediaController controller : controllers) {
            String stateName = getStateName(controller.getPlaybackState() != null
                    ? controller.getPlaybackState().getState()
                    : PlaybackState.STATE_NONE);
            Log.d(TAG, String.format("\t%s: %s",
                    controller.getPackageName(),
                    stateName));
        }
    }

    private String getStateName(@PlaybackState.State int state) {
        switch (state) {
            case PlaybackState.STATE_NONE:
                return "NONE";
            case PlaybackState.STATE_STOPPED:
                return "STOPPED";
            case PlaybackState.STATE_PAUSED:
                return "PAUSED";
            case PlaybackState.STATE_PLAYING:
                return "PLAYING";
            case PlaybackState.STATE_FAST_FORWARDING:
                return "FORWARDING";
            case PlaybackState.STATE_REWINDING:
                return "REWINDING";
            case PlaybackState.STATE_BUFFERING:
                return "BUFFERING";
            case PlaybackState.STATE_ERROR:
                return "ERROR";
            case PlaybackState.STATE_CONNECTING:
                return "CONNECTING";
            case PlaybackState.STATE_SKIPPING_TO_PREVIOUS:
                return "SKIPPING_TO_PREVIOUS";
            case PlaybackState.STATE_SKIPPING_TO_NEXT:
                return "SKIPPING_TO_NEXT";
            case PlaybackState.STATE_SKIPPING_TO_QUEUE_ITEM:
                return "SKIPPING_TO_QUEUE_ITEM";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * @return the controller most likely to be the currently active one, out of the list of
     * active controllers repoted by {@link MediaSessionManager}. It does so by picking the first
     * one (in order of priority) which an active state as reported by
     * {@link MediaController#getPlaybackState()}
     */
    private MediaController getTopMostController(List<MediaController> controllers) {
        if (controllers != null && controllers.size() > 0) {
            for (MediaController candidate : controllers) {
                @PlaybackState.State int state = candidate.getPlaybackState() != null
                        ? candidate.getPlaybackState().getState()
                        : PlaybackState.STATE_NONE;
                if (state == PlaybackState.STATE_BUFFERING
                        || state == PlaybackState.STATE_CONNECTING
                        || state == PlaybackState.STATE_FAST_FORWARDING
                        || state == PlaybackState.STATE_PLAYING
                        || state == PlaybackState.STATE_REWINDING
                        || state == PlaybackState.STATE_SKIPPING_TO_NEXT
                        || state == PlaybackState.STATE_SKIPPING_TO_PREVIOUS
                        || state == PlaybackState.STATE_SKIPPING_TO_QUEUE_ITEM) {
                    return candidate;
                }
            }
            // If no source is active, we go for the last known source
            String packageName = getLastKnownActivePackageName();
            if (packageName != null) {
                for (MediaController candidate : controllers) {
                    if (candidate.getPackageName().equals(packageName)) {
                        return candidate;
                    }
                }
            }
            return controllers.get(0);
        }
        return null;
    }

    private void changeMediaController(MediaController mediaController) {
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
        mMediaController = mediaController;
        setLastKnownActivePackageName(mMediaController != null
                ? mMediaController.getPackageName()
                : null);
        notify(Observer::onActiveSourceChanged);
    }

    /**
     * Starts following changes on the list of active media sources. If any changes happen, all
     * observers registered through {@link #registerObserver(Observer)} will be notified.
     * <p>
     * Calling this method might cause an immediate {@link Observer#onActiveSourceChanged()}
     * event in case the current media source is different than the last known one.
     */
    private void start() {
        mMediaSessionManager.addOnActiveSessionsChangedListener(mSessionChangeListener, null);
        selectMediaController(mMediaSessionManager.getActiveSessions(null));
        mIsStarted = true;
    }

    /**
     * Stops following changes on the list of active media sources. This method could cause an
     * immediate {@link PlaybackModel.PlaybackObserver#onSourceChanged()} event if a media source
     * was already connected.
     */
    private void stop() {
        mMediaSessionUpdater.setControllersByPackageName(new ArrayList<>());
        mMediaSessionManager.removeOnActiveSessionsChangedListener(mSessionChangeListener);
        changeMediaController(null);
        mIsStarted = false;
    }

    private void notify(Consumer<Observer> notification) {
        mHandler.post(() -> {
            List<Observer> observers = new ArrayList<>(mObservers);
            for (Observer observer : observers) {
                notification.accept(observer);
            }
        });
    }

    /**
     * @return a {@link MediaController} providing access to metadata of the currently playing media
     * source, or NULL if no media source has an active session. Changes on this value will
     * be notified through {@link Observer#onActiveSourceChanged()}
     */
    @Nullable
    public MediaController getMediaController() {
        return mIsStarted
                ? mMediaController
                : getTopMostController(mMediaSessionManager.getActiveSessions(null));
    }

    /**
     * Registers an observer to be notified of media events. If the model is not started yet it
     * will start right away. If the model was already started, the observer will receive an
     * immediate {@link Observer#onActiveSourceChanged()} event.
     */
    public void registerObserver(Observer observer) {
        mObservers.add(observer);
        if (!mIsStarted) {
            start();
        } else {
            observer.onActiveSourceChanged();
        }
    }

    /**
     * Unregisters an observer previously registered using
     * {@link #registerObserver(Observer)}. There are no other observers the model will
     * stop tracking changes right away.
     */
    public void unregisterObserver(Observer observer) {
        mObservers.remove(observer);
        if (mObservers.isEmpty() && mIsStarted) {
            stop();
        }
    }

    private String getLastKnownActivePackageName() {
        return mSharedPreferences.getString(PLAYBACK_MODEL_ACTIVE_PACKAGE_NAME_KEY, null);
    }

    private void setLastKnownActivePackageName(String packageName) {
        mSharedPreferences.edit()
                .putString(PLAYBACK_MODEL_ACTIVE_PACKAGE_NAME_KEY, packageName)
                .apply();
    }

    /**
     * Returns the {@link MediaController} corresponding to the given package name, or NULL if
     * no active session exists for it.
     */
    public @Nullable MediaController getControllerForPackage(String packageName) {
        List<MediaController> controllers = mMediaSessionManager.getActiveSessions(null);
        for (MediaController controller : controllers) {
            if (controller.getPackageName().equals(packageName)) {
                return controller;
            }
        }
        return null;
    }

    /**
     * Returns true if the given package name corresponds to the top most media source.
     */
    public boolean isPlaying(String packageName) {
        return mMediaController != null && mMediaController.getPackageName().equals(packageName);
    }
}
