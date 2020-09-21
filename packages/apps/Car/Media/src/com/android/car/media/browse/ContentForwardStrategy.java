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

import android.os.Bundle;
import com.android.car.media.common.ContentStyleMediaConstants;
import com.android.car.media.common.MediaItemMetadata;

/**
 * Strategy used to group and expand media items in the {@link BrowseAdapter}
 */
public interface ContentForwardStrategy {
    /**
     * @return true if a header should be included when expanding the given media item into a
     * section. Only used if {@link #shouldBeExpanded(MediaItemMetadata)} returns true.
     */
    boolean includeHeader(MediaItemMetadata mediaItem);

    /**
     * @return maximum number of rows to use when when expanding the given media item into a
     * section. The number can be different depending on the {@link BrowseItemViewType} that
     * will be used to represent media item children (i.e.: we might allow more rows for lists
     * than for grids). Only used if {@link #shouldBeExpanded(MediaItemMetadata)} returns true.
     */
    int getMaxRows(MediaItemMetadata mediaItem, BrowseItemViewType viewType);

    /**
     * @return whether the given media item should be expanded or not. If not expanded, the item
     * will be displayed according to its parent preferred view type.
     */
    boolean shouldBeExpanded(MediaItemMetadata mediaItem);

    /**
     * @return view type to use to render browsable children of the given media item. Only used if
     * {@link #shouldBeExpanded(MediaItemMetadata)} returns true.
     */
    BrowseItemViewType getBrowsableViewType(MediaItemMetadata mediaItem);

    /**
     * @return view type to use to render playable children fo the given media item. Only used if
     * {@link #shouldBeExpanded(MediaItemMetadata)} returns true.
     */
    BrowseItemViewType getPlayableViewType(MediaItemMetadata mediaItem);

    /**
     * @return true if a "more" button should be displayed as a footer for a section displaying the
     * given media item, in case that there item has more children than the ones that can be
     * displayed according to {@link #getMaxQueueRows()}. Only used if
     * {@link #shouldBeExpanded(MediaItemMetadata)} returns true.
     */
    boolean showMoreButton(MediaItemMetadata mediaItem);

    /**
     * @return maximum number of items to show for the media queue, if one is provided.
     */
    int getMaxQueueRows();

    /**
     * @return view type to use to display queue items.
     */
    BrowseItemViewType getQueueViewType();

    /**
     * Default strategy
     * TODO(b/77646944): Expand this implementation to honor the media source expectations.
     */
    ContentForwardStrategy DEFAULT_STRATEGY = new ContentForwardStrategy() {
        @Override
        public boolean includeHeader(MediaItemMetadata mediaItem) {
            return true;
        }

        @Override
        public int getMaxRows(MediaItemMetadata mediaItem, BrowseItemViewType viewType) {
            return viewType == BrowseItemViewType.GRID_ITEM ? 2 : 8;
        }

        @Override
        public boolean shouldBeExpanded(MediaItemMetadata mediaItem) {
            return true;
        }

        @Override
        public BrowseItemViewType getBrowsableViewType(MediaItemMetadata mediaItem) {
            return (mediaItem.getBrowsableContentStyleHint()
                    == ContentStyleMediaConstants.CONTENT_STYLE_LIST_ITEM_HINT_VALUE)
                    ? BrowseItemViewType.LIST_ITEM
                    : BrowseItemViewType.PANEL_ITEM;
        }

        @Override
        public BrowseItemViewType getPlayableViewType(MediaItemMetadata mediaItem) {
            return (mediaItem.getPlayableContentStyleHint()
                    == ContentStyleMediaConstants.CONTENT_STYLE_LIST_ITEM_HINT_VALUE)
                    ? BrowseItemViewType.LIST_ITEM
                    : BrowseItemViewType.GRID_ITEM;
        }

        @Override
        public boolean showMoreButton(MediaItemMetadata mediaItem) {
            return false;
        }

        @Override
        public int getMaxQueueRows() {
            return 8;
        }

        @Override
        public BrowseItemViewType getQueueViewType() {
            return BrowseItemViewType.LIST_ITEM;
        }
    };
}
