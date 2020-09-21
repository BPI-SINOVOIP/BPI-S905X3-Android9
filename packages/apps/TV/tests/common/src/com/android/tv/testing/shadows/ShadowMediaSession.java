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
 * limitations under the License
 */

package com.android.tv.testing.shadows;

import android.app.PendingIntent;
import android.content.Context;
import android.media.MediaMetadata;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

/** Shadow {@link MediaSession}. */
@Implements(MediaSession.class)
public class ShadowMediaSession {

    public MediaSession.Callback mCallback;
    public PendingIntent mMediaButtonReceiver;
    public PendingIntent mSessionActivity;
    public PlaybackState mPlaybackState;
    public MediaMetadata mMediaMetadata;
    public int mFlags;
    public boolean mActive;
    public boolean mReleased;

    /** Stand-in for the MediaSession constructor with the same parameters. */
    public void __constructor__(Context context, String tag, int userID) {
        // This empty method prevents the real MediaSession constructor from being called.
    }

    @Implementation
    public void setCallback(MediaSession.Callback callback) {
        mCallback = callback;
    }

    @Implementation
    public void setMediaButtonReceiver(PendingIntent mbr) {
        mMediaButtonReceiver = mbr;
    }

    @Implementation
    public void setSessionActivity(PendingIntent activity) {
        mSessionActivity = activity;
    }

    @Implementation
    public void setPlaybackState(PlaybackState state) {
        mPlaybackState = state;
    }

    @Implementation
    public void setMetadata(MediaMetadata metadata) {
        mMediaMetadata = metadata;
    }

    @Implementation
    public void setFlags(int flags) {
        mFlags = flags;
    }

    @Implementation
    public boolean isActive() {
        return mActive;
    }

    @Implementation
    public void setActive(boolean active) {
        mActive = active;
    }

    @Implementation
    public void release() {
        mReleased = true;
    }
}
