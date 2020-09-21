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

package com.android.car.media.widgets;

import android.annotation.NonNull;
import android.content.Context;
import android.content.res.TypedArray;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.car.media.R;
import com.android.car.media.common.MediaItemMetadata;

/**
 * A view representing a media item to be included in the tab bar at the top of the UI.
 */
public class MediaItemTabView extends LinearLayout {
    private final TextView mTitleView;
    private final ImageView mImageView;
    private final MediaItemMetadata mItem;

    /**
     * Creates a new tab for the given media item.
     */
    public MediaItemTabView(@NonNull Context context, @NonNull MediaItemMetadata item) {
        super(context);
        LayoutInflater inflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.tab_view, this, true);
        setOrientation(LinearLayout.VERTICAL);
        setFocusable(true);
        setGravity(Gravity.CENTER);
        setBackground(context.getDrawable(R.drawable.app_item_background));

        int[] attrs = new int[]{android.R.attr.selectableItemBackground};
        TypedArray typedArray = context.obtainStyledAttributes(attrs);
        int backgroundResource = typedArray.getResourceId(0, 0);
        setBackgroundResource(backgroundResource);

        mItem = item;
        mImageView = findViewById(R.id.icon);
        MediaItemMetadata.updateImageView(context, item, mImageView, 0);
        mTitleView = findViewById(R.id.title);
        mTitleView.setText(item.getTitle());
    }

    /**
     * Returns the item represented by this view
     */
    @NonNull
    public MediaItemMetadata getItem() {
        return mItem;
    }
}
