/**
 * Copyright (c) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.car.radio.service;

import android.hardware.radio.ProgramSelector;
import android.hardware.radio.RadioManager;

import com.android.car.broadcastradio.support.Program;
import com.android.car.radio.service.IRadioCallback;

/**
 * Interface for apps to communicate with the radio.
 */
interface IRadioManager {
    /**
     * Tunes to a given program.
     */
    void tune(in ProgramSelector sel);

    /**
     * Seeks the radio forward.
     */
    void seekForward();

    /**
     * Seeks the radio backwards.
     */
    void seekBackward();

    /**
     * Mutes the radioN
     *
     * @return {@code true} if the mute was successful.
     */
    boolean mute();

    /**
     * Un-mutes the radio and causes audio to play.
     *
     * @return {@code true} if the un-mute was successful.
     */
    boolean unMute();

    /**
     * Returns {@code true} if the radio is currently muted.
     */
    boolean isMuted();

    /**
     * Adds new program to favorites list.
     *
     * @param favorite A program to add to favorites list.
     */
    void addFavorite(in Program favorite);

    /**
     * Removes a program from favorites list.
     *
     * @param sel ProgramSelector of a favorite to remove.
     */
    void removeFavorite(in ProgramSelector sel);

    /**
     * Switches radio band.
     *
     * Usually, this means tuning to the recently listened program on a given band.
     *
     * @param radioBand One of {@link RadioManager#BAND_FM}, {@link RadioManager#BAND_AM}.
     */
    void switchBand(int radioBand);

    /**
     * Adds the given {@link IRadioCallback} to be notified of any radio metadata changes.
     */
    void addRadioTunerCallback(in IRadioCallback callback);

    /**
     * Removes the given {@link IRadioCallback} from receiving any radio metadata chagnes.
     */
    void removeRadioTunerCallback(in IRadioCallback callback);

    // TODO(b/73950974): use callback only (and make sure it's always called for new listeners)
    RadioManager.ProgramInfo getCurrentProgramInfo();

    /**
     * Returns {@code true} if the radio was able to successfully initialize. A value of
     * {@code false} here could mean that the {@code RadioService} was not able to connect to
     * the {@link RadioManager} or there were no radio modules on the current device.
     */
    boolean isInitialized();

    /**
     * Returns {@code true} if the radio currently has focus and is therefore the application that
     * is supplying music.
     */
    boolean hasFocus();

    /**
     * Returns a list of programs found with the tuner's background scan
     */
    List<RadioManager.ProgramInfo> getProgramList();
}
