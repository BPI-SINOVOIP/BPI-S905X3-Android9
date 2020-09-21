/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

package com.droidlogic.mboxlauncher;

import android.content.Context;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.ImageView;
import android.view.View;
import android.view.ViewGroup;
import android.view.MotionEvent;
import android.view.animation.Animation;
import android.view.animation.ScaleAnimation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AnimationUtils;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.util.ArrayMap;
import android.util.Log;

import java.lang.Character;
import java.util.List;

public class AppLayout extends RelativeLayout{
    private final static String TAG = "AppLayout";
    public final static int ANIM_LEFT = 0;
    public final static int ANIM_RIGHT = 1;
    private final static int TYPE_ANIM_IN = 0;
    private final static int TYPE_ANIM_OUT = 1;

    private Context mContext;
    private ImageView icon;
    private ImageView prompt;
    private TextView title;
    private MyGridLayout grid_layout;
    private int password = 0;
    private int currenPassword = 0;
    Animation animLeftIn, animLeftOut, animRightIn, animRightOut;
    public AppLayout(Context context){
        super(context);
        mContext = context;
    }

    public AppLayout(Context context, AttributeSet attrs){
        super(context, attrs);
        mContext = context;
        initlayout();
    }

    public AppLayout(Context context, AttributeSet attrs, int defStyle){
        super(context, attrs, defStyle);
    }

    private void initlayout() {
        inflate(mContext, R.layout.layout_second_screen, this);
        icon = (ImageView)findViewById(R.id.image_icon);
        prompt = (ImageView)findViewById(R.id.image_prompt);
        title = (TextView)findViewById(R.id.tx_title);
        grid_layout = (MyGridLayout)findViewById(R.id.gl_shortcut);

        animLeftIn = AnimationUtils.loadAnimation(mContext, R.anim.push_left_in);
        animLeftOut = AnimationUtils.loadAnimation(mContext, R.anim.push_left_out);
        animRightIn = AnimationUtils.loadAnimation(mContext, R.anim.push_right_in);
        animRightOut = AnimationUtils.loadAnimation(mContext, R.anim.push_right_out);
    }

    public void setLayout(int mode, List<ArrayMap<String, Object>> list) {
        switch (mode) {
            case Launcher.MODE_VIDEO:
                setImageAndText(R.drawable.video, R.drawable.prompt_video, R.string.str_video);
                break;
            case Launcher.MODE_RECOMMEND:
                setImageAndText(R.drawable.recommend, R.drawable.prompt_recommend, R.string.str_recommend);
                break;
            case Launcher.MODE_MUSIC:
                setImageAndText(R.drawable.music, R.drawable.prompt_music, R.string.str_music);
                break;
            case Launcher.MODE_APP:
                setImageAndText(R.drawable.app, R.drawable.prompt_app, R.string.str_app);
                break;
            case Launcher.MODE_LOCAL:
                setImageAndText(R.drawable.local, R.drawable.prompt_local, R.string.str_local);
                break;
        }
        grid_layout.clearFocus();
        grid_layout.setLayoutView(mode, list);
        if (mode != Launcher.MODE_HOME && grid_layout.getChildCount() > 0) {
            MyRelativeLayout firstChild = (MyRelativeLayout)grid_layout.getChildAt(0);
            firstChild.requestFocus();
        }
    }

    public void setImageAndText(int resIcon, int resPrompt, int resTitle) {
        icon.setImageDrawable(mContext.getResources().getDrawable(resIcon));
        prompt.setImageDrawable(mContext.getResources().getDrawable(resPrompt));
        title.setText(resTitle);
    }

    public void setLayoutWithAnim(int animType, int mode, List<ArrayMap<String, Object>> list) {
        setLayout(mode, list);
        if (animType == ANIM_LEFT) {
            animLeftIn.setAnimationListener(new MyAnimationListener(TYPE_ANIM_IN, password));
            startAnimation(animLeftIn);
        } else {
            animRightIn.setAnimationListener(new MyAnimationListener(TYPE_ANIM_IN, password));
            startAnimation(animRightIn);
        }
        password++;
    }

    private class MyAnimationListener implements AnimationListener {
        private int mType = -1;
        private int mPassword = -1;
        public MyAnimationListener(int type, int password) {
            mType = type;
            mPassword = password;
        }

        @Override
        public void onAnimationStart(Animation animation) {
            ((Launcher)mContext).getHoverView().setVisibility(View.INVISIBLE);
            //setVisibility(View.INVISIBLE);
            currenPassword = mPassword;
        }

        @Override
        public void onAnimationEnd(Animation animation) {
            if (mType == TYPE_ANIM_IN) {
                //setVisibility(View.VISIBLE);
                if (currenPassword == mPassword) {
                    ((Launcher)mContext).getHoverView().setVisibility(View.VISIBLE);
                }
            }
        }

        @Override
        public void onAnimationRepeat(Animation animation) {
        }
    }

}
