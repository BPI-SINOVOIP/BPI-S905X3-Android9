/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.app;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemProperties;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.Color;
import android.graphics.Typeface;
import android.util.AttributeSet;
import java.lang.Exception;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.LinkedList;
import java.util.Queue;

import android.util.Log;
import android.view.accessibility.CaptioningManager;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import com.droidlogic.app.CcImplement;


/**
 * CCSubtitleView for caption close display
 */
public class CCSubtitleView extends View {
    private static final String TAG = "CCSubtitleView";

    private static final String CC_ONTENT = "\"content\":";
    private static Object lock = new Object();
    private static final int BUFFER_W = 1920;
    private static final int BUFFER_H = 1080;

    public static final int CC_CAPTION_DEFAULT = 0;
    /*NTSC CC channels*/
    public static final int CC_CAPTION_CC1 = 1;
    public static final int CC_CAPTION_CC2 = 2;
    public static final int CC_CAPTION_CC3 = 3;
    public static final int CC_CAPTION_CC4 = 4;
    public static final int CC_CAPTION_TEXT1 =5;
    public static final int CC_CAPTION_TEXT2 = 6;
    public static final int CC_CAPTION_TEXT3 = 7;
    public static final int CC_CAPTION_TEXT4 = 8;
    /*DTVCC services*/
    public static final int CC_CAPTION_SERVICE1 = 9;
    public static final int CC_CAPTION_SERVICE2 = 10;
    public static final int CC_CAPTION_SERVICE3 = 11;
    public static final int CC_CAPTION_SERVICE4 = 12;
    public static final int CC_CAPTION_SERVICE5 = 13;
    public static final int CC_CAPTION_SERVICE6 = 14;

    public static final int CC_FONTSIZE_DEFAULT = 0;
    public static final int CC_FONTSIZE_SMALL = 1;
    public static final int CC_FONTSIZE_STANDARD = 2;
    public static final int CC_FONTSIZE_BIG = 3;

    public static final int CC_FONTSTYLE_DEFAULT = 0;
    public static final int CC_FONTSTYLE_MONO_SERIF = 1;
    public static final int CC_FONTSTYLE_PROP_SERIF = 2;
    public static final int CC_FONTSTYLE_MONO_NO_SERIF = 3;
    public static final int CC_FONTSTYLE_PROP_NO_SERIF = 4;
    public static final int CC_FONTSTYLE_CASUAL = 5;
    public static final int CC_FONTSTYLE_CURSIVE = 6;
    public static final int CC_FONTSTYLE_SMALL_CAPITALS = 7;

    public static final int CC_OPACITY_DEFAULT = 0;
    public static final int CC_OPACITY_TRANSPARET = 1;
    public static final int CC_OPACITY_TRANSLUCENT= 2;
    public static final int CC_OPACITY_SOLID = 3;
    public static final int CC_OPACITY_FLASH = 4;

    public static final int CC_COLOR_DEFAULT = 0;
    public static final int CC_COLOR_WHITE = 1;
    public static final int CC_COLOR_BLACK = 2;
    public static final int CC_COLOR_RED = 3;
    public static final int CC_COLOR_GREEN = 4;
    public static final int CC_COLOR_BLUE = 5;
    public static final int CC_COLOR_YELLOW = 6;
    public static final int CC_COLOR_MAGENTA = 7;
    public static final int CC_COLOR_CYAN = 8;

    private boolean mDebug = false;

    public static final int JSON_MSG_NORMAL = 1234567;

    private static int mInitCount = 0;//init_count = 0;
    private static CaptioningManager mCaptioningManager = null; //mCaptioningManager = null;
    private static CustomFonts mCustomFont = null;//cf = null;
    private static CcImplement mCcImplement = null; //mCcImplement = null;
    private Paint mClearPaint;//clear_paint;
    private Queue<CcImplement.CaptionWindow> mQueueCaption = new LinkedList<CcImplement.CaptionWindow>();


    static public class DTVCCParams {
        protected int vfmt;
        protected int mCaptionMode; //caption_mode;
        protected int mFgColor; //fg_color;
        protected int mFgOpcity;//fg_opacity;
        protected int mBgColor;//bg_color;
        protected int mBgOpacity;//bg_opacity;
        protected int mFontStyle;//font_style;
        protected float mFontSize;

        public DTVCCParams(int vfmt, int caption, int fgColor, int fgOpacity,
                int bgColor, int bgOpacity, int fontStyle, float fontSize) {
            this.vfmt = vfmt;
            this.mCaptionMode = caption;
            this.mFgColor = fgColor;
            this.mFgOpcity = fgOpacity;
            this.mBgColor = bgColor;
            this.mBgOpacity = bgOpacity;
            this.mFontStyle = fontStyle;
            this.mFontSize = fontSize;
        }
    }

    private int mDisplayLeft = 0;
    private int mDisplayRight = 0;
    private int mDisplayTop = 0;
    private int mDisplayBottom = 0;
    private boolean mActive = true;
    private boolean   mVisible;

    private void update() {
        Log.e(TAG, "update");
         postInvalidate();
    }

    private void init() {
        synchronized(lock) {
            if (mInitCount == 0) {
                mVisible    = true;
               // checkDebug();
                setActive(true);
                setLayerType(View.LAYER_TYPE_SOFTWARE, null);
                mCustomFont = new CustomFonts(getContext());
                if (mCcImplement == null) {
                    mCcImplement = new CcImplement(getContext(), mCustomFont);
                }
                mCaptioningManager = (CaptioningManager) getContext().getSystemService(Context.CAPTIONING_SERVICE);
                mCaptioningManager.addCaptioningChangeListener(new CaptioningManager.CaptioningChangeListener() {
                    @Override
                    public void onEnabledChanged(boolean enabled) {
                        super.onEnabledChanged(enabled);
                        Log.e(TAG, "onenableChange");
                        mCcImplement.mCcSetting.mIsEnabled = mCaptioningManager.isEnabled();
                    }

                    @Override
                    public void onFontScaleChanged(float fontScale) {
                        super.onFontScaleChanged(fontScale);
                        Log.e(TAG, "onfontscaleChange");
                        mCcImplement.mCcSetting.mFontScale = mCaptioningManager.getFontScale();
                    }

                    @Override
                    public void onLocaleChanged(Locale locale) {
                        super.onLocaleChanged(locale);
                        Log.e(TAG, "onlocaleChange");
                        mCcImplement.mCcSetting.mCcLocale = mCaptioningManager.getLocale();
                    }

                    @Override
                    public void onUserStyleChanged(CaptioningManager.CaptionStyle userStyle) {
                        super.onUserStyleChanged(userStyle);
                        Log.e(TAG, "onUserStyleChange");
                        mCcImplement.mCcSetting.mHasForegroundColor = userStyle.hasForegroundColor();
                        mCcImplement.mCcSetting.mHasBackgroundColor = userStyle.hasBackgroundColor();
                        mCcImplement.mCcSetting.mHasWindowColor = userStyle.hasWindowColor();
                        mCcImplement.mCcSetting.mHasEdgeColor = userStyle.hasEdgeColor();
                        mCcImplement.mCcSetting.mHasEdgeType = userStyle.hasEdgeType();
                        mCcImplement.mCcSetting.mEdgeType = userStyle.edgeType;
                        mCcImplement.mCcSetting.mEdgeColor = userStyle.edgeColor;
                        mCcImplement.mCcSetting.mForegroundColor = userStyle.foregroundColor;
                        mCcImplement.mCcSetting.mForegroundOpacity = userStyle.foregroundColor >>> 24;
                        mCcImplement.mCcSetting.mBackgroundColor = userStyle.backgroundColor;
                        mCcImplement.mCcSetting.mBackgroundOpcity = userStyle.backgroundColor >>> 24;
                        mCcImplement.mCcSetting.mWindowColor = userStyle.windowColor;
                        mCcImplement.mCcSetting.mWindowOpcity = userStyle.windowColor >>> 24;
                        /* Typeface is obsolete, we use local font */
                        mCcImplement.mCcSetting.mTypeFace = userStyle.getTypeface();
                    }
                });
                mCcImplement.mCcSetting.UpdateCcSetting(mCaptioningManager);
                Log.e(TAG, "subtitle view init");

            }
            mInitCount = 1;
            mClearPaint = new Paint();
            mClearPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        }
    }

    /**
     * creat TVSubtitle
     */
    public CCSubtitleView(Context context) {
        super(context);
        init();
    }

    /**
     * creat TVSubtitle
     */
    public CCSubtitleView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    /**
     * creat TVSubtitle
     */
    public CCSubtitleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    /**
     *set show margin blank
     *@param left  left margin width
     *@param top   top margin height
     *@param right right margin width
     *@param bottom bottom margin height
     */
    public void setMargin(int left, int top, int right, int bottom) {
        mDisplayLeft   = left;
        mDisplayTop    = top;
        mDisplayRight  = right;
        mDisplayBottom = bottom;
    }

    /**
     * set active
     * @param active y/n
     */
    public void setActive(boolean active) {
        Log.d(TAG, "[setActive]active:"+active);
        synchronized (lock) {
            this.mActive = active;
        }
    }

    /**
     * show subtitle/teletext
     */
    public void show() {
        if (mVisible)
            return;
        mVisible = true;
        update();
    }

    /**
     * hide subtitle/teletext
     */
    public void hide() {
        if (!mVisible)
            return;
        mVisible = false;
        update();
    }


    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        mCcImplement.mCaptionScreen.updateCaptionScreen(w, h);
    }

    @Override
    public void onDraw(Canvas canvas) {
        Log.d(TAG, "[onDraw] 0 active =  "+mActive+";mVisible="+mVisible);
       /* if (!mActive || !mVisible ) {
            if (mCcImplement == null)
                return;
            canvas.drawPaint(clear_paint);
            mCaptionWindow = null;
            return;
        }*/

        CcImplement.CaptionWindow  captionWin = mQueueCaption.poll();
        if (captionWin != null) {
            captionWin.draw(canvas);
        }
    }

    private boolean IsCloseCaptionShowEnable() {
        return SystemProperties.getBoolean("vendor.sys.subtitleService.closecaption.enable", false);
    }
    protected void finalize() throws Throwable {
        // Resource may not be available during gc process
        super.finalize();
    }

    public void setVisible(boolean value) {
        Log.d(TAG, "force set visible to:" + value);
        mVisible = value;
        postInvalidate();
    }

    Handler handler = new Handler()
    {
        public void handleMessage(Message msg) {
            Log.d(TAG, "msg.what =" + msg.what);
            switch (msg.what) {
                case JSON_MSG_NORMAL:
                    CcImplement.CaptionWindow  captionWindow = (CcImplement.CaptionWindow)msg.obj;
                    mQueueCaption.offer(captionWindow);
                    postInvalidate();
                    break;
            }
        }
    };

   private boolean filterVoidSubtitle(String str) {
        if (str.contains(CC_ONTENT)) {
            return false;
        }
        return true;
   }
    public void showJsonStr(String str) {
        Log.d(TAG, "[showJsonStr]str:" + str);
        if (filterVoidSubtitle(str))
            return;
        if (!mVisible)
            mVisible = true;
        if (!TextUtils.isEmpty(str)) {
            handler.removeMessages(JSON_MSG_NORMAL);
            handler.obtainMessage(JSON_MSG_NORMAL, mCcImplement.new CaptionWindow(str)).sendToTarget();
        }
    }

    private void checkDebug() {
        try {
            if (SystemProperties.getBoolean("vendor.sys.subtitleService.dump.enable", false)) {
                mDebug = true;
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception:" + e);
        }
    }

}

