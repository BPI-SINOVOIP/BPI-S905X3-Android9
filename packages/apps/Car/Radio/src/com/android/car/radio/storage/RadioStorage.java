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

package com.android.car.radio.storage;

import android.annotation.NonNull;
import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.Observer;
import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.radio.ProgramSelector;
import android.hardware.radio.RadioManager;
import android.os.AsyncTask;
import android.util.Log;

import com.android.car.broadcastradio.support.Program;
import com.android.car.radio.utils.ProgramSelectorUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

/**
 * Class that manages persistent storage of various radio options.
 */
public class RadioStorage {
    private static final String TAG = "Em.RadioStorage";
    private static final String PREF_NAME = "com.android.car.radio.RadioStorage";

    // Keys used for storage in the SharedPreferences.
    private static final String PREF_KEY_RADIO_BAND = "radio_band";
    private static final String PREF_KEY_RADIO_CHANNEL_AM = "radio_channel_am";
    private static final String PREF_KEY_RADIO_CHANNEL_FM = "radio_channel_fm";

    public static final int INVALID_RADIO_CHANNEL = -1;
    public static final int INVALID_RADIO_BAND = -1;

    private static SharedPreferences sSharedPref;
    private static RadioStorage sInstance;
    private static RadioDatabase sRadioDatabase;

    /**
     * Listener that will be called when something in the radio storage changes.
     */
    public interface PresetsChangeListener {
        /**
         * Called when favorite list has changed.
         */
        void onPresetsRefreshed();
    }

    private final LiveData<List<Program>> mFavorites;

    // TODO(b/73950974): use Observer<> directly
    private final Map<PresetsChangeListener, Observer<List<Program>>> mPresetListeners =
            new HashMap<>();

    private RadioStorage(Context context) {
        if (sSharedPref == null) {
            sSharedPref = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        }

        if (sRadioDatabase == null) {
            sRadioDatabase = RadioDatabase.buildInstance(context);
        }

        mFavorites = sRadioDatabase.getAllFavorites();
    }

    /**
     * Returns singleton instance of {@link RadioStorage}.
     */
    public static RadioStorage getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new RadioStorage(context.getApplicationContext());
        }

        return sInstance;
    }

    /**
     * Registers the given {@link PresetsChangeListener} to be notified when any radio preset state
     * has changed.
     */
    public void addPresetsChangeListener(PresetsChangeListener listener) {
        Observer<List<Program>> observer = list -> listener.onPresetsRefreshed();
        synchronized (mPresetListeners) {
            mFavorites.observeForever(observer);
            mPresetListeners.put(listener, observer);
        }
    }

    /**
     * Unregisters the given {@link PresetsChangeListener}.
     */
    public void removePresetsChangeListener(PresetsChangeListener listener) {
        Observer<List<Program>> observer;
        synchronized (mPresetListeners) {
            observer = mPresetListeners.remove(listener);
            mFavorites.removeObserver(observer);
        }
    }

    /**
     * Returns all currently loaded presets. If there are no stored presets, this method will
     * return an empty {@link List}.
     *
     * <p>Register as a {@link PresetsChangeListener} to be notified of any changes in the
     * preset list.
     */
    public @NonNull List<Program> getPresets() {
        List<Program> favorites = mFavorites.getValue();
        if (favorites != null) return favorites;

        // It won't be a problem when we use Observer<> directly.
        Log.w(TAG, "Database is not ready yet");
        return new ArrayList<>();
    }

    /**
     * Returns {@code true} if the given {@link ProgramSelector} is a user saved favorite.
     */
    public boolean isPreset(@NonNull ProgramSelector selector) {
        return mFavorites.getValue().contains(new Program(selector, ""));
    }

    /**
     * Stores that given {@link Program} as a preset. This operation will override any
     * previously stored preset that matches the given preset.
     *
     * <p>Upon a successful store, the presets list will be refreshed via a call to
     * {@link #refreshPresets()}.
     *
     * @see #refreshPresets()
     */
    public void storePreset(@NonNull Program preset) {
        new StorePresetAsyncTask().execute(Objects.requireNonNull(preset));
    }

    /**
     * Removes the given {@link Program} as a preset.
     *
     * <p>Upon a successful removal, the presets list will be refreshed via a call to
     * {@link #refreshPresets()}.
     *
     * @see #refreshPresets()
     */
    public void removePreset(@NonNull ProgramSelector preset) {
        new RemovePresetAsyncTask().execute(Objects.requireNonNull(preset));
    }

    /**
     * Returns the stored radio band that was set in {@link #storeRadioChannel}. If a radio band
     * has not previously been stored, then {@link RadioManager#BAND_FM} is returned.
     *
     * @return One of {@link RadioManager#BAND_FM} or {@link RadioManager#BAND_AM}.
     */
    public int getStoredRadioBand() {
        return sSharedPref.getInt(PREF_KEY_RADIO_BAND, RadioManager.BAND_FM);
    }

    /**
     * Returns the stored radio channel that was set in {@link #storeRadioChannel(int, int)}. If a
     * radio channel for the given band has not been previously stored, then
     * {@link #INVALID_RADIO_CHANNEL} is returned.
     *
     * @param band One of the BAND_* values from {@link RadioManager}. For example,
     *             {@link RadioManager#BAND_AM}.
     */
    public long getStoredRadioChannel(int band) {
        switch (band) {
            case RadioManager.BAND_AM:
                return sSharedPref.getLong(PREF_KEY_RADIO_CHANNEL_AM, INVALID_RADIO_CHANNEL);

            case RadioManager.BAND_FM:
                return sSharedPref.getLong(PREF_KEY_RADIO_CHANNEL_FM, INVALID_RADIO_CHANNEL);

            default:
                return INVALID_RADIO_CHANNEL;
        }
    }

    /**
     * Stores a radio channel (i.e. the radio frequency) for a particular band so it can be later
     * retrieved via {@link #getStoredRadioChannel(int band)}.
     */
    public void storeRadioChannel(@NonNull ProgramSelector sel) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "storeRadioChannel(" + sel + ")");
        }

        // TODO(b/73950974): don't store if it's already the same

        int band = ProgramSelectorUtils.getRadioBand(sel);
        if (band != RadioManager.BAND_AM && band != RadioManager.BAND_FM) return;

        SharedPreferences.Editor editor = sSharedPref.edit();
        editor.putInt(PREF_KEY_RADIO_BAND, band);

        long freq = sel.getFirstId(ProgramSelector.IDENTIFIER_TYPE_AMFM_FREQUENCY);
        if (band == RadioManager.BAND_AM) {
            editor.putLong(PREF_KEY_RADIO_CHANNEL_AM, freq);
        }
        if (band == RadioManager.BAND_FM) {
            editor.putLong(PREF_KEY_RADIO_CHANNEL_FM, freq);
        }

        editor.apply();
    }

    /**
     * {@link AsyncTask} that will store a single {@link Program} that is passed to its
     * {@link AsyncTask#execute(Object[])}.
     */
    private class StorePresetAsyncTask extends AsyncTask<Program, Void, Boolean> {
        private static final String TAG = "Em.StorePresetAT";

        @Override
        protected Boolean doInBackground(Program... programs) {
            sRadioDatabase.insertFavorite(programs[0]);
            return true;
        }
    }

    /**
     * {@link AsyncTask} that will remove a single {@link Program} that is passed to its
     * {@link AsyncTask#execute(Object[])}.
     */
    private class RemovePresetAsyncTask extends AsyncTask<ProgramSelector, Void, Boolean> {
        private static final String TAG = "Em.RemovePresetAT";

        @Override
        protected Boolean doInBackground(ProgramSelector... selectors) {
            sRadioDatabase.removeFavorite(selectors[0]);
            return true;
        }
    }
}
