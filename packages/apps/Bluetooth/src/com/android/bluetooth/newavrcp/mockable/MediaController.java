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

package com.android.bluetooth.avrcp;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.PendingIntent;
import android.content.Context;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.Rating;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.ResultReceiver;
import android.view.KeyEvent;

import java.util.List;

/**
 * Provide a mockable interface in order to test classes that use MediaController.
 * We need this class due to the fact that the MediaController class is marked as final and
 * there is no way to currently mock final classes in Android. Once this is possible this class
 * can be deleted.
 */
public class MediaController {
    @NonNull public android.media.session.MediaController mDelegate;
    public android.media.session.MediaController.TransportControls mTransportDelegate;
    public TransportControls mTransportControls;

    MediaController(@NonNull android.media.session.MediaController delegate) {
        mDelegate = delegate;
        mTransportDelegate = delegate.getTransportControls();
        mTransportControls = new TransportControls();
    }

    MediaController(Context context, MediaSession.Token token) {
        mDelegate = new android.media.session.MediaController(context, token);
        mTransportDelegate = mDelegate.getTransportControls();
        mTransportControls = new TransportControls();
    }

    public android.media.session.MediaController getWrappedInstance() {
        return mDelegate;
    }

    @NonNull
    public TransportControls getTransportControls() {
        return mTransportControls;
    }

    /**
     * Wrapper for MediaController.dispatchMediaButtonEvent(KeyEvent keyEvent)
     */
    public boolean dispatchMediaButtonEvent(@NonNull KeyEvent keyEvent) {
        return mDelegate.dispatchMediaButtonEvent(keyEvent);
    }

    /**
     * Wrapper for MediaController.getPlaybackState()
     */
    @Nullable
    public PlaybackState getPlaybackState() {
        return mDelegate.getPlaybackState();
    }


    /**
     * Wrapper for MediaController.getMetadata()
     */
    @Nullable
    public MediaMetadata getMetadata() {
        return mDelegate.getMetadata();
    }

    /**
     * Wrapper for MediaController.getQueue()
     */
    @Nullable
    public List<MediaSession.QueueItem> getQueue() {
        return mDelegate.getQueue();
    }

    /**
     * Wrapper for MediaController.getQueueTitle()
     */
    @Nullable
    public CharSequence getQueueTitle() {
        return mDelegate.getQueueTitle();
    }

    /**
     * Wrapper for MediaController.getExtras()
     */
    @Nullable
    public Bundle getExtras() {
        return mDelegate.getExtras();
    }

    /**
     * Wrapper for MediaController.getRatingType()
     */
    public int getRatingType() {
        return mDelegate.getRatingType();
    }

    /**
     * Wrapper for MediaController.getFlags()
     */
    public long getFlags() {
        return mDelegate.getFlags();
    }

    /**
     * Wrapper for MediaController.getPlaybackInfo()
     */
    @Nullable
    public android.media.session.MediaController.PlaybackInfo getPlaybackInfo() {
        return mDelegate.getPlaybackInfo();
    }


    /**
     * Wrapper for MediaController.getSessionActivity()
     */
    @Nullable
    public PendingIntent getSessionActivity() {
        return mDelegate.getSessionActivity();
    }

    /**
     * Wrapper for MediaController.getSessionToken()
     */
    @NonNull
    public MediaSession.Token getSessionToken() {
        return mDelegate.getSessionToken();
    }

    /**
     * Wrapper for MediaController.setVolumeTo(int value, int flags)
     */
    public void setVolumeTo(int value, int flags) {
        mDelegate.setVolumeTo(value, flags);
    }

    /**
     * Wrapper for MediaController.adjustVolume(int direction, int flags)
     */
    public void adjustVolume(int direction, int flags) {
        mDelegate.adjustVolume(direction, flags);
    }

    /**
     * Wrapper for MediaController.registerCallback(Callback callback)
     */
    public void registerCallback(@NonNull Callback callback) {
        //TODO(apanicke): Add custom callback struct to be able to analyze and
        // delegate callbacks
        mDelegate.registerCallback(callback);
    }

    /**
     * Wrapper for MediaController.registerCallback(Callback callback, Handler handler)
     */
    public void registerCallback(@NonNull Callback callback, @Nullable Handler handler) {
        mDelegate.registerCallback(callback, handler);
    }

    /**
     * Wrapper for MediaController.unregisterCallback(Callback callback)
     */
    public void unregisterCallback(@NonNull Callback callback) {
        mDelegate.unregisterCallback(callback);
    }

    /**
     * Wrapper for MediaController.sendCommand(String command, Bundle args, ResultReceiver cb)
     */
    public void sendCommand(@NonNull String command, @Nullable Bundle args,
            @Nullable ResultReceiver cb) {
        mDelegate.sendCommand(command, args, cb);
    }

    /**
     * Wrapper for MediaController.getPackageName()
     */
    public String getPackageName() {
        return mDelegate.getPackageName();
    }

    /**
     * Wrapper for MediaController.getTag()
     */
    public String getTag() {
        return mDelegate.getTag();
    }

    /**
     * Wrapper for MediaController.controlsSameSession(MediaController other)
     */
    public boolean controlsSameSession(MediaController other) {
        return mDelegate.controlsSameSession(other.getWrappedInstance());
    }

    /**
     * Wrapper for MediaController.controlsSameSession(MediaController other)
     */
    public boolean controlsSameSession(android.media.session.MediaController other) {
        return mDelegate.controlsSameSession(other);
    }

    /**
     * Wrapper for MediaController.equals(Object other)
     */
    @Override
    public boolean equals(Object o) {
        if (o instanceof android.media.session.MediaController) {
            return mDelegate.equals(o);
        } else if (o instanceof MediaController) {
            MediaController other = (MediaController) o;
            return mDelegate.equals(other.mDelegate);
        }
        return false;
    }

    /**
     * Wrapper for MediaController.toString()
     */
    @Override
    public String toString() {
        MediaMetadata data = getMetadata();
        MediaDescription desc = (data == null) ? null : data.getDescription();
        return "MediaController (" + getPackageName() + "@" + Integer.toHexString(
                mDelegate.hashCode()) + ") " + desc;
    }

    /**
     * Wrapper for MediaController.Callback
     */
    public abstract static class Callback extends android.media.session.MediaController.Callback {}

    /**
     * Wrapper for MediaController.TransportControls
     */
    public class TransportControls {

        /**
         * Wrapper for MediaController.TransportControls.prepare()
         */
        public void prepare() {
            mTransportDelegate.prepare();
        }

        /**
         * Wrapper for MediaController.TransportControls.prepareFromMediaId()
         */
        public void prepareFromMediaId(String mediaId, Bundle extras) {
            mTransportDelegate.prepareFromMediaId(mediaId, extras);
        }

        /**
         * Wrapper for MediaController.TransportControls.prepareFromSearch()
         */
        public void prepareFromSearch(String query, Bundle extras) {
            mTransportDelegate.prepareFromSearch(query, extras);
        }

        /**
         * Wrapper for MediaController.TransportControls.prepareFromUri()
         */
        public void prepareFromUri(Uri uri, Bundle extras) {
            mTransportDelegate.prepareFromUri(uri, extras);
        }

        /**
         * Wrapper for MediaController.TransportControls.play()
         */
        public void play() {
            mTransportDelegate.play();
        }

        /**
         * Wrapper for MediaController.TransportControls.playFromMediaId()
         */
        public void playFromMediaId(String mediaId, Bundle extras) {
            mTransportDelegate.playFromMediaId(mediaId, extras);
        }

        /**
         * Wrapper for MediaController.TransportControls.playFromSearch()
         */
        public void playFromSearch(String query, Bundle extras) {
            mTransportDelegate.playFromSearch(query, extras);
        }

        /**
         * Wrapper for MediaController.TransportControls.playFromUri()
         */
        public void playFromUri(Uri uri, Bundle extras) {
            mTransportDelegate.playFromUri(uri, extras);
        }

        /**
         * Wrapper for MediaController.TransportControls.skipToQueueItem()
         */
        public void skipToQueueItem(long id) {
            mTransportDelegate.skipToQueueItem(id);
        }

        /**
         * Wrapper for MediaController.TransportControls.pause()
         */
        public void pause() {
            mTransportDelegate.pause();
        }

        /**
         * Wrapper for MediaController.TransportControls.stop()
         */
        public void stop() {
            mTransportDelegate.stop();
        }

        /**
         * Wrapper for MediaController.TransportControls.seekTo()
         */
        public void seekTo(long pos) {
            mTransportDelegate.seekTo(pos);
        }

        /**
         * Wrapper for MediaController.TransportControls.fastForward()
         */
        public void fastForward() {
            mTransportDelegate.fastForward();
        }

        /**
         * Wrapper for MediaController.TransportControls.skipToNext()
         */
        public void skipToNext() {
            mTransportDelegate.skipToNext();
        }

        /**
         * Wrapper for MediaController.TransportControls.rewind()
         */
        public void rewind() {
            mTransportDelegate.rewind();
        }

        /**
         * Wrapper for MediaController.TransportControls.skipToPrevious()
         */
        public void skipToPrevious() {
            mTransportDelegate.skipToPrevious();
        }

        /**
         * Wrapper for MediaController.TransportControls.setRating()
         */
        public void setRating(Rating rating) {
            mTransportDelegate.setRating(rating);
        }

        /**
         * Wrapper for MediaController.TransportControls.sendCustomAction()
         */
        public void sendCustomAction(@NonNull PlaybackState.CustomAction customAction,
                @Nullable Bundle args) {
            mTransportDelegate.sendCustomAction(customAction, args);
        }

        /**
         * Wrapper for MediaController.TransportControls.sendCustomAction()
         */
        public void sendCustomAction(@NonNull String action, @Nullable Bundle args) {
            mTransportDelegate.sendCustomAction(action, args);
        }
    }
}

