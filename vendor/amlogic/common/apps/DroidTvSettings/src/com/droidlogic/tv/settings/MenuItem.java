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

import com.droidlogic.tv.settings.util.UriUtils;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;

/**
 * Represents a Menu Item
 */
public class MenuItem {

    public interface UriGetter {
        String getUri();
    }

    public interface TextGetter {
        String getText();
    }

    public static class Builder {
        private int mId;
        private String mTitle;
        private TextGetter mDescriptionGetter;
        private UriGetter mImageUriGetter;
        private Intent mIntent;

        public Builder from(MenuItem item) {
            mId = item.mId;
            mTitle = item.mDisplayName;
            mDescriptionGetter = item.mDisplayDescriptionTextGetter;
            mImageUriGetter = item.mImageUriGetter;
            mIntent = item.mIntent;
            return this;
        }

        public Builder id(int id) {
            mId = id;
            return this;
        }

        public Builder title(String title) {
            mTitle = title;
            return this;
        }

        public Builder descriptionGetter(TextGetter descriptionGetter) {
            mDescriptionGetter = descriptionGetter;
            return this;
        }

        public Builder description(String description) {
            return descriptionGetter(new MenuItem.ConstantTextGetter(description));
        }

        public Builder imageUriGetter(UriGetter imageUriGetter) {
            mImageUriGetter = imageUriGetter;
            return this;
        }

        public Builder imageResourceId(Context context, int imageResourceId) {
            return imageResourceId(context.getResources(), imageResourceId);
        }

        public Builder imageResourceId(Resources resources, int imageResourceId) {
            return imageUri(
                    UriUtils.getAndroidResourceUri(resources, imageResourceId));
        }

        public Builder imageUri(String uri) {
            return imageUriGetter(new MenuItem.ConstantUriGetter(uri));
        }

        public Builder intent(Intent intent) {
            mIntent = intent;
            return this;
        }

        public MenuItem build() {
            return new MenuItem(mId, mTitle, mDescriptionGetter, mImageUriGetter,
                    mIntent);
        }
    }

    private static class ConstantUriGetter implements MenuItem.UriGetter {

        private final String mUri;

        public ConstantUriGetter(String uri) {
            mUri = uri;
        }

        @Override
        public String getUri() {
            return mUri;
        }
    }

    private static class ConstantTextGetter implements MenuItem.TextGetter {

        private final String mText;

        public ConstantTextGetter(String text) {
            mText = text;
        }

        @Override
        public String getText() {
            return mText;
        }
    }

    private final int mId;
    private final String mDisplayName;
    private final TextGetter mDisplayDescriptionTextGetter;
    private final UriGetter mImageUriGetter;
    private final Intent mIntent;

    private MenuItem(int id, String displayName,
            TextGetter displayDescriptionTextGetter, UriGetter imageUriGetter,
            Intent intent) {
        mId = id;
        mDisplayName = displayName;
        mDisplayDescriptionTextGetter = displayDescriptionTextGetter;
        mImageUriGetter = imageUriGetter;
        mIntent = intent;
    }

    public int getId() {
        return mId;
    }

    public String getTitle() {
        return mDisplayName;
    }

    public TextGetter getDescriptionGetter() {
        return mDisplayDescriptionTextGetter;
    }

    public UriGetter getImageUriGetter() {
        return mImageUriGetter;
    }

    public Intent getIntent() {
        return mIntent;
    }
}
