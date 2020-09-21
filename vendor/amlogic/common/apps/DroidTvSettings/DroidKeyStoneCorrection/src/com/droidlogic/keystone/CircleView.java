/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.keystone;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;
import android.view.KeyEvent;

import android.graphics.Rect;
import android.graphics.Path;
import android.graphics.Region;
import android.graphics.Matrix;
import android.view.WindowManager;

import com.droidlogic.app.SystemControlManager;


/**
 * Created by lu.wang.
 */

public class CircleView extends View {
    private static String TAG = "CircleView";
    private Paint mCirclePaint;
    private Paint mUpNumPaint;
    private Paint mDownNumPaint;
    private Paint mAniPaint;
    private Paint mLinePaint;

    private Path mPath;

    private int mRadius = 100;
    private int pointX = mRadius;
    private int pointY = mRadius;

    private int up_value= 0;
    private int down_value= 0;

    private int screenWidth= 0;
    private int screenHeigh= 0;

    private int screenXaxis= 0;
    private int screenYaxis= 0;

    private int screenWidth_bac= 0;
    private int screenHeigh_bac= 0;

    private boolean moveable;
    private boolean mHasCircleDraw;

    private int aniBitWidth , aniBitHeight;
    private Bitmap upBitmap;
    private Bitmap downBitmap;
    private Bitmap leftBitmap;
    private Bitmap rightBitmap;

    private Bitmap animationBitmap = null;
    private Rect mSrcRect, mDestRect;

    private static final int upKey = 0;
    private static final int downKey = 1;
    private static final int leftKey = 2;
    private static final int rightKey = 3;
    private int pressKeyState = upKey;
    private Context mcontext;

    //for debug degree
    private int AX;
    private int AY;       //A translate

    private int BX;
    private int BY;       //B translate

    private int CX;
    private int CY;       //C translate

    private int DX;
    private int DY;       //D translate

    private WindowManager mWindowManager;

    private int mPointOne[];
    private int mPointTwo[];
    private int mPointThree[];
    private int mPointFour[];

    private static final int A_POINT = 0;
    private static final int B_POINT = 1;
    private static final int C_POINT = 2;
    private static final int D_POINT = 3;
    private int pointState = A_POINT;

    private String OMXVIDEO_ON = "1";
    private String OMXVIDEO_OFF = "0";

    private static final String PROP_HWC_KEYSTONE = "persist.vendor.hwc.keystone";
    private static final String PROP_HWC_KEYSTONE_INT = "persist.vendor.hwc.keystone.debug";
    private static final String PROP_OMXVIDEO_OPTIMIZE = "media.sf.omxvideo-optmize";
    private SystemControlManager mSystemControl;

    double unitX;
    double unitY;
    String axisStr = null;
    String axisStrDebug = null;


    public CircleView(Context context) {
        super(context, null);
        mcontext = context;
        mSystemControl = SystemControlManager.getInstance();
    }

    public CircleView(Context context,  AttributeSet attrs ) {
        super(context, attrs );
        mcontext = context;
        mCirclePaint = new Paint();
        mCirclePaint.setColor(Color.GREEN);
        mCirclePaint.setAntiAlias(true);

        mUpNumPaint = new Paint();
        mUpNumPaint.setColor(Color.BLACK);
        mUpNumPaint.setAntiAlias(true);
        mUpNumPaint.setTextSize(30);

        mDownNumPaint = new Paint();
        mDownNumPaint.setColor(Color.BLACK);
        mDownNumPaint.setAntiAlias(true);
        mDownNumPaint.setTextSize(30);

        mAniPaint = new Paint();
        mAniPaint.setAntiAlias(true);
        mPath = new Path();
        mLinePaint= new Paint();
        mLinePaint.setAntiAlias(true);
        mLinePaint.setStrokeWidth(10);
        mLinePaint.setStyle(Paint.Style.STROKE);
        mLinePaint.setColor(Color.GREEN);

        setFocusableInTouchMode(true);
        setFocusable(true);

        animationBitmap = BitmapFactory.decodeResource(mcontext.getResources(), R.drawable.animation);
        mWindowManager = (WindowManager) mcontext.getSystemService (Context.WINDOW_SERVICE);
        screenWidth = mWindowManager.getDefaultDisplay().getWidth();
        screenHeigh = mWindowManager.getDefaultDisplay().getHeight();
        unitX = screenWidth / 250.0;
        unitY = screenHeigh / 250.0;
        screenXaxis = screenWidth;
        screenYaxis = 0;
        mSystemControl = SystemControlManager.getInstance();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        Log.i(TAG, "[onDraw]");
        pointX = screenWidth / 2;
        pointY = screenHeigh /2;
        Log.i(TAG, "[CircleView]pointX:" + pointX + ",pointY:" + pointY);

        super.onDraw(canvas);
        initBitmap();
        mSrcRect = new Rect(0, 0, aniBitWidth, aniBitWidth);
        mDestRect= new Rect((screenWidth-aniBitWidth)/2, (screenHeigh-aniBitHeight)/2, aniBitWidth + (screenWidth-aniBitWidth)/2, aniBitHeight + (screenHeigh-aniBitHeight)/2);

        mPath.reset();
        //trasferAaxis();
        trasferFixedaxis();
        DrawLinePath();
        canvas.drawPath(mPath, mLinePaint);

        Matrix matrix = new Matrix();
        int bw = aniBitWidth;
        int bh = aniBitHeight;
        float[] src = {0, 0, 0, bh, bw, bh, bw, 0};
        float[] dst2 = {0 + AX , 0 + AY, 0 + BX , bh - BY, bw - CX, bh - CY, bw - DX, 0 + DY};
        //float[] dst = {0 + DX , 0 + DY , 0 , screenHeigh, screenWidth, screenHeigh, screenWidth, 0};
        float[] dst1 = { (screenWidth - aniBitWidth) / 2 + AX, (screenHeigh - aniBitHeight) / 2 + AY, (screenWidth - aniBitWidth) / 2 + BX, screenHeigh - (screenHeigh - aniBitHeight) / 2 - BY,
                            screenWidth - (screenWidth - aniBitWidth) / 2 - CX, screenHeigh -(screenHeigh - aniBitHeight) / 2 - CY, screenWidth - (screenWidth - aniBitWidth) / 2 - DX,
                            (screenHeigh-aniBitHeight) / 2 + DY};
        matrix.setPolyToPoly(src, 0, dst1, 0, 4);
        //canvas.drawBitmap(animationBitmap, matrix, mAniPaint);

        //canvas.drawColor(getResources().getColor(R.color.semiTransparent));
        canvas.drawBitmap(animationBitmap,null,mDestRect,mAniPaint);
        canvas.drawCircle(pointX,pointY,mRadius,mCirclePaint);
        //canvas.drawText(""+DX,pointX,pointY-20,mUpNumPaint);
        //canvas.drawText(""+down_value,pointX,pointY+30,mDownNumPaint);
        if (pointState == A_POINT) {
            canvas.drawText(""+AX,pointX,pointY-20,mUpNumPaint);
            canvas.drawText(""+AY,pointX,pointY+30,mDownNumPaint);
            canvas.drawBitmap(rightBitmap, pointX-60, pointY-50, null);
            canvas.drawBitmap(downBitmap, pointX-60, pointY, null);
            Toast.makeText(mcontext, R.string.first_str, Toast.LENGTH_SHORT).show();
        }
        else if (pointState == B_POINT) {
            canvas.drawText(""+BY,pointX,pointY-20,mUpNumPaint);
            canvas.drawText(""+BX,pointX,pointY+30,mDownNumPaint);
            canvas.drawBitmap(upBitmap, pointX-60, pointY-50, null);
            canvas.drawBitmap(rightBitmap, pointX-60, pointY, null);
            Toast.makeText(mcontext, R.string.second_str, Toast.LENGTH_SHORT).show();
        }
        else if (pointState == C_POINT) {
            canvas.drawText(""+CY,pointX,pointY-20,mUpNumPaint);
            canvas.drawText(""+CX,pointX,pointY+30,mDownNumPaint);
            canvas.drawBitmap(upBitmap, pointX-60, pointY-50, null);
            canvas.drawBitmap(leftBitmap, pointX-60, pointY, null);
            Toast.makeText(mcontext, R.string.third_str, Toast.LENGTH_SHORT).show();
        }
        else if (pointState == D_POINT) {
            canvas.drawText(""+DX,pointX,pointY-20,mUpNumPaint);
            canvas.drawText(""+DY,pointX,pointY+30,mDownNumPaint);
            canvas.drawBitmap(leftBitmap, pointX-60, pointY-50, null);
            canvas.drawBitmap(downBitmap, pointX-60, pointY, null);
            Toast.makeText(mcontext, R.string.fourth_str, Toast.LENGTH_SHORT).show();
        }
        if (getKeystoneEnable()) {
            mSystemControl.setProperty(PROP_HWC_KEYSTONE, getAngleAxis());
            mSystemControl.setProperty(PROP_OMXVIDEO_OPTIMIZE, OMXVIDEO_ON);
        } else {
            mSystemControl.setProperty(PROP_HWC_KEYSTONE, "0");
            mSystemControl.setProperty(PROP_OMXVIDEO_OPTIMIZE, OMXVIDEO_OFF);
        }
        mSystemControl.setProperty(PROP_HWC_KEYSTONE_INT, getAngleAxisDebug());

    }

    private boolean getKeystoneEnable() {
        boolean ret = mSystemControl.getPropertyBoolean("persist.vendor.sys.hwc.enable", false);
        return ret;
    }

    private void trasferAaxis() {
         mPointOne = new int[] { (screenWidth-aniBitWidth)/2 + AX, (screenHeigh-aniBitHeight)/2 + AY};
         mPointTwo = new int[] { (screenWidth-aniBitWidth)/2 + BX, screenHeigh - (screenHeigh-aniBitHeight)/2 -BY};
         mPointThree = new int[] { screenWidth - (screenWidth - aniBitWidth) / 2 - CX, screenHeigh - (screenHeigh - aniBitHeight) / 2 - CY};
         mPointFour = new int[] { screenWidth - (screenWidth-aniBitWidth)/2 -DX, (screenHeigh-aniBitHeight)/2 + DY};
    }

    private void trasferFixedaxis() {
         mPointOne = new int[] { (screenWidth-aniBitWidth)/2, (screenHeigh-aniBitHeight)/2};
         mPointTwo = new int[] { (screenWidth-aniBitWidth)/2, screenHeigh - (screenHeigh-aniBitHeight)/2};
         mPointThree = new int[] { screenWidth - (screenWidth - aniBitWidth)/2, screenHeigh - (screenHeigh - aniBitHeight)/2};
         mPointFour = new int[] { screenWidth - (screenWidth-aniBitWidth)/2, (screenHeigh-aniBitHeight)/2};
    }

    private void DrawLinePath() {
            mPath.moveTo(mPointOne[0], mPointOne[1]);
            mPath.lineTo(mPointTwo[0], mPointTwo[1]);
            mPath.lineTo(mPointThree[0], mPointThree[1]);
            mPath.lineTo(mPointFour[0], mPointFour[1]);
            mPath.close();
    }

    private int[] getAPoint() {
            return mPointOne;
    }

    private int[] getBPoint() {
            return mPointTwo;
    }

    private int[] getCPoint() {
            return mPointThree;
    }

    private int[] getDPoint() {
            return mPointFour;
    }

   /* public String getAngleAxis() {
            axisStr = String.valueOf(unitX * BX ) + "," + String.valueOf(unitY * BY) + ","
                                + String.valueOf(screenWidth - unitX * CX) + "," + String.valueOf(unitY * CY) + ","
                                + String.valueOf(screenWidth - unitX * CX) + "," + String.valueOf(screenHeigh - unitY * CY) + ","
                                + String.valueOf(unitX * AX) + "," + String.valueOf(screenHeigh - unitY * AY);
            Log.i(TAG,"[getAngleAxis]axisStr:" + axisStr);
            return axisStr;
    }*/

    public String getAngleAxis() {
            axisStr = Double.toString(unitX * BX ) + "," + Double.toString(unitY * BY) + ","
                                + Double.toString(screenWidth - unitX * CX) + "," + Double.toString(unitY * CY) + ","
                                + Double.toString(screenWidth - unitX * DX) + "," + Double.toString(screenHeigh - unitY * DY) + ","
                                + Double.toString(unitX * AX) + "," + Double.toString(screenHeigh - unitY * AY);
            Log.i(TAG,"[getAngleAxis]axisStr:" + axisStr);
            return axisStr;
    }

    public String getAngleAxisDebug() {
            axisStrDebug = String.valueOf(BX ) + "," + String.valueOf(BY) + ","
                                + String.valueOf(CX) + "," + String.valueOf(CY) + ","
                                + String.valueOf(DX) + "," + String.valueOf(DY) + ","
                                + String.valueOf(AX) + "," + String.valueOf(AY);
            Log.i(TAG,"[getAngleAxisDebug]axisStrDebug:" + axisStrDebug);
            return axisStrDebug;
    }

    public int getPointState() {
            return pointState;
    }

    public void setPointState(int ps) {
            pointState = ps;
    }

    public void setAngleAxis(String value) {
            Log.i(TAG,"[setAngleAxisDebug]value:" + value);
            axisStrDebug = value;
            String [] angleAxisArray = axisStrDebug.split(",");
            if (angleAxisArray.length == 8) {
                BX = Integer.parseInt(angleAxisArray[0]);
                BY = Integer.parseInt(angleAxisArray[1]);
                CX = Integer.parseInt(angleAxisArray[2]);
                CY = Integer.parseInt(angleAxisArray[3]);
                DX = Integer.parseInt(angleAxisArray[4]);
                DY = Integer.parseInt(angleAxisArray[5]);
                AX = Integer.parseInt(angleAxisArray[6]);
                AY = Integer.parseInt(angleAxisArray[7]);
            }

   }

    private void initBitmap() {
        upBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.up);
        downBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.down);
        leftBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.left);
        rightBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.right);
        animationBitmap = BitmapFactory.decodeResource(mcontext.getResources(), R.drawable.animation);
        aniBitWidth = animationBitmap.getWidth();
        aniBitHeight = animationBitmap.getHeight();

    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int touchX;

        int touchY;

        switch (event.getAction()) {

            case MotionEvent.ACTION_DOWN:
                touchX = (int)event.getX();
                touchY = (int)event.getY();
                if (touchX > pointX - mRadius && touchX < pointX + mRadius && touchY > pointY - mRadius && touchY < pointY + mRadius) {
                    moveable = true;
                    Toast.makeText(getContext(), "press ....", Toast.LENGTH_LONG).show();
                } else {
                    moveable = false;
                }
                break;

            case MotionEvent.ACTION_MOVE:
                if (moveable) {

                    pointX = (int)event.getX();
                    pointY = (int)event.getY();

                    invalidate();
                }
                break;

            case MotionEvent.ACTION_UP:
                break;
        }
        return true;
    }

     @Override
     public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.i(TAG, "[onKeyDown]keyCode:"+keyCode);
         if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
             if (pointState == A_POINT && AY > 0 && AY <= 100) {
                 AY--;
             } else if (pointState == B_POINT && BY >= 0 && BY < 100) {
                 BY++;
             } else if (pointState == C_POINT && CY >= 0 && CY < 100) {
                 CY++;
             } else if (pointState == D_POINT && DY > 0 && DY <= 100) {
                 DY--;
             }
             invalidate();
         } else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
             if (pointState == A_POINT && AY >= 0 && AY < 100) {
                 AY++;
             } else if (pointState == B_POINT && BY > 0 && BY <= 100) {
                 BY--;
             } else if (pointState == C_POINT && CY > 0 && CY <= 100) {
                 CY--;
             } else if (pointState == D_POINT && DY >= 0 && DY < 100) {
                 DY++;
             }
             invalidate();
         } else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
             if (pointState == A_POINT && AX > 0 && AX <= 100) {
                 AX--;
             } else if (pointState == B_POINT && BX > 0 && BX <= 100) {
                 BX--;
             } else if (pointState == C_POINT && CX >= 0 && CX < 100) {
                 CX++;
             } else if (pointState == D_POINT && DX >= 0 && DX < 100) {
                 DX++;
             }
             invalidate();
         } else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
             if (pointState == A_POINT && AX >= 0 && AX < 100) {
                 AX++;
             } else if (pointState == B_POINT && BX >= 0 && BX < 100) {
                 BX++;
             } else if (pointState == C_POINT && CX > 0 && CX <= 100) {
                 CX--;
             } else if (pointState == D_POINT && DX > 0 && DX <= 100) {
                 DX--;
             }
             invalidate();
         } else if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
             if ((pointState >=0) && (pointState < 3)) {
                pointState++;
             } else if (pointState == 3) {
                pointState = 0;
             }
             invalidate();
         }
         return super.onKeyDown(keyCode, event);
     }

}
