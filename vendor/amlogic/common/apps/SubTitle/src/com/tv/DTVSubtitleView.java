/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.tv;

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

import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.File;

import android.util.Log;
import android.view.accessibility.CaptioningManager;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import com.tv.CcImplement;

//import com.droidlogic.app.ISubTitleServiceCallback;

/**
 * DTVSubtitleView support A/D television subtitle and teletext
 * At current , support DVB subtitle, DTV/ATV teletext, ATSC/NTSC closed caption.
 */
public class DTVSubtitleView extends View {
    private static final String TAG = "DTVSubtitleView";

    private int decoderTimeS= 5;        //closecaption decoder timeout  default 5s.

    //DVB subtitle and scte27 decoder timeout report
    private int decoderTimeOut= 60;          //default 60s
    private long decoderTimeOutStart= 0;
    private long dTimeUpdate= 0;
    private boolean hasDecoderErrorEvent= false;
    private boolean isSubUpdate= false;


    private static Object lock = new Object();
    private static final int BUFFER_W = 1920;
    private static final int BUFFER_H = 1080;

    private static final int MODE_NONE = 0;
    private static final int MODE_DTV_TT = 1;
    private static final int MODE_DTV_CC = 2;
    private static final int MODE_DVB_SUB = 3;
    private static final int MODE_ATV_TT = 4;
    private static final int MODE_ATV_CC = 5;
    private static final int MODE_AV_CC = 6;
    private static final int MODE_SCTE27_SUB = 8;

    private static final int EDGE_NONE = 0;
    private static final int EDGE_OUTLINE = 1;
    private static final int EDGE_DROP_SHADOW = 2;
    private static final int EDGE_RAISED = 3;
    private static final int EDGE_DEPRESSED = 4;

    private static final int PLAY_NONE = 0;
    private static final int PLAY_SUB = 1;
    private static final int PLAY_TT  = 2;

    public static final int COLOR_RED = 0;
    public static final int COLOR_GREEN = 1;
    public static final int COLOR_YELLOW = 2;
    public static final int COLOR_BLUE = 3;

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
    static final int VFMT_ATV = 100;
    private static Bitmap bitmap = null;
    private static Paint mPaint;
    private Paint clear_paint;

    private String jsonStr = null;
    private String mLanguage = null;
    private int dataCount = 0;
    private int index = 0;
    private boolean mDebug = false;
    private int MAX_NO_DATA_COUNT = 8;
    private boolean hasFirstSubtitle = false;
    private boolean hasReportAvail = false;

    //report decoder error info
    public static final int ERROR_DECODER_TIMEOUT = 2;
    public static final int ERROR_DECODER_LOSEDATA = 3;
    public static final int ERROR_DECODER_INVALIDDATA = 4;
    public static final int ERROR_DECODER_TIMEERROR = 5;
    public static final int ERROR_DECODER_END = 6;

    public static final int JSON_MSG_NORMAL = 0;
    public List<Integer> backList = new ArrayList<Integer>();
    //interger: cc id, Long:start decoder time
    public Map<Integer, Long> ccAddIdMap = new HashMap<Integer, Long>();
    public long startDecoderTime = 0;
    public long endDecoderTime = 0;
    public int decoderTime = 0;

    //subtitleservice callback
    private static SubtitleDataListener mCallback = null;

    private static int init_count = 0;
    private static CaptioningManager captioningManager = null;
    private static CcImplement.CaptionWindow cw = null;
    private static CustomFonts cf = null;
    private static CcImplement ci = null;
    private static String json_str;

    public static boolean cc_is_started = false;

    private boolean isPreWindowMode = false;

    private native int native_sub_init();
    private native int native_sub_destroy();
    private native int native_sub_lock();
    private native int native_sub_unlock();
    private native int native_sub_clear();
    private native int native_sub_start_dvb_sub(int dmx_id, int pid, int page_id, int anc_page_id);
    private native int native_sub_start_dtv_tt(int dmx_id, int region_id, int pid, int page, int sub_page, boolean is_sub);
    private native int native_sub_stop_dvb_sub();
    private native int native_sub_stop_dtv_tt();
    private native int native_sub_tt_goto(int page);
    private native int native_sub_tt_color_link(int color);
    private native int native_sub_tt_home_link();
    private native int native_sub_tt_next(int dir);
    private native int native_sub_tt_set_search_pattern(String pattern, boolean casefold);
    private native int native_sub_tt_search_next(int dir);
    protected native int native_get_subtitle_picture_width();
    protected native int native_get_subtitle_picture_height();
    private native int native_sub_start_atsc_cc(int vfmt, int caption, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_sub_start_atsc_atvcc(int caption, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_sub_stop_atsc_cc();
    private native static int native_sub_set_atsc_cc_options(int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_sub_set_active(boolean active);
    private native int native_sub_start_scte27(int dmx_id, int pid);
    private native int native_sub_stop_scte27();

    static {
        //System.loadLibrary("am_adp");
        //System.loadLibrary("am_mw");
        //System.loadLibrary("zvbi");
        bitmap = Bitmap.createBitmap(BUFFER_W, BUFFER_H, Bitmap.Config.ARGB_8888);
        System.loadLibrary("tvsubtitle_tv");
    }

    /**
     * DVB subtitle param
     */
    static public class DVBSubParams {
        private int dmx_id;
        private int pid;
        private int composition_page_id;
        private int ancillary_page_id;

        /**
         * creat DVB subtitle param
         * @param dmx_id revice use demux device's ID
         * @param pid subtitle stream PID
         * @param page_id subtitle page_id
         * @param anc_page_id subtitle ancillary_page_id
         */
        public DVBSubParams(int dmx_id, int pid, int page_id, int anc_page_id) {
            this.dmx_id              = dmx_id;
            this.pid                 = pid;
            this.composition_page_id = page_id;
            this.ancillary_page_id   = anc_page_id;
        }
    }

    /**
     * Digital television teletext param
     */
    static public class DTVTTParams {
        private int dmx_id;
        private int pid;
        private int page_no;
        private int sub_page_no;
        private int region_id;

        /**
         * creat Digital television teletext param
         * @param dmx_id recive user demux device's ID
         * @param pid teletext PID
         * @param page_no page_id
         * @param sub_page_no
         */
        public DTVTTParams(int dmx_id, int pid, int page_no, int sub_page_no, int region_id) {
            this.dmx_id      = dmx_id;
            this.pid         = pid;
            this.page_no     = page_no;
            this.sub_page_no = sub_page_no;
            this.region_id   = region_id;
        }
    }

    static public class Scte27Params {
        private int dmx_id;
        private int pid;

        public Scte27Params(int dmx_id, int pid) {
            this.dmx_id      = dmx_id;
            this.pid         = pid;
        }
    }

    static public class ATVTTParams {
    }

    static public class DTVCCParams {
        protected int vfmt;
        protected int caption_mode;
        protected int fg_color;
        protected int fg_opacity;
        protected int bg_color;
        protected int bg_opacity;
        protected int font_style;
        protected float font_size;

        public DTVCCParams(int vfmt, int caption, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            this.vfmt = vfmt;
            this.caption_mode = caption;
            this.fg_color = fg_color;
            this.fg_opacity = fg_opacity;
            this.bg_color = bg_color;
            this.bg_opacity = bg_opacity;
            this.font_style = font_style;
            this.font_size = font_size;
        }
    }

    static public class ATVCCParams extends DTVCCParams {
        public ATVCCParams(int caption, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            super(VFMT_ATV, caption, fg_color, fg_opacity,
                    bg_color, bg_opacity, font_style, font_size);
        }
    }
    static public class AVCCParams extends DTVCCParams {
        public AVCCParams(int caption, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            super(VFMT_ATV, caption, fg_color, fg_opacity,
                    bg_color, bg_opacity, font_style, font_size);
        }
    }
    private class SubParams {
        int mode;
        int vfmt;
        DVBSubParams dvb_sub;
        DTVTTParams  dtv_tt;
        ATVTTParams  atv_tt;
        DTVCCParams  dtv_cc;
        ATVCCParams  atv_cc;
        AVCCParams  av_cc;
        Scte27Params scte27_sub;

        private SubParams() {
            mode = MODE_NONE;
        }
    }

    private class TTParams {
        int mode;
        DTVTTParams dtv_tt;
        ATVTTParams atv_tt;

        private TTParams() {
            mode = MODE_NONE;
        }
    }

    private int disp_left = 0;
    private int disp_right = 0;
    private int disp_top = 0;
    private int disp_bottom = 0;
    private boolean active = true;

    //add for skyworth for dect cc
    private static SubParams cc_sub_params;

    private static SubParams sub_params;
    private static TTParams  tt_params;
    private static int       play_mode;
    private boolean   visible;
    private boolean   destroy;
    private static DTVSubtitleView activeView = null;
    private void update() {
        Log.e(TAG, "update");
         //isSubUpdate = true;
    // if (!isPreWindowMode)
         postInvalidate();
    }

    private void stopDecoder() {
        synchronized(lock) {
            if (!cc_is_started)
                return;

            switch (play_mode) {
                case PLAY_NONE:
                    break;
                case PLAY_TT:
                    switch (tt_params.mode) {
                        case MODE_DTV_TT:
                            native_sub_stop_dtv_tt();
                            break;
                        default:
                            break;
                    }
                    break;
                case PLAY_SUB:
                    switch (sub_params.mode) {
                        case MODE_DTV_TT:
                            native_sub_stop_dtv_tt();
                            break;
                        case MODE_DVB_SUB:
                            native_sub_stop_dvb_sub();
                            break;
                        //case MODE_DTV_CC:
                        case MODE_ATV_CC:
                        case MODE_AV_CC:
                            native_sub_stop_atsc_cc();
                            break;
                        case MODE_SCTE27_SUB:
                            native_sub_stop_scte27();
                            break;
                        default:
                            break;
                    }
                    break;
            }
            cc_is_started = false;
            play_mode = PLAY_NONE;
            backList.clear();
            ccAddIdMap.clear();
        }
    }

    private void stopDecoderAll() {
        synchronized(lock) {
            if (!cc_is_started)
                return;

            switch (play_mode) {
                case PLAY_NONE:
                    break;
                case PLAY_TT:
                    switch (tt_params.mode) {
                        case MODE_DTV_TT:
                            native_sub_stop_dtv_tt();
                            break;
                        default:
                            break;
                    }
                    break;
                case PLAY_SUB:
                    switch (sub_params.mode) {
                        case MODE_DTV_TT:
                            native_sub_stop_dtv_tt();
                            break;
                        case MODE_DVB_SUB:
                            native_sub_stop_dvb_sub();
                            break;
                        case MODE_DTV_CC:
                        case MODE_ATV_CC:
                        case MODE_AV_CC:
                            native_sub_stop_atsc_cc();
                            break;
                        case MODE_SCTE27_SUB:
                            native_sub_stop_scte27();
                            break;
                        default:
                            break;
                    }
                    switch (cc_sub_params.mode) {
                        case MODE_DTV_CC:
                            native_sub_stop_atsc_cc();
                            break;
                        default:
                            break;
                    }
                    break;
            }
            cc_is_started = false;
            play_mode = PLAY_NONE;
            backList.clear();
            ccAddIdMap.clear();
        }
    }

    private void init() {
        synchronized(lock) {
            if (init_count == 0) {
                play_mode  = PLAY_NONE;
                visible    = true;
                destroy    = false;
                tt_params  = new TTParams();
                sub_params = new SubParams();
                cc_sub_params = new SubParams();
                backList.clear();
                ccAddIdMap.clear();
                index = 0;
                checkDebug();

                if (native_sub_init() < 0) {
                }

                setLayerType(View.LAYER_TYPE_SOFTWARE, null);
                mPaint = new Paint();
                cf = new CustomFonts(getContext());
                if (ci == null) {
                    ci = new CcImplement(getContext(), cf);
                }
                captioningManager = (CaptioningManager) getContext().getSystemService(Context.CAPTIONING_SERVICE);
                captioningManager.addCaptioningChangeListener(new CaptioningManager.CaptioningChangeListener() {
                    @Override
                    public void onEnabledChanged(boolean enabled) {
                        super.onEnabledChanged(enabled);
                        Log.e(TAG, "onenableChange");
                        ci.cc_setting.is_enabled = captioningManager.isEnabled();
                    }

                    @Override
                    public void onFontScaleChanged(float fontScale) {
                        super.onFontScaleChanged(fontScale);
                        Log.e(TAG, "onfontscaleChange");
                        ci.cc_setting.font_scale = captioningManager.getFontScale();
                    }

                    @Override
                    public void onLocaleChanged(Locale locale) {
                        super.onLocaleChanged(locale);
                        Log.e(TAG, "onlocaleChange");
                        ci.cc_setting.cc_locale = captioningManager.getLocale();
                    }

                    @Override
                    public void onUserStyleChanged(CaptioningManager.CaptionStyle userStyle) {
                        super.onUserStyleChanged(userStyle);
                        Log.e(TAG, "onUserStyleChange");
                        ci.cc_setting.has_foreground_color = userStyle.hasForegroundColor();
                        ci.cc_setting.has_background_color = userStyle.hasBackgroundColor();
                        ci.cc_setting.has_window_color = userStyle.hasWindowColor();
                        ci.cc_setting.has_edge_color = userStyle.hasEdgeColor();
                        ci.cc_setting.has_edge_type = userStyle.hasEdgeType();
                        ci.cc_setting.edge_type = userStyle.edgeType;
                        ci.cc_setting.edge_color = userStyle.edgeColor;
                        ci.cc_setting.foreground_color = userStyle.foregroundColor;
                        ci.cc_setting.foreground_opacity = userStyle.foregroundColor >>> 24;
                        ci.cc_setting.background_color = userStyle.backgroundColor;
                        ci.cc_setting.background_opacity = userStyle.backgroundColor >>> 24;
                        ci.cc_setting.window_color = userStyle.windowColor;
                        ci.cc_setting.window_opacity = userStyle.windowColor >>> 24;
                        /* Typeface is obsolete, we use local font */
                        ci.cc_setting.type_face = userStyle.getTypeface();
                    }
                });
                ci.cc_setting.UpdateCcSetting(captioningManager);
                Log.e(TAG, "subtitle view init");

            }
            init_count = 1;
            hasFirstSubtitle = false;
            hasReportAvail = false;
            dataCount = 0;
            clear_paint = new Paint();
            clear_paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        }
    }

    /**
     * creat TVSubtitle
     */
    public DTVSubtitleView(Context context) {
        super(context);
        init();
    }

    /**
     * creat TVSubtitle
     */
    public DTVSubtitleView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    /**
     * creat TVSubtitle
     */
    public DTVSubtitleView(Context context, AttributeSet attrs, int defStyle) {
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
        disp_left   = left;
        disp_top    = top;
        disp_right  = right;
        disp_bottom = bottom;
    }

    /**
     * set active
     * @param active y/n
     */
    public void setActive(boolean active) {
        Log.d(TAG, "[setActive]active:"+active);
        synchronized (lock) {
            if (active && (activeView != this) && (activeView != null)) {
                activeView.stopDecoder();
                activeView.active = false;
            }
            native_sub_set_active(active);
            this.active = active;
            if (active) {
                activeView = this;
                /*}else if (activeView == this){
                  activeView = null;*/
            }
            if (!isPreWindowMode) {
                isSubUpdate = false;
                postInvalidate();
            }

        }
    }

    /**
     * set subtitle param
     * @param params subtitle param
     */
    public void setSubParams(DVBSubParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DVB_SUB;
            sub_params.dvb_sub = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    /**
     * set subtitle param
     * @param params subtitle param
     */
    public void setSubParams(DTVTTParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DTV_TT;
            sub_params.dtv_tt = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    /**
     * set close caption param
     * @param params
     */
    public void setSubParams(DTVCCParams params) {
        synchronized(lock) {
            cc_sub_params.mode = MODE_DTV_CC;
            cc_sub_params.dtv_cc = params;
            cc_sub_params.vfmt = params.vfmt;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    /**
     * set close caption param
     * @param params
     */
    public void setSubParams(ATVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ATV_CC;
            sub_params.atv_cc = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(Scte27Params params) {
        synchronized(lock) {
            sub_params.mode = MODE_SCTE27_SUB;
            sub_params.scte27_sub = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(AVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_AV_CC;
            sub_params.av_cc = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }
    /**
     * set Teletext param
     * @param params
     */
    public void setTTParams(DTVTTParams params) {
        synchronized(lock) {
            tt_params.mode = MODE_DTV_TT;
            tt_params.dtv_tt = params;

            if (play_mode == PLAY_TT)
                startTT();
        }
    }

    public void setCaptionParams(DTVCCParams params) {
        synchronized(lock) {
            sub_params.dtv_cc = params;
            native_sub_set_atsc_cc_options(
                    params.fg_color,
                    params.fg_opacity,
                    params.bg_color,
                    params.bg_opacity,
                    params.font_style,
                    new Float(params.font_size).intValue());
        }
    }

    /**
     * show subtitle/teletext
     */
    public void show() {
        if (visible)
            return;

        Log.d(TAG, "show");

        visible = true;
        update();
    }

    /**
     * hide subtitle/teletext
     */
    public void hide() {
        if (!visible)
            return;

        Log.d(TAG, "hide");

        visible = false;
        update();
    }

    /**
     * start parset teletext
     */
    public void startTT() {
        synchronized(lock) {
            if (activeView != this)
                return;

            stopDecoder();

            if (tt_params.mode == MODE_NONE)
                return;

            int ret = 0;
            switch (tt_params.mode) {
                case MODE_DTV_TT:
                    ret = native_sub_start_dtv_tt(tt_params.dtv_tt.dmx_id,
                            tt_params.dtv_tt.region_id,
                            tt_params.dtv_tt.pid,
                            tt_params.dtv_tt.page_no,
                            tt_params.dtv_tt.sub_page_no,
                            false);
                    break;
                default:
                    break;
            }

            if (ret >= 0)
                play_mode = PLAY_TT;
        }
    }

    public void switchCloseCaption(int vfmt, int channelId) {
            Log.d(TAG, "[switchCloseCaption]vfmt:" + vfmt + ",channelId:" + channelId);
            int cc_default_ret = 0;
            //add for skyworth for switch cc.
            cc_default_ret = native_sub_start_atsc_cc(
                            cc_sub_params.vfmt,
                            cc_sub_params.dtv_cc.caption_mode,
                            cc_sub_params.dtv_cc.fg_color,
                            cc_sub_params.dtv_cc.fg_opacity,
                            cc_sub_params.dtv_cc.bg_color,
                            cc_sub_params.dtv_cc.bg_opacity,
                            cc_sub_params.dtv_cc.font_style,
                            new Float(cc_sub_params.dtv_cc.font_size).intValue());
            if (cc_default_ret >= 0) {
                cc_is_started = true;
                play_mode = PLAY_SUB;
            }
    }

    /**
     * start sub parser
     */
    public void startSub() {
        synchronized(lock) {
            if (activeView != this)
                return;

            stopDecoder();

            if (sub_params.mode == MODE_NONE && cc_sub_params.mode == MODE_NONE)
                return;

            int cc_default_ret = 0;
            //add for skyworth for dect cc.
            cc_default_ret = native_sub_start_atsc_cc(
                            cc_sub_params.vfmt,
                            cc_sub_params.dtv_cc.caption_mode,
                            cc_sub_params.dtv_cc.fg_color,
                            cc_sub_params.dtv_cc.fg_opacity,
                            cc_sub_params.dtv_cc.bg_color,
                            cc_sub_params.dtv_cc.bg_opacity,
                            cc_sub_params.dtv_cc.font_style,
                            new Float(cc_sub_params.dtv_cc.font_size).intValue());
            int ret = 0;
            switch (sub_params.mode) {
                case MODE_DVB_SUB:
                    ret = native_sub_start_dvb_sub(sub_params.dvb_sub.dmx_id,
                            sub_params.dvb_sub.pid,
                            sub_params.dvb_sub.composition_page_id,
                            sub_params.dvb_sub.ancillary_page_id);
                    decoderTimeOutStart = System.currentTimeMillis();
                    SubtitleDecoderCountDown();
                    break;
                case MODE_DTV_TT:
                    ret = native_sub_start_dtv_tt(sub_params.dtv_tt.dmx_id,
                            sub_params.dtv_tt.region_id,
                            sub_params.dtv_tt.pid,
                            sub_params.dtv_tt.page_no,
                            sub_params.dtv_tt.sub_page_no,
                            true);
                    break;
                case MODE_DTV_CC:
                    ret = native_sub_start_atsc_cc(
                            sub_params.vfmt,
                            sub_params.dtv_cc.caption_mode,
                            sub_params.dtv_cc.fg_color,
                            sub_params.dtv_cc.fg_opacity,
                            sub_params.dtv_cc.bg_color,
                            sub_params.dtv_cc.bg_opacity,
                            sub_params.dtv_cc.font_style,
                            new Float(sub_params.dtv_cc.font_size).intValue());
                    break;
                case MODE_ATV_CC:
                    ret = native_sub_start_atsc_atvcc(
                            sub_params.atv_cc.caption_mode,
                            sub_params.atv_cc.fg_color,
                            sub_params.atv_cc.fg_opacity,
                            sub_params.atv_cc.bg_color,
                            sub_params.atv_cc.bg_opacity,
                            sub_params.atv_cc.font_style,
                            new Float(sub_params.atv_cc.font_size).intValue());
                    break;
                case MODE_AV_CC:
                    ret = native_sub_start_atsc_atvcc(
                            sub_params.av_cc.caption_mode,
                            sub_params.av_cc.fg_color,
                            sub_params.av_cc.fg_opacity,
                            sub_params.av_cc.bg_color,
                            sub_params.av_cc.bg_opacity,
                            sub_params.av_cc.font_style,
                            new Float(sub_params.av_cc.font_size).intValue());
                    break;
                case MODE_SCTE27_SUB:
                    native_sub_start_scte27(sub_params.scte27_sub.dmx_id, sub_params.scte27_sub.pid);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    decoderTimeOutStart = System.currentTimeMillis();
                    SubtitleDecoderCountDown();
                    break;
                default:
                    ret = -1;
                    break;
            }

            if (ret >= 0 || cc_default_ret >= 0) {
                cc_is_started = true;
                play_mode = PLAY_SUB;
            }
        }
    }

    /**
     * stop subtitle/teletext parser
     */
    public void stop() {
        synchronized(lock) {
            if (activeView != this)
                return;
            stopDecoder();
            mLanguage = null;
            //hasCcData = false;
            hasFirstSubtitle = false;
            hasReportAvail = false;
            dataCount = 0;
            stopSubDecoderTimeout();
            backList.clear();
            ccAddIdMap.clear();
            sub_params.mode = MODE_NONE;
        }
    }
    public void reportAvail (int v) {
        if (mCallback != null) {
            if (v == 1) {
                if (!hasReportAvail) {
                    hasReportAvail = true;
                    mCallback.onSubTitleAvailable(v);
                }
            } else {
                hasReportAvail = false;
                mCallback.onSubTitleAvailable(v);
            }
        }
    }

    //cc event 1:add 0:remove
    public void reportEvent (int event, int id) {
        if (mCallback != null) {
            mCallback.onSubTitleEvent(event, id);
        }
    }

    public void stopAll() {
        synchronized(lock) {
            if (activeView != this)
                return;
            stopDecoderAll();
            mLanguage = null;
            //hasCcData = false;
            cc_sub_params.mode = MODE_NONE;
            sub_params.mode = MODE_NONE;
            backList.clear();
            ccAddIdMap.clear();
            stopSubDecoderTimeout();
        }
    }

    /**
     * stop subtitle/teletext parser & clear cache
     */
    public void clear() {
        synchronized(lock) {
            if (activeView != this)
                return;
            stopDecoder();
            native_sub_clear();
            tt_params.mode  = MODE_NONE;
            sub_params.mode = MODE_NONE;
        }
    }

    /**
     * next page at TT
     */
    public void nextPage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_next(1);
        }
    }

    /**
     * previous page at TT
     */
    public void previousPage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_next(-1);
        }
    }

    /**
     * GOTO page at TT
     * @param page goto page num
     */
    public void gotoPage(int page) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_goto(page);
        }
    }

    /**
     * goto home at TT
     */
    public void goHome() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_home_link();
        }
    }

    /**
     * goto color link
     * @param color ，COLOR_RED/COLOR_GREEN/COLOR_YELLOW/COLOR_BLUE
     */
    public void colorLink(int color) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_color_link(color);
        }
    }

    /**
     * set search string at TT
     * @param pattern search string
     * @param casefold
     */
    public void setSearchPattern(String pattern, boolean casefold) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_set_search_pattern(pattern, casefold);
        }
    }

    /**
     * search next page
     */
    public void searchNext() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_search_next(1);
        }
    }

    /**
     * search previous page
     */
    public void searchPrevious() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_search_next(-1);
        }
    }


    /**
     * set the flag to indecate the preview window mode
     * @param flag [description]
     */
    public void setPreviewWindowMode(boolean flag) {
        isPreWindowMode = flag;
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        ci.caption_screen.updateCaptionScreen(w, h);
    }

    @Override
    public void onDraw(Canvas canvas) {
        Log.d(TAG, "[onDraw]");
        Rect sr, dr;
        if (!active || !visible || (play_mode == PLAY_NONE)) {
            /* Clear canvas */
            if (ci == null)
                return;
            canvas.drawPaint(clear_paint);
            cw = null;
            return;
        }
        //if (!ci.cc_setting.is_enabled)
            //return;
        Log.i(TAG, "closecaption mode:" + cc_sub_params.dtv_cc.caption_mode);
        if (mDebug) {
            saveBitmapToFile(bitmap, index);
            index++;
        }

        if (cc_sub_params.dtv_cc.caption_mode == 15 || cc_sub_params.dtv_cc.caption_mode == -1) { //15: default detect, -1: invalid
            if (sub_params.mode == MODE_SCTE27_SUB && native_get_subtitle_picture_height() != 0) {
                int native_w = native_get_subtitle_picture_width();
                int native_h = native_get_subtitle_picture_height();
                int edge_l = 0;//(getWidth() - native_w * getHeight() / native_h) / 2;
                sr = new Rect(0, 0, native_w, native_h);
                dr = new Rect(0/*edge_l*/, 0, getWidth(), getHeight());
                Log.d(TAG, "[onDraw]native_w:" + native_w + ",native_h:" + native_h + "edge_l:" +edge_l);
                native_sub_lock();
                canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                canvas.drawBitmap(bitmap, sr, dr, mPaint);
                native_sub_unlock();
                return;
           }

            if (sub_params.mode == MODE_DVB_SUB) {
                Log.e(TAG, "ondraw MODE_DVB_SUB");
                if (play_mode == PLAY_SUB) {
                    sr = new Rect(0, 0, native_get_subtitle_picture_width(), native_get_subtitle_picture_height());
                    dr = new Rect(disp_left, disp_top, getWidth() - disp_right, getHeight() - 0/*disp_bottom*/);
                } else {
                    sr = new Rect(0, 0, BUFFER_W, BUFFER_H);
                    dr = new Rect(disp_left, disp_top, getWidth() - disp_right, getHeight() - 0/*disp_bottom*/);
                }
                native_sub_lock();
                canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                canvas.drawBitmap(bitmap, sr, dr, mPaint);
                native_sub_unlock();
                return;
            }
        }


        if (play_mode == PLAY_TT || sub_params.mode == MODE_DTV_TT || sub_params.mode == MODE_ATV_TT) {
            sr = new Rect(0, 0, 12 * 41, 10 * 25);
        } else if (play_mode == PLAY_SUB) {
            sr = new Rect(0, 0, native_get_subtitle_picture_width(), native_get_subtitle_picture_height());
        } else {
            sr = new Rect(0, 0, BUFFER_W, BUFFER_H);
        }
        if (cw != null) {
            cw.draw(canvas);
            cw = null;
             //add callback 1:sub display 0:no sub
            /*Log.d(TAG, "[onDraw]dataCount:" + dataCount + ",hasFirstSubtitle:" + hasFirstSubtitle);
            if (hasFirstSubtitle)
                return;
            dataCount++;
            if (hasSubtitle(jsonStr)) {
                    Log.d(TAG, "[onDraw]cc onSubTitleAvailable 1");
                    reportAvail(1);
                    hasFirstSubtitle= true;
                    return;
            }

            if (dataCount == MAX_NO_DATA_COUNT) {
                    Log.d(TAG, "[onDraw]cc onSubTitleAvailable 0");
                    reportAvail(0);
            }*/
        }
    }

    private boolean saveBitmapToFile(Bitmap mBitmap, int num) {
        File mFile = new File("/data/bitmap/" + num + ".png");
        try {
            FileOutputStream fileOutputStream = new FileOutputStream(mFile);
            if (mBitmap != null) {
                mBitmap.compress(Bitmap.CompressFormat.PNG, 100, fileOutputStream);
            }
        } catch (FileNotFoundException ex) {
            ex.printStackTrace();
            return false;
        }
        return true;
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

    private static final int MSG_COUNT_DOWN = 0xE1;//random value
    private Timer decoderTimer = new Timer();

    protected void SubtitleDecoderCountDown() {
        Log.e(TAG, "decoder-time count down begin....");
        final Handler decoderhandler = new Handler(Looper.getMainLooper()) {
            public void handleMessage (Message msg) {
                switch (msg.what) {
                    case MSG_COUNT_DOWN:
                        if (mCallback != null) {
                                Log.e(TAG, "decoder-time count down end.....data loss!");
                                reportAvail(ERROR_DECODER_TIMEOUT);  //event: 2  decoder time out
                                hasDecoderErrorEvent = true;
                                decoderTimeOutStart = System.currentTimeMillis();
                        }
                        break;
                }
                super.handleMessage (msg);
            }
        };
        TimerTask task = new TimerTask() {
            public void run() {
                Message message = Message.obtain();
                message.what = MSG_COUNT_DOWN;
                decoderhandler.sendMessage (message);
            }
        };
        stopSubDecoderTimeout();
        if (decoderTimer == null) {
            decoderTimer = new Timer();
        }
        if (decoderTimer != null) {
            decoderTimer.schedule (task, getSubDecoderTimeOut());
        }
    }

    private void stopSubDecoderTimeout() {
        if (decoderTimer != null) {
            decoderTimer.cancel();
        }
        decoderTimer = null;
    }

    private void stopCcNoSigTimeout() {
        if (decoderTimer != null) {
            Log.e(TAG, "ccdecoderTimer count down cancel....");
            decoderTimer.cancel();
        }
        decoderTimer = null;
    }

    private boolean IsCloseCaptionShowEnable() {
        return SystemProperties.getBoolean("vendor.sys.subtitleService.closecaption.enable", false);
    }
    public void dispose() {
        synchronized(lock) {
            if (!destroy) {
                destroy = true;
                if (init_count == 0) {
                    stopDecoder();
                    native_sub_clear();
                    native_sub_destroy();
                }
            }
        }
    }

    protected void finalize() throws Throwable {
        // Resource may not be available during gc process
        // dispose();
        super.finalize();
    }

    public int getSubHeight() {
        Log.d(TAG, "[getSubHeight]height:" + native_get_subtitle_picture_height());
        return native_get_subtitle_picture_height();
    }

    public int getSubWidth() {
        Log.d(TAG, "[getSubWidth]width:" + native_get_subtitle_picture_width());
        return native_get_subtitle_picture_width();
    }


    public void setVisible(boolean value) {
        Log.d(TAG, "force set visible to:" + value);
        visible = value;
        isSubUpdate = false;
        postInvalidate();
    }

    private SubtitleDataListener mSubtitleDataListener = null;
    Handler handler = new Handler()
    {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case JSON_MSG_NORMAL:
                    cw = (CcImplement.CaptionWindow)msg.obj;
                    isSubUpdate = false;
                    postInvalidate();
                    break;
            }
        }
    };

    public boolean hasSubtitle(String str) {
        if (str != null) {
            return str.contains("data");
        }
        return false;
    }

    public void saveJsonStr(String str) {
        Log.d(TAG, "[saveJsonStr]str:" + str);
        if (activeView != this)
            return;
        Log.d(TAG, "[saveJsonStr]-1-");
        if (!TextUtils.isEmpty(str)) {
            jsonStr = str;
            handler.obtainMessage(JSON_MSG_NORMAL, ci.new CaptionWindow(str)).sendToTarget();
        }
    }

    public static int getObjectValueInt(String jsonString, String objName, String valueName, int defaultValue) {
        if (jsonString == null || jsonString.isEmpty()
            || objName == null || objName.isEmpty()
            || valueName == null || valueName.isEmpty())
            return defaultValue;

        /*
            objName1:{valueName1:0,valueName2:0,valueName3:1}
        */
        JSONObject obj = null;
        try {
            obj = new JSONObject(jsonString);
        } catch (JSONException e) {
            throw new RuntimeException("Json parse fail: ("+jsonString+")", e);
        }
        JSONObject nObj = null;
        try {
            nObj = obj.getJSONObject(objName);
        } catch (JSONException e) {
            return defaultValue;
        }

        Log.d(TAG, objName+":"+nObj.toString());

        return nObj.optInt(valueName, defaultValue);
    }

    public String int2Binary(int m) {
        String s = "";
        while (m > 0) {
            s = s + m % 2;
            m = m / 2;
        }
        Log.d(TAG, "[int2Binary]s:" + s);
        return s;
    }

    private int getDecoderTime() {
        decoderTimeS = SystemProperties.getInt("vendor.sys.subtitleService.decodeTime", 5);
        return decoderTimeS * 1000;
    }

    private int getSubDecoderTimeOut() {
        decoderTimeOut = SystemProperties.getInt("vendor.sys.subtitleService.decodeTimeOut", 60);
        //Log.d(TAG, "[getSubDecoderTimeOut]decoder timeout:" + decoderTimeOut * 1000);
        return decoderTimeOut * 1000;
    }

    public void reportAvailable() {
        //Log.d(TAG, "[reportAvailable]");
        if (decoderTimer != null) {
             Log.e(TAG, "decoder-time count down cancel....");
             decoderTimer.cancel();
             decoderTimer.purge();
        }
        decoderTimer = null;
         dTimeUpdate = System.currentTimeMillis();
         if ((int)(dTimeUpdate - decoderTimeOutStart) < getSubDecoderTimeOut()) {
             decoderTimeOutStart = dTimeUpdate;
         }
        Log.d(TAG, "[reportAvailable]onSubTitleAvailable hasDecoderErrorEvent:" + hasDecoderErrorEvent + ",hasFirstSubtitle:" + hasFirstSubtitle);
        if (mCallback != null && (hasDecoderErrorEvent || !hasFirstSubtitle)) {
            reportAvail(1);
        }
        SubtitleDecoderCountDown();
        hasFirstSubtitle= true;
        hasDecoderErrorEvent = false;
    }

    public void reportError(int error) {
        Log.d(TAG, "[reportError]error:" + error);
        if (mCallback != null) {
            switch (error) {
                case 0:
                        reportAvail(ERROR_DECODER_LOSEDATA);  //event: 3  decoder error
                    hasDecoderErrorEvent = true;
                    break;
                case 1:
                        reportAvail(ERROR_DECODER_INVALIDDATA);
                    hasDecoderErrorEvent = true;
                    break;
                case 2:
                        reportAvail(ERROR_DECODER_TIMEERROR);
                    hasDecoderErrorEvent = true;
                    break;
                case 3:
                        reportAvail(ERROR_DECODER_END);
                    hasDecoderErrorEvent = true;
                    break;
                default:
                    Log.e(TAG, "[reportError]error:" + error);
                    break;
            }
        }
    }
    public void updateData(String json) {
        //if (mSubtitleDataListener != null) {
        int mask = getObjectValueInt(json, "cc", "data", -1);
        Log.d(TAG, "[updateData]json:" + json + ",mask:" + mask);
        if (mask > 0) {
            stopCcNoSigTimeout();
            String s = int2Binary(mask);
            Log.d(TAG, "[updateData]s:" + s);
            int i = 0;
            if (s != null && !s.contains("1")) {
                Log.d(TAG, "[updateData]cc onSubTitleAvailable 0");
                //reportAvail(0);
                return;
            }

            if (s == null)
                return;

            for (i = 0; i < s.length(); i++) {
                if (i < 15 && s.charAt(i) == 49/*1*/) {
                    if (!backList.contains(i)) {  //add new channel data
                        Log.d(TAG, "[updateData]onSubTitleEvent add index:" + i);
                        reportEvent(1, i);
                        backList.add(i);
                        ccAddIdMap.put(i, System.currentTimeMillis());
                    } else {
                        //Log.d(TAG, "[updateData]onSubTitleEvent update start decodertime");
                        ccAddIdMap.put(i, System.currentTimeMillis());
                    }
                } else if (i < 15 && s.charAt(i) == 48/*0*/) {  //loss new channel data
                    if (backList.contains(i)) {
                        endDecoderTime = System.currentTimeMillis();
                        decoderTime = (int) (endDecoderTime - ccAddIdMap.get(i));
                        if (decoderTime >= getDecoderTime() && decoderTime < (getDecoderTime() + 100)) {   //set N s to set message if decoder error
                            Log.d(TAG, "[updateData]onSubTitleEvent remove index:" + i);
                            reportEvent(0, i);
                            int idx = backList.indexOf(i);
                            backList.remove(idx);
                            ccAddIdMap.remove(i);
                        }
                    }
                }
            }
        }
        else {
            Log.d(TAG, "[updateData]onSubTitleEvent cc no signal!!");
            CcNoSigCountDown();
        }
    }

    //add for cc no signal event detect
    private static final int MSG_CC_COUNT_DOWN = 0xE3;//random value
    private Timer ccdecoderTimer = new Timer();
    protected void CcNoSigCountDown() {
        Log.e(TAG, "ccdecoderTimer count down begin....");
        final Handler ccdecoderhandler = new Handler(Looper.getMainLooper()) {
            public void handleMessage (Message msg) {
                switch (msg.what) {
                    case MSG_CC_COUNT_DOWN:
                        CcNoSigRemoveEvent();
                        break;
                }
                super.handleMessage (msg);
            }
        };
        TimerTask task = new TimerTask() {
            public void run() {
                Message message = Message.obtain();
                message.what = MSG_CC_COUNT_DOWN;
                ccdecoderhandler.sendMessage (message);
            }
        };
        stopCcNoSigTimeout();
        if (ccdecoderTimer == null) {
            ccdecoderTimer = new Timer();
        }
        if (ccdecoderTimer != null) {
            Log.e(TAG, "ccdecoderTimer count down schedule time:" + decoderTimeS + " s");
            ccdecoderTimer.schedule (task, getDecoderTime());
        }
    }

    public void CcNoSigRemoveEvent() {
        Log.d(TAG, "[CcNoSigRemoveEvent]");
        int j = 0;
        int addIndex = 0;
        int size = 0;
        if (backList.size() > 0) {
            size = backList.size();
            Log.d(TAG, "[CcNoSigRemoveEvent]size:" + size);
            for (j = 0; j < size; j++) {
                addIndex = backList.get(0);
                Log.d(TAG, "[CcNoSigRemoveEvent] onSubTitleEvent no signal remove index:" + addIndex);
                reportEvent(0, addIndex);
                backList.remove(0);
                ccAddIdMap.remove(addIndex);
            }
        }
    }

    public String getSubLanguage() {
        Log.d(TAG, "[getSubLanguage]mLanguage:" + mLanguage);
        return mLanguage;
    }

    public void saveLanguage(String language) {
        Log.d(TAG, "[saveLanguage]language:" + language);
        mLanguage = language;
    }

    public void setSubtitleDataListener(SubtitleDataListener l) {
        mSubtitleDataListener = l;
    }

    /*public static void setCallback(ISubTitleServiceCallback cb) {
        mCallback= cb;
    }
    public interface SubtitleDataListener {
        public void onSubtitleData(String json);
    }*/

    public static void setCallback(SubtitleDataListener cb) {
        mCallback = cb;
    }
    public interface SubtitleDataListener {
        public void onSubTitleEvent(int event, int id);
        public void onSubTitleAvailable(int available);
    }

}

