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

package com.android.tv.droidlogic.subtitleui;

import android.text.TextUtils;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.media.tv.TvView;

import com.android.tv.data.api.Channel;
import com.android.tv.MainActivity;
import com.android.tv.ui.TunableTvView;

import com.android.tv.R;

public class TeleTextAdvancedSettings {
    public static final String TAG ="TeleTextAdvancedSettings";
    //add for dtvkit teletext action
    public static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";
    public static final String ACTION_EVENT_QUICK_NAVIGATE_1 = "quick_navigate_1";
    public static final String ACTION_EVENT_QUICK_NAVIGATE_2 = "quick_navigate_2";
    public static final String ACTION_EVENT_QUICK_NAVIGATE_3 = "quick_navigate_3";
    public static final String ACTION_EVENT_QUICK_NAVIGATE_4 = "quick_navigate_4";
    public static final String ACTION_EVENT_PREVIOUS_PAGE = "previous_page";
    public static final String ACTION_EVENT_NEXT_PAGE = "next_page";
    public static final String ACTION_EVENT_INDEXPAGE = "index_page";
    public static final String ACTION_EVENT_PREVIOUSSUBPAGE = "previous_sub_page";
    public static final String ACTION_EVENT_NEXTSUBPAGE = "next_sub_page";
    public static final String ACTION_EVENT_BACKPAGE = "back_page";
    public static final String ACTION_EVENT_FORWARDPAGE = "forward_page";
    public static final String ACTION_EVENT_HOLD = "hold";
    public static final String ACTION_EVENT_REVEAL = "reveal";
    public static final String ACTION_EVENT_CLEAR = "clear";
    public static final String ACTION_EVENT_MIX_VIDEO = "mix_video";
    public static final String ACTION_EVENT_DOUBLE_HEIGHT = "double_height";
    public static final String ACTION_EVENT_DOUBLE_SCROLL_UP = "double_scroll_up";
    public static final String ACTION_EVENT_DOUBLE_SCROLL_DOWN = "double_scroll_down";
    //public static final String ACTION_EVENT_TIMER = "timer";

    private final int[] BUTTON_RES = {
            R.id.button_navigate1, R.id.button_navigate2, R.id.button_navigate3, R.id.button_navigate4,//4
            R.id.button_previous_page, R.id.button_next_page, R.id.button_index_page, R.id.button_sub_previous , R.id.button_sub_next, R.id.button_clock,//5
            R.id.button_hold, R.id.button_reveal, R.id.button_clear, R.id.button_mix_video , R.id.button_size,/* R.id.button_timer,*///5
            R.id.button_back_page, R.id.button_forward_page, R.id.button_scroll_up, R.id.button_scroll_down//4
    };
    private final int[] BUTTON_TEXT_RES = {
            R.string.subtitle_teletext_quick_navigate_1, R.string.subtitle_teletext_quick_navigate_2, R.string.subtitle_teletext_quick_navigate_3, R.string.subtitle_teletext_quick_navigate_4,
            R.string.subtitle_teletext_previous_page, R.string.subtitle_teletext_next_page, R.string.subtitle_teletext_index_page, R.string.subtitle_teletext_previous_sub_page, R.string.subtitle_teletext_next_sub_page, R.string.subtitle_teletext_clock,
            R.string.subtitle_teletext_hold, R.string.subtitle_teletext_reveal, R.string.subtitle_teletext_clear, R.string.subtitle_teletext_mix_video, R.string.subtitle_teletext_double_height,/* R.string.subtitle_teletext_timer,*/
            R.string.subtitle_teletext_back_page, R.string.subtitle_teletext_forward_page, R.string.subtitle_teletext_double_scroll_up, R.string.subtitle_teletext_double_scroll_down
    };
    private final String[] ADVANCED_OPTION_ACTION = {
            "quick_navigate_1", "quick_navigate_2", "quick_navigate_3", "quick_navigate_4",//4
            "previous_page", "next_page", "index_page", "previous_sub_page" , "next_sub_page", "clock",//5
            "hold", "reveal", "clear", "mix_video" , "double_height",/* "timer",*///5
            "back_page", "forward_page", "double_scroll_up", "double_scroll_down"//4
    };

    private Context mContext;
    private TvView mTvView;
    private AlertDialog mAlertDialog;

    public TeleTextAdvancedSettings(Context context, TvView tvview) {
        this.mContext = context;
        this.mTvView = tvview;
    }

    public void setTvView(TvView tvview) {
        mTvView = tvview;
    }

    public TvView getTvView() {
        return mTvView;
    }

    private int getIndexByResId(int res) {
        int result = -1;
        for (int i = 0; i < BUTTON_RES.length; i++) {
            if (res == BUTTON_RES[i]) {
                result = i;
                break;
            }
        }
        return result;
    }

    public void hideTeletextAdvancedDialog() {
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
            mAlertDialog = null;
        }
    }

    public void creatTeletextAdvancedDialog() {
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
            mAlertDialog = null;
        }
        final DialogInterface.OnKeyListener OnKeyListener = new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                Log.d(TAG, "creatTeletextAdvancedDialog onKey " + event);
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    dealKeyEvent(keyCode);
                }
                return false;
            }
        };
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext).setOnKeyListener(OnKeyListener);
        mAlertDialog = builder.create();
        final Button.OnClickListener OnClickListener = new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                int index = getIndexByResId(v.getId());
                if (index >= 0) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(ADVANCED_OPTION_ACTION[index],true);
                    if (mTvView != null) {
                        sendCommandByTif(ADVANCED_OPTION_ACTION[index], bundle);
                    }
                }
            }
        };
        final View dialogView = (View)View.inflate(mContext, R.layout.advanced_teletext_function, null);
        for (int i = 0; i < BUTTON_RES.length; i++) {
            dialogView.findViewById(BUTTON_RES[i]).setOnClickListener(OnClickListener);
        }

        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "creatTeletextAdvancedDialog onDismiss");
            }
        });
        mAlertDialog.setView(dialogView);
        mAlertDialog.show();
        mAlertDialog.getWindow().setGravity(Gravity.RIGHT | Gravity.BOTTOM);
        mAlertDialog.getWindow().setBackgroundDrawable(mContext.getDrawable(R.drawable.button_frame)/*new ColorDrawable(0)*/);
        WindowManager.LayoutParams lp = mAlertDialog.getWindow().getAttributes();
        lp.width = (int)mContext.getResources().getDimension(R.dimen.teletext_advanced_ui_width);
        //lp.width = WindowManager.LayoutParams.WRAP_CONTENT;;
        //lp.height = WindowManager.LayoutParams.WRAP_CONTENT;;
        mAlertDialog.getWindow().setAttributes(lp);
    }

    public void dealKeyEvent(int keyCode) {
        String key = null;
        switch (keyCode) {
            case KeyEvent.KEYCODE_PROG_RED:
                key = ACTION_EVENT_QUICK_NAVIGATE_1;
                break;
            case KeyEvent.KEYCODE_PROG_GREEN:
                key = ACTION_EVENT_QUICK_NAVIGATE_2;
                break;
            case KeyEvent.KEYCODE_PROG_YELLOW:
                key = ACTION_EVENT_QUICK_NAVIGATE_3;
                break;
            case KeyEvent.KEYCODE_PROG_BLUE:
                key = ACTION_EVENT_QUICK_NAVIGATE_4;
                break;
            case KeyEvent.KEYCODE_MEDIA_PAUSE:
                key = ACTION_EVENT_INDEXPAGE;
                break;
            case KeyEvent.KEYCODE_MEDIA_STOP:
                key = ACTION_EVENT_CLEAR;
                break;
            case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                key = ACTION_EVENT_PREVIOUS_PAGE;
                break;
            case KeyEvent.KEYCODE_MEDIA_NEXT:
                key = ACTION_EVENT_NEXT_PAGE;
                break;
            case KeyEvent.KEYCODE_MEDIA_REWIND:
                key = ACTION_EVENT_PREVIOUSSUBPAGE;
                break;
            case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
                key = ACTION_EVENT_NEXTSUBPAGE;
                break;
            default:
                break;
        }
        if (key != null) {
            Bundle bundle = new Bundle();
            bundle.putBoolean(key,true);
            if (mTvView != null) {
                mTvView.sendAppPrivateCommand(key, bundle);
            }
        }
    }

    public void sendCommandByTif(String action, Bundle bundle) {
        if (mTvView != null) {
            mTvView.sendAppPrivateCommand(action, bundle);
        }
    }
}