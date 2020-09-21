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

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.android.car.media.common.MediaItemMetadata;

import java.util.Objects;

/**
 * Information necessary to update a {@link BrowseViewHolder}
 */
class BrowseViewData {
    /** {@link com.android.car.media.common.MediaItemMetadata} associated with this item */
    public final MediaItemMetadata mMediaItem;
    /** View type associated with this item */
    @NonNull public final BrowseItemViewType mViewType;
    /** Current state of this item */
    public final BrowseAdapter.State mState;
    /** Text associated with this item */
    public final CharSequence mText;
    /** Click listener to set for this item */
    public final View.OnClickListener mOnClickListener;

    /**
     * Creates a {@link BrowseViewData} for a particular {@link MediaItemMetadata}.
     *
     * @param mediaItem {@link MediaItemMetadata} metadata
     * @param viewType view type to use to represent this item
     * @param state current item state
     * @param onClickListener optional {@link android.view.View.OnClickListener}
     */
    BrowseViewData(MediaItemMetadata mediaItem, @NonNull BrowseItemViewType viewType,
            @NonNull BrowseAdapter.State state, View.OnClickListener onClickListener) {
        mMediaItem = mediaItem;
        mViewType = viewType;
        mState = state;
        mText = null;
        mOnClickListener = onClickListener;
    }

    /**
     * Creates a {@link BrowseViewData} for a given text (normally used for headers or footers)
     * @param text text to set
     * @param viewType view type to use
     * @param onClickListener optional {@link android.view.View.OnClickListener}
     */
    BrowseViewData(@NonNull CharSequence text, @NonNull BrowseItemViewType viewType,
            View.OnClickListener onClickListener) {
        mText = text;
        mViewType = viewType;
        mMediaItem = null;
        mState = null;
        mOnClickListener = onClickListener;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        BrowseViewData item = (BrowseViewData) o;
        return Objects.equals(mMediaItem, item.mMediaItem) &&
                mState == item.mState &&
                mViewType == item.mViewType;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mMediaItem, mState, mViewType);
    }
}
