/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description: java file
*/

package com.droidlogic.mboxlauncher;

import android.content.Intent;
import android.content.ComponentName;
import android.content.Context;
import android.os.Bundle;

import android.view.View;
import android.view.KeyEvent;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import android.graphics.drawable.Drawable;
import android.graphics.Rect;

import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.GridView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.FrameLayout;
import android.widget.Toast;

import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AccelerateInterpolator;
import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.view.ViewGroup;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.List;

public class CustomView extends FrameLayout implements OnItemClickListener, OnGlobalLayoutListener{
    private final static String TAG = "CustomView";
    private final static String NAME = "item_name";
    private final static String ICON = "item_icon";
    private final static String SELECTION = "item_selection";
    private final static String BACKGROUND = "item_background";
    private final static String COMPONENT_NAME = "component name";

    private final static int duration = 300;
    private static boolean allowAnimation = true;

    private ImageView img_screen_shot = null;
    private ImageView img_screen_shot_keep = null;
    private ImageView img_dim = null;
    private GridView gv = null;
    private CustomView thisView = null;
    private Context mContext = null;

    private String[] list_custom_apps;
    private String str_custom_apps;

    private int transY = 0;
    private int homeShortcutCount;

    final static Object mLock = new Object[0];
    private View mSource;
    private int mMode = -1;
    public CustomView(Context context, View source, int mode){
        super(context);
        mContext = context;
        mSource = source;
        mMode = mode;
        initLayout();
    }

    private void initLayout() {
        inflate(mContext, R.layout.layout_custom, this);
        thisView = this;
        gv = (GridView)findViewById(R.id.grid_add_apps);
        gv.setBackgroundDrawable(mContext.getResources().getDrawable(R.drawable.bg_add_apps));
        gv.setOnItemClickListener(this);
        displayView();
        str_custom_apps = ((Launcher)mContext).getAppDataLoader().getShortcutString(mMode);
        getViewTreeObserver().addOnGlobalLayoutListener(this);
    }

    public  void onGlobalLayout () {
        Rect rect = new Rect();
        mSource.getGlobalVisibleRect(rect);
        if (rect.top > mContext.getResources().getDisplayMetrics().heightPixels / 2) {
            transY = -this.getHeight();
        } else {
            transY = this.getHeight();
        }
        setCustomView();
        getViewTreeObserver().removeGlobalOnLayoutListener(this);
    }

    private void setCustomView(){
        ((Launcher)mContext).getMainView().setDescendantFocusability(ViewGroup.FOCUS_BLOCK_DESCENDANTS);
        View view = ((Launcher)mContext).getMainView();//getWindow().getDecorView();
        view.animate().
            translationY(transY).
            setDuration(duration).
            alpha(0.5f).
            start();
        Rect rect = new Rect();
        gv.getGlobalVisibleRect(rect);
        gv.requestFocus();

        TranslateAnimation translateAnimation = new TranslateAnimation(0.0f, 0.0f, -transY,  0.0f);
        translateAnimation.setDuration(duration);
        translateAnimation.setInterpolator(new AccelerateInterpolator());
        gv.startAnimation(translateAnimation);
    }

    public void recoverMainView() {
        Launcher context = (Launcher) mContext;
        View view = context.getMainView();
        this.setVisibility(View.VISIBLE);
        if (allowAnimation) {
            allowAnimation = false;
            TranslateAnimation translateAnimation = new TranslateAnimation(0.0f, 0.0f, 0.0f, -transY);
            translateAnimation.setDuration(duration);
            translateAnimation.setInterpolator(new AccelerateInterpolator());
            gv.startAnimation(translateAnimation);
            view.animate().
                translationY(0).
                setDuration(duration).
                alpha(1f).
                setInterpolator(new AccelerateInterpolator()).
                setListener(new mAnimatorListener()).
                start();
            context.getAppDataLoader().saveShortcut(mMode, str_custom_apps);
        }
    }

    private  List<ArrayMap<String, Object>> getAppList() {
        List<ArrayMap<String, Object>> list = new ArrayList<ArrayMap<String, Object>>();
        List<ArrayMap<String, Object>> list_all = ((Launcher)mContext).getAppDataLoader().getShortcutList(Launcher.MODE_APP);
        List<ArrayMap<String, Object>> list_current = ((Launcher)mContext).getAppDataLoader().getShortcutList(mMode);
        homeShortcutCount = 0;

        for (int i = 0; i < list_all.size(); i++) {
            ArrayMap<String, Object> map = new ArrayMap<String, Object>();
            map.put(SELECTION, R.drawable.item_img_unsel);
            for (int j = 0; j < list_current.size() - 1; j++) {
                if (TextUtils.equals(list_all.get(i).get(AppDataLoader.COMPONENT_NAME).toString(),
                      list_current.get(j).get(AppDataLoader.COMPONENT_NAME).toString())) {
                    map.put(SELECTION, R.drawable.item_img_sel);
                    if (mMode == Launcher.MODE_HOME) {
                        homeShortcutCount++;
                    }
                    break;
                }
            }
            map.put(NAME, list_all.get(i).get(AppDataLoader.NAME));
            map.put(ICON, list_all.get(i).get(AppDataLoader.ICON));
            map.put(BACKGROUND, parseItemBackground(i));
            map.put(COMPONENT_NAME, list_all.get(i).get(AppDataLoader.COMPONENT_NAME));
            list.add(map);
        }

        return list;
    }

    private void displayView() {
        LocalAdapter ad = new LocalAdapter(mContext,
                getAppList(),
                R.layout.add_apps_grid_item,
                new String[] {ICON, NAME, SELECTION, BACKGROUND},
                new int[] {R.id.item_type, R.id.item_name, R.id.item_sel, R.id.relative_layout});
        gv.setAdapter(ad);
    }

    private void updateView() {
        ((BaseAdapter)gv.getAdapter()).notifyDataSetChanged();
    }

    public boolean dispatchKeyEvent (KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_BACK:
                    recoverMainView();
                    break;
            }
        }

        return super.dispatchKeyEvent(event);
    }

    private int  parseItemBackground(int num){
        switch (num % 20 + 1) {
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
            case 18:
                return R.drawable.item_child_4;
            case 19:
                return R.drawable.item_child_2;
            case 0:
                return R.drawable.item_child_3;
            default:
                return R.drawable.item_child_1;
        }
    }

    public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
        ArrayMap<String, Object> item = (ArrayMap<String, Object>)parent.getItemAtPosition(pos);
        synchronized(mLock){
            if (item.get(SELECTION).equals(R.drawable.item_img_unsel)) {
                if (mMode == Launcher.MODE_HOME && homeShortcutCount >= Launcher.HOME_SHORTCUT_COUNT) {
                    Toast.makeText(mContext, mContext.getResources().getString(R.string.str_nospace),Toast.LENGTH_SHORT).show();
                    return;
                }
                String str_package_name = ((ComponentName)item.get(COMPONENT_NAME)).getPackageName();
                str_custom_apps += str_package_name  + ";";

                ((ArrayMap<String, Object>)parent.getItemAtPosition(pos)).put(SELECTION, R.drawable.item_img_sel);
                updateView();

                if (mMode == Launcher.MODE_HOME) {
                    homeShortcutCount++;
                }
            } else {
                String str_package_name = ((ComponentName)item.get(COMPONENT_NAME)).getPackageName();
                if (!TextUtils.isEmpty(str_custom_apps)) {
                    str_custom_apps = str_custom_apps.replaceAll(str_package_name + ";", "");
                }
                ((ArrayMap<String, Object>)parent.getItemAtPosition(pos)).put(SELECTION, R.drawable.item_img_unsel);
                updateView();
                if (mMode == Launcher.MODE_HOME) {
                    homeShortcutCount--;
                }
            }
        }
    }

    private class mAnimatorListener implements AnimatorListener {
        @Override
        public void onAnimationCancel(Animator animation) {
        }
        @Override
        public void onAnimationStart(Animator animation) {
        }
        @Override
        public void onAnimationEnd(Animator animation) {
            allowAnimation = true;
            ((Launcher)mContext).getRootView().removeView(thisView);
            ((Launcher)mContext).getMainView().animate().setListener(null);
            ((Launcher)mContext).recoverFromCustom();
        }
        @Override
        public void onAnimationRepeat(Animator animation) {
        }
    }
}
