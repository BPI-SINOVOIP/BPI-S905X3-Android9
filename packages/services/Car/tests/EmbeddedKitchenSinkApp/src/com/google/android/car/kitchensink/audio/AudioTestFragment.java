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

package com.google.android.car.kitchensink.audio;

import android.car.Car;
import android.car.CarAppFocusManager;
import android.car.CarAppFocusManager.OnAppFocusChangedListener;
import android.car.CarAppFocusManager.OnAppFocusOwnershipCallback;
import android.car.CarNotConnectedException;
import android.car.media.CarAudioManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.google.android.car.kitchensink.CarEmulator;
import com.google.android.car.kitchensink.R;

public class AudioTestFragment extends Fragment {
    private static final String TAG = "CAR.AUDIO.KS";
    private static final boolean DBG = true;

    private AudioManager mAudioManager;
    private FocusHandler mAudioFocusHandler;
    private ToggleButton mEnableMocking;

    private AudioPlayer mMusicPlayer;
    private AudioPlayer mMusicPlayerShort;
    private AudioPlayer mNavGuidancePlayer;
    private AudioPlayer mVrPlayer;
    private AudioPlayer mSystemPlayer;
    private AudioPlayer mWavPlayer;
    private AudioPlayer[] mAllPlayers;

    private Handler mHandler;
    private Context mContext;

    private Car mCar;
    private CarAppFocusManager mAppFocusManager;
    private CarAudioManager mCarAudioManager;
    private AudioAttributes mMusicAudioAttrib;
    private AudioAttributes mNavAudioAttrib;
    private AudioAttributes mVrAudioAttrib;
    private AudioAttributes mRadioAudioAttrib;
    private AudioAttributes mSystemSoundAudioAttrib;
    private CarEmulator mCarEmulator;

    private final AudioManager.OnAudioFocusChangeListener mNavFocusListener = (focusChange) -> {
        Log.i(TAG, "Nav focus change:" + focusChange);
    };
    private final AudioManager.OnAudioFocusChangeListener mVrFocusListener = (focusChange) -> {
        Log.i(TAG, "VR focus change:" + focusChange);
    };
    private final AudioManager.OnAudioFocusChangeListener mRadioFocusListener = (focusChange) -> {
        Log.i(TAG, "Radio focus change:" + focusChange);
    };

    private final CarAppFocusManager.OnAppFocusOwnershipCallback mOwnershipCallbacks =
            new OnAppFocusOwnershipCallback() {
                @Override
                public void onAppFocusOwnershipLost(int focus) {
                }
                @Override
                public void onAppFocusOwnershipGranted(int focus) {
                }
    };

    private void init() {
        mContext = getContext();
        mHandler = new Handler(Looper.getMainLooper());
        mCar = Car.createCar(mContext, new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                try {
                    mAppFocusManager =
                            (CarAppFocusManager) mCar.getCarManager(Car.APP_FOCUS_SERVICE);
                } catch (CarNotConnectedException e) {
                    throw new RuntimeException("Failed to create app focus manager", e);
                }
                try {
                    OnAppFocusChangedListener listener = new OnAppFocusChangedListener() {
                        @Override
                        public void onAppFocusChanged(int appType, boolean active) {
                        }
                    };
                    mAppFocusManager.addFocusListener(listener,
                            CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION);
                    mAppFocusManager.addFocusListener(listener,
                            CarAppFocusManager.APP_FOCUS_TYPE_VOICE_COMMAND);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to register focus listener", e);
                }
                try {
                    mCarAudioManager = (CarAudioManager) mCar.getCarManager(Car.AUDIO_SERVICE);
                } catch (CarNotConnectedException e) {
                    throw new RuntimeException("Failed to create audio manager", e);
                }
                mMusicAudioAttrib = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .build();
                mNavAudioAttrib = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .build();
                mVrAudioAttrib = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE)
                        .build();
                mRadioAudioAttrib = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .build();
                mSystemSoundAudioAttrib = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION)
                        .build();

                mMusicPlayer = new AudioPlayer(mContext, R.raw.well_worth_the_wait,
                        mMusicAudioAttrib);
                mMusicPlayerShort = new AudioPlayer(mContext, R.raw.ring_classic_01,
                        mMusicAudioAttrib);
                mNavGuidancePlayer = new AudioPlayer(mContext, R.raw.turnright,
                        mNavAudioAttrib);
                // no Usage for voice command yet.
                mVrPlayer = new AudioPlayer(mContext, R.raw.one2six,
                        mVrAudioAttrib);
                mSystemPlayer = new AudioPlayer(mContext, R.raw.ring_classic_01,
                        mSystemSoundAudioAttrib);
                mWavPlayer = new AudioPlayer(mContext, R.raw.free_flight,
                        mMusicAudioAttrib);
                mAllPlayers = new AudioPlayer[] {
                        mMusicPlayer,
                        mMusicPlayerShort,
                        mNavGuidancePlayer,
                        mVrPlayer,
                        mSystemPlayer,
                        mWavPlayer
                };
            }
            @Override
            public void onServiceDisconnected(ComponentName name) {
            }
            });
        mCar.connect();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle bundle) {
        Log.i(TAG, "onCreateView");
        init();
        View view = inflater.inflate(R.layout.audio, container, false);
        mAudioManager = (AudioManager) mContext.getSystemService(
                Context.AUDIO_SERVICE);
        mAudioFocusHandler = new FocusHandler(
                view.findViewById(R.id.button_focus_request_selection),
                view.findViewById(R.id.button_audio_focus_request),
                view.findViewById(R.id.text_audio_focus_state));
        view.findViewById(R.id.button_media_play_start).setOnClickListener(v -> {
            boolean requestFocus = true;
            boolean repeat = true;
            mMusicPlayer.start(requestFocus, repeat, AudioManager.AUDIOFOCUS_GAIN);
        });
        view.findViewById(R.id.button_media_play_once).setOnClickListener(v -> {
            mMusicPlayerShort.start(true, false, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
            // play only for 1 sec and stop
            mHandler.postDelayed(() -> mMusicPlayerShort.stop(), 1000);
        });
        view.findViewById(R.id.button_media_play_stop).setOnClickListener(v -> mMusicPlayer.stop());
        view.findViewById(R.id.button_wav_play_start).setOnClickListener(
                v -> mWavPlayer.start(true, true, AudioManager.AUDIOFOCUS_GAIN));
        view.findViewById(R.id.button_wav_play_stop).setOnClickListener(v -> mWavPlayer.stop());
        view.findViewById(R.id.button_nav_play_once).setOnClickListener(v -> {
            if (mAppFocusManager == null) {
                return;
            }
            if (DBG) {
                Log.i(TAG, "Nav start");
            }
            try {
                mAppFocusManager.requestAppFocus(
                        CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION, mOwnershipCallbacks);
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to set active focus", e);
            }
            if (!mNavGuidancePlayer.isPlaying()) {
                mNavGuidancePlayer.start(true, false,
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK,
                        () -> mAppFocusManager.abandonAppFocus(mOwnershipCallbacks,
                                CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION));
            }
        });
        view.findViewById(R.id.button_vr_play_once).setOnClickListener(v -> {
            if (mAppFocusManager == null) {
                return;
            }
            if (DBG) {
                Log.i(TAG, "VR start");
            }
            try {
                mAppFocusManager.requestAppFocus(
                        CarAppFocusManager.APP_FOCUS_TYPE_VOICE_COMMAND, mOwnershipCallbacks);
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to set active focus", e);
            }
            if (!mVrPlayer.isPlaying()) {
                mVrPlayer.start(true, false,
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT,
                        () -> mAppFocusManager.abandonAppFocus(mOwnershipCallbacks,
                                CarAppFocusManager.APP_FOCUS_TYPE_VOICE_COMMAND));
            }
        });
        view.findViewById(R.id.button_system_play_once).setOnClickListener(v -> {
            if (DBG) {
                Log.i(TAG, "System start");
            }
            if (!mSystemPlayer.isPlaying()) {
                // system sound played without focus
                mSystemPlayer.start(false, false, 0);
            }
        });
        view.findViewById(R.id.button_nav_start).setOnClickListener(v -> handleNavStart());
        view.findViewById(R.id.button_nav_end).setOnClickListener(v -> handleNavEnd());
        view.findViewById(R.id.button_vr_start).setOnClickListener(v -> handleVrStart());
        view.findViewById(R.id.button_vr_end).setOnClickListener(v -> handleVrEnd());
        view.findViewById(R.id.button_radio_start).setOnClickListener(v -> handleRadioStart());
        view.findViewById(R.id.button_radio_end).setOnClickListener(v -> handleRadioEnd());
        view.findViewById(R.id.button_speaker_phone_on).setOnClickListener(
                v -> mAudioManager.setSpeakerphoneOn(true));
        view.findViewById(R.id.button_speaker_phone_off).setOnClickListener(
                v -> mAudioManager.setSpeakerphoneOn(false));
        view.findViewById(R.id.button_microphone_on).setOnClickListener(
                v -> mAudioManager.setMicrophoneMute(false));
        view.findViewById(R.id.button_microphone_off).setOnClickListener(
                v -> mAudioManager.setMicrophoneMute(true));


        mEnableMocking = view.findViewById(R.id.button_mock_audio);
        mEnableMocking.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (mCarEmulator == null) {
                //TODO(pavelm): need to do a full switch between emulated and normal mode
                // all Car*Manager references should be invalidated.
                Toast.makeText(AudioTestFragment.this.getContext(),
                        "Not supported yet :(", Toast.LENGTH_SHORT).show();
                return;
            }
            if (isChecked) {
                mCarEmulator.start();
            } else {
                mCarEmulator.stop();
                mCarEmulator = null;
            }
        });
        return view;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        Log.i(TAG, "onDestroyView");
        if (mCarEmulator != null) {
            mCarEmulator.stop();
        }
        for (AudioPlayer p : mAllPlayers) {
            p.stop();
        }
        if (mAudioFocusHandler != null) {
            mAudioFocusHandler.release();
            mAudioFocusHandler = null;
        }
        if (mAppFocusManager != null) {
            mAppFocusManager.abandonAppFocus(mOwnershipCallbacks);
        }
    }

    private void handleNavStart() {
        if (mAppFocusManager == null) {
            return;
        }
        if (DBG) {
            Log.i(TAG, "Nav start");
        }
        try {
            mAppFocusManager.requestAppFocus(CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION,
                    mOwnershipCallbacks);
            mAudioManager.requestAudioFocus(mNavFocusListener, mNavAudioAttrib,
                    AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK, 0);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to set active focus", e);
        }
    }

    private void handleNavEnd() {
        if (mAppFocusManager == null) {
            return;
        }
        if (DBG) {
            Log.i(TAG, "Nav end");
        }
        mAudioManager.abandonAudioFocus(mNavFocusListener, mNavAudioAttrib);
        mAppFocusManager.abandonAppFocus(mOwnershipCallbacks,
                CarAppFocusManager.APP_FOCUS_TYPE_NAVIGATION);
    }

    private void handleVrStart() {
        if (mAppFocusManager == null) {
            return;
        }
        if (DBG) {
            Log.i(TAG, "VR start");
        }
        try {
            mAppFocusManager.requestAppFocus(CarAppFocusManager.APP_FOCUS_TYPE_VOICE_COMMAND,
                    mOwnershipCallbacks);
            mAudioManager.requestAudioFocus(mVrFocusListener, mVrAudioAttrib,
                    AudioManager.AUDIOFOCUS_GAIN_TRANSIENT, 0);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to set active focus", e);
        }
    }

    private void handleVrEnd() {
        if (mAppFocusManager == null) {
            return;
        }
        if (DBG) {
            Log.i(TAG, "VR end");
        }
        mAudioManager.abandonAudioFocus(mVrFocusListener, mVrAudioAttrib);
        mAppFocusManager.abandonAppFocus(mOwnershipCallbacks,
                CarAppFocusManager.APP_FOCUS_TYPE_VOICE_COMMAND);
    }

    private void handleRadioStart() {
        if (DBG) {
            Log.i(TAG, "Radio start");
        }
        mAudioManager.requestAudioFocus(mRadioFocusListener, mRadioAudioAttrib,
                AudioManager.AUDIOFOCUS_GAIN, 0);
    }

    private void handleRadioEnd() {
        if (DBG) {
            Log.i(TAG, "Radio end");
        }
        mAudioManager.abandonAudioFocus(mRadioFocusListener, mRadioAudioAttrib);
    }

    private class FocusHandler {
        private static final String AUDIO_FOCUS_STATE_GAIN = "gain";
        private static final String AUDIO_FOCUS_STATE_RELEASED_UNKNOWN = "released / unknown";

        private final RadioGroup mRequestSelection;
        private final TextView mText;
        private final AudioFocusListener mFocusListener;

        public FocusHandler(RadioGroup radioGroup, Button requestButton, TextView text) {
            mText = text;
            mRequestSelection = radioGroup;
            mRequestSelection.check(R.id.focus_gain);
            setFocusText(AUDIO_FOCUS_STATE_RELEASED_UNKNOWN);
            mFocusListener = new AudioFocusListener();
            requestButton.setOnClickListener(v -> {
                int selectedButtonId = mRequestSelection.getCheckedRadioButtonId();
                int focusRequest = AudioManager.AUDIOFOCUS_GAIN;
                if (selectedButtonId == R.id.focus_gain_transient_duck) {
                    focusRequest = AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK;
                } else if (selectedButtonId == R.id.focus_release) {
                    mAudioManager.abandonAudioFocus(mFocusListener);
                    setFocusText(AUDIO_FOCUS_STATE_RELEASED_UNKNOWN);
                    return;
                }
                int ret = mAudioManager.requestAudioFocus(mFocusListener,
                        AudioManager.STREAM_MUSIC, focusRequest);
                Log.i(TAG, "requestAudioFocus returned " + ret);
                if (ret == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                    setFocusText(AUDIO_FOCUS_STATE_GAIN);
                }
            });
        }

        public void release() {
            abandonAudioFocus();
        }

        private void abandonAudioFocus() {
            if (DBG) {
                Log.i(TAG, "abandonAudioFocus");
            }
            mAudioManager.abandonAudioFocus(mFocusListener);
            setFocusText(AUDIO_FOCUS_STATE_RELEASED_UNKNOWN);
        }

        private void setFocusText(String msg) {
            mText.setText("focus state:" + msg);
        }

        private class AudioFocusListener implements AudioManager.OnAudioFocusChangeListener {
            @Override
            public void onAudioFocusChange(int focusChange) {
                Log.i(TAG, "onAudioFocusChange " + focusChange);
                if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
                    setFocusText(AUDIO_FOCUS_STATE_GAIN);
                } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS) {
                    setFocusText("loss");
                } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                    setFocusText("loss,transient");
                } else if (focusChange == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) {
                    setFocusText("loss,transient,duck");
                }
            }
        }
    }
}
