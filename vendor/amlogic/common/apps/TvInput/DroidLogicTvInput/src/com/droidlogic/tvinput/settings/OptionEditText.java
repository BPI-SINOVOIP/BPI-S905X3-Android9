/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.content.Context;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.EditText;
import android.util.AttributeSet;
import android.util.Log;

import com.droidlogic.tvinput.R;

public class OptionEditText extends EditText implements OnKeyListener {
    private static final String TAG = "OptionEditText";
    private Context mContext;

    public OptionEditText (Context context) {
        super(context);
    }
    public OptionEditText (Context context, AttributeSet attrs) {
        super(context, attrs);
        setOnKeyListener(this);
    }

    @Override
    public boolean onKey (View v, int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN && keyCode == KeyEvent.KEYCODE_CLEAR) {
            String text = getText().toString();
            if (text != null) {
                if (text.length() == 1)
                    setText("");
                else if (text.length() > 1) {
                    setText(text.substring(0, text.length() - 1));
                    setSelection(text.length() - 1);
                }
            }
        }
        return false;
    }
}

