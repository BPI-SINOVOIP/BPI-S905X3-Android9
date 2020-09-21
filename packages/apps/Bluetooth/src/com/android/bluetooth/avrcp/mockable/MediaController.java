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

    public MediaController(@NonNull android.media.session.MediaController delegate) {
        mDelegate = delegate;
        mTransportDelegate = delegate.getTransportControls();
        mTransportControls = new TransportControls();
    }

    public MediaController(Context context, MediaSession.Token token) {
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

    public boolean dispatchMediaButtonEvent(@NonNull KeyEvent keyEvent) {
        return mDelegate.dispatchMediaButtonEvent(keyEvent);
    }

    @Nullable
    public PlaybackState getPlaybackState() {
        return mDelegate.getPlaybackState();
    }

    @Nullable
    public MediaMetadata getMetadata() {
        return mDelegate.getMetadata();
    }

    @Nullable
    public List<MediaSession.QueueItem> getQueue() {
        return mDelegate.getQueue();
    }

    @Nullable
    public CharSequence getQueueTitle() {
        return mDelegate.getQueueTitle();
    }

    @Nullable
    public Bundle getExtras() {
        return mDelegate.getExtras();
    }

    public int getRatingType() {
        return mDelegate.getRatingType();
    }

    public long getFlags() {
        return mDelegate.getFlags();
    }

    @Nullable
    public android.media.session.MediaController.PlaybackInfo getPlaybackInfo() {
        return mDelegate.getPlaybackInfo();
    }

    @Nullable
    public PendingIntent getSessionActivity() {
        return mDelegate.getSessionActivity();
    }

    @NonNull
    public MediaSession.Token getSessionToken() {
        return mDelegate.getSessionToken();
    }

    public void setVolumeTo(int value, int flags) {
        mDelegate.setVolumeTo(value, flags);
    }

    public void adjustVolume(int direction, int flags) {
        mDelegate.adjustVolume(direction, flags);
    }

    public void registerCallback(@NonNull Callback callback) {
        //TODO(apanicke): Add custom callback struct to be able to analyze and
        // delegate callbacks
        mDelegate.registerCallback(callback);
    }

    public void registerCallback(@NonNull Callback callback, @Nullable Handler handler) {
        mDelegate.registerCallback(callback, handler);
    }

    public void unregisterCallback(@NonNull Callback callback) {
        mDelegate.unregisterCallback(callback);
    }

    public void sendCommand(@NonNull String command, @Nullable Bundle args,
            @Nullable ResultReceiver cb) {
        mDelegate.sendCommand(command, args, cb);
    }

    public String getPackageName() {
        return mDelegate.getPackageName();
    }

    public String getTag() {
        return mDelegate.getTag();
    }

    public boolean controlsSameSession(MediaController other) {
        return mDelegate.controlsSameSession(other.getWrappedInstance());
    }

    public boolean controlsSameSession(android.media.session.MediaController other) {
        return mDelegate.controlsSameSession(other);
    }

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

    @Override
    public String toString() {
        MediaMetadata data = getMetadata();
        MediaDescription desc = (data == null) ? null : data.getDescription();
        return "MediaController (" + getPackageName() + "@" + Integer.toHexString(
                mDelegate.hashCode()) + ") " + desc;
    }

    public abstract static class Callback extends android.media.session.MediaController.Callback {}

    public class TransportControls {

        public void prepare() {
            mTransportDelegate.prepare();
        }

        public void prepareFromMediaId(String mediaId, Bundle extras) {
            mTransportDelegate.prepareFromMediaId(mediaId, extras);
        }

        public void prepareFromSearch(String query, Bundle extras) {
            mTransportDelegate.prepareFromSearch(query, extras);
        }

        public void prepareFromUri(Uri uri, Bundle extras) {
            mTransportDelegate.prepareFromUri(uri, extras);
        }

        public void play() {
            mTransportDelegate.play();
        }

        public void playFromMediaId(String mediaId, Bundle extras) {
            mTransportDelegate.playFromMediaId(mediaId, extras);
        }

        public void playFromSearch(String query, Bundle extras) {
            mTransportDelegate.playFromSearch(query, extras);
        }

        public void playFromUri(Uri uri, Bundle extras) {
            mTransportDelegate.playFromUri(uri, extras);
        }

        public void skipToQueueItem(long id) {
            mTransportDelegate.skipToQueueItem(id);
        }

        public void pause() {
            mTransportDelegate.pause();
        }

        public void stop() {
            mTransportDelegate.stop();
        }

        public void seekTo(long pos) {
            mTransportDelegate.seekTo(pos);
        }

        public void fastForward() {
            mTransportDelegate.fastForward();
        }

        public void skipToNext() {
            mTransportDelegate.skipToNext();
        }

        public void rewind() {
            mTransportDelegate.rewind();
        }

        public void skipToPrevious() {
            mTransportDelegate.skipToPrevious();
        }

        public void setRating(Rating rating) {
            mTransportDelegate.setRating(rating);
        }

        public void sendCustomAction(@NonNull PlaybackState.CustomAction customAction,
                @Nullable Bundle args) {
            mTransportDelegate.sendCustomAction(customAction, args);
        }

        public void sendCustomAction(@NonNull String action, @Nullable Bundle args) {
            mTransportDelegate.sendCustomAction(action, args);
        }
    }
}

