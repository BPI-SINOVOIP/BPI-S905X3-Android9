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

package com.android.tv.droidlogic.channelui;

import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.util.Log;

import com.android.tv.ui.sidepanel.Item;
import com.android.tv.R;

public abstract class SingleStepSeekBarItem extends Item {
    private final static String TAG = "SingleStepSeekBarItem";
    private final String mCheckedTitle;
    private final String mUncheckedTitle;
    private final int mDescription;
    private TextView mTextView;
    private SeekBar mSeekBar;
    private TextView mDescriptionView;
    private int mSeekBarMaxVaule = 100;
    private int mShowMaxVaule = 100;
    private int mOffSet = 0;
    private String mExtraDescription = null;

    public SingleStepSeekBarItem(String title, int description) {
        this(title, title, description);
    }

    public SingleStepSeekBarItem(String title, int description, String frequency, int max, int showmax, int offset) {
        this(title, title, description);
        mSeekBarMaxVaule = max;
        mShowMaxVaule = showmax;
        mOffSet = offset;
        mExtraDescription = frequency;
    }

    public SingleStepSeekBarItem(String checkedTitle, String uncheckedTitle, int description) {
        mCheckedTitle = checkedTitle;
        mUncheckedTitle = uncheckedTitle;
        mDescription = description;
    }

    @Override
    protected int getResourceId() {
        return R.layout.option_item_normal_seekbar;
    }

    protected int getSeekBarId() {
        return R.id.seekbar;
    }

    protected int getTitleViewId() {
        return R.id.title;
    }

    protected int getDescriptionViewId() {
        return R.id.description;
    }

    public void setMaxValue(int value) {
        mSeekBarMaxVaule = value;
    }

    public void setDescription(String value) {
        if (mExtraDescription == null) {
            return;
        }
        mExtraDescription = value;
        mDescriptionView.setVisibility(View.VISIBLE);
        mDescriptionView.setText(value);
    }

    public void setTitleText(String value) {
        mTextView.setText(value);
    }

    @Override
    protected void onBind(View view) {
        super.onBind(view);
        mSeekBar = (SeekBar) view.findViewById(getSeekBarId());
        mTextView = (TextView) view.findViewById(getTitleViewId());
        mDescriptionView = (TextView) view.findViewById(getDescriptionViewId());
        if (mExtraDescription != null) {
            mDescriptionView.setVisibility(View.VISIBLE);
            mDescriptionView.setText(mExtraDescription);
        } else {
            mDescriptionView.setVisibility(View.GONE);
        }
        mTextView.setText((mDescription + mOffSet) * mShowMaxVaule/ mSeekBarMaxVaule + "%");
        //mSeekBar.setMax(mSeekBarMaxVaule);
        mSeekBar.setProgress(mDescription);
    }

    @Override
    protected void onUnbind() {
        super.onUnbind();
        mTextView = null;
        mSeekBar = null;
        mDescriptionView.setVisibility(View.GONE);
        mDescriptionView = null;
        mExtraDescription = null;
    }

    @Override
    protected void onUpdate() {
        super.onUpdate();
        if (mTextView != null) {
            mTextView.setText((mDescription + mOffSet) * mShowMaxVaule/ mSeekBarMaxVaule + "%");
        }
        if (mDescriptionView != null && mExtraDescription != null) {
            mDescriptionView.setText(mExtraDescription);
        }
        //updateInternal();
    }
    /*
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
    }*/
    @Override
    protected void onSelected() {
        //setChecked(true);
    }

    @Override
    protected void onProgressChanged(SeekBar seekBar, int progress) {
        mTextView.setText((progress + mOffSet) * mShowMaxVaule / mSeekBarMaxVaule + "%");
    }
}
