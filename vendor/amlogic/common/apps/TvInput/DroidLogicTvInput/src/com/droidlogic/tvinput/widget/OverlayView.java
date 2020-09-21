/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.droidlogic.app.tv.DroidLogicOverlayView;
import com.droidlogic.tvinput.R;

public class OverlayView extends DroidLogicOverlayView {

    public OverlayView(Context context) {
        this(context, null);
    }

    public OverlayView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public OverlayView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    protected void initSubView() {
        mImageView = (ImageView) findViewById(R.id.msg_image);
        mTextView = (TextView) findViewById(R.id.msg_text);
        mSubtitleView = (DTVSubtitleView) findViewById(R.id.subtitle);
        mEasTextView = (TextView) findViewById(R.id.msg_text_eas);
        mTuningImageView = (ImageView) findViewById(R.id.tuning_image);
        mTeletextNumber = (TextView) findViewById(R.id.msg_teletext_number);
    }
}
