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

package com.android.settings.wifi;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceViewHolder;
import android.text.Spannable;
import android.text.method.LinkMovementMethod;
import android.text.style.TextAppearanceSpan;
import android.util.AttributeSet;
import android.widget.TextView;
import com.android.settings.LinkifyUtils;

/**
 * A preference with a title that can have linkable content on click.
 */
public class LinkablePreference extends Preference {

    private LinkifyUtils.OnClickListener mClickListener;
    private CharSequence mContentTitle;
    private CharSequence mContentDescription;

    public LinkablePreference(Context ctx, AttributeSet attrs, int defStyle) {
        super(ctx, attrs, defStyle);
        setSelectable(false);
    }

    public LinkablePreference(Context ctx, AttributeSet attrs) {
        super(ctx, attrs);
        setSelectable(false);
    }

    public LinkablePreference(Context ctx) {
        super(ctx);
        setSelectable(false);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder view) {
        super.onBindViewHolder(view);

        TextView textView = (TextView) view.findViewById(android.R.id.title);
        if (textView == null) {
            return;
        }
        textView.setSingleLine(false);

        if (mContentTitle == null || mClickListener == null) {
            return;
        }

        StringBuilder contentBuilder = new StringBuilder().append(mContentTitle);
        if (mContentDescription != null) {
            contentBuilder.append("\n\n");
            contentBuilder.append(mContentDescription);
        }

        boolean linked = LinkifyUtils.linkify(textView, contentBuilder, mClickListener);
        if (linked && mContentTitle != null) {
            // Embolden and enlarge the title.
            Spannable boldSpan = (Spannable) textView.getText();
            boldSpan.setSpan(
                    new TextAppearanceSpan(
                            getContext(), android.R.style.TextAppearance_Medium),
                    0,
                    mContentTitle.length(),
                    Spannable.SPAN_INCLUSIVE_EXCLUSIVE);
            textView.setText(boldSpan);
            textView.setMovementMethod(new LinkMovementMethod());
        }
    }

    /**
     * Sets the linkable text for the Preference title.
     * @param contentTitle text to set the Preference title.
     * @param contentDescription description text to append underneath title, can be null.
     * @param clickListener OnClickListener for the link portion of the text.
     */
    public void setText(
            CharSequence contentTitle,
            @Nullable CharSequence contentDescription,
            LinkifyUtils.OnClickListener clickListener) {
        mContentTitle = contentTitle;
        mContentDescription = contentDescription;
        mClickListener = clickListener;
        // sets the title so that the title TextView is not hidden in super.onBindViewHolder()
        super.setTitle(contentTitle);
    }

    /**
     * Sets the title of the LinkablePreference. resets linkable text for reusability.
     */
    @Override
    public void setTitle(int titleResId) {
        mContentTitle = null;
        mContentDescription = null;
        super.setTitle(titleResId);
    }

    /**
     * Sets the title of the LinkablePreference. resets linkable text for reusability.
     */
    @Override
    public void setTitle(CharSequence title) {
        mContentTitle = null;
        mContentDescription = null;
        super.setTitle(title);
    }
}
