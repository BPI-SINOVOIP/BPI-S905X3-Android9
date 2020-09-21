/**
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.radio.audio;

import android.annotation.NonNull;
import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.os.RemoteException;
import android.support.v4.media.session.PlaybackStateCompat;
import android.util.Log;

import com.android.car.radio.platform.RadioManagerExt;
import com.android.car.radio.platform.RadioTunerExt;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

/**
 * Manages radio's audio stream.
 */
public class AudioStreamController {
    private static final String TAG = "BcRadioApp.AudioSCntrl";

    private final Object mLock = new Object();
    private final AudioManager mAudioManager;
    private final RadioTunerExt mRadioTunerExt;

    private final AudioFocusRequest mGainFocusReq;

    /**
     * Indicates that the app has *some* focus or a promise of it.
     *
     * It may be ducked, transiently lost or delayed.
     */
    private boolean mHasSomeFocus = false;

    private boolean mIsTuning = false;

    private int mCurrentPlaybackState = PlaybackStateCompat.STATE_NONE;
    private final List<IPlaybackStateListener> mPlaybackStateListeners = new ArrayList<>();

    public AudioStreamController(@NonNull Context context, @NonNull RadioManagerExt radioManager) {
        mAudioManager = Objects.requireNonNull(
                (AudioManager) context.getSystemService(Context.AUDIO_SERVICE));
        mRadioTunerExt = Objects.requireNonNull(radioManager.getRadioTunerExt());

        AudioAttributes playbackAttr = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                .build();
        mGainFocusReq = new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(playbackAttr)
                .setAcceptsDelayedFocusGain(true)
                .setWillPauseWhenDucked(true)
                .setOnAudioFocusChangeListener(this::onAudioFocusChange)
                .build();
    }

    /**
     * Add playback state listener.
     *
     * @param listener listener to add
     */
    public void addPlaybackStateListener(@NonNull IPlaybackStateListener listener) {
        synchronized (mLock) {
            mPlaybackStateListeners.add(Objects.requireNonNull(listener));
            try {
                listener.onPlaybackStateChanged(mCurrentPlaybackState);
            } catch (RemoteException e) {
                Log.e(TAG, "Couldn't notify new listener about current playback state", e);
            }
        }
    }

    /**
     * Remove playback state listener.
     *
     * @param listener listener to remove
     */
    public void removePlaybackStateListener(IPlaybackStateListener listener) {
        synchronized (mLock) {
            mPlaybackStateListeners.remove(listener);
        }
    }

    private void notifyPlaybackStateChangedLocked(@PlaybackStateCompat.State int state) {
        if (mCurrentPlaybackState == state) return;
        mCurrentPlaybackState = state;
        for (IPlaybackStateListener listener : mPlaybackStateListeners) {
            try {
                listener.onPlaybackStateChanged(state);
            } catch (RemoteException e) {
                Log.e(TAG, "Couldn't notify listener about playback state change", e);
            }
        }
    }

    private boolean requestAudioFocusLocked() {
        if (mHasSomeFocus) return true;
        int res = mAudioManager.requestAudioFocus(mGainFocusReq);
        if (res == AudioManager.AUDIOFOCUS_REQUEST_DELAYED) {
            Log.i(TAG, "Audio focus request is delayed");
            mHasSomeFocus = true;
            return true;
        }
        if (res != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            Log.w(TAG, "Couldn't obtain audio focus, res=" + res);
            return false;
        }

        Log.v(TAG, "Audio focus request succeeded");
        mHasSomeFocus = true;

        // we assume that audio focus was requested only when we mean to unmute
        if (!mRadioTunerExt.setMuted(false)) return false;

        return true;
    }

    private boolean abandonAudioFocusLocked() {
        if (!mHasSomeFocus) return true;
        if (!mRadioTunerExt.setMuted(true)) return false;

        int res = mAudioManager.abandonAudioFocusRequest(mGainFocusReq);
        if (res != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            Log.e(TAG, "Couldn't abandon audio focus, res=" + res);
            return false;
        }

        Log.v(TAG, "Audio focus abandoned");
        mHasSomeFocus = false;

        return true;
    }

    /**
     * Prepare playback for ongoing tune/scan operation.
     *
     * @param skipDirectionNext true if it's skipping to next station;
     *                          false if skipping to previous;
     *                          empty if tuning to arbitrary selector.
     */
    public boolean preparePlayback(Optional<Boolean> skipDirectionNext) {
        synchronized (mLock) {
            if (!requestAudioFocusLocked()) return false;

            int state = PlaybackStateCompat.STATE_CONNECTING;
            if (skipDirectionNext.isPresent()) {
                state = skipDirectionNext.get() ? PlaybackStateCompat.STATE_SKIPPING_TO_NEXT
                        : PlaybackStateCompat.STATE_SKIPPING_TO_PREVIOUS;
            }
            notifyPlaybackStateChangedLocked(state);

            mIsTuning = true;
            return true;
        }
    }

    /**
     * Notifies AudioStreamController that radio hardware is done with tune/scan operation.
     *
     * TODO(b/73950974): use callbacks, don't hardcode
     *
     * @see #preparePlayback
     */
    public void notifyProgramInfoChanged() {
        synchronized (mLock) {
            if (!mIsTuning) return;
            mIsTuning = false;
            notifyPlaybackStateChangedLocked(PlaybackStateCompat.STATE_PLAYING);
        }
    }

    /**
     * Request audio stream muted or unmuted.
     *
     * @param muted true, if audio stream should be muted, false if unmuted
     * @return true, if request has succeeded (maybe delayed)
     */
    public boolean requestMuted(boolean muted) {
        synchronized (mLock) {
            if (muted) {
                notifyPlaybackStateChangedLocked(PlaybackStateCompat.STATE_STOPPED);
                return abandonAudioFocusLocked();
            } else {
                if (!requestAudioFocusLocked()) return false;
                notifyPlaybackStateChangedLocked(PlaybackStateCompat.STATE_PLAYING);
                return true;
            }
        }
    }

    // TODO(b/73950974): depend on callbacks only
    public boolean isMuted() {
        return !mHasSomeFocus;
    }

    private void onAudioFocusChange(int focusChange) {
        Log.v(TAG, "onAudioFocusChange(" + focusChange + ")");

        synchronized (mLock) {
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_GAIN:
                    mHasSomeFocus = true;
                    // we assume that audio focus was requested only when we mean to unmute
                    mRadioTunerExt.setMuted(false);
                    break;
                case AudioManager.AUDIOFOCUS_LOSS:
                    Log.i(TAG, "Unexpected audio focus loss");
                    mHasSomeFocus = false;
                    mRadioTunerExt.setMuted(true);
                    notifyPlaybackStateChangedLocked(PlaybackStateCompat.STATE_STOPPED);
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    mRadioTunerExt.setMuted(true);
                    break;
                default:
                    Log.w(TAG, "Unexpected audio focus state: " + focusChange);
            }
        }
    }
}
