/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DisplayPositionManager
 */

package com.droidlogic.app;

import android.content.Context;
import android.util.Log;

public class DisplayPositionManager {
    private final static String TAG = "DisplayPositionManager";
    private final static boolean DEBUG = false;
    private Context mContext = null;
    private SystemControlManager mSystenControl = null;
    private OutputModeManager mOutputModeManager = null;

    private final static int MAX_Height = 100;
    private final static int MIN_Height = 80;
    private static int screen_rate = MAX_Height;

    private String mCurrentLeftString = null;
    private String mCurrentTopString = null;
    private String mCurrentWidthString = null;
    private String mCurrentHeightString = null;

    private int mCurrentLeft = 0;
    private int mCurrentTop = 0;
    private int mCurrentWidth = 0;
    private int mCurrentHeight = 0;
    private int mCurrentRate = MAX_Height;
    private int mCurrentRight = 0;
    private int mCurrentBottom = 0;

    private int mPreLeft = 0;
    private int mPreTop = 0;
    private int mPreRight = 0;
    private int mPreBottom = 0;
    private int mPreWidth = 0;
    private int mPreHeight = 0;

    private  String mCurrentMode = null;

    private int mMaxRight = 0;
    private int mMaxBottom=0;
    private int offsetStep = 2;  // because 20% is too large ,so we divide a value to smooth the view

    public DisplayPositionManager(Context context) {
        mContext = context;
        mSystenControl = SystemControlManager.getInstance();
        mOutputModeManager = new OutputModeManager(mContext);
        initPostion();
    }

    public void initPostion() {
        mCurrentMode = mOutputModeManager.getCurrentOutputMode();
        initStep(mCurrentMode);
        initCurrentPostion();
        screen_rate = getInitialRateValue();
    }

    private void initCurrentPostion() {
        int [] position = mOutputModeManager.getPosition(mCurrentMode);
        mPreLeft = mCurrentLeft = position[0];
        mPreRight = mCurrentTop  = position[1];
        mPreWidth = mCurrentWidth = position[2];
        mPreHeight = mCurrentHeight= position[3];
    }

    public int getInitialRateValue() {
        mCurrentMode = mOutputModeManager.getCurrentOutputMode();
        initStep(mCurrentMode);
        int m = (100*2*offsetStep)*mPreLeft ;
        if (m == 0) {
            return 100;
        }
        int rate =  100 - m/(mMaxRight+1) - 1;
        return rate;
    }

    public int getCurrentRateValue(){
        return screen_rate;
    }

    private void zoom(int step) {
        screen_rate = screen_rate + step;
        if (screen_rate >MAX_Height) {
            screen_rate = MAX_Height;
        }else if (screen_rate <MIN_Height) {
            screen_rate = MIN_Height ;
        }
        zoomByPercent(screen_rate);
    }

    public void zoomIn(){
        zoom(1);
    }

    public void zoomOut(){
        zoom(-1);
    }

    public void saveDisplayPosition() {
        if ( !isScreenPositionChanged())
            return;

        mOutputModeManager.savePosition(mCurrentLeft, mCurrentTop, mCurrentWidth, mCurrentHeight);
    }

    private void writeFile(String file, String value) {
        mSystenControl.writeSysFs(file, value);
    }

    private void initStep(String mode) {
        if (mode.contains(OutputModeManager.HDMI_480)) {
            mMaxRight = 719;
            mMaxBottom = 479;
        }else if (mode.contains(OutputModeManager.HDMI_576)) {
            mMaxRight = 719;
            mMaxBottom = 575;
        }else if (mode.contains(OutputModeManager.HDMI_720)) {
            mMaxRight = 1279;
            mMaxBottom = 719;
        }else if (mode.contains(OutputModeManager.HDMI_1080)) {
            mMaxRight = 1919;
            mMaxBottom = 1079;
        }else if (mode.contains(OutputModeManager.HDMI_4K2K)) {
            mMaxRight = 3839;
            mMaxBottom = 2159;
        } else if (mode.contains(OutputModeManager.HDMI_SMPTE)) {
            mMaxRight = 4095;
            mMaxBottom = 2159;
        } else {
            mMaxRight = 1919;
            mMaxBottom = 1079;
        }
    }

    public void zoomByPercent(int percent){

        if (percent > 100 ) {
            percent = 100;
            return ;
        }

        if (percent < 80 ) {
            percent = 80;
            return ;
        }

        mCurrentMode = mOutputModeManager.getCurrentOutputMode();
        initStep(mCurrentMode);

        mCurrentLeft = (100-percent)*(mMaxRight)/(100*2*offsetStep);
        mCurrentTop  = (100-percent)*(mMaxBottom)/(100*2*offsetStep);
        mCurrentRight = mMaxRight - mCurrentLeft;
        mCurrentBottom = mMaxBottom - mCurrentTop;
        mCurrentWidth = mCurrentRight - mCurrentLeft + 1;
        mCurrentHeight = mCurrentBottom - mCurrentTop + 1;

        setPosition(mCurrentLeft, mCurrentTop,mCurrentRight, mCurrentBottom, 0);
    }
    private void setPosition(int l, int t, int r, int b, int mode) {
        String str = "";
        int left =  l;
        int top =  t;
        int right =  r;
        int bottom =  b;
        int width = mCurrentWidth;
        int height = mCurrentHeight;

        if (left < 0) {
            left = 0 ;
        }

        if (top < 0) {
            top = 0 ;
        }
        right = Math.min(right,mMaxRight);
        bottom = Math.min(bottom,mMaxBottom);

        /*
        ** no need to set "/sys/class/graphics/fb0/window_axis"
        ** writeFile(OutputModeManager.FB0_WINDOW_AXIS, left + " " + top + " " + right + " " + bottom);
        **
        */
        mOutputModeManager.savePosition(left, top, width, height);
        mOutputModeManager.setOsdMouse(left, top, width, height);
    }

    public boolean isScreenPositionChanged(){
        if (mPreLeft== mCurrentLeft && mPreTop == mCurrentTop
            && mPreWidth == mCurrentWidth && mPreHeight == mCurrentHeight)
            return false;
        else
            return true;
    }

    public void zoomByPosition(int x, int y, int w, int h){
        int right = x+w-1;
        int bottom = y+h-1;
        writeFile(OutputModeManager.FB0_WINDOW_AXIS, x + " " + y + " " + right + " " + bottom);
        mOutputModeManager.savePosition(x, y, w, h);
        mOutputModeManager.setOsdMouse(x, y, w, h);
    }

    /*
    **return (position[0], position[1]) is left-top coordinate
    **       (position[2], position[3]) is right-bottom coordinate
     */
    public int[] getCoordinate(){
        int [] position = mOutputModeManager.getPosition(mCurrentMode);
        position[2] = position[0] + position[2] - 1;
        position[3] = position[1] + position[3] - 1;
        return position;
    }

    /*
    ** parames: leftOffset, topOffset, rightOffset, bottomOffset
    **          positive number the coordinate will add
    **          negative number the coordinate will lessen
    **          (0, 0, 1919, 1079) ---> (5, 6, -2, 0) = (5, 6, 1917, 1079)
    */
    public void zoomByOffset(int leftOffset, int topOffset, int rightOffset, int bottomOffset) {
        int [] position = mOutputModeManager.getPosition(mCurrentMode);

        int x1 = position[0] + leftOffset;
        int y1 = position[1] + topOffset;
        int x2 = position[0] + position[2] + rightOffset - 1;
        int y2 = position[1] + position[3] + bottomOffset - 1;
        if (x1 < 0) {
            x1 = 0;
        } else if (x1 > (20 * mMaxBottom) / 100) {
            x1 = (20 * mMaxBottom) / 100;
        }

        if (y1 < 0) {
            y1 = 0;
        } else if (y1 > (20 * mMaxBottom) / 100) {
            y1 = (20 * mMaxBottom) / 100;
        }

        if (x2 < (80 * mMaxRight) / 100) {
            x2 = (80 * mMaxRight) / 100;
        } else if (x2 > mMaxRight) {
            x2 =  mMaxRight;
        }

        if (y2 < (80 * mMaxBottom) / 100) {
            y2 = (80 * mMaxBottom) / 100;
        } else if (y2 > mMaxBottom) {
            y2 =  mMaxBottom;
        }

        zoomByPosition(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    }
}
