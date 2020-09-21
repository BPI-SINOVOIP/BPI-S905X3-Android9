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
package com.android.tv;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import com.android.tv.receiver.AudioCapabilitiesReceiver;
import com.android.tv.ui.TunableTvView;
import com.android.tv.ui.TunableTvViewPlayingApi;

/** A helper class to help {@link MainActivity} to handle audio-related stuffs. */
class AudioManagerHelper implements AudioManager.OnAudioFocusChangeListener {
    private static final float AUDIO_MAX_VOLUME = 1.0f;
    private static final float AUDIO_MIN_VOLUME = 0.0f;
    private static final float AUDIO_DUCKING_VOLUME = 0.3f;

    private final Activity mActivity;
    private final TunableTvViewPlayingApi mTvView;
    private final AudioManager mAudioManager;
    private final AudioCapabilitiesReceiver mAudioCapabilitiesReceiver;

    private boolean mAc3PassthroughSupported;
    private int mAudioFocusStatus = AudioManager.AUDIOFOCUS_LOSS;

    AudioManagerHelper(Activity activity, TunableTvViewPlayingApi tvView) {
        mActivity = activity;
        mTvView = tvView;
        mAudioManager = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
        mAudioCapabilitiesReceiver =
                new AudioCapabilitiesReceiver(
                        activity,
                        new AudioCapabilitiesReceiver.OnAc3PassthroughCapabilityChangeListener() {
                            @Override
                            public void onAc3PassthroughCapabilityChange(boolean capability) {
                                mAc3PassthroughSupported = capability;
                            }
                        });
        mAudioCapabilitiesReceiver.register();
    }

    /**
     * Sets suitable volume to {@link TunableTvView} according to the current audio focus. If the
     * focus status is {@link AudioManager#AUDIOFOCUS_LOSS} and the activity is under PIP mode, this
     * method will finish the activity.
     */
    void setVolumeByAudioFocusStatus() {
        if (mTvView.isPlaying()) {
            switch (mAudioFocusStatus) {
                case AudioManager.AUDIOFOCUS_GAIN:
                    if (mTvView.isTimeShiftAvailable()) {
                        mTvView.timeshiftPlay();
                    } else {
                        mTvView.setStreamVolume(AUDIO_MAX_VOLUME);
                    }
                    break;
                case AudioManager.AUDIOFOCUS_LOSS:
                    if (TvFeatures.PICTURE_IN_PICTURE.isEnabled(mActivity)
                            && Build.VERSION.SDK_INT >= Build.VERSION_CODES.N
                            && mActivity.isInPictureInPictureMode()) {
                        mActivity.finish();
                        break;
                    }
                    // fall through
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                    if (mTvView.isTimeShiftAvailable()) {
                        mTvView.timeshiftPause();
                    } else {
                        mTvView.setStreamVolume(AUDIO_MIN_VOLUME);
                    }
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    if (mTvView.isTimeShiftAvailable()) {
                        mTvView.timeshiftPause();
                    } else {
                        mTvView.setStreamVolume(AUDIO_DUCKING_VOLUME);
                    }
                    break;
            }
        }
    }

    /**
     * Tries to request audio focus from {@link AudioManager} and set volume according to the
     * returned result.
     */
    void requestAudioFocus() {
        int result =
                mAudioManager.requestAudioFocus(
                        this, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        mAudioFocusStatus =
                (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED)
                        ? AudioManager.AUDIOFOCUS_GAIN
                        : AudioManager.AUDIOFOCUS_LOSS;
        setVolumeByAudioFocusStatus();
    }

    /** Abandons audio focus. */
    void abandonAudioFocus() {
        mAudioFocusStatus = AudioManager.AUDIOFOCUS_LOSS;
        mAudioManager.abandonAudioFocus(this);
    }

    /** Returns {@code true} if the device supports AC3 pass-through. */
    boolean isAc3PassthroughSupported() {
        return mAc3PassthroughSupported;
    }

    /** Release the resources the helper class may occupied. */
    void release() {
        mAudioCapabilitiesReceiver.unregister();
    }

    @Override
    public void onAudioFocusChange(int focusChange) {
        mAudioFocusStatus = focusChange;
        setVolumeByAudioFocusStatus();
    }
}
