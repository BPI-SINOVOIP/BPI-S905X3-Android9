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

import android.annotation.Nullable;
import android.hardware.radio.ProgramSelector;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.car.widget.PagedListView;

import com.android.car.broadcastradio.support.Program;

import java.util.List;
import java.util.Objects;

/**
 * Adapter that will display a list of radio stations that represent the user's presets.
 */
public class PresetsAdapter extends RecyclerView.Adapter<PresetsViewHolder>
        implements PagedListView.ItemCap {
    private static final String TAG = "Em.PresetsAdapter";

    // Only one type of view in this adapter.
    private static final int PRESETS_VIEW_TYPE = 0;

    private Program mActiveProgram;

    private List<Program> mPresets;
    private OnPresetItemClickListener mPresetClickListener;
    private OnPresetItemFavoriteListener mPresetFavoriteListener;

    /**
     * Interface for a listener that will be notified when an item in the presets list has been
     * clicked.
     */
    public interface OnPresetItemClickListener {
        /**
         * Method called when an item in the preset list has been clicked.
         *
         * @param selector The {@link ProgramSelector} corresponding to the clicked preset.
         */
        void onPresetItemClicked(ProgramSelector selector);
    }

    /**
     * Interface for a listener that will be notified when a favorite in the presets list has been
     * toggled.
     */
    public interface OnPresetItemFavoriteListener {

        /**
         * Method called when an item's favorite status has been toggled
         *
         * @param program The {@link Program} corresponding to the clicked preset.
         * @param saveAsFavorite Whether the program should be saved or removed as a favorite.
         */
        void onPresetItemFavoriteChanged(Program program, boolean saveAsFavorite);
    }

    /**
     * Set a listener to be notified whenever a preset card is pressed.
     */
    public void setOnPresetItemClickListener(@Nullable OnPresetItemClickListener listener) {
        mPresetClickListener = Objects.requireNonNull(listener);
    }

    /**
     * Set a listener to be notified whenever a preset favorite is changed.
     */
    public void setOnPresetItemFavoriteListener(@Nullable OnPresetItemFavoriteListener listener) {
        mPresetFavoriteListener = listener;
    }

    /**
     * Sets the given list as the list of presets to display.
     */
    public void setPresets(List<Program> presets) {
        mPresets = presets;
        notifyDataSetChanged();
    }

    /**
     * Indicates which radio station is the active one inside the list of presets that are set on
     * this adapter. This will cause that station to be highlighted in the list. If the station
     * passed to this method does not match any of the presets, then none will be highlighted.
     */
    public void setActiveProgram(Program program) {
        mActiveProgram = program;
        notifyDataSetChanged();
    }

    @Override
    public PresetsViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.radio_preset_stream_card, parent, false);

        return new PresetsViewHolder(
                view, this::handlePresetClicked, this::handlePresetFavoriteChanged);
    }

    @Override
    public void onBindViewHolder(PresetsViewHolder holder, int position) {
        if (getPresetCount() == 0) {
            holder.bindPreset(mActiveProgram, true, getItemCount(), false);
            return;
        }
        Program station = mPresets.get(position);
        boolean isActiveStation = station.getSelector().equals(mActiveProgram.getSelector());
        holder.bindPreset(station, isActiveStation, getItemCount(), true);
    }

    @Override
    public int getItemViewType(int position) {
        return PRESETS_VIEW_TYPE;
    }

    @Override
    public int getItemCount() {
        int numPresets = getPresetCount();
        return (numPresets == 0) ? 1 : numPresets;
    }

    private int getPresetCount() {
        return (mPresets == null) ? 0 : mPresets.size();
    }

    @Override
    public void setMaxItems(int max) {
        // No-op. A PagedListView needs the ItemCap interface to be implemented. However, the
        // list of presets should not be limited.
    }

    private void handlePresetClicked(int position) {
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, String.format("onPresetClicked(); item count: %d; position: %d",
                    getItemCount(), position));
        }
        if (mPresetClickListener != null && getItemCount() > position) {
            if (getPresetCount() == 0) {
                mPresetClickListener.onPresetItemClicked(mActiveProgram.getSelector());
                return;
            }
            mPresetClickListener.onPresetItemClicked(mPresets.get(position).getSelector());
        }
    }

    private void handlePresetFavoriteChanged (int position, boolean saveAsFavorite) {
        if (mPresetFavoriteListener != null && getItemCount() > position) {
            if (getPresetCount() == 0) {
                mPresetFavoriteListener.onPresetItemFavoriteChanged(mActiveProgram, saveAsFavorite);
                return;
            }
            mPresetFavoriteListener.onPresetItemFavoriteChanged(
                    mPresets.get(position), saveAsFavorite);
        }
    }
}
