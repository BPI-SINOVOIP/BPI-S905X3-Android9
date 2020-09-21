/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.droidlogic.droidlivetv.ui;

import android.view.View;
import android.widget.CompoundButton;
import android.widget.TextView;

import com.droidlogic.droidlivetv.R;

public abstract class CompoundButtonItem extends Item {
    private final String mCheckedTitle;
    private final String mUncheckedTitle;
    private final String mDescription;
    private boolean mChecked;
    private TextView mTextView;
    private CompoundButton mCompoundButton;

    public CompoundButtonItem(String title, String description) {
        this(title, title, description);
    }

    public CompoundButtonItem(String checkedTitle, String uncheckedTitle, String description) {
        mCheckedTitle = checkedTitle;
        mUncheckedTitle = uncheckedTitle;
        mDescription = description;
    }

    protected abstract int getCompoundButtonId();

    protected int getTitleViewId() {
        return R.id.title;
    }

    protected int getDescriptionViewId() {
        return R.id.description;
    }

    @Override
    protected void onBind(View view) {
        super.onBind(view);
        mCompoundButton = (CompoundButton) view.findViewById(getCompoundButtonId());
        mTextView = (TextView) view.findViewById(getTitleViewId());
        TextView descriptionView = (TextView) view.findViewById(getDescriptionViewId());
        if (mDescription != null) {
            descriptionView.setVisibility(View.VISIBLE);
            descriptionView.setText(mDescription);
        } else {
            descriptionView.setVisibility(View.GONE);
        }
    }

    @Override
    protected void onUnbind() {
        super.onUnbind();
        mTextView = null;
        mCompoundButton = null;
    }

    @Override
    protected void onUpdate() {
        super.onUpdate();
        updateInternal();
    }

    public void setChecked(boolean checked) {
        if (mChecked != checked) {
            mChecked = checked;
            updateInternal();
        }
    }

    public boolean isChecked() {
        return mChecked;
    }

    private void updateInternal() {
        if (isBound()) {
            mTextView.setText(mChecked ? mCheckedTitle : mUncheckedTitle);
            mCompoundButton.setChecked(mChecked);
        }
    }
}
