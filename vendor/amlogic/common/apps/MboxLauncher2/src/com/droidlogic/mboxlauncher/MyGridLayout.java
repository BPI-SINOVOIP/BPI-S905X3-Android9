/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description: java file
*/

package com.droidlogic.mboxlauncher;

import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.widget.GridLayout;
import android.widget.TextView;
import android.widget.ImageView;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.graphics.drawable.Drawable;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.util.ArrayMap;
import android.util.Log;

import java.util.List;
import java.lang.ref.SoftReference;

public class MyGridLayout extends GridLayout{
    private final static String TAG="MyGridLayout";
    private Context mContext;
    private Object mLock;

    public MyGridLayout(Context context){
        super(context);
    }

    public MyGridLayout(Context context, AttributeSet attrs){
        super(context, attrs);
        mContext = context;
        mLock = ((Launcher)mContext).getLock();
    }

    public MyGridLayout(Context context, AttributeSet attrs, int defStyle){
        super(context, attrs, defStyle);
    }

    @Override
    public void onDraw(Canvas canvas) {
       // TODO Auto-generated method stub
       super.onDraw(canvas);
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        //The input source is a pointing device associated with a display.
        if (0 != (event.getSource() & InputDevice.SOURCE_CLASS_POINTER)) {
            switch (event.getAction()) {
                // process the scroll wheel movement
                case MotionEvent.ACTION_SCROLL:
                    ((Launcher)mContext).getHoverView().clear();
                    break;
            }
        }
        return super.onGenericMotionEvent(event);
    }

    @Override
    protected void onLayout (boolean changed, int left, int top, int right, int bottom) {
        super.onLayout (changed, left, top, right, bottom);
    }

    public void setLayoutView(int mode, List<ArrayMap<String, Object>> list){
        synchronized (mLock) {
            if (this.getChildCount() > 0)
                this.removeAllViews();

            for (int i = 0; i < list.size(); i++) {
                MyRelativeLayout view;
                if (mode == Launcher.MODE_HOME) {
                    view = (MyRelativeLayout)View.inflate(mContext,R.layout.homegrid_item, null);
                    view.setType(Launcher.TYPE_HOME_SHORTCUT);
                } else {
                    view = (MyRelativeLayout)View.inflate(mContext,R.layout.childgrid_item, null);
                    ((TextView)view.getChildAt(1)).setText((String)list.get(i).get(AppDataLoader.NAME));
                    view.setType(Launcher.TYPE_APP_SHORTCUT);
                    view.setNumber(i);
                }

                ImageView img_bg = (ImageView)view.getChildAt(0);
                SoftReference<Drawable> bg = new SoftReference<Drawable>(mContext.getResources().getDrawable(parseItemBackground(i, mode)));
                img_bg.setBackgroundDrawable(bg.get());
                if (list.get(i).get(AppDataLoader.ICON) instanceof Drawable) {
                    SoftReference<Drawable> icon = new SoftReference<Drawable>((Drawable)list.get(i).get(AppDataLoader.ICON));
                    img_bg.setImageDrawable(icon.get());
                    view.setIntent((Intent)list.get(i).get(AppDataLoader.INTENT));
                } else {
                    SoftReference<Drawable> add = new SoftReference<Drawable>(mContext.getResources().getDrawable(R.drawable.item_img_add));
                    img_bg.setImageDrawable(add.get());
                    img_bg.setContentDescription("img_add");
                    view.setAddButton(true);
                }
                this.addView(view);
            }
        }
    }

    private int  parseItemBackground(int num, int mode){
        if (mode == Launcher.MODE_HOME) {
            switch (num % 11) {
                case 1:
                    return R.drawable.item_child_1;
                case 2:
                    return R.drawable.item_child_2;
                case 3:
                    return R.drawable.item_child_3;
                case 4:
                    return R.drawable.item_child_4;
                case 5:
                    return R.drawable.item_child_5;
                case 6:
                    return R.drawable.item_child_6;
                case 7:
                    return R.drawable.item_child_3;
                case 8:
                    return R.drawable.item_child_4;
                case 9:
                    return R.drawable.item_child_1;
                case 10:
                    return R.drawable.item_child_2;
                case 0:
                    return R.drawable.item_child_6;
                default:
                    return R.drawable.item_child_1;
            }
        }else {
            switch (num % 18) {
                case 1:
                    return R.drawable.item_child_1;
                case 2:
                    return R.drawable.item_child_2;
                case 3:
                    return R.drawable.item_child_3;
                case 4:
                    return R.drawable.item_child_4;
                case 5:
                    return R.drawable.item_child_5;
                case 6:
                    return R.drawable.item_child_6;
                case 7:
                    return R.drawable.item_child_3;
                case 8:
                    return R.drawable.item_child_4;
                case 9:
                    return R.drawable.item_child_1;
                case 10:
                    return R.drawable.item_child_2;
                case 11:
                    return R.drawable.item_child_6;
                case 12:
                    return R.drawable.item_child_5;
                case 13:
                    return R.drawable.item_child_6;
                case 14:
                    return R.drawable.item_child_2;
                case 15:
                    return R.drawable.item_child_5;
                case 16:
                    return R.drawable.item_child_3;
                case 17:
                    return R.drawable.item_child_1;
                case 0:
                    return R.drawable.item_child_4;
                default:
                    return R.drawable.item_child_1;
            }
        }
    }
}
