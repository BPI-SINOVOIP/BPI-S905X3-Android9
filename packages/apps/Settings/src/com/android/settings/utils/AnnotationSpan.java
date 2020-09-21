/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.settings.utils;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.text.Annotation;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.TextPaint;
import android.text.style.URLSpan;
import android.util.Log;
import android.view.View;

/**
 * This class is used to add {@link View.OnClickListener} for the text been wrapped by
 * annotation.
 */
public class AnnotationSpan extends URLSpan {

    private final View.OnClickListener mClickListener;

    private AnnotationSpan(View.OnClickListener lsn) {
        super((String) null);
        mClickListener = lsn;
    }

    @Override
    public void onClick(View widget) {
        if (mClickListener != null) {
            mClickListener.onClick(widget);
        }
    }

    @Override
    public void updateDrawState(TextPaint ds) {
        super.updateDrawState(ds);
        ds.setUnderlineText(false);
    }

    public static CharSequence linkify(CharSequence rawText, LinkInfo... linkInfos) {
        SpannableString msg = new SpannableString(rawText);
        Annotation[] spans = msg.getSpans(0, msg.length(), Annotation.class);
        SpannableStringBuilder builder = new SpannableStringBuilder(msg);
        for (Annotation annotation : spans) {
            final String key = annotation.getValue();
            int start = msg.getSpanStart(annotation);
            int end = msg.getSpanEnd(annotation);
            AnnotationSpan link = null;
            for (LinkInfo linkInfo : linkInfos) {
                if (linkInfo.mAnnotation.equals(key)) {
                    link = new AnnotationSpan(linkInfo.mListener);
                    break;
                }
            }
            if (link != null) {
                builder.setSpan(link, start, end, msg.getSpanFlags(link));
            }
        }
        return builder;
    }

    /**
     * Data class to store the annotation and the click action
     */
    public static class LinkInfo {
        private static final String TAG = "AnnotationSpan.LinkInfo";
        public static final String DEFAULT_ANNOTATION = "link";
        private final String mAnnotation;
        private final Boolean mActionable;
        private final View.OnClickListener mListener;

        public LinkInfo(String annotation, View.OnClickListener listener) {
            mAnnotation = annotation;
            mListener = listener;
            mActionable = true; // assume actionable
        }

        public LinkInfo(Context context, String annotation, Intent intent) {
            mAnnotation = annotation;
            if (intent != null) {
                mActionable = context.getPackageManager()
                        .resolveActivity(intent, 0 /* flags */) != null;
            } else {
                mActionable = false;
            }
            if (!mActionable) {
                mListener = null;
            } else {
                mListener = view -> {
                    try {
                        view.startActivityForResult(intent, 0);
                    } catch (ActivityNotFoundException e) {
                        Log.w(TAG, "Activity was not found for intent, " + intent);
                    }
                };
            }
        }

        public boolean isActionable() {
            return mActionable;
        }
    }
}
