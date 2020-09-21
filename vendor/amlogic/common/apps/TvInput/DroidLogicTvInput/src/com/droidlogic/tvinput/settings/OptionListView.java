/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.app.Activity;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ListView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.util.AttributeSet;
import android.util.Log;

import com.droidlogic.tvinput.R;

public class OptionListView extends ListView implements OnItemSelectedListener {
    private static final String TAG = "OptionListView";
    private Context mContext;
    private int selectedPosition = 0;

    public OptionListView (Context context) {
        super(context);
    }
    public OptionListView (Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        setOnItemSelectedListener(this);
    }

    public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    selectedPosition = 0;
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    return true;
                case KeyEvent.KEYCODE_DPAD_UP:
                    if (selectedPosition == 0 &&
                        (getOptionUiManager().getOptionTag() != OptionUiManager.OPTION_AUTO_SEARCH
                         && getOptionUiManager().getOptionTag() != OptionUiManager.OPTION_MANUAL_SEARCH))
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (selectedPosition == getAdapter().getCount() - 1)
                        return true;
                    break;
            }

            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    View selectedView = getSelectedView();
                    if ( selectedView != null)
                        setItemTextColor(selectedView, false);
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        selectedPosition = position;
        if (view != null) {
            if (hasFocus()) {
                setItemTextColor(view, true);
            } else {
                setItemTextColor(view, false);
            }
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    protected void onFocusChanged (boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        View item = getChildAt(selectedPosition);

        if (item != null) {
            if (gainFocus) {
                setItemTextColor(item, true);
            } else {
                setItemTextColor(item, false);
            }
        }
    }

    private void setItemTextColor (View view, boolean focused) {
        TextView item_name = null;
        TextView item_status = null;

        View child0 = ((ViewGroup)view).getChildAt(0);
        if (child0 instanceof TextView)
            item_name = (TextView)child0;

        View child1 = ((ViewGroup)view).getChildAt(1);
        if (child1 instanceof TextView)
            item_status = (TextView)child1;

        if (focused) {
            int color_text_focused = mContext.getResources().getColor(R.color.color_text_focused);
            if (item_name != null)
                item_name.setTextColor(color_text_focused);
            if (item_status != null)
                item_status.setTextColor(color_text_focused);
        } else {
            int color_text_item = mContext.getResources().getColor(R.color.color_text_item);
            if (item_name != null)
                item_name.setTextColor(color_text_item);
            if (item_status != null)
                item_status.setTextColor(color_text_item);
        }
    }

    private OptionUiManager getOptionUiManager() {
        return ((TvSettingsActivity)mContext).getOptionUiManager();
    }
}

