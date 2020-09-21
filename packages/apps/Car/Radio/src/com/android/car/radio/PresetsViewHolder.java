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

import android.annotation.NonNull;
import android.content.Context;
import android.graphics.drawable.GradientDrawable;
import android.hardware.radio.ProgramSelector;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.TextView;

import com.android.car.broadcastradio.support.Program;
import com.android.car.broadcastradio.support.platform.ProgramSelectorExt;
import com.android.car.view.CardListBackgroundResolver;

import java.util.Objects;

/**
 * A {@link RecyclerView.ViewHolder} that can bind a {@link Program} to the layout
 * {@code R.layout.radio_preset_item}.
 */
public class PresetsViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
    private static final String TAG = "Em.PresetVH";

    private final RadioChannelColorMapper mColorMapper;

    private final OnPresetClickListener mPresetClickListener;
    private final OnPresetFavoriteListener mPresetFavoriteListener;

    private final Context mContext;
    private final View mPresetsCard;
    private GradientDrawable mPresetItemChannelBg;
    private final TextView mPresetItemChannel;
    private final TextView mPresetItemMetadata;
    private ImageButton mPresetButton;

    /**
     * Interface for a listener when the View held by this ViewHolder has been clicked.
     */
    public interface OnPresetClickListener {
        /**
         * Method to be called when the View in this ViewHolder has been clicked.
         *
         * @param position The position of the View within the RecyclerView this ViewHolder is
         *                 populating.
         */
        void onPresetClicked(int position);
    }


    /**
     * Interface for a listener that will be notified when a favorite in the presets list has been
     * toggled.
     */
    public interface OnPresetFavoriteListener {

        /**
         * Method called when an item's favorite status has been toggled
         */
        void onPresetFavoriteChanged(int position, boolean saveAsFavorite);
    }


    /**
     * @param presetsView A view that contains the layout {@code R.layout.radio_preset_item}.
     */
    public PresetsViewHolder(@NonNull View presetsView, @NonNull OnPresetClickListener listener,
            @NonNull OnPresetFavoriteListener favoriteListener) {
        super(presetsView);

        mContext = presetsView.getContext();

        mPresetsCard = presetsView.findViewById(R.id.preset_card);;
        mPresetsCard.setOnClickListener(this);

        mColorMapper = RadioChannelColorMapper.getInstance(mContext);
        mPresetClickListener = Objects.requireNonNull(listener);
        mPresetFavoriteListener = Objects.requireNonNull(favoriteListener);

        mPresetItemChannel = presetsView.findViewById(R.id.preset_station_channel);
        mPresetItemMetadata = presetsView.findViewById(R.id.preset_item_metadata);
        mPresetButton = presetsView.findViewById(R.id.preset_button);

        mPresetItemChannelBg = (GradientDrawable)
                presetsView.findViewById(R.id.preset_station_background).getBackground();
    }

    @Override
    public void onClick(View view) {
        if (Log.isLoggable(TAG, Log.DEBUG)) {
            Log.d(TAG, "onClick() for view at position: " + getAdapterPosition());
        }

        mPresetClickListener.onPresetClicked(getAdapterPosition());
    }

    /**
     * Binds the given {@link Program} to this View within this ViewHolder.
     */
    public void bindPreset(Program program, boolean isActiveStation, int itemCount,
            boolean isFavorite) {
        // If the preset is null, clear any existing text.
        if (program == null) {
            mPresetItemChannel.setText(null);
            mPresetItemMetadata.setText(null);
            mPresetItemChannelBg.setColor(mColorMapper.getDefaultColor());
            return;
        }

        ProgramSelector sel = program.getSelector();

        CardListBackgroundResolver.setBackground(mPresetsCard, getAdapterPosition(), itemCount);

        mPresetItemChannel.setText(ProgramSelectorExt.getDisplayName(
                sel, ProgramSelectorExt.NAME_NO_MODULATION));
        if (isActiveStation) {
            mPresetItemChannel.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    R.drawable.ic_equalizer, 0, 0, 0);
        } else {
            mPresetItemChannel.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
        }

        mPresetItemChannelBg.setColor(mColorMapper.getColorForProgram(sel));

        String programName = program.getName();
        if (programName.isEmpty()) {
            // If there is no metadata text, then use text to indicate the favorite number to the
            // user so that list does not appear empty.
            mPresetItemMetadata.setText(mContext.getString(
                    R.string.radio_default_preset_metadata_text, getAdapterPosition() + 1));
        } else {
            mPresetItemMetadata.setText(programName);
        }
        setFavoriteButtonFilled(isFavorite);
        mPresetButton.setOnClickListener(v -> {
            boolean favoriteToggleOn =
                    ((Integer) mPresetButton.getTag() == R.drawable.ic_star_empty);
            setFavoriteButtonFilled(favoriteToggleOn);
            mPresetFavoriteListener.onPresetFavoriteChanged(getAdapterPosition(), favoriteToggleOn);
        });
    }

    private void setFavoriteButtonFilled(boolean favoriteToggleOn) {
        if (favoriteToggleOn) {
            mPresetButton.setImageResource(R.drawable.ic_star_filled);
            mPresetButton.setTag(R.drawable.ic_star_filled);
        } else {
            mPresetButton.setImageResource(R.drawable.ic_star_empty);
            mPresetButton.setTag(R.drawable.ic_star_empty);
        }
    }
}
