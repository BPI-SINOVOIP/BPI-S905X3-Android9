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

package com.android.car.media.browse;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.car.media.common.MediaItemMetadata;

/**
 * Generic {@link RecyclerView.ViewHolder} to use for all views in the {@link BrowseAdapter}
 */
class BrowseViewHolder extends RecyclerView.ViewHolder {
    final TextView mTitle;
    final TextView mSubtitle;
    final ImageView mAlbumArt;
    final ViewGroup mContainer;

    /**
     * Creates a {@link BrowseViewHolder} for the given view.
     */
    BrowseViewHolder(View itemView) {
        super(itemView);
        mTitle = itemView.findViewById(com.android.car.media.R.id.title);
        mSubtitle = itemView.findViewById(com.android.car.media.R.id.subtitle);
        mAlbumArt = itemView.findViewById(com.android.car.media.R.id.thumbnail);
        mContainer = itemView.findViewById(com.android.car.media.R.id.container);
    }

    /**
     * Updates this {@link BrowseViewHolder} with the given data
     */
    public void bind(Context context, BrowseViewData data) {
        if (mTitle != null) {
            mTitle.setText(data.mText != null
                    ? data.mText : data.mMediaItem != null
                    ? data.mMediaItem.getTitle()
                    : null);
        }
        if (mSubtitle != null) {
            mSubtitle.setText(data.mMediaItem != null
                    ? data.mMediaItem.getSubtitle()
                    : null);
            mSubtitle.setVisibility(data.mMediaItem != null && data.mMediaItem.getSubtitle() != null
                    && !data.mMediaItem.getSubtitle().toString().isEmpty()
                    ? View.VISIBLE
                    : View.GONE);
        }
        if (mAlbumArt != null) {
            MediaItemMetadata.updateImageView(context, data.mMediaItem, mAlbumArt, 0);
        }
        if (mContainer != null && data.mOnClickListener != null) {
            mContainer.setOnClickListener(data.mOnClickListener);
        }
    }
}
