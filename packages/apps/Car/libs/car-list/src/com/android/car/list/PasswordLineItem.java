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

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;

import com.android.car.list.R;

/**
 * Contains logic for a line item represents text only view of a title and a EditText
 * as password input followed by a checkbox to toggle between show/hide password.
 */
public class PasswordLineItem extends EditTextLineItem<PasswordLineItem.ViewHolder> {
    private boolean mShowPassword = false;

    /**
     * Constructs PasswordLineItem with an empty default EditText
     */
    public PasswordLineItem(CharSequence title) {
        super(title, null);
    }

    @Override
    public int getType() {
        return PASSWORD_TYPE;
    }

    /**
     * ViewHolder that extends the EditTextLineItem ViewHolder and adds
     * a show password checkbox.
     */
    public static class ViewHolder extends EditTextLineItem.ViewHolder {
        public final CheckBox checkbox;

        public ViewHolder(View view) {
            super(view);
            checkbox = view.findViewById(R.id.checkbox);
        }
    }

    @Override
    public void bindViewHolder(PasswordLineItem.ViewHolder viewHolder) {
        // setTextType is public but with PasswordLineItem it should only be
        // set to be one of the two types as follows so we use super and
        // throw exception on our setTextType.
        super.setTextType(mShowPassword ? TextType.VISIBLE_PASSWORD : TextType.HIDDEN_PASSWORD);
        super.bindViewHolder(viewHolder);
        viewHolder.checkbox.setChecked(mShowPassword);
        viewHolder.checkbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            mShowPassword = isChecked;
            super.setTextType(mShowPassword
                    ? TextType.VISIBLE_PASSWORD : TextType.HIDDEN_PASSWORD);
            viewHolder.editText.setInputType(mTextType.getValue());
        });
    }

    @Override
    public void setTextType(TextType textType) {
        throw new IllegalArgumentException("checkbox will automatically set TextType.");
    }

    /**
     * Creates a ViewHolder with the elements of a PasswordLineItem contained
     */
    public static RecyclerView.ViewHolder createViewHolder(ViewGroup parent) {
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                .inflate(R.layout.password_line_item, parent, false));
    }
}
