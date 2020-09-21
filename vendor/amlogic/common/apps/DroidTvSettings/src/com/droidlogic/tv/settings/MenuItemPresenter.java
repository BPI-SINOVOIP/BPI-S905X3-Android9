/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.droidlogic.tv.settings;

import com.droidlogic.tv.settings.widget.BitmapDownloader;
import com.droidlogic.tv.settings.widget.BitmapWorkerOptions;
import com.droidlogic.tv.settings.widget.BitmapDownloader.BitmapCallback;

import android.animation.ObjectAnimator;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.net.Uri;
import android.support.v17.leanback.widget.Presenter;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageView;
import android.widget.TextView;

public class MenuItemPresenter extends Presenter {

    private static final String TAG = "MenuItemPresenter";

    private static class MenuItemViewHolder extends ViewHolder {
        public final ImageView mIconView;
        public final TextView mTitleView;
        public final TextView mDescriptionView;
        public BitmapCallback mBitmapCallBack;

        MenuItemViewHolder(View v) {
            super(v);
            mIconView = (ImageView) v.findViewById(R.id.icon);
            mTitleView = (TextView) v.findViewById(R.id.title);
            mDescriptionView = (TextView) v.findViewById(R.id.description);
        }

    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent) {
        LayoutInflater inflater = (LayoutInflater) parent.getContext().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        View v = inflater.inflate(R.layout.browse_item, parent, false);
        return new MenuItemViewHolder(v);
    }

    @Override
    public void onBindViewHolder(ViewHolder viewHolder, Object item) {
        if (item instanceof MenuItem && viewHolder instanceof MenuItemViewHolder) {
            final MenuItem menuItem = (MenuItem) item;
            MenuItemViewHolder menuItemViewHolder = (MenuItemViewHolder) viewHolder;

            prepareTextView(menuItemViewHolder.mTitleView, menuItem.getTitle());
            boolean hasDescription = prepareTextView(menuItemViewHolder.mDescriptionView,
                    menuItem.getDescriptionGetter() == null ? null
                    : menuItem.getDescriptionGetter().getText());

            Resources res = menuItemViewHolder.mTitleView.getContext().getResources();
            ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams)
                    menuItemViewHolder.mTitleView.getLayoutParams();
            if (hasDescription) {
                lp.bottomMargin = (int) res.getDimension(R.dimen.browse_item_title_marginBottom);
                menuItemViewHolder.mTitleView.setSingleLine(true);
                menuItemViewHolder.mTitleView.setLines(1);
            } else {
                lp.bottomMargin = (int) res.getDimension(R.dimen.browse_item_description_marginBottom);
                menuItemViewHolder.mTitleView.setSingleLine(false);
                menuItemViewHolder.mTitleView.setLines(2);
            }
            menuItemViewHolder.mTitleView.setLayoutParams(lp);

            viewHolder.view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (v != null && menuItem.getIntent() != null) {
                        try {
                            v.getContext().startActivity(menuItem.getIntent());
                        } catch (ActivityNotFoundException e) {
                            Log.e(TAG, "Activity not found", e);
                        }
                    }
                }
            });

            prepareImageView(menuItemViewHolder, menuItem.getImageUriGetter().getUri());
        }
    }

    @Override
    public void onUnbindViewHolder(ViewHolder viewHolder) {
        if (viewHolder instanceof MenuItemViewHolder) {
            MenuItemViewHolder menuItemViewHolder = (MenuItemViewHolder) viewHolder;
            menuItemViewHolder.mIconView.setImageBitmap(null);
            BitmapDownloader.getInstance(viewHolder.view.getContext()).cancelDownload(
                    menuItemViewHolder.mBitmapCallBack);
        }
    }

    private void prepareImageView(final MenuItemViewHolder menuItemViewHolder, String imageUri) {
        menuItemViewHolder.mIconView.setVisibility(View.INVISIBLE);
        LayoutParams lp = menuItemViewHolder.mIconView.getLayoutParams();
        if (imageUri != null) {
            menuItemViewHolder.mBitmapCallBack = new BitmapCallback() {
                @Override
                public void onBitmapRetrieved(Bitmap bitmap) {
                    if (bitmap != null) {
                        menuItemViewHolder.mIconView.setImageBitmap(bitmap);
                        menuItemViewHolder.mIconView.setVisibility(View.VISIBLE);
                        menuItemViewHolder.mIconView.setAlpha(0f);
                        fadeIn(menuItemViewHolder.mIconView);
                    }
                }
            };

            Context context = menuItemViewHolder.view.getContext();

            BitmapWorkerOptions bitmapWorkerOptions = new BitmapWorkerOptions.Builder(context)
                    .resource(Uri.parse(imageUri)).height(lp.height).width(lp.width).build();

            BitmapDownloader.getInstance(context).getBitmap(bitmapWorkerOptions,
                    menuItemViewHolder.mBitmapCallBack);
        }
    }

    private void fadeIn(View v) {
        ObjectAnimator alphaAnimator = ObjectAnimator.ofFloat(v, "alpha", 1f);
        alphaAnimator.setDuration(v.getContext().getResources().getInteger(
                android.R.integer.config_shortAnimTime));
        alphaAnimator.start();
    }

    private boolean prepareTextView(TextView textView, String title) {
        boolean hasTextView = !TextUtils.isEmpty(title);
        if (hasTextView) {
            textView.setText(title);
            textView.setVisibility(View.VISIBLE);
        } else {
            textView.setVisibility(View.GONE);
        }
        return hasTextView;
    }
}
