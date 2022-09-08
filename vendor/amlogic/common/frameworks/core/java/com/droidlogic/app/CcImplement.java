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
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.Xfermode;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.accessibility.CaptioningManager;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Locale;
import java.util.Objects;
import com.droidlogic.app.SystemControlManager;

/**
 * Created by daniel on 16/10/2017.
 */
public class CcImplement {
    public final String TAG = "CcImplement";
    CaptionScreen mCaptionScreen;
    CcSetting mCcSetting;
    private Paint mBackgroundPaint;
    private Paint mTextPaint;
    private Paint mWindowPaint;
    private Paint mFadePaint;
    private Paint mWipePaint;
    private Paint mShadowPaint;

    private Path path1;
    private Path path2;

    private final int EDGE_SIZE_PERCENT = 15;
    private Context context;

    private Typeface mUndefTf;
    private Typeface mUndefItTf;
    private Typeface mMonoSerifTf;
    private Typeface mMonoSerifItTf;
    private Typeface mSerifTf;
    private Typeface mSerifItTf;
    private Typeface mMonoNoSerifTf;
    private Typeface mMonoNoSerifItTf;
    private Typeface mNoSerifTf;
    private Typeface mNoSerifItTf;
    private Typeface mCasualTf;
    private Typeface mCasualItTf;
    private Typeface mCursiveTf;
    private Typeface mCursiveItTf;
    private Typeface mSmallCapitalTf;
    private Typeface mSmallCapitalItTf;
    private Typeface mPropSansTf;
    private Typeface mPropSansItTf;

    private SystemControlManager mSystemControlManager;

    CcImplement(Context context, CustomFonts cf) {
        /* TODO: how to fetch this setting? No trigger in tv input now */
        this.context = context;
        mCcSetting = new CcSetting();
        mCaptionScreen = new CaptionScreen();
        mWindowPaint = new Paint();
        mBackgroundPaint = new Paint();
        mTextPaint = new Paint();
        mFadePaint = new Paint();
        mWipePaint = new Paint();
        mShadowPaint = new Paint();
        path1 = new Path();
        path2 = new Path();

        mSystemControlManager = SystemControlManager.getInstance();

        if (cf != null)
        {
            mMonoSerifTf = cf.mMonoSerifTf;
            mMonoSerifItTf = cf.mMonoSerifItTf;
            mCasualTf = cf.mCasualTf;
            mCasualItTf = cf.mCasualItTf;
            mPropSansTf = cf.mPropSansTf;
            mPropSansItTf = cf.mPropSansItTf;
            mSmallCapitalTf = cf.mSmallCapitalTf;
            mSmallCapitalItTf = cf.mSmallCapitalItTf;
            mCursiveTf = cf.mCursiveTf;
            mCursiveItTf = cf.mCursiveItTf;
        }
    }

    private boolean isStyle_use_broadcast()
    {
        int styleSetting = 1;
        /*try {
            styleSetting = Settings.Secure.getInt(context.getContentResolver(), "accessibility_captioning_style_enabled");
        } catch (Settings.SettingNotFoundException e) {
            Log.w(TAG, e.toString());
            styleSetting = 0;
        }*/
        /* 0 for broadcast 1 for using caption manager */
        if (styleSetting == 1) {
            return false;
        } else {
            return true;
        }
    }

    private int convertCcColor(int CcColor)
    {
        int convertColor;
        convertColor = (CcColor&0x3)*85 |
            (((CcColor&0xc)>>2)*85) << 8 |
            (((CcColor&0x30)>>4)*85) << 16 |
            0xff<<24;
        return convertColor;
    }

    private Typeface getTypefaceFromString(String fontFace, boolean italics) {
        Typeface convertFace;
        //Log.e(TAG, "font_face " + font_face);
        if (fontFace.equalsIgnoreCase("default")) {
            if (italics) {
                convertFace = mMonoSerifItTf;
            } else {
                convertFace = mMonoSerifTf;
            }
        } else if (fontFace.equalsIgnoreCase("mono_serif")) {
            if (italics) {
                convertFace = mMonoSerifItTf;
            } else {
                convertFace = mMonoSerifTf;
            }
        } else if (fontFace.equalsIgnoreCase("prop_serif")) {
            if (italics) {
                convertFace = mPropSansItTf;
            } else {
                convertFace = mPropSansTf;
            }
        } else if (fontFace.equalsIgnoreCase("mono_sans")) {
            if (italics) {
                convertFace = mMonoSerifItTf;
            } else {
                convertFace = mMonoSerifTf;
            }
        } else if (fontFace.equalsIgnoreCase("prop_sans")) {
            if (italics) {
                convertFace = mPropSansItTf;
            } else {
                convertFace = mPropSansTf;
            }
        } else if (fontFace.equalsIgnoreCase("casual")) {
            if (italics) {
                convertFace = mCasualItTf;
            } else {
                convertFace = mCasualTf;
            }
        } else if (fontFace.equalsIgnoreCase("cursive")) {
            if (italics) {
                convertFace = mCursiveItTf;
            } else {
                convertFace = mCursiveTf;
            }
        } else if (fontFace.equalsIgnoreCase("small_caps")) {
            if (italics) {
                convertFace = mSmallCapitalItTf;
            } else {
                convertFace = mSmallCapitalTf;
            }
        }
        /* For caption manager convert */
        else if (fontFace.equalsIgnoreCase("sans-serif")) {
            convertFace = mPropSansTf;
        } else if (fontFace.equalsIgnoreCase("sans-serif-condensed")) {
            convertFace = mPropSansTf;
        } else if (fontFace.equalsIgnoreCase("sans-serif-monospace")) {
            convertFace = mMonoSerifTf;
        } else if (fontFace.equalsIgnoreCase("serif")) {
            convertFace = mPropSansTf;
        } else if (fontFace.equalsIgnoreCase("serif-monospace")) {
            convertFace = mMonoSerifTf;
        } else if (fontFace.equalsIgnoreCase("casual")) {
            convertFace = mCasualTf;
        } else if (fontFace.equalsIgnoreCase("cursive")) {
            convertFace = mMonoSerifTf;
        } else if (fontFace.equalsIgnoreCase("small-capitals")) {
            convertFace = mSmallCapitalTf;
        } else {
            Log.w(TAG, "font face exception " + fontFace);
            if (italics) {
                convertFace = mMonoSerifItTf;
            } else {
                convertFace = mMonoSerifTf;
            }
        }
        return convertFace;
    }


    private boolean isFontfaceMono(String fontFace) {
        if (fontFace.equalsIgnoreCase("default")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("mono_serif")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("prop_serif")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("mono_sans")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("prop_sans")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("casual")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("cursive")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("small_caps")) {
            return false;
        }
        /* For caption manager convert */
        else if (fontFace.equalsIgnoreCase("sans-serif")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("sans-serif-condensed")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("sans-serif-monospace")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("serif")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("serif-monospace")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("casual")) {
            return false;
        } else if (fontFace.equalsIgnoreCase("cursive")) {
            return true;
        } else if (fontFace.equalsIgnoreCase("small-capitals")) {
            return false;
        } else {
            return true;
        }
    }


    class CaptionScreen
    {
        int mWidth;
        int mHeight;
        double mSafeTitleLeft;
        double mSafeTitleRight;
        double mSafeTitleTop;
        double mSafeTitleButton;
        double mSafeTitleWidth;
        double mSafeTitleHeight;
        final double mSafeTitleWPercent = 0.80;
        final double mSafeTitleHPercent = 0.85;

        int mVideoLeft;
        int mVideoRight;
        int mVideoTop;
        int mVideoBottom;
        int mVideoHeight;
        int mVideoWidth;

        double mVideohvRateOnScreen;
        int mVideohvRateOrigin;
        double mHvRate;

        double mScreenLeft;
        double mScreenRight;

        int mAnchorVertical;
        int mAnchorHorizon;
        double mAnchorVerticalDensity;
        double mAnchorHorizonDensity;
        double mMaxFontHeight;
        double mMaxFontWidth;
        double mMaxFontSize;
        double mFixedCharWidth;
        float mWindowBorderWidth;
        final int mCcRowCount = 15;
        final int mCcColCount = 32;

        void updateCaptionScreen(int w, int h)
        {
            mWidth = w;
            mHeight = h;
            mHvRate = (double)mWidth/(double)mHeight;
        }

        //Must called before calling updateCaptionScreen
        void updateVideoPosition()
        {
            String ratio = mSystemControlManager.readSysFs("/sys/class/video/frame_aspect_ratio");

            String screen_mode = mSystemControlManager.readSysFs("/sys/class/video/screen_mode");

            String video_status = mSystemControlManager.readSysFs("/sys/class/video/video_state");
            try {
                String hsStr = video_status.split("VPP_hsc_startp 0x")[1].split("\\.")[0];;
                String heStr = video_status.split("VPP_hsc_endp 0x")[1].split("\\.")[0];;
                String vsStr = video_status.split("VPP_vsc_startp 0x")[1].split("\\.")[0];;
                String veStr = video_status.split("VPP_vsc_endp 0x")[1].split("\\.")[0];;

                mVideoLeft = Integer.valueOf(hsStr, 16);
                mVideoRight = Integer.valueOf(heStr, 16);
                mVideoTop = Integer.valueOf(vsStr, 16);
                mVideoBottom = Integer.valueOf(veStr, 16);
                mVideoHeight = mVideoBottom - mVideoTop;
                mVideoWidth = mVideoRight - mVideoLeft;

                mVideohvRateOnScreen = (double)(mVideoRight - mVideoLeft) / (double)(mVideoBottom - mVideoTop);
                Log.i(TAG, "position: "+ mVideoLeft + " " + mVideoRight + " " + mVideoTop + " " + mVideoBottom + " " + mVideohvRateOnScreen);
            } catch (Exception e) {
                Log.d(TAG, "position exception " + e.toString());
            }
            try {
                mVideohvRateOrigin = Integer.valueOf(ratio.split("0x")[1], 16);
                Log.d(TAG, "ratio: " + mVideohvRateOrigin);
            } catch (Exception e) {
                //0x90 means 16:9 a fallback value
                mVideohvRateOrigin = 0x90;
                Log.d(TAG, "ratio exception " + e.toString());
            }

        }

        void updateLayout()
        {
            //Safe title must be calculated using video width and height.
            mSafeTitleHeight = mHeight * mSafeTitleHPercent;
            if (mVideohvRateOnScreen > 1.7) {
                mSafeTitleWidth = mWidth * mSafeTitleWPercent;
            } else {
                mSafeTitleWidth = mWidth * 12 / 16 * mSafeTitleWPercent;
            }
            //Font height is relative with safe title height, but now ratio of width and height can not be changed.
            mMaxFontHeight = mSafeTitleHeight / mCcRowCount;
            if (mVideohvRateOnScreen > 1.7) {
                mMaxFontWidth = mMaxFontHeight * 0.8;
            } else {
                mMaxFontWidth = mMaxFontHeight * 0.6;
            }
            mMaxFontSize = mMaxFontHeight;

            //This is used for postioning character in 608 mode.
            mFixedCharWidth = mSafeTitleWidth / (mCcColCount + 1);

            mAnchorHorizon = ((mVideohvRateOrigin & 1) == 0)?210:160; //16:9 or 4:3
            mAnchorVertical = 75;

            //This is used for calculate coordinate in non-relative anchor mode
            mAnchorHorizonDensity = mSafeTitleWidth / mAnchorHorizon;
            mAnchorVerticalDensity = mSafeTitleHeight / mAnchorVertical;

            mWindowBorderWidth = (float)(mMaxFontHeight/6);
            mSafeTitleLeft = (mWidth - mSafeTitleWidth)/2;
            mSafeTitleRight = mSafeTitleLeft + mSafeTitleWidth;
            mSafeTitleTop = (mHeight - mSafeTitleHeight)/2;
            mSafeTitleButton = mSafeTitleTop + mSafeTitleHeight;
            mScreenLeft = mCaptionScreen.getWindowLeftTopX(true, 0, 0, 0);
            mScreenRight = mCaptionScreen.getWindowLeftTopY(true, 0, 0, 0);
//            Log.i(TAG, "updateCaptionScreen " + " " + mScreenLeft+ " " + mScreenRight
//                    +mVideohvRateOnScreen + ' '+mAnchorHorizon+' '+mAnchorHorizonDensity);
        }

        double getWindowLeftTopX(boolean anchorRelative, int anchorH, int anchorPoint, double rowLength)
        {
            double offset;
            /* Get anchor coordinate x */
            if (!anchorRelative) {
                /* mAnchorH is horizontal steps */
                offset = mSafeTitleWidth * anchorH / mAnchorHorizon + mSafeTitleLeft;
            } else {
                /* mAnchorH is percentage */
                offset = mSafeTitleWidth * anchorH / 100 + mSafeTitleLeft;
            }
            switch (anchorPoint)
            {
                case 0:
                case 3:
                case 6:
                    return offset;
                case 1:
                case 4:
                case 7:
                    return offset - rowLength/2 + mSafeTitleWidth / 2;
                case 2:
                case 5:
                case 8:
                    return offset - rowLength + mSafeTitleWidth;
                default:
                    return -1;
            }
        }

        double getWindowLeftTopY(boolean anchorRelative, int anchorV, int anchorPoint, int rowCount)
        {
            double offset;
            double position;

            if (!anchorRelative) {
                /* mAnchorV is vertical steps */
                offset = mSafeTitleHeight * anchorV / mAnchorVertical + mSafeTitleTop;
            } else {
                /* mAnchorV is percentage */
                offset = mSafeTitleHeight * anchorV / 100 + mSafeTitleTop;
            }

            switch (anchorPoint)
            {
                case 0:
                case 1:
                case 2:
                    position = offset;
                    break;
                case 3:
                case 4:
                case 5:
                    position = offset - (rowCount * mMaxFontHeight)/2;
                    break;
                case 6:
                case 7:
                case 8:
                    position = offset - rowCount * mMaxFontHeight;
                    break;
                default:
                    position = mSafeTitleTop - rowCount * mMaxFontHeight;
                    break;
            }

            if ((position) < mSafeTitleTop) {
                position = mSafeTitleTop;
            } else if ((position + rowCount * mMaxFontHeight) > mSafeTitleButton) {
                position = mSafeTitleButton - rowCount * mMaxFontHeight;
            }
            return position;
        }
    }

    class CcSetting {
        Locale mCcLocale;
        float mFontScale;
        boolean mIsEnabled;
        Typeface mTypeFace;
        boolean mHasBackgroundColor;
        boolean mHasEdgeColor;
        boolean mHasEdgeType;
        boolean mHasForegroundColor;
        boolean mHasWindowColor;
        int mForegroundColor;
        int mForegroundOpacity;
        int mWindowColor;
        int mWindowOpcity;
        int mBackgroundColor;
        int mBackgroundOpcity;
        int mEdgeColor;
        int mEdgeType;
        int mStrockeWidth;
        final Object lock;

        CcSetting()
        {
            lock = new Object();
        }

        void UpdateCcSetting(CaptioningManager cm)
        {
            synchronized (lock) {
                if (cm != null) {
                    CaptioningManager.CaptionStyle cs = cm.getUserStyle();
                    mCcLocale = cm.getLocale();
                    mFontScale = cm.getFontScale();
                    mIsEnabled = cm.isEnabled();
                    // Log.e(TAG, "Caption manager enable: " + mIsEnabled);
                    mStrockeWidth = 0;
                    /* When use preset setting of cc, such as black on white,
                     * font face will be null */
                    //mTypeFace = cs.getTypeface();
                    /* We need to find out the selected fontface name ourself,
                     * and load the face from local area */
                    // Log.e(TAG, "typeface name " + Settings.Secure.getString(context.getContentResolver(),
                    //            "accessibility_captioning_typeface"));
                    mTypeFace = cs.getTypeface();
                    mHasBackgroundColor = cs.hasBackgroundColor();
                    mHasEdgeColor = cs.hasEdgeColor();
                    mHasEdgeType = cs.hasEdgeType();
                    mHasForegroundColor = cs.hasForegroundColor();
                    mHasWindowColor = cs.hasWindowColor();
                    mForegroundColor = cs.foregroundColor;
                    mForegroundOpacity = mForegroundColor >>> 24;
                    mWindowColor = cs.windowColor;
                    mWindowOpcity = mWindowColor >>> 24;
                    mBackgroundColor = cs.backgroundColor;
                    mBackgroundOpcity = mBackgroundColor >>> 24;
                    mEdgeColor = cs.edgeColor;
                    mEdgeType = cs.edgeType;
                }
            }
            // dump();
        }
        void dump()
        {
            Log.i(TAG, "enable "+ mIsEnabled +
                    " locale " + mCcLocale +
                    " mFontScale " + mFontScale +
                    " mStrockeWidth " + mStrockeWidth +
                    " mTypeFace " + mTypeFace +
                    " mHasBackgroundColor " + mHasBackgroundColor +
                    " mHasEdgeColor " + mHasEdgeColor +
                    " mHasEdgeType " + mHasEdgeColor +
                    " mHasForegroundColor " + mHasForegroundColor +
                    " mHasWindowColor " + mHasWindowColor +
                    " mForegroundColor " + Integer.toHexString(mForegroundColor) +
                    " mForegroundOpacity " + mForegroundOpacity +
                    " mWindowColor " + Integer.toHexString(mWindowColor) +
                    " mWindowOpcity " + mWindowOpcity +
                    " mBackgroundColor " + Integer.toHexString(mBackgroundColor) +
                    " mBackgroundOpcity " + mBackgroundOpcity +
                    " mEdgeColor " + Integer.toHexString(mEdgeColor) +
                    " mEdgeType " + mEdgeType);
        }
    }

    class CaptionWindow
    {
        JSONObject mCcObj = null;
        JSONArray mWindowArr = null;
        String mCcVersion;
        int mWindowsCount;
        Window mWindows[];
        boolean mInitFlag;
        boolean mStyleUseBroadcast;

        CaptionWindow(String jsonStr)
        {
            mStyleUseBroadcast = isStyle_use_broadcast();
            mCaptionScreen.updateVideoPosition();
            mCaptionScreen.updateLayout();
            mInitFlag = false;
            try {
                if (!TextUtils.isEmpty(jsonStr)) {
                    mCcObj = new JSONObject(jsonStr);
                } else {
                    return;
                }
                mCcVersion = mCcObj.getString("type");
                Log.i(TAG, "[CaptionWindow]mCcVersion:" + mCcVersion);
                if (mCcVersion.matches("cea608"))
                {
                    mWindowArr = mCcObj.getJSONArray("windows");
                    mWindowsCount = mWindowArr.length();
                    //Log.e(TAG, "ccType 608" + " window number: " + mWindowsCount);
                    mWindows = new Window[mWindowsCount+1];
                    for (int i=0; i<mWindowsCount; i++)
                        mWindows[i] = new Window(mWindowArr.getJSONObject(i));
                    Log.i(TAG, "[CaptionWindow]-1--");
                }
                else if (mCcVersion.matches("cea708"))
                {
                    mWindowArr = mCcObj.getJSONArray("windows");
                    mWindowsCount = mWindowArr.length();
                    //Log.e(TAG, "ccType 708" + " window number: " + mWindowsCount);
                    mWindows = new Window[mWindowsCount+1];
                    for (int i=0; i<mWindowsCount; i++)
                        mWindows[i] = new Window(mWindowArr.getJSONObject(i));
                }
                else {
                    Log.d(TAG, "ccType unknown");
                    return;
                }
            }
            catch (JSONException e)
            {
                Log.e(TAG, "Window init failed, exception: " + e.toString());
                mInitFlag = false;
            }
            mInitFlag = true;
    }

    class Window
    {
        int mAnchorPointer;
        int mAnchorV;
        int mAnchorH;
        boolean mAnchorRelative;
        int mRowCount;
        int mColCount;

        boolean mRowLock;
        boolean mColoumnLock;
        String mJustify;
        String mPrintDirection;
        String mScrollDirection;
        boolean mWordwrap;
        int mEffectSpeed;
        String mFillOpacity;
        int mFillColor;
        String mBorderType;
        int mBorderColor;
        double mPensizeWindowDepend;

        String mDisplayEffect;
        String mEffectDirection;
        String mEffectStatus;
        int mEffectPercent;

        final double mWindowEdgeRate = 0.15;
        double mWindowEdgeWidth;

        double mWindowWidth;
        double mWindowLeftMost;
        double mWindowStartX;
        double mWindowStartY;

        double mWindowLeft;
        double mWindowTop;
        double mWindowButtom;
        double mWindowRight;
        double mRowLength;
        int mHeartBeat;

        double mWindowMaxFontSize;

        JSONArray mJsonRows;
        int mFileeOpcityInt;

        Rows rows[];
            //Temp use system property

        Window(JSONObject windowStr) {
            mWindowWidth = 0;
            mWindowLeftMost = 10000;
            mWindowMaxFontSize = 0;
            mWindowEdgeWidth = (float)(mCaptionScreen.mMaxFontHeight * mWindowEdgeRate);
            try {
                mAnchorPointer = windowStr.getInt("anchor_point");
                mAnchorV = windowStr.getInt("anchor_vertical");
                mAnchorH = windowStr.getInt("anchor_horizontal");
                mAnchorRelative = windowStr.getBoolean("anchor_relative");
                mRowCount = windowStr.getInt("row_count") ;
                mColCount = windowStr.getInt("column_count");
                mRowLock = windowStr.getBoolean("row_lock");
                mColoumnLock = windowStr.getBoolean("column_lock");
                mJustify = windowStr.getString("justify");
                mPrintDirection = windowStr.getString("print_direction");
                mScrollDirection = windowStr.getString("scroll_direction");
                mWordwrap = windowStr.getBoolean("wordwrap");
                mDisplayEffect = windowStr.getString("display_effect");
                mEffectDirection = windowStr.getString("effect_direction");
                mEffectSpeed = windowStr.getInt("effect_speed");
                //mHeartBeat = windowStr.getInt("mHeartBeat");
            } catch (Exception e) {
                Log.e(TAG, "window property exception " + e.toString());
                mInitFlag = false;
            }

            if (!mStyleUseBroadcast && mCcVersion.matches("cea708"))
            {
                mFileeOpcityInt = mCcSetting.mWindowOpcity;
                mFillColor = mCcSetting.mWindowColor;
                mBorderType = "none";

            } else {
                if (mCcVersion.matches("cea708")) {
                    try {
                        mEffectPercent = windowStr.getInt("effect_percent");
                        mEffectStatus = windowStr.getString("effect_status");
                    } catch (Exception e) {
                        mEffectPercent = 100;
                        mEffectStatus = null;
                        Log.e(TAG, "window effect attr exception: " + e.toString());
                    }
                }
                try {
                    mFillOpacity = windowStr.getString("fill_opacity");
                    mFillColor = windowStr.getInt("fill_color");
                    mBorderType = windowStr.getString("border_type");
                    mBorderColor = windowStr.getInt("border_color");
                } catch (Exception e) {
                    Log.e(TAG, "window style exception " + e.toString());
                }

                    /* Value from stream need to be converted */
                mFillColor = convertCcColor(mFillColor);
                mBorderColor = convertCcColor(mBorderColor);

                if (mFillOpacity.equalsIgnoreCase("solid")) {
                    mFileeOpcityInt = 0xff;
                } else if (mFillOpacity.equalsIgnoreCase("transparent")){
                    mFileeOpcityInt = 0;
                } else if (mFillOpacity.equalsIgnoreCase("translucent")){
                    mFileeOpcityInt = 0x80;
                } else if (mFillOpacity.equalsIgnoreCase("flash")) {
                    /*if (mHeartBeat == 0)
                        mFileeOpcityInt = 0;
                    else*/
                        mFileeOpcityInt = 0xff;
                } else
                    mFileeOpcityInt = 0xff;
            }
            try {
                mJsonRows = windowStr.getJSONArray("rows");
            } catch (Exception e) {
                Log.e(TAG, "json fatal exception "+ e.toString());
                mInitFlag = false;
            }
            //Log.e(TAG, "mJsonRows: " + mJsonRows.toString());

            if (mRowCount > mJsonRows.length()) {
                Log.i(TAG, "window loses "+ (mRowCount - mJsonRows.length()) + " rows");
            }
            rows = new Rows[mRowCount];

                /* Find pensize
                 * I know this is shit, initialize json two times,
                 * but i do not know how to get pensize and font size
                 * the first time, that means i do not know window
                 * width neither.
                 * The problem is to implement full justification
                 * */
                /*
                   for (int i=0; i<mJsonRows.length(); i++) {
                   rows[i] = new Rows(new JSONObject(mJsonRows.optString(i)));
                   try {
                       rows[i] = new Rows(new JSONObject(mJsonRows.optString(i)));
                   } catch (Exception e) {
                       Log.e(TAG, "json rows construct exception " + e.toString());
                       mInitFlag = false;
                   }
                   rows[i].mRowNumberInWindow = i;
                   mRowLength = rows[i].mRowLengthOnPaint;
                   Log.e(TAG, "Row right most: " + i + " " + mRowLength);
                   mWindowLeftMost = rows[i].mRowStartX < mWindowLeftMost ?
                   rows[i].mRowStartX : mWindowLeftMost;
                   double mRowMaxFontSize = rows[i].mRowMaxFontSize;
                   mWindowMaxFontSize = (mWindowMaxFontSize > mRowMaxFontSize)
                   ?mWindowMaxFontSize:mRowMaxFontSize;
                   }
                */
            mWindowMaxFontSize = mCaptionScreen.mSafeTitleWidth / 32;
            mWindowWidth = mColCount * mWindowMaxFontSize;
            /* ugly repeat */
            for (int i=0; i<mRowCount; i++) {
                try {
                    rows[i] = new Rows(new JSONObject(mJsonRows.optString(i)));
                } catch (Exception e) {
                    Log.e(TAG, "json rows construct exception " + e.toString());
                    mInitFlag = false;
                }
                rows[i].mRowNumberInWindow = i;
                mRowLength = rows[i].mRowLengthOnPaint;
                //Log.e(TAG, "Row right most: " + i + " " + mRowLength);
                /* mWindowWidth = (mWindowWidth > mRowLength)
                   ? mWindowWidth : mRowLength; */
                //mWindowWidth = mColCount * caption_screen.mMaxFontWidth;
                mWindowLeftMost = rows[i].mRowStartX < mWindowLeftMost ?
                    rows[i].mRowStartX : mWindowLeftMost;
            }

            //mWindowLeftMost *= mCaptionScreen.mMaxFontWidth;
            mWindowLeftMost *= mPensizeWindowDepend;
            //max_row_str_length = mColCount * mCaptionScreen.mMaxFontWidth;
            //Log.e(TAG, "Max row length "+ mWindowWidth);
            mWindowStartX = mCaptionScreen.getWindowLeftTopX(mAnchorRelative, mAnchorH, mAnchorPointer, mWindowWidth);

            mWindowStartY = mCaptionScreen.getWindowLeftTopY(mAnchorRelative, mAnchorV, mAnchorPointer, mRowCount);

            dump();
        }

        void draw(Canvas canvas)
        {
            Log.d(TAG, "[draw]=====mCcVersion:"+mCcVersion);
            /* Draw window */
            if (mCcVersion.equalsIgnoreCase("cea708")) {
                double columnsWidth;
                columnsWidth = mColCount * mCaptionScreen.mMaxFontWidth;
                //mWindowLeft = mWindowStartX - mWindowEdgeWidth + mWindowLeftMost;
                mWindowLeft = mWindowStartX;
                mWindowTop = mWindowStartY;
                /* Use columns count to get window right margin */
                mWindowRight = mWindowStartX + mWindowWidth;
                mWindowButtom = mWindowStartY + mCaptionScreen.mMaxFontHeight * mRowCount;

                mWindowPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
                /* Draw border */
                /* Draw border color */
                draw_border(canvas, mWindowPaint, mShadowPaint, mBorderType,
                        (float)mWindowLeft, (float)mWindowTop, (float)mWindowRight, (float)mWindowButtom,
                        mBorderColor);

                /* Draw window */
                mWindowPaint.setColor(mFillColor);
                mWindowPaint.setAlpha(mFileeOpcityInt);
                //Log.e(TAG, "window rect: "+ " color " + mFillColor + " opacity "+mFileeOpcityInt);
            }
            /* This is only for 608 text mode, and the window is background */
            else {
                mWindowLeft = mWindowStartX;
                mWindowRight = mWindowStartX + mCaptionScreen.mSafeTitleWidth;
                mWindowTop = mWindowStartY;
                mWindowButtom = mWindowStartY + mCaptionScreen.mSafeTitleHeight;
                mWindowPaint.setColor(mFillColor);
                mWindowPaint.setAlpha(mFileeOpcityInt);
            }
            canvas.drawRect((float) mWindowLeft, (float) mWindowTop, (float) mWindowRight, (float) mWindowButtom, mWindowPaint);
            Log.e(TAG, "window rect " + mWindowLeft + " " + mWindowRight + " " + mWindowTop + " " + mWindowButtom);


            /* Draw rows */
            for (int i=0; i<mRowCount; i++) {
                if (rows[i].mRowArray.length() != 0) {
                    rows[i].draw(canvas);
                }
            }

            if (mCcVersion.equalsIgnoreCase("cea708")) {
                double rectLeft, rectRight, rectTop, rectBottom;
                float border = mCaptionScreen.mWindowBorderWidth;
                if (mDisplayEffect.equalsIgnoreCase("fade")) {
                    mFadePaint.setColor(Color.WHITE);
                    mFadePaint.setAlpha(mEffectPercent*255/100);
                    mFadePaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SCREEN));
                    canvas.drawRect((float)mWindowLeft - border,
                            (float)mWindowTop - border,
                            (float)mWindowRight + border,
                            (float)mWindowButtom + border,
                            mFadePaint);

                } else if (mDisplayEffect.equalsIgnoreCase("wipe")) {
                    if (mEffectDirection.equalsIgnoreCase("left_right")) {
                        rectLeft = mWindowStartX - border;
                        rectRight = mWindowStartX + mWindowWidth * mEffectPercent /100 + border;
                        rectTop = mWindowTop - border;
                        rectBottom = mWindowButtom + border;
                    } else if (mEffectDirection.equalsIgnoreCase("right_left")) {
                        rectLeft = mWindowStartX + mWindowWidth * (100 - mEffectPercent)/100 - border;
                        rectRight = mWindowStartX + mWindowWidth + border;
                        rectTop = mWindowTop - border;
                        rectBottom = mWindowButtom + border;
                    } else if (mEffectDirection.equalsIgnoreCase("top_bottom")) {
                        rectLeft = mWindowStartX - border;
                        rectRight = mWindowStartX + mWindowWidth + border;
                        rectTop = mWindowTop - border;
                        rectBottom = mWindowTop + (mWindowButtom - mWindowTop) * mEffectPercent/100 + border;
                    } else if (mEffectDirection.equalsIgnoreCase("bottom_top")) {
                        rectLeft = mWindowStartX - border;
                        rectRight = mWindowStartX + mWindowWidth + border;
                        rectTop = mWindowTop + (mWindowButtom - mWindowTop) * (100 - mEffectPercent)/100 - border;
                        rectBottom = mWindowButtom + border;
                    } else {
                        rectLeft = 0;
                        rectBottom = 0;
                        rectTop = 0;
                        rectRight = 0;
                    }
                    mWipePaint.setColor(Color.WHITE);
                    mWipePaint.setAlpha(0);
                    mWipePaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
                    canvas.drawRect((float)rectLeft,
                            (float)rectTop,
                            (float)rectRight,
                            (float)rectBottom,
                            mWipePaint);
                }
            }
        }

        void draw_border(Canvas canvas, Paint borderPaint, Paint shadowPaint, String borderType,
                             float l, float t, float r, float b,
                             int borderColor)
        {
            float gap = (float)(mCaptionScreen.mWindowBorderWidth);
            //Log.e(TAG, "Border type " + mBorderType + " left " + l + " right " + r + " top " + t + " bottom " + b);
            //mShadowPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.LIGHTEN));
            shadowPaint.setColor(Color.GRAY);
            shadowPaint.setAlpha(0x90);
            if (borderType.equalsIgnoreCase("none")) {
              //                     We do nothing??
            }
            if (borderType.equalsIgnoreCase("raised") ||
                    borderType.equalsIgnoreCase("depressed")) {
                float og = (float)(gap * 0.6);
                int left_top_color, right_bottom_color;
                borderPaint.setStyle(Paint.Style.FILL);
                path1.reset();
                path2.reset();

                // Left top
                borderPaint.setColor(borderColor);
                if (borderType.equalsIgnoreCase("raised")) {
                //Right
                    path1.moveTo(r - 1, t - 1);
                    path1.lineTo(r + og, t - og);
                    path1.lineTo(r + og, b - og);
                    path1.lineTo(r - 1, b);
                    path1.close();

                //Left
                    path2.moveTo(l + 1, b);
                    path2.lineTo(l - og, b - og);
                    path2.lineTo(l - og, t - og);
                    path2.lineTo(l + 1, t);
                    path2.close();

                    canvas.drawPath(path1, borderPaint);
                    canvas.drawPath(path2, borderPaint);
                //Top
                    canvas.drawRect(l - og, t - og, r + og, t, borderPaint);

                } else if (borderType.equalsIgnoreCase("depressed")) {
                //Right
                    path1.moveTo(r - 1, t);
                    path1.lineTo(r + og, t + og);
                    path1.lineTo(r + og, b + og);
                    path1.lineTo(r - 1, b);
                    path1.close();

                //Left
                    path2.moveTo(l + 1, b);
                    path2.lineTo(l - og, b + og);
                    path2.lineTo(l - og, t + og);
                    path2.lineTo(l + 1, t);
                    path2.close();

                    canvas.drawPath(path1, borderPaint);
                    canvas.drawPath(path2, borderPaint);
                    canvas.drawRect(l - og, b, r + og, b + og, borderPaint);
                }
            }
            if (borderType.equalsIgnoreCase("uniform")) {
                mWindowPaint.setColor(borderColor);
                canvas.drawRect(l-gap, t-gap, r+gap, b+gap, mWindowPaint);
            }
            if (borderType.equalsIgnoreCase("shadow_left")) {
                mWindowPaint.setColor(borderColor);
                canvas.drawRect(l-gap, t+gap, r-gap, b+gap, mWindowPaint);
            }
            if (borderType.equalsIgnoreCase("shadow_right"))
            {
                mWindowPaint.setColor(borderColor);
                canvas.drawRect(l+gap, t+gap, r+gap, b+gap, mWindowPaint);
            }
        }

        void dump()
        {
            Log.i(TAG, "Window attr: " +
                    " mAnchorPointer " + mAnchorPointer +
                    " mAnchorV " + mAnchorV +
                    " mAnchorH " + mAnchorH +
                    " mAnchorRelative " + mAnchorRelative +
                    " mRowCount " + mRowCount +
                    " mColCount " + mColCount +
                    " mRowLock " + mRowLock +
                    " mColoumnLock " + mColoumnLock +
                    " mJustify " + mJustify +
                    " mPrintDirection " + mPrintDirection +
                    " mScrollDirection " + mScrollDirection +
                    " wordwrap " + mWordwrap +
                    " mDisplayEffect " + mDisplayEffect +
                    " mEffectDirection " + mEffectDirection +
                    " mEffectSpeed " + mEffectSpeed +
                    " mEffectPercent " + mEffectPercent +
                    " mFillOpacity " + mFillOpacity +
                    " mFillColor " + mFillColor +
                    " mBorderType " + mBorderType +
                    " mBorderColor " + mBorderColor +
                    " window_length " + mWindowWidth +
                    " mWindowStartX " + mWindowStartX +
                    " mWindowStartY " + mWindowStartY +
                    " width " + mCaptionScreen.mWidth +
                    " height " + mCaptionScreen.mHeight);
        }

        class Rows
        {
            int mStrCount;//mStrCount;
            RowStr mRowStrs[];//mRowStrs[];
            JSONArray mRowArray;//mRowArray;
            /* Row length is sum of each string */
            double mRowLengthOnPaint;//mRowLengthOnPaint;
            double mRowStartX;//mRowStartX;
            double mRowStartY;//mRowStartY;
            int mRowNumberInWindow;//mRowNumberInWindow;
            int mRowCharactersCount;//mRowCharactersCount;
            double mPriorStrPositionForDraw;//mPriorStrPositionForDraw;
            /* This is for full justification use */
            double mCharacterGap;//mCharacterGap;
            double mRowMaxFontSize;//mRowMaxFontSize;

            Rows(JSONObject rows)
            {
                mPriorStrPositionForDraw = -1;
                mRowCharactersCount = 0;
                mRowMaxFontSize = 0;
                try {
                    mRowArray = rows.optJSONArray("content");
                    mRowStartX = rows.optInt("row_start");
                    mStrCount = mRowArray.length();
                    mRowStrs = new RowStr[mStrCount];
                    double single_char_width = mCcVersion.matches("cea708") ?
                            mWindowMaxFontSize : mCaptionScreen.mFixedCharWidth;
                    for (int i=0; i<mStrCount; i++) {
                        mRowStrs[i] = new RowStr(mRowArray.getJSONObject(i));
                        //Every string starts at prior string's tail
                        mRowStrs[i].mStrStartX = mRowLengthOnPaint + mRowStartX * single_char_width;
                        mRowCharactersCount += mRowStrs[i].mStrCharactersCount;
                        mRowLengthOnPaint += mRowStrs[i].mStringLengthOnPaint;
                        double strMaxFontSize = mRowStrs[i].mMaxSingleFontWidth;
                        mRowMaxFontSize = (strMaxFontSize > mRowMaxFontSize)
                            ?strMaxFontSize:mRowMaxFontSize;
                    }
                    mCharacterGap = mWindowWidth/mRowCharactersCount;
                } catch (JSONException e) {
                    Log.w(TAG, "Str exception: " + e.toString());
                    mRowLengthOnPaint = 0;
                    mInitFlag = false;
                }
            }

            void draw(Canvas canvas)
            {
                if (mRowLengthOnPaint == 0 || mStrCount == 0)
                    return;
                for (int i=0; i < mStrCount; i++)
                    mRowStrs[i].draw(canvas);
            }

            class RowStr {
                /* For parse json use */
                boolean mItalics;
                boolean mUnderline;
                int mEdgeColor;
                int mFgColor;
                int mBgColor;
                String mPenSize;
                String mFontStyle;
                String mOffset;
                String mEdgeType;
                String mFgOpacity;
                String mBgOpacity;
                String mData;
                double mStringLengthOnPaint;
                /* TODO: maybe there is more efficient way to do this */
                double mMaxSingleFontWidth;
                double mStrStartX;
                double mStrLeft;
                double mStrTop;
                double mStrRight;
                double mStrBottom;
                double mFontSize;
                int mStrCharactersCount;
                Paint.FontMetricsInt mFontMetrics;


                /* below is the actual parameters we used */
                int mFgOpacityInt = 0xff;
                int mBgOpacityInt = 0xff;
                double mFontScale;
                Typeface mFontFace;
                double mEdgeWidth;
                boolean mUseCaptionManagerStyle;
                RowStr(JSONObject rowStr)
                {
                    mStringLengthOnPaint = 0;
                    /* Get parameters from json */
                    if (!mStyleUseBroadcast && mCcVersion.matches("cea708")) {
                        mUseCaptionManagerStyle = true;
                        /* Get parameters from Caption manager */
                        /* Retreat font scale: 0.5 1 1.5 2 */
                        //mFontScale = (mCcSetting.mFontScale - 2) / 5 + 1;
                        mFontScale = mCcSetting.mFontScale/(2/0.8);
                        mFontSize = mCaptionScreen.mMaxFontSize;

                        switch (mCcSetting.mEdgeType)
                        {
                            case 0:
                                mEdgeType = "none";
                                break;
                            case 1:
                                /* Uniform is outline */
                                mEdgeType = "uniform";
                                break;
                            case 2:
                                mEdgeType = "shadow_right";
                                break;
                            case 3:
                                mEdgeType = "raised";
                                break;
                            case 4:
                                mEdgeType = "depressed";
                                break;
                            default:
                                Log.w(TAG, "Edge not supported: " + mCcSetting.mEdgeType);
                        }
                        mEdgeColor = mCcSetting.mEdgeColor;
                        mFgColor = mCcSetting.mForegroundColor;
                        mFgOpacityInt = mCcSetting.mForegroundOpacity;
                        mBgColor = mCcSetting.mBackgroundColor;
                        mBgOpacityInt = mCcSetting.mBackgroundOpcity;
                        mFontStyle = "mono_serif";
                        mOffset = "normal";
                        /* Add italic and underline support */
                        mItalics = false;
                        mUnderline = false;

                        try {
                            mData = rowStr.getString("data");
                        } catch (JSONException e) {
                            Log.e(TAG, "Get string failed: " + e.toString());
                            mInitFlag = false;
                            return;
                        }
                        setDrawerConfig(mData, null, mFontSize, mFontScale,
                                mOffset,
                                mFgColor, mFgOpacityInt,
                                mBgColor, mBgOpacityInt,
                                mEdgeColor, mEdgeWidth, mEdgeType,
                                mItalics, mUnderline, mUseCaptionManagerStyle);
                    } else {
                        mUseCaptionManagerStyle = false;
                        try {
                            mPenSize = rowStr.getString("pen_size");
                        } catch (Exception e) {
                            mPenSize = "standard";
                        }
                        if (mPenSize.equalsIgnoreCase("small")) {
                            this.mFontScale = 0.5;
                        } else if (mPenSize.equalsIgnoreCase("large")) {
                            this.mFontScale = 0.8;
                        } else if (mPenSize.equalsIgnoreCase("standard")) {
                            this.mFontScale = 0.65;
                        } else {
                            Log.w(TAG, "Font scale not supported: " + mPenSize);
                            this.mFontScale = 0.8;
                        }

                        try {
                            mFontStyle = rowStr.getString("font_style");
                        } catch (Exception e) {
                            mFontStyle = "mono_serif";
                        }

                        try {
                            mOffset = rowStr.getString("offset");
                        } catch (Exception e) {
                            mOffset = "normal";
                        }

                        try {
                            mItalics = rowStr.getBoolean("italics");
                            mUnderline = rowStr.getBoolean("underline");
                            mFgColor = rowStr.getInt("fg_color");
                            mFgOpacity = rowStr.getString("fg_opacity");
                            mBgColor = rowStr.getInt("bg_color");
                            mBgOpacity = rowStr.getString("bg_opacity");
                            mData = rowStr.getString("data");
                        } catch (Exception e) {
                            mItalics = false;
                            mUnderline = false;
                            mFgColor = 0;
                            mFgOpacity = "transparent";
                            mBgColor = 0;
                            mBgOpacity = "transparent";
                            mData = "";
                            Log.e(TAG, "Row str exception: " + e.toString());
                        }

                        if (mCcVersion.equalsIgnoreCase("cea708")) {
                            try {
                                mEdgeType = rowStr.getString("edge_type");
                            } catch (Exception e) {
                                mEdgeType = "none";
                            }

                            try {
                                mEdgeColor = rowStr.getInt("edge_color");
                            } catch (Exception e) {
                                mEdgeColor = 0;
                            }
                        }

                        mFontSize = mCaptionScreen.mMaxFontSize;// * mFontScale;

                        mFgColor = convertCcColor(mFgColor);
                        mBgColor = convertCcColor(mBgColor);
                        mEdgeColor = convertCcColor(mEdgeColor);

//                              1. Solid --> opacity = 100
//                              2. Transparent --> opacity = 0
//                              3. Translucent --> opacity = 50
//                              4. flashing
                        if (mFgOpacity.equalsIgnoreCase("solid")) {
                            mFgOpacityInt = 0xff;
                        } else if (mFgOpacity.equalsIgnoreCase("transparent")) {
                            mFgOpacityInt = 0;
                        } else if (mFgOpacity.equalsIgnoreCase("translucent")) {
                            mFgOpacityInt = 0x80;
                        } else {
                            Log.w(TAG, "Fg opacity Not supported yet " + mFgOpacity);
                        }

                        /* --------------------Background----------------- */
                        if (mBgOpacity.equalsIgnoreCase("solid")) {
                            mBgOpacityInt = 0xff;
                        } else if (mBgOpacity.equalsIgnoreCase("transparent")){
                            mBgOpacityInt = 0x0;
                        } else if (mBgOpacity.equalsIgnoreCase("translucent")){
                            mBgOpacityInt = 0x80;
                        } else if (mBgOpacity.equalsIgnoreCase("flash")) {
                            /*if (mHeartBeat == 0)
                                bg_opacity_int = 0;
                            else*/
                                mBgOpacityInt = 0xff;
                        }
                        setDrawerConfig(mData, mFontStyle, mFontSize, mFontScale,
                                mOffset,
                                mFgColor, mFgOpacityInt,
                                mBgColor, mBgOpacityInt,
                                mEdgeColor, mEdgeWidth, mEdgeType,
                                mItalics, mUnderline, mUseCaptionManagerStyle);
                    }


                    /* Set parameters */
                    mWindowPaint.setTypeface(mFontFace);
                    /* Get a largest metric to get the baseline */
                    mWindowPaint.setTextSize((float)mCaptionScreen.mMaxFontHeight);
                    mFontMetrics = mWindowPaint.getFontMetricsInt();
                    /* Return to normal */
                    mWindowPaint.setTextSize((float)(mFontSize * mFontScale));

                    //Log.e(TAG, "str on paint " + string_length_on_paint + " " + data);
                    mEdgeWidth = mFontSize/EDGE_SIZE_PERCENT;
                    if (mPensizeWindowDepend == 0) {
                        mPensizeWindowDepend = mWindowPaint.measureText("H");
                    }
                    mStrCharactersCount = mData.length();

                }
                void setDrawerConfig(String data, String fontFace, double fontSize, double fontScale,
                        String offset,
                        int fgColor, int fgOpacity,
                        int bgColor, int bgOpacity,
                        int edgeColor, double edgeWidth, String edgeType,
                        boolean italics, boolean underline, boolean useCaptionManagerStyle)
                {
                    this.mData = data;
                    /* Convert font scale to a logical range */
                    this.mFontSize = fontSize * fontScale;
                    boolean isMonospace = false;

                    if (fontFace == null) {
                        fontFace = "not set";
                    }
                    /* Typeface handle:
                     * Temporarily leave caption manager's config, although it is lack of some characters
                     * Now, only switch typeface for stream
                     */
                    if (!useCaptionManagerStyle) {
                        this.mFontFace = getTypefaceFromString(fontFace, italics);
                        isMonospace = isFontfaceMono(fontFace);
                    } else {
                        String cmFontfaceName = Settings.Secure.getString(context.getContentResolver(),
                                "accessibility_captioning_typeface");
                        if (cmFontfaceName == null) {
                            cmFontfaceName = "not set";
                        }
                        this.mFontFace = getTypefaceFromString(cmFontfaceName, false);
                        isMonospace = isFontfaceMono(cmFontfaceName);
                    }

                    this.mFgColor = fgColor;
                    this.mFgOpacityInt = fgOpacity;
                    this.mBgColor = bgColor;
                    this.mBgOpacityInt = bgOpacity;
                    this.mEdgeColor = edgeColor;
                    this.mEdgeWidth = edgeWidth;
                    this.mEdgeType = edgeType;
                    this.mItalics = italics;
                    this.mUnderline = underline;
                    this.mOffset = offset;

                    mWindowPaint.setTypeface(this.mFontFace);
                    mWindowPaint.setTextSize((float)this.mFontSize);
                    // mWindowPaint.setLetterSpacing((float) 0.05);
                    mMaxSingleFontWidth = mWindowPaint.measureText("_");
                    if (mCcVersion.matches("cea708")) {
                        if (isMonospace) {
                            mStringLengthOnPaint = (data.length() + 1) * mMaxSingleFontWidth;
                        } else {
                            mStringLengthOnPaint = mWindowPaint.measureText(data) + mMaxSingleFontWidth;
                            // string_length_on_paint = (data.length()+1) * max_single_font_width;
                        }
                    } else {
                        mStringLengthOnPaint = mWindowPaint.measureText(data) + mMaxSingleFontWidth;
                    }
                    /* Convert */
                    /*
                       Log.e(TAG, "str attr: " +
                       " use_user_style " + use_caption_manager_style +
                       " mMaxFontHeight " + caption_screen.mMaxFontSize +
                       " font_size " + this.font_size +
                       " mFontScale " + mFontScale +
                       " font_style " + font_face +
                       " offset " + this.offset +
                       " italics " + this.italics +
                       " underline " + this.underline +
                       " mEdgeType " + this.mEdgeType +
                       " mEdgeColor " + this.mEdgeColor +
                       " fg_color " + this.fg_color +
                       " fg_opacity " + this.fg_opacity_int +
                       " bg_color " + this.bg_color +
                       " bg_opacity " + this.bg_opacity_int +
                       " data " + this.data);
                       */
                }

                /* Draw font and background
                 * 1. Make sure backgroud was drew first
                 * */
                void draw(Canvas canvas)
                {
                    mStrTop = mWindowStartY + mRowNumberInWindow * mCaptionScreen.mMaxFontHeight;
                    mStrBottom = mWindowStartY + (mRowNumberInWindow + 1) * mCaptionScreen.mMaxFontHeight;
                    /* Handle mJustify here */
                    if (mJustify.equalsIgnoreCase("left")) {
                        if (mPriorStrPositionForDraw == -1) {
                            mPriorStrPositionForDraw = mWindowStartX + mStrStartX;
                        }
                        mStrLeft = mPriorStrPositionForDraw;
                        mStrRight = mStrLeft + mStringLengthOnPaint;
                        mPriorStrPositionForDraw = mStrRight;
                    } else if (mJustify.equalsIgnoreCase("right")) {
                        if (mPriorStrPositionForDraw == -1) {
                            mPriorStrPositionForDraw = mWindowStartX + mWindowWidth;
                        }
                        mStrRight = mPriorStrPositionForDraw;
                        mStrLeft = mStrRight - mStringLengthOnPaint;
                        mPriorStrPositionForDraw = mStrLeft;
                    } else if (mJustify.equalsIgnoreCase("full")) {
                        if (mPriorStrPositionForDraw == -1) {
                            mPriorStrPositionForDraw = mWindowStartX;
                        }
                        mStrLeft = mPriorStrPositionForDraw;
                        mStrRight = mStrLeft + mCharacterGap * mStrCharactersCount;
                        mPriorStrPositionForDraw = mStrRight;
                        /*
                           Log.e(TAG, "prior " + mPriorStrPositionForDraw +
                           " mCharacterGap " + mCharacterGap +
                           " mStrCount " + str_characters_count +
                           " str_left " + str_left +
                           " str_right " + str_right);
                           */
                    } else if (mJustify.equalsIgnoreCase("center")) {
                        if (mPriorStrPositionForDraw == -1) {
                            mPriorStrPositionForDraw = (mWindowWidth - mRowLengthOnPaint)/2 + mWindowStartX;
                        }
                        mStrLeft = mPriorStrPositionForDraw;
                        mStrRight = mStrLeft + mStringLengthOnPaint;
                        mPriorStrPositionForDraw = mStrRight;
                    } else {
                        /* default using left justfication */
                        if (mPriorStrPositionForDraw == -1) {
                            mPriorStrPositionForDraw = mWindowStartX + mStrStartX;
                        }
                        mStrLeft = mPriorStrPositionForDraw;
                        mStrRight = mStrLeft + mStringLengthOnPaint;
                        mPriorStrPositionForDraw = mStrRight;
                    }


                    //Log.e(TAG, "str "+str_left + " " + str_top + " "+str_right+" "+str_bottom);
                    /* Dig a empty hole on window */
                    //                        mWindowPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_OUT));
                    //                        canvas.drawRect(str_left, str_top, str_right, str_bottom, mWindowPaint);

                    /* Draw background, a rect, if opacity == 0, skip it */
                    if (mStrBottom < mCaptionScreen.mSafeTitleTop + mCaptionScreen.mMaxFontHeight) {
                        mStrBottom = mCaptionScreen.mSafeTitleTop + mCaptionScreen.mMaxFontHeight;
                    }
                    if (mStrTop < mCaptionScreen.mSafeTitleTop) {
                        mStrTop = mCaptionScreen.mSafeTitleTop;
                    }
                    if (mBgOpacityInt != 0) {
                        mBackgroundPaint.setColor(mBgColor);
                        mBackgroundPaint.setAlpha(mBgOpacityInt);
                        if (mFileeOpcityInt != 0xff) {
                            mBackgroundPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
                        }
                        canvas.drawRect((float) mStrLeft, (float) mStrTop, (float) mStrRight, (float) mStrBottom, mBackgroundPaint);
                    }
                    if (!mJustify.equalsIgnoreCase("full")) {
                        draw_text(canvas, mData, mFontFace, mFontSize,
                                (float) mStrLeft, (float) (mStrBottom - mFontMetrics.descent),
                                mFgColor, mFgOpacity, mFgOpacityInt,
                                mUnderline,
                                mEdgeColor, (float) mEdgeWidth, mEdgeType, mTextPaint);
                    } else {
                        double priorCharacterPosition = mStrLeft;
                        for (int i=0; i < mData.length(); i++) {
                            draw_text(canvas, "" + mData.charAt(i), mFontFace, mFontSize,
                                    (float) priorCharacterPosition, (float) (mStrBottom - mFontMetrics.descent),
                                    mFgColor, mFgOpacity, mFgOpacityInt,
                                    mUnderline,
                                    mEdgeColor, (float) mEdgeWidth, mEdgeType, mTextPaint);

                            priorCharacterPosition += mCharacterGap;
                        }
                    }

                    /* Draw text */
                    /*Log.e(TAG, "Draw str, " + data +
                      " start x,y: "+(str_start_x+mWindowStartX) +
                      " " + (mRowStartY+mWindowStartY));
                      */
                }

                void draw_str(Canvas canvas, String str, float left, float bottom, Paint paint)
                {
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
                    //if (mCcVersion.matches("cea708")) {
                        canvas.drawText(str, left, bottom, paint);
                    //} else {
                    //    int i, l = str.length();


                     //       String sub = str.substring(i, i + 1);
                     //       canvas.drawText(sub, x, bottom, paint);
                     //       x += caption_screen.mFixedCharWidth;
                     //   }
                    //}
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.ADD));
                    //if (mCcVersion.matches("cea708")) {
                        canvas.drawText(str, left, bottom, paint);
                    //} else {
                    //    int i, l = str.length();


                     //       String sub = str.substring(i, i + 1);
                     //       canvas.drawText(sub, x, bottom, paint);
                     //       x += caption_screen.mFixedCharWidth;
                     //   }
                    //}
                }

                void draw_text(Canvas canvas, String data,
                               Typeface face, double fontSize,
                               float left, float bottom, int fgColor, String opacity, int opacityInt,
                               boolean underline,
                               int edgeColor, float edgeWidth, String edgeType, Paint paint) {

                    //Log.e(TAG, "draw_text "+data + " fg_color: "+ fg_color +" opa:"+ opacity + mEdgeType + "edge color: "+ mEdgeColor);
                    paint.reset();
                    if (mStyleUseBroadcast) {
                        if (opacity.equalsIgnoreCase("flash")) {
                            /*if (mHeartBeat == 0)
                                return;
                            else*/
                                opacityInt = 0xff;
                        }
                    }
                    paint.setSubpixelText(true);
                    paint.setTypeface(face);
                    if (opacityInt != 0xff) {
                        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));
                    }
                    paint.setAntiAlias(true);
                    paint.setTextSize((float)fontSize);

                    if (mEdgeType == null || mEdgeType.equalsIgnoreCase("none")) {
                        paint.setColor(fgColor);
                        paint.setAlpha(opacityInt);
                        draw_str(canvas, data, left, bottom, paint);
                    } else if (mEdgeType.equalsIgnoreCase("uniform")) {
                        //paint.setStrokeJoin(Paint.Join.ROUND);
                        paint.setStrokeWidth((float)(edgeWidth*1.5));
                        paint.setColor(edgeColor);
                        paint.setStyle(Paint.Style.STROKE);
                        draw_str(canvas, data, left, bottom, paint);
                        paint.setColor(fgColor);
                        paint.setAlpha(opacityInt);
                        paint.setStyle(Paint.Style.FILL);
                        draw_str(canvas, data, left, bottom, paint);
                    } else if (mEdgeType.equalsIgnoreCase("shadow_right")) {
                        paint.setShadowLayer((float)edgeWidth, (float) edgeWidth, (float) edgeWidth, edgeColor);
                        paint.setColor(fgColor);
                        paint.setAlpha(opacityInt);
                        draw_str(canvas, data, left, bottom, paint);
                    } else if (mEdgeType.equalsIgnoreCase("shadow_left")) {
                        paint.setShadowLayer((float)edgeWidth, (float) -edgeWidth, (float) -edgeWidth, edgeColor);
                        paint.setColor(fgColor);
                        paint.setAlpha(opacityInt);
                        draw_str(canvas, data, left, bottom, paint);
                    } else if (edgeType.equalsIgnoreCase("raised") ||
                            edgeType.equalsIgnoreCase("depressed")) {
                        boolean raised;
                        if (edgeType.equalsIgnoreCase("depressed")) {
                            raised = false;
                        } else {
                            raised = true;
                        }
                        int colorUp = raised ? fgColor : mEdgeColor;
                        int colorDown = raised ? edgeColor : fgColor;
                        float offset = (float)edgeWidth;
                        paint.setColor(fgColor);
                        paint.setStyle(Paint.Style.FILL);
                        paint.setShadowLayer(edgeWidth, -offset, -offset, colorUp);
                        draw_str(canvas, data, left, bottom, paint);
                        paint.setShadowLayer(edgeWidth, offset, offset, colorDown);
                        draw_str(canvas, data, left, bottom, paint);
                    } else if (edgeType.equalsIgnoreCase("none")) {
                        paint.setColor(fgColor);
                        paint.setAlpha(opacityInt);
                        draw_str(canvas, data, left, bottom, paint);
                    }
                    if (underline) {
                        paint.setStrokeWidth((float)(2.0));
                        paint.setColor(fgColor);
                        canvas.drawLine(left, (float)(bottom + edgeWidth * 2),
                                (float) (left + mStringLengthOnPaint),
                                (float)(bottom + edgeWidth * 2), paint);
                    }
                }
            }
        }
    }

    void draw(Canvas canvas)
    {
        /* Windows come in rising queue,
         * so we need to revert the draw sequence */
        if (!mInitFlag) {
            Log.e(TAG, "Init failed, skip draw");
            return;
        }
        Log.d(TAG, "[draw]-mWindowsCount:"+mWindowsCount);
        for (int i = mWindowsCount - 1; i >= 0; i--)
            mWindows[i].draw(canvas);
    }
  }
}
