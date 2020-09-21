/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.droidlivetv.favlistsettings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.Button;

public class MarqueeButton extends Button {

    private boolean mIsFocused = false;

    public MarqueeButton(Context context) {
        super(context);
    }

    public MarqueeButton(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public MarqueeButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    /*@Override
    public boolean isFocused() {
        return mIsFocused;
    }

    public void setFocused(boolean value) {
        mIsFocused = value;
        setText(getText());//update marquee effects
    }*/
}

