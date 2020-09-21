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
 * limitations under the License
 */

package com.android.car.list;

import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.android.car.list.R;

/**
 * Contains logic for a line item represents text only view of a title and a EditText as input.
 */
public class EditTextLineItem<VH extends EditTextLineItem.ViewHolder>
        extends TypedPagedListAdapter.LineItem<VH> {
    private final CharSequence mTitle;
    private final CharSequence mInitialInputText;

    /**
     * Interface that can be implemented to control behavior
     * when the text changes in the EditText.
     */
    public interface TextChangeListener {
        /**
         * Called when the EditText is changed.
         *
         * @param s Contents of EditText
         */
        void textChanged(Editable s);
    }

    private TextChangeListener mTextChangeListener;
    private EditText mEditText;
    protected TextType mTextType = TextType.NONE;

    /**
     * Input flags that determine the way the EditText takes input.
     */
    public enum TextType {
        // None editable text
        NONE(0),
        // text input
        TEXT(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL),
        // password, input is replaced by dot
        HIDDEN_PASSWORD(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD),
        // password, visible.
        VISIBLE_PASSWORD(
                InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);

        private int mValue;

        TextType(int value) {
          mValue = value;
        }

        public int getValue() {
            return mValue;
        }
    }

    /**
     * Construct new EditTextLineItem with no initial text
     */
    public EditTextLineItem(CharSequence title) {
        this(title, null);
    }

    /**
     * Construct new EditTextLineItem with title and an initial text value.
     */
    public EditTextLineItem(CharSequence title, CharSequence initialInputText) {
        mTitle = title;
        mInitialInputText = initialInputText;
    }

    /**
     * Change the text type for the EditText
     */
    public void setTextType(TextType textType) {
        mTextType = textType;
    }

    /**
     * Change which object receives the TextChange calls
     *
     * @param listener listener to be called on text change
     */
    public void setTextChangeListener(TextChangeListener listener) {
        mTextChangeListener = listener;
    }

    /**
     * Returns the text contained in the EditText
     */
    @Nullable
    public String getInput() {
        return mEditText == null ? null : mEditText.getText().toString();
    }

    @Override
    @LineItemType
    public int getType() {
        return EDIT_TEXT_TYPE;
    }

    @Override
    public void bindViewHolder(VH viewHolder) {
        super.bindViewHolder(viewHolder);
        viewHolder.titleView.setText(mTitle);
        mEditText = viewHolder.editText;
        mEditText.setInputType(mTextType.getValue());
        if (mInitialInputText != null) {
            mEditText.setText(mInitialInputText);
        }
        mEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                // don't care
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // dont' care
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (mTextChangeListener != null) {
                    mTextChangeListener.textChanged(s);
                }
            }
        });
    }

    /**
     * ViewHolder that contains the elements that make up an EditTextLineItem,
     * including title and editText.
     */
    public static class ViewHolder extends RecyclerView.ViewHolder {
        public final TextView titleView;
        public final EditText editText;

        public ViewHolder(View view) {
            super(view);
            titleView = view.findViewById(R.id.title);
            editText = view.findViewById(R.id.input);
        }
    }

    /**
     * Creates a ViewHolder with the elements of an EditTextLineItem.
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.edit_text_line_item, parent, false));
    }

    @Override
    public CharSequence getDesc() {
        return null;
    }

    @Override
    public boolean isExpandable() {
        return false;
    }
}
