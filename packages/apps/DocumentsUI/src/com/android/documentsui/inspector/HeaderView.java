/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.documentsui.inspector;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.text.Selection;
import android.text.Spannable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.documentsui.ProviderExecutor;
import com.android.documentsui.R;
import com.android.documentsui.ThumbnailLoader;
import com.android.documentsui.base.Display;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.inspector.InspectorController.HeaderDisplay;

import java.util.function.Consumer;

import javax.annotation.Nullable;

/**
 * Organizes and displays the title and thumbnail for a given document
 */
public final class HeaderView extends RelativeLayout implements HeaderDisplay {

    private final Context mContext;
    private final View mHeader;
    private ImageView mThumbnail;
    private final TextView mTitle;
    private Point mImageDimensions;

    public HeaderView(Context context) {
        this(context, null);
    }

    public HeaderView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public HeaderView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mContext = context;
        mHeader = inflater.inflate(R.layout.inspector_header, null);
        mThumbnail = (ImageView) mHeader.findViewById(R.id.inspector_thumbnail);
        mTitle = (TextView) mHeader.findViewById(R.id.inspector_file_title);

        int width = (int) Display.screenWidth((Activity)context);
        int height = mContext.getResources().getDimensionPixelSize(R.dimen.inspector_header_height);
        mImageDimensions = new Point(width, height);
        addView(mHeader);
    }

    @Override
    public void accept(DocumentInfo info, String displayName) {
        loadHeaderImage(info);
        mTitle.setText(displayName);
        mTitle.setCustomSelectionActionModeCallback(
                new HeaderTextSelector(mTitle, this::selectText));
    }

    private void selectText(Spannable text, int start, int stop) {
        Selection.setSelection(text, start, stop);
    }

    private void loadHeaderImage(DocumentInfo doc) {
        if (!doc.isThumbnailSupported()) {
            showImage(doc, null);
        } else {
            Consumer<Bitmap> callback = new Consumer<Bitmap>() {
                @Override
                public void accept(Bitmap thumbnail) {
                    showImage(doc, thumbnail);
                }
            };
            // load the thumbnail async.
            final ThumbnailLoader task = new ThumbnailLoader(doc.derivedUri, mThumbnail,
                    mImageDimensions, doc.lastModified, callback, false);
            task.executeOnExecutor(ProviderExecutor.forAuthority(doc.derivedUri.getAuthority()),
                    doc.derivedUri);
        }
    }

    /**
     * Shows the supplied image, falling back to a mimetype icon if the image is null.
     */
    private void showImage(DocumentInfo info, @Nullable Bitmap thumbnail) {
        if (thumbnail != null) {
            mThumbnail.resetPaddingToInitialValues();
            mThumbnail.setScaleType(ScaleType.CENTER_CROP);
            mThumbnail.setImageBitmap(thumbnail);
        } else {
            mThumbnail.setPadding(0, 0, 0, mTitle.getHeight());

            Drawable mimeIcon =
                    mContext.getContentResolver().getTypeDrawable(info.mimeType);
            mThumbnail.setScaleType(ScaleType.FIT_CENTER);
            mThumbnail.setImageDrawable(mimeIcon);
        }
        mThumbnail.animate().alpha(1.0f).start();
    }
}