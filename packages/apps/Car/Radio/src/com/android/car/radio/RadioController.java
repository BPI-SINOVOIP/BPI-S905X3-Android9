/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.car.radio;

import android.animation.ArgbEvaluator;
import android.animation.ValueAnimator;
import android.annotation.ColorInt;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.RadioManager;
import android.hardware.radio.RadioManager.ProgramInfo;
import android.hardware.radio.RadioMetadata;
import android.hardware.radio.RadioTuner;
import android.media.AudioManager;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.View;

import com.android.car.broadcastradio.support.Program;
import com.android.car.broadcastradio.support.platform.ProgramInfoExt;
import com.android.car.broadcastradio.support.platform.ProgramSelectorExt;
import com.android.car.radio.service.IRadioCallback;
import com.android.car.radio.service.IRadioManager;
import com.android.car.radio.storage.RadioStorage;
import com.android.car.radio.utils.ProgramSelectorUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * A controller that handles the display of metadata on the current radio station.
 */
public class RadioController implements RadioStorage.PresetsChangeListener {
    private static final String TAG = "Em.RadioController";

    /**
     * The percentage by which to darken the color that should be set on the status bar.
     * This darkening gives the status bar the illusion that it is transparent.
     *
     * @see RadioController#setShouldColorStatusBar(boolean)
     */
    private static final float STATUS_BAR_DARKEN_PERCENTAGE = 0.4f;

    /**
     * The animation time for when the background of the radio shifts to a different color.
     */
    private static final int BACKGROUND_CHANGE_ANIM_TIME_MS = 450;
    private static final int INVALID_BACKGROUND_COLOR = 0;

    private static final int CHANNEL_CHANGE_DURATION_MS = 200;

    private final ValueAnimator mAnimator = new ValueAnimator();
    private int mCurrentlyDisplayedChannel;  // for animation purposes
    private ProgramInfo mCurrentProgram;

    private final Activity mActivity;
    private IRadioManager mRadioManager;

    private View mRadioBackground;
    private boolean mShouldColorStatusBar;
    private boolean mShouldColorBackground;

    /**
     * An additional layer on top of the background that should match the color of
     * {@link #mRadioBackground}. This view should only exist in the preset list. The reason this
     * layer cannot be transparent is because it needs to be elevated, and elevation does not
     * work if the background is undefined or transparent.
     */
    private View mRadioPresetBackground;

    private View mRadioErrorDisplay;

    private final RadioChannelColorMapper mColorMapper;
    @ColorInt private int mCurrentBackgroundColor = INVALID_BACKGROUND_COLOR;

    private final RadioDisplayController mRadioDisplayController;

    /**
     * Keeps track of if the user has manually muted the radio. This value is used to determine
     * whether or not to un-mute the radio after an {@link AudioManager#AUDIOFOCUS_LOSS_TRANSIENT}
     * event has been received.
     */
    private boolean mUserHasMuted;

    private final RadioStorage mRadioStorage;

    private final String mAmBandString;
    private final String mFmBandString;

    private List<ProgramInfoChangeListener> mProgramInfoChangeListeners = new ArrayList<>();
    private List<RadioServiceConnectionListener> mRadioServiceConnectionListeners =
            new ArrayList<>();

    /**
     * Interface for a class that will be notified when the current radio station has been changed.
     */
    public interface ProgramInfoChangeListener {
        /**
         * Called when the current radio station has changed in the radio.
         *
         * @param info The current radio station.
         */
        void onProgramInfoChanged(@NonNull ProgramInfo info);
    }

    /**
     * Interface for a class that will be notified when RadioService is successfuly bound
     */
    public interface RadioServiceConnectionListener {

        /**
         * Called when the RadioService is successfully connected
         */
        void onRadioServiceConnected();
    }

    public RadioController(Activity activity) {
        mActivity = activity;

        mRadioDisplayController = new RadioDisplayController(mActivity);
        mColorMapper = RadioChannelColorMapper.getInstance(mActivity);

        mAmBandString = mActivity.getString(R.string.radio_am_text);
        mFmBandString = mActivity.getString(R.string.radio_fm_text);

        mRadioStorage = RadioStorage.getInstance(mActivity);
        mRadioStorage.addPresetsChangeListener(this);
        mShouldColorBackground = true;
    }

    /**
     * Initializes this {@link RadioController} to control the UI whose root is the given container.
     */
    public void initialize(View container) {
        mCurrentBackgroundColor = INVALID_BACKGROUND_COLOR;

        mRadioDisplayController.initialize(container);

        mRadioDisplayController.setBackwardSeekButtonListener(mBackwardSeekClickListener);
        mRadioDisplayController.setForwardSeekButtonListener(mForwardSeekClickListener);
        mRadioDisplayController.setPlayButtonListener(mPlayPauseClickListener);
        mRadioDisplayController.setAddPresetButtonListener(mPresetButtonClickListener);

        mRadioBackground = container;
        mRadioPresetBackground = container.findViewById(R.id.preset_current_card_container);

        mRadioErrorDisplay = container.findViewById(R.id.radio_error_display);

        updateRadioDisplay();
    }

    /**
     * Set whether or not this controller should also update the color of the status bar to match
     * the current background color of the radio. The color that will be set on the status bar
     * will be slightly darker, giving the illusion that the status bar is transparent.
     *
     * <p>This method is needed because of scene transitions. Scene transitions do not take into
     * account padding that is added programmatically. Since there is no way to get the height of
     * the status bar and set it in XML, it needs to be done in code. This breaks the scene
     * transition.
     *
     * <p>To make this work, the status bar is not actually translucent; it is colored to appear
     * that way via this method.
     */
    public void setShouldColorStatusBar(boolean shouldColorStatusBar) {
       mShouldColorStatusBar = shouldColorStatusBar;
    }

    /**
     * Set whether this controller should update the background color.
     * This behavior is enabled by defaullt
     */
    public void setShouldColorBackground(boolean shouldColorBackground) {
        mShouldColorBackground = shouldColorBackground;
    }

    /**
     * Adds a listener that will be notified whenever the radio station changes.
     */
    public void addProgramInfoChangeListener(ProgramInfoChangeListener listener) {
        mProgramInfoChangeListeners.add(listener);
    }

    /**
     * Removes a listener that will be notified whenever the radio station changes.
     */
    public void removeProgramInfoChangeListener(ProgramInfoChangeListener listener) {
        mProgramInfoChangeListeners.remove(listener);
    }

    /**
     * Sets the listeners that will be notified when the radio service is connected.
     */
    public void addRadioServiceConnectionListener(RadioServiceConnectionListener listener) {
        mRadioServiceConnectionListeners.add(listener);
    }

    /**
     * Removes a listener that will be notified when the radio service is connected.
     */
    public void removeRadioServiceConnectionListener(RadioServiceConnectionListener listener) {
        mRadioServiceConnectionListeners.remove(listener);
    }

    /**
     * Starts the controller to handle radio tuning. This method should be called to begin
     * radio playback.
     */
    public void start() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "starting radio");
        }

        Intent bindIntent = new Intent(RadioService.ACTION_UI_SERVICE, null /* uri */,
                mActivity, RadioService.class);
        if (!mActivity.bindService(bindIntent, mServiceConnection, Context.BIND_AUTO_CREATE)) {
            Log.e(TAG, "Failed to connect to RadioService.");
        }

        updateRadioDisplay();
    }

    /**
     * Retrieves information about the current radio station from {@link #mRadioManager} and updates
     * the display of that information accordingly.
     */
    private void updateRadioDisplay() {
        if (mRadioManager == null) {
            return;
        }

        try {
            mRadioDisplayController.setSingleChannelDisplay(mRadioBackground);

            // Ensure the play button properly reflects the current mute state.
            mRadioDisplayController.setPlayPauseButtonState(mRadioManager.isMuted());

            // TODO(b/73950974): use callback only
            ProgramInfo current = mRadioManager.getCurrentProgramInfo();
            if (current != null) mCallback.onCurrentProgramInfoChanged(current);
        } catch (RemoteException e) {
            Log.e(TAG, "updateRadioDisplay(); remote exception: " + e.getMessage());
        }
    }

    /**
     * Tunes the radio to the given channel if it is valid and a {@link RadioTuner} has been opened.
     */
    public void tune(ProgramSelector sel) {
        if (mRadioManager == null) return;

        try {
            mRadioManager.tune(sel);
        } catch (RemoteException ex) {
            Log.e(TAG, "Failed to tune", ex);
        }
    }

    /**
     * Returns the band this radio is currently tuned to.
     *
     * TODO(b/73950974): don't be AM/FM exclusive
     */
    public int getCurrentRadioBand() {
        return ProgramSelectorUtils.getRadioBand(mCurrentProgram.getSelector());
    }

    /**
     * Returns the radio station that is currently playing on the radio. If this controller is
     * not connected to the {@link RadioService} or a radio station cannot be retrieved, then
     * {@code null} is returned.
     *
     * TODO(b/73950974): use callback only
     */
    @Nullable
    public ProgramInfo getCurrentProgramInfo() {
        return mCurrentProgram;
    }

    /**
     * Switch radio band. Currently, this only supports FM and AM bands.
     *
     * @param radioBand One of {@link RadioManager#BAND_FM}, {@link RadioManager#BAND_AM}.
     */
    public void switchBand(int radioBand) {
        try {
            mRadioManager.switchBand(radioBand);
        } catch (RemoteException e) {
            Log.e(TAG, "Couldn't switch band", e);
        }
    }

    /**
     * Delegates to the {@link RadioDisplayController} to highlight the radio band.
     */
    private void updateAmFmDisplayState(int band) {
        switch (band) {
            case RadioManager.BAND_FM:
                mRadioDisplayController.setChannelBand(mFmBandString);
                break;

            case RadioManager.BAND_AM:
                mRadioDisplayController.setChannelBand(mAmBandString);
                break;

            // TODO: Support BAND_FM_HD and BAND_AM_HD.

            default:
                mRadioDisplayController.setChannelBand(null);
        }
    }

    // TODO(b/73950974): move channel animation to RadioDisplayController
    private void updateRadioChannelDisplay(@NonNull ProgramSelector sel) {
        int priType = sel.getPrimaryId().getType();

        mAnimator.cancel();

        if (!ProgramSelectorExt.isAmFmProgram(sel)
                || !ProgramSelectorExt.hasId(sel, ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY)) {
            // channel animation is implemented for AM/FM only
            mCurrentlyDisplayedChannel = 0;
            mRadioDisplayController.setChannelNumber("");

            updateAmFmDisplayState(RadioStorage.INVALID_RADIO_BAND);
            return;
        }

        int freq = (int)sel.getFirstId(ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY);

        boolean wasAm = ProgramSelectorExt.isAmFrequency(mCurrentlyDisplayedChannel);
        boolean wasFm = ProgramSelectorExt.isFmFrequency(mCurrentlyDisplayedChannel);
        boolean isAm = ProgramSelectorExt.isAmFrequency(freq);
        int band = isAm ? RadioManager.BAND_AM : RadioManager.BAND_FM;

        updateAmFmDisplayState(band);

        if (isAm && wasAm || !isAm && wasFm) {
            mAnimator.setIntValues((int)mCurrentlyDisplayedChannel, (int)freq);
            mAnimator.setDuration(CHANNEL_CHANGE_DURATION_MS);
            mAnimator.addUpdateListener(animation -> mRadioDisplayController.setChannelNumber(
                    ProgramSelectorExt.formatAmFmFrequency((int)animation.getAnimatedValue(),
                            ProgramSelectorExt.NAME_NO_MODULATION)));
            mAnimator.start();
        } else {
            // it's a different band - don't animate
            mRadioDisplayController.setChannelNumber(
                    ProgramSelectorExt.getDisplayName(sel, ProgramSelectorExt.NAME_NO_MODULATION));
        }
        mCurrentlyDisplayedChannel = freq;

        maybeUpdateBackgroundColor(freq);
    }

    /**
     * Checks if the color of the radio background should be changed, and if so, animates that
     * color change.
     */
    private void maybeUpdateBackgroundColor(int channel) {
        if (mRadioBackground == null || !mShouldColorBackground) {
            return;
        }

        int newColor = mColorMapper.getColorForChannel(channel);

        // No animation required if the colors are the same.
        if (newColor == mCurrentBackgroundColor) {
            return;
        }

        // If the current background color is invalid, then just set as the new color without any
        // animation.
        if (mCurrentBackgroundColor == INVALID_BACKGROUND_COLOR) {
            mCurrentBackgroundColor = newColor;
            setBackgroundColor(newColor);
        }

        // Otherwise, animate the background color change.
        ValueAnimator colorAnimation = ValueAnimator.ofObject(new ArgbEvaluator(),
                mCurrentBackgroundColor, newColor);
        colorAnimation.setDuration(BACKGROUND_CHANGE_ANIM_TIME_MS);
        colorAnimation.addUpdateListener(mBackgroundColorUpdater);
        colorAnimation.start();

        mCurrentBackgroundColor = newColor;
    }

    private void setBackgroundColor(int backgroundColor) {
        mRadioBackground.setBackgroundColor(backgroundColor);

        if (mRadioPresetBackground != null) {
            mRadioPresetBackground.setBackgroundColor(backgroundColor);
        }

        if (mShouldColorStatusBar) {
            int red = darkenColor(Color.red(backgroundColor));
            int green = darkenColor(Color.green(backgroundColor));
            int blue = darkenColor(Color.blue(backgroundColor));
            int alpha = Color.alpha(backgroundColor);

            mActivity.getWindow().setStatusBarColor(
                    Color.argb(alpha, red, green, blue));
        }
    }

    /**
     * Darkens the given color by {@link #STATUS_BAR_DARKEN_PERCENTAGE}.
     */
    private int darkenColor(int color) {
        return (int) Math.max(color - (color * STATUS_BAR_DARKEN_PERCENTAGE), 0);
    }

    /**
     * Clears all metadata including song title, artist and station information.
     */
    private void clearMetadataDisplay() {
        mRadioDisplayController.setCurrentStation(null);
        mRadioDisplayController.setCurrentSongTitleAndArtist(null, null);
    }

    /**
     * Closes any active {@link RadioTuner}s and releases audio focus.
     */
    private void close() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "close()");
        }

        // Lost focus, so display that the radio is not playing anymore.
        mRadioDisplayController.setPlayPauseButtonState(true);
    }

    /**
     * Closes all active connections in the {@link RadioController}.
     */
    public void shutdown() {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "shutdown()");
        }

        mActivity.unbindService(mServiceConnection);
        mRadioStorage.removePresetsChangeListener(this);

        if (mRadioManager != null) {
            try {
                mRadioManager.removeRadioTunerCallback(mCallback);
            } catch (RemoteException e) {
                Log.e(TAG, "tuneToRadioChannel(); remote exception: " + e.getMessage());
            }
        }

        close();
    }

    @Override
    public void onPresetsRefreshed() {
        // Check if the current channel's preset status has changed.
        ProgramInfo info = mCurrentProgram;
        boolean isPreset = (info != null) && mRadioStorage.isPreset(info.getSelector());
        mRadioDisplayController.setChannelIsPreset(isPreset);
    }

    /**
     * Gets a list of programs from the radio tuner's background scan
     */
    public List<ProgramInfo> getProgramList() {
        if (mRadioManager != null) {
            try {
                return mRadioManager.getProgramList();
            } catch (RemoteException e) {
                Log.e(TAG, "getProgramList(); remote exception: " + e.getMessage());
            }
        }
        return null;
    }

    private final IRadioCallback.Stub mCallback = new IRadioCallback.Stub() {
        @Override
        public void onCurrentProgramInfoChanged(ProgramInfo info) {
            mCurrentProgram = Objects.requireNonNull(info);
            ProgramSelector sel = info.getSelector();

            updateRadioChannelDisplay(sel);

            mRadioDisplayController.setCurrentStation(
                    ProgramInfoExt.getProgramName(info, ProgramInfoExt.NAME_NO_CHANNEL_FALLBACK));
            RadioMetadata meta = ProgramInfoExt.getMetadata(mCurrentProgram);
            mRadioDisplayController.setCurrentSongTitleAndArtist(
                    meta.getString(RadioMetadata.METADATA_KEY_TITLE),
                    meta.getString(RadioMetadata.METADATA_KEY_ARTIST));

            mRadioDisplayController.setChannelIsPreset(mRadioStorage.isPreset(sel));

            // Notify that the current radio station has changed.
            if (mProgramInfoChangeListeners != null) {
                for (ProgramInfoChangeListener listener : mProgramInfoChangeListeners) {
                    listener.onProgramInfoChanged(info);
                }
            }
        }

        @Override
        public void onRadioMuteChanged(boolean isMuted) {
            mRadioDisplayController.setPlayPauseButtonState(isMuted);
        }

        @Override
        public void onError(int status) {
            Log.e(TAG, "Radio callback error with status: " + status);
            close();
        }
    };

    private final View.OnClickListener mBackwardSeekClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mRadioManager == null) return;

            // TODO(b/73950974): show some kind of animation
            clearMetadataDisplay();

            try {
                // TODO(b/73950974): watch for timeout and if it happens, display metadata back
                mRadioManager.seekBackward();
            } catch (RemoteException e) {
                Log.e(TAG, "backwardSeek(); remote exception: " + e.getMessage());
            }
        }
    };

    private final View.OnClickListener mForwardSeekClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mRadioManager == null) return;

            clearMetadataDisplay();

            try {
                mRadioManager.seekForward();
            } catch (RemoteException e) {
                Log.e(TAG, "Couldn't seek forward", e);
            }
        }
    };

    /**
     * Click listener for the play/pause button. Currently, all this does is mute/unmute the radio
     * because the {@link RadioManager} does not support the ability to pause/start again.
     */
    private final View.OnClickListener mPlayPauseClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mRadioManager == null) {
                return;
            }

            try {
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "Play button clicked. Currently muted: " + mRadioManager.isMuted());
                }

                if (mRadioManager.isMuted()) {
                    mRadioManager.unMute();
                } else {
                    mRadioManager.mute();
                }

                boolean isMuted = mRadioManager.isMuted();

                mUserHasMuted = isMuted;
                mRadioDisplayController.setPlayPauseButtonState(isMuted);
            } catch (RemoteException e) {
                Log.e(TAG, "playPauseClickListener(); remote exception: " + e.getMessage());
            }
        }
    };

    private final View.OnClickListener mPresetButtonClickListener = new View.OnClickListener() {
        // TODO: Maybe add a check to send a store/remove preset event after a delay so that
        // there aren't multiple writes if the user presses the button quickly.
        @Override
        public void onClick(View v) {
            ProgramInfo info = mCurrentProgram;
            if (info == null) return;

            ProgramSelector sel = mCurrentProgram.getSelector();
            boolean isPreset = mRadioStorage.isPreset(sel);

            if (isPreset) {
                mRadioStorage.removePreset(sel);
            } else {
                mRadioStorage.storePreset(Program.fromProgramInfo(info));
            }

            // Update the UI immediately. If the preset failed for some reason, the RadioStorage
            // will notify us and UI update will happen then.
            mRadioDisplayController.setChannelIsPreset(!isPreset);
        }
    };

    private ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder binder) {
            mRadioManager = ((IRadioManager) binder);

            try {
                if (mRadioManager == null || !mRadioManager.isInitialized()) {
                    mRadioDisplayController.setEnabled(false);

                    if (mRadioErrorDisplay != null) {
                        mRadioErrorDisplay.setVisibility(View.VISIBLE);
                    }

                    return;
                }

                mRadioDisplayController.setEnabled(true);

                if (mRadioErrorDisplay != null) {
                    mRadioErrorDisplay.setVisibility(View.GONE);
                }

                mRadioDisplayController.setSingleChannelDisplay(mRadioBackground);

                mRadioManager.addRadioTunerCallback(mCallback);

                // Notify listeners
                for (RadioServiceConnectionListener listener : mRadioServiceConnectionListeners) {
                    listener.onRadioServiceConnected();
                }
            } catch (RemoteException e) {
                Log.e(TAG, "onServiceConnected(); remote exception: " + e.getMessage());
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            mRadioManager = null;
        }
    };

    private final ValueAnimator.AnimatorUpdateListener mBackgroundColorUpdater =
            animator -> {
                int backgroundColor = (int) animator.getAnimatedValue();
                setBackgroundColor(backgroundColor);
            };
}
