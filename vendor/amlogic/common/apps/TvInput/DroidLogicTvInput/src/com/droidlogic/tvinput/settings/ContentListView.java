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
import android.widget.RadioButton;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.Switch;
import android.widget.TextView;
import android.util.AttributeSet;
import android.util.Log;

import com.droidlogic.tvinput.R;
import com.droidlogic.app.tv.TvControlManager;

public class ContentListView extends ListView implements OnItemSelectedListener {
    private static final String TAG = "ContentListView";
    private Context mContext;
    private int selectedPosition = 0;

    public ContentListView (Context context) {
        super(context);
    }
    public ContentListView (Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
        setOnItemSelectedListener(this);
    }

    public ContentListView (Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_UP:
                    String currentTag = getSettingsManager().getTag();
                    if (selectedPosition == 0)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (selectedPosition == getChildCount() - 1)
                        return true;
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    //selectedPosition = 0;
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    setMenuAlpha(false);
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
        if (hasFocus()) {
            setItemTextColor(view, true);
            createOptionView(position);
        } else {
            setItemTextColor(view, false);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    protected void onFocusChanged (boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        if (gainFocus) {
            setMenuAlpha(true);
            setItemTextColor(getChildAt(selectedPosition), true);
            createOptionView(selectedPosition);
        } else {
            setItemTextColor(getChildAt(selectedPosition), false);
        }
    }

    private void createOptionView (int position) {
        RelativeLayout main_view = (RelativeLayout)((TvSettingsActivity)mContext).findViewById(R.id.main);
        View item_view = getChildAt(position);

        if (((TvSettingsActivity)mContext).mOptionLayout != null)
            main_view.removeView(((TvSettingsActivity)mContext).mOptionLayout);

        ((TvSettingsActivity)mContext).mOptionLayout = new RelativeLayout(mContext);

        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        lp.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        Rect rect = new Rect();
        item_view.getGlobalVisibleRect(rect);
        lp.leftMargin = rect.right - dipToPx(mContext, 30f);

        //set optionview positon, 100f means make option up, 466f means option view backgroud png's height.
        if (rect.top - dipToPx(mContext, 100f) + dipToPx(mContext, 526f)
            <= mContext.getResources().getDisplayMetrics().heightPixels) {
            lp.topMargin = rect.top - dipToPx(mContext, 100f);
        } else {
            lp.topMargin = mContext.getResources().getDisplayMetrics().heightPixels - dipToPx(mContext, 526f);
        }
        ((TvSettingsActivity)mContext).mOptionLayout.setLayoutParams(lp);
        ((TvSettingsActivity)mContext).mOptionLayout.setBackgroundResource(R.drawable.background_option);

        main_view.addView(((TvSettingsActivity)mContext).mOptionLayout);
        createOptionChildView(((TvSettingsActivity)mContext).mOptionLayout, position);
    }

    private void createOptionChildView (View option_view, int position) {
        LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        getOptionUiManager().setOptionTag(position);
        int layout_option_child = getOptionUiManager().getLayoutId();
        if (layout_option_child > 0) {
            View view = inflater.inflate(layout_option_child, null);
            ((RelativeLayout)option_view).addView(view);
            getOptionUiManager().setProgressStatus();
            getOptionUiManager().setOptionListener(view);
            if (getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_CHANNEL_INFO
                || getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_AUDIO_TRACK
                || getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_DEFAULT_LANGUAGE
                || getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_FBC_UPGRADE
                || getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_AD_LIST) {
                OptionListLayout optionListLayout = new OptionListLayout(mContext, view, getOptionUiManager().getOptionTag());
            } else if (getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_CHANNEL_EDIT) {
                ChannelEdit channelEdit = new ChannelEdit(mContext, view);
            }

            //set options view's focus
            if (getOptionUiManager().getOptionTag() == OptionUiManager.OPTION_MANUAL_SEARCH) {
                getOptionUiManager().setManualSearchEditStyle(view);
            }
            View firstFocusableChild = null;
            View lastFocusableChild = null;
            for (int i = 0; i < ((ViewGroup)view).getChildCount(); i++) {
                View child = ((ViewGroup)view).getChildAt(i);
                if (child != null && child.hasFocusable()) {
                    if (firstFocusableChild == null) {
                        firstFocusableChild = child;
                    }
                    child.setNextFocusLeftId(R.id.content_list);
                    lastFocusableChild = child;
                }
            }
            if (firstFocusableChild != null && lastFocusableChild != null) {
                firstFocusableChild.setNextFocusUpId(firstFocusableChild.getId());
                lastFocusableChild.setNextFocusDownId(lastFocusableChild.getId());
            }

            getOptionUiManager().setChoosedIcon();
            if (layout_option_child == R.layout.layout_sound_virtual_surround) {
                ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
                TextView mVirtualSurroundIn = (TextView) parent.findViewById(R.id.virtual_surround_increase);
                TextView mVirtualSurroundDe = (TextView) parent.findViewById(R.id.virtual_surround_decrease);
                RelativeLayout mVirtualSurroundProcess = (RelativeLayout) parent.findViewById(R.id.virtual_surround_persent);
                TvControlManager mTvControlManager = TvControlManager.getInstance();
                if (mTvControlManager.GetAudioVirtualizerEnable() == 0) {
                    mVirtualSurroundIn.setVisibility(View.INVISIBLE);
                    mVirtualSurroundDe.setVisibility(View.INVISIBLE);
                    mVirtualSurroundProcess.setVisibility(View.INVISIBLE);
                } else {
                    mVirtualSurroundIn.setVisibility(View.VISIBLE);
                    mVirtualSurroundDe.setVisibility(View.VISIBLE);
                    mVirtualSurroundProcess.setVisibility(View.VISIBLE);
                }
            }

            if (layout_option_child == R.layout.layout_channel_auto_search_dtv_for_atsc) {
                ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
                ((TvSettingsActivity)mContext).mScanEdit = new ScanEdit(mContext, parent);
            }
            if (layout_option_child == R.layout.layout_channel_manual_serch_dtv_for_atsc) {
                ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
                ((TvSettingsActivity)mContext).mManualScanEdit = new ManualScanEdit(mContext, parent);
            }
        }
    }

    public void setContentFocus (View relatedButton) {
        setNextFocusLeftId(relatedButton.getId());
        setNextFocusUpId(getId());
        setNextFocusDownId(getId());
    }

    private void setItemTextColor (View view, boolean focused) {
        if (view != null) {
            TextView item_name = (TextView)view.findViewById(R.id.item_name);
            TextView item_status = (TextView)view.findViewById(R.id.item_status);
            if (focused) {
                int color_text_focused = mContext.getResources().getColor(R.color.color_text_focused);
                item_name.setTextColor(color_text_focused);
                item_status.setTextColor(color_text_focused);
            } else {
                int color_text_item = mContext.getResources().getColor(R.color.color_text_item);
                item_name.setTextColor(color_text_item);
                item_status.setTextColor(color_text_item);
            }
        }
    }

    private void setMenuAlpha (boolean focused) {
        View view = ((TvSettingsActivity)mContext).findViewById(R.id.menu_and_content);
        if (focused) {
            view.getBackground().setAlpha(OptionUiManager.ALPHA_FOCUSED);
        } else {
            view.getBackground().setAlpha(OptionUiManager.ALPHA_NO_FOCUS);
        }
    }

    public static int pxToDip(Context context, float pxValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (pxValue / scale + 0.5f);
    }

    public static int dipToPx(Context context, float dpValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dpValue * scale + 0.5f);
    }

    private SettingsManager getSettingsManager() {
        return ((TvSettingsActivity)mContext).getSettingsManager();
    }

    private OptionUiManager getOptionUiManager() {
        return ((TvSettingsActivity)mContext).getOptionUiManager();
    }
}
