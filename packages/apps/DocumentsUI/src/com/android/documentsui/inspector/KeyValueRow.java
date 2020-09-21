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

import android.annotation.StringRes;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Paint;
import android.support.annotation.Nullable;
import android.text.Selection;
import android.text.Spannable;
import android.util.AttributeSet;
import android.view.View;
import android.view.textclassifier.TextClassifier;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.documentsui.R;

/**
 * Class representing a row in the table.
 */
public class KeyValueRow extends LinearLayout {

    private final Resources mRes;
    private @Nullable ColorStateList mDefaultTextColor;
    private @Nullable TextClassifier mClassifier;

    public KeyValueRow(Context context) {
        this(context, null);
    }

    public KeyValueRow(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public KeyValueRow(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mRes = context.getResources();
    }

    public void setTextClassifier(TextClassifier classifier) {
        mClassifier = classifier;
    }

    /**
     * Sets the raw value of the key. Only localized values
     * should be passed.
     */
    public void setKey(CharSequence key) {
        ((TextView) findViewById(R.id.table_row_key)).setText(key);
    }

    public void setKey(@StringRes int id) {
        setKey(mRes.getString(id));
    }

    public void setValue(CharSequence value) {
        TextView text = ((TextView) findViewById(R.id.table_row_value));
        text.setText(value);
        text.setTextClassifier(mClassifier);
        text.setOnLongClickListener((View view) -> {

            CharSequence textValue = text.getText();
            if (textValue instanceof Spannable) {
                Spannable spn = (Spannable) textValue;
                Selection.selectAll(spn);
            }
            // we still want the default selection arrows and menu after we specified to select
            // all text in the TextView.
            return false;
        });
    }

    @Override
    public void setOnClickListener(OnClickListener callback) {
        TextView clickable = ((TextView) findViewById(R.id.table_row_value));
        mDefaultTextColor = clickable.getTextColors();
        clickable.setTextColor(R.color.inspector_link);
        clickable.setPaintFlags(clickable.getPaintFlags() | Paint.UNDERLINE_TEXT_FLAG);
        clickable.setOnClickListener(callback);
    }

    public void removeOnClickListener() {
        TextView reset = ((TextView) findViewById(R.id.table_row_value));
        if (mDefaultTextColor != null) {
            reset.setTextColor(mDefaultTextColor);
        }
        reset.setPaintFlags(reset.getPaintFlags() & ~Paint.UNDERLINE_TEXT_FLAG);
        reset.setOnClickListener(null);
    }
}
