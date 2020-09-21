/*
 * Copyright (C) 2017 Amlogic Corporation.
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

package com.droidlogic.tvinput.widget;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.media.tv.TvInputService;
import android.os.Handler;
import android.content.res.Resources;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
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

import java.io.File;
import java.io.FileInputStream;
import java.lang.Exception;
import java.lang.reflect.Type;
import java.util.Locale;

import android.util.Log;
import android.view.accessibility.CaptioningManager;
import android.widget.Toast;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.ChannelInfo;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.droidlogic.tvinput.R;
import com.droidlogic.tvinput.widget.CcImplement;

public class DTVSubtitleView extends View {
    private static final String TAG = "DTVSubtitleView";

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
    private static final int MODE_ISDB_CC = 7;
    private static final int MODE_SCTE27_SUB = 8;

    private static final int EDGE_NONE = 0;
    private static final int EDGE_OUTLINE = 1;
    private static final int EDGE_DROP_SHADOW = 2;
    private static final int EDGE_RAISED = 3;
    private static final int EDGE_DEPRESSED = 4;

    private static final int PLAY_NONE = 0;
    private static final int PLAY_SUB = 1;
    private static final int PLAY_TT  = 2;
    private static final int PLAY_ISDB  = 3;

    public static final int COLOR_RED = 0;
    public static final int COLOR_GREEN = 1;
    public static final int COLOR_YELLOW = 2;
    public static final int COLOR_BLUE = 3;

    public static final int TT_NOTIFY_SEARCHING = 0;
    public static final int TT_NOTIFY_NOSIG = 1;
    public static final int TT_NOTIFY_CANCEL = 2;

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

    private static SystemControlManager mSystemControlManager;

    public static final int JSON_MSG_NORMAL = 0;
    public static final int SUB_VIEW_SHOW = 1;
    public static final int SUB_VIEW_CLEAN = 2;
    public static final int TT_STOP_REFRESH = 3;
    public static final int TT_ZOOM = 4;
    public static final int TT_PAGE_TYPE = 5;
    public static final int TT_NO_SIGAL = 6;

    private static final int TT_DETECT_TIMEOUT = 5 * 1000;

    private static int init_count = 0;
    private static CaptioningManager captioningManager = null;
    private static CcImplement.CaptionWindow cw = null;
    private static IsdbImplement isdbi = null;
    private static CustomFonts cf = null;
    private static CcImplement ci = null;
    private static PorterDuffXfermode mXfermode;
    private static PaintFlagsDrawFilter paint_flag;
    private static String json_str;
    private static Bitmap bitmap = null;
    private static Paint mPaint;
    private static Paint clear_paint;
    private static boolean teletext_have_data = false;
    private static boolean tt_reveal_mode = false;
    private static int tt_zoom_state = 0;
    private static boolean hide_reveal_state = false;
    private static boolean draw_no_subpg_notification = false;
    private static int tt_notify_status = TT_NOTIFY_CANCEL;
    private static int notify_pgno = 0;

    private static int tt_page_type;
    public static String TT_REGION_DB = "teletext_region_id";

    private boolean need_clear_canvas;
    private boolean tt_refresh_switch = true;
    private boolean decoding_status;

    public static boolean cc_is_started = false;

    private boolean isPreWindowMode = false;

    private native int native_sub_init();
    private native int native_sub_destroy();
    private native int native_sub_lock();
    private native int native_sub_unlock();
    private native int native_sub_clear();
    private native int native_sub_start_dvb_sub(int dmx_id, int pid, int page_id, int anc_page_id);
    private native int native_sub_start_dtv_tt(int dmx_id, int region_id, int pid, int page, int sub_page, boolean is_sub);
    private native int native_sub_start_atv_tt(int region_id, int page_no, int sub_page_no, boolean is_sub);
    private native int native_sub_stop_dvb_sub();
    private native int native_sub_stop_dtv_tt();
    private native int native_sub_stop_atv_tt();
    private native int native_sub_tt_goto(int page, int subpg);
    private native int native_sub_tt_color_link(int color);
    private native int native_sub_tt_home_link();
    private native int native_sub_tt_next(int dir);
    private native int native_sub_tt_set_display_mode(int mode);
    private native int native_sub_tt_set_region(int region_id);
    private native int native_sub_tt_set_reveal_mode(boolean mode);
    private native int native_sub_tt_set_search_pattern(String pattern, boolean casefold);
    private native int native_sub_tt_lock_subpg(int lock);
    private native int native_sub_tt_search_next(int dir);
    private native int native_sub_tt_goto_subtitle();
    protected native int native_get_subtitle_picture_width();
    protected native int native_get_subtitle_picture_height();
    private native int native_sub_start_atsc_cc(int vfmt, int caption, int decoder_param, String lang, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_sub_start_atsc_atvcc(int caption, int decoder_param, String lang, int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_sub_stop_atsc_cc();
    private native static int native_sub_set_atsc_cc_options(int fg_color, int fg_opacity, int bg_color, int bg_opacity, int font_style, int font_size);
    private native int native_sub_set_active(boolean active);
    private native int native_sub_start_isdbt(int dmx_id, int pid, int caption_id);
    private native int native_sub_stop_isdbt();
    private native int native_sub_start_scte27(int dmx_id, int pid);
    private native int native_sub_stop_scte27();

    static {
//        System.loadLibrary("am_adp");
//        System.loadLibrary("am_mw");
//        System.loadLibrary("zvbi");
        bitmap = Bitmap.createBitmap(BUFFER_W, BUFFER_H, Bitmap.Config.ARGB_8888);

        System.loadLibrary("jnidtvsubtitle");
    }

    static public class DVBSubParams {
        private int dmx_id;
        private int pid;
        private int composition_page_id;
        private int ancillary_page_id;
        public DVBSubParams(int dmx_id, int pid, int page_id, int anc_page_id) {
            this.dmx_id              = dmx_id;
            this.pid                 = pid;
            this.composition_page_id = page_id;
            this.ancillary_page_id   = anc_page_id;
        }
    }

    static public class ISDBParams {
        private int dmx_id;
        private int pid;
        private int caption_id;

        public ISDBParams(int dmx_id, int pid, int caption_id) {
            this.pid = pid;
            this.dmx_id = dmx_id;
            this.caption_id = caption_id;
        }
    }

    static public class DTVTTParams {
        private int dmx_id;
        private int pid;
        private int page_no;
        private int sub_page_no;
        private int region_id;
        private int type;
        private int stype;

        public DTVTTParams(int dmx_id, int pid, int page_no, int sub_page_no, int region_id, int type, int stype) {
            this.dmx_id      = dmx_id;
            this.pid         = pid;
            this.page_no     = page_no;
            this.sub_page_no = sub_page_no;
            this.region_id   = region_id;
            this.type = type;
            this.stype = stype;
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
        private int page_no;
        private int sub_page_no;
        private int region_id;
        public ATVTTParams(int page_no, int sub_page_no, int region_id) {
            this.page_no     = page_no;
            this.sub_page_no = sub_page_no;
            this.region_id   = region_id;
        }
    }

    static public class DTVCCParams {
        protected int vfmt;
        protected int caption_mode;
        protected int decoder_param;
        protected int fg_color;
        protected int fg_opacity;
        protected int bg_color;
        protected int bg_opacity;
        protected int font_style;
        protected float font_size;
        protected String lang;

        public DTVCCParams(int vfmt, int caption, int decoder_param, String lang, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            this.vfmt = vfmt;
            this.caption_mode = caption;
            this.decoder_param = decoder_param;
            this.lang = lang;
            this.fg_color = fg_color;
            this.fg_opacity = fg_opacity;
            this.bg_color = bg_color;
            this.bg_opacity = bg_opacity;
            this.font_style = font_style;
            this.font_size = font_size;
        }
    }

    static public class ATVCCParams extends DTVCCParams {
        public ATVCCParams(int caption, int decoder_param, String lang, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            super(VFMT_ATV, caption, decoder_param, lang, fg_color, fg_opacity,
                    bg_color, bg_opacity, font_style, font_size);
        }
    }
    static public class AVCCParams extends DTVCCParams {
        public AVCCParams(int caption, int decoder_param, String lang, int fg_color, int fg_opacity,
                int bg_color, int bg_opacity, int font_style, float font_size) {
            super(VFMT_ATV, caption, decoder_param, lang, fg_color, fg_opacity,
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
        ISDBParams isdb_cc;
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

    private static SubParams sub_params;
    private static int       play_mode;
    private boolean   visible;
    private boolean   destroy;
    private static DTVSubtitleView activeView = null;
    private static int tt_red_value;
    private static int tt_green_value;
    private static int tt_yellow_value;
    private static int tt_blue_value;
    private static int tt_curr_pgno;
    private static int tt_curr_subpg;
    private static String tt_navi_subpg_string;
    private static byte[] tt_subs;

    private void tt_update(int page_type, int pgno, byte subs[], int red, int green, int yellow, int blue, int curr_subpg) {
        Log.i(TAG, "page_type " + page_type + " subno " + subs.length + " red " + red + " green " + green + " yellow " + yellow + " blue " + blue +
                " currsub " + curr_subpg);
        tt_subs = subs;
        tt_red_value = red;
        tt_green_value = green;
        tt_yellow_value = yellow;
        tt_blue_value = blue;
        tt_curr_pgno = pgno;
        tt_curr_subpg = curr_subpg;
        if (!tt_subpg_walk_mode)
            tt_set_subpn_text(Integer.toString(tt_curr_subpg));

        if (!tt_refresh_switch)
            return;
        handler.obtainMessage(TT_PAGE_TYPE, page_type).sendToTarget();

        postInvalidate();
    }

    private void update() {
        postInvalidate();
    }

    private void stopDecoder() {
        Log.d(TAG, "subtitleView stopSub cc:" + cc_is_started + " playmode " + play_mode + " sub_mode " + sub_params.mode);
        synchronized(lock) {
            if (!cc_is_started)
                return;

            switch (play_mode) {
                case PLAY_NONE:
                    break;
                case PLAY_TT:
                    handler.removeMessages(TT_NO_SIGAL);
                    tt_notify_status = TT_NOTIFY_CANCEL;
                    need_clear_canvas = true;
                    postInvalidate();
                    switch (sub_params.mode) {
                        case MODE_DTV_TT:
                            native_sub_stop_dtv_tt();
                            teletext_have_data = false;
                            break;
                        case MODE_ATV_TT:
                            native_sub_stop_atv_tt();
                            teletext_have_data = false;
                        default:
                            break;
                    }
                    break;
                case PLAY_SUB:
                    switch (sub_params.mode) {
                        case MODE_DTV_TT:
                            native_sub_stop_dtv_tt();
                            teletext_have_data = false;
                            break;
                        case MODE_ATV_TT:
                            native_sub_stop_atv_tt();
                            teletext_have_data = false;
                            break;
                        case MODE_DVB_SUB:
                            native_sub_stop_dvb_sub();
                            break;
                        case MODE_DTV_CC:
                        case MODE_ATV_CC:
                        case MODE_AV_CC:
                            native_sub_stop_atsc_cc();
                            break;
                        case MODE_ISDB_CC:
                            native_sub_stop_isdbt();
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
            need_clear_canvas = true;
        }
    }

    private void init(Context context) {
        synchronized(lock) {
            if (init_count == 0) {
                play_mode = PLAY_NONE;
                visible = true;
                destroy = false;
                sub_params = new SubParams();
                need_clear_canvas = true;

                if (native_sub_init() < 0) {
                }

                decoding_status = false;
                cf = new CustomFonts(context);
                ci = new CcImplement(context, cf);
                cw = ci.new CaptionWindow();
                isdbi = new IsdbImplement(context);
                mPaint = new Paint();
                clear_paint = new Paint();
                clear_paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
                mSystemControlManager = SystemControlManager.getInstance();
                mXfermode = new PorterDuffXfermode(PorterDuff.Mode.SRC);
                paint_flag = new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
                captioningManager = (CaptioningManager) context.getSystemService(Context.CAPTIONING_SERVICE);
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
            }
            Log.e(TAG, "subtitle view init");
            init_count += 1;
            teletext_have_data = false;
        }
    }

    private void reset_bitmap_to_black()
    {
        if (bitmap != null) {
            Canvas newcan = new Canvas(bitmap);
            newcan.drawColor(Color.BLACK);
        }
    }

    public DTVSubtitleView(Context context) {
        super(context);
        init(context);
    }

    public DTVSubtitleView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public DTVSubtitleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    public void setMargin(int left, int top, int right, int bottom) {
        disp_left   = left;
        disp_top    = top;
        disp_right  = right;
        disp_bottom = bottom;
    }

    public void setActive(boolean active) {
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
            if (!isPreWindowMode)
                postInvalidate();
        }
    }

    public void setSubParams(ATVTTParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ATV_TT;
            sub_params.atv_tt = params;

            if (play_mode == PLAY_TT)
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

    public void setSubParams(DVBSubParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DVB_SUB;
            sub_params.dvb_sub = params;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(ISDBParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ISDB_CC;
            sub_params.isdb_cc = params;
            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(DTVTTParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DTV_TT;
            sub_params.dtv_tt = params;

            if (play_mode == PLAY_TT)
                startSub();
        }
    }

    public void setSubParams(DTVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_DTV_CC;
            sub_params.dtv_cc = params;
            sub_params.vfmt = params.vfmt;

            if (play_mode == PLAY_SUB)
                startSub();
        }
    }

    public void setSubParams(ATVCCParams params) {
        synchronized(lock) {
            sub_params.mode = MODE_ATV_CC;
            sub_params.atv_cc = params;

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

    public void setTTParams(DTVTTParams params) {
        synchronized(lock) {
            if (play_mode == PLAY_TT)
                startSub();
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

    public void show() {
        if (visible)
            return;

        handler.obtainMessage(SUB_VIEW_SHOW, true).sendToTarget();
        update();
    }

    public void hide() {
        if (!visible)
            return;

        handler.obtainMessage(SUB_VIEW_SHOW, false).sendToTarget();
        update();
    }

    public void startSub() {
        Log.e(TAG, "startSub" + play_mode + " " + sub_params.mode);
        synchronized(lock) {
            if (activeView != this)
                return;

            stopDecoder();

            if (sub_params.mode == MODE_NONE)
                return;
            int ret = 0;
            switch (sub_params.mode) {
                case MODE_DVB_SUB:
                    ret = native_sub_start_dvb_sub(sub_params.dvb_sub.dmx_id,
                            sub_params.dvb_sub.pid,
                            sub_params.dvb_sub.composition_page_id,
                            sub_params.dvb_sub.ancillary_page_id);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_ATV_TT:
                    reset_bitmap_to_black();
                    Log.e(TAG, "native_sub_start_atv_tt");
                    tt_notify_status = TT_NOTIFY_SEARCHING;
                    notify_pgno = 0;
                    postInvalidate();
                    handler.sendEmptyMessageDelayed(TT_NO_SIGAL, TT_DETECT_TIMEOUT);
                    ret = native_sub_start_atv_tt(
                            sub_params.atv_tt.region_id,
                            sub_params.atv_tt.page_no,
                            sub_params.atv_tt.sub_page_no,
                            false);
                    if (ret >= 0) {
                        play_mode = PLAY_TT;
                    }
                    break;
                case MODE_DTV_TT:
                    boolean is_subtitle;
                    if (sub_params.dtv_tt.type == ChannelInfo.Subtitle.TYPE_DTV_TELETEXT_IMG)
                        is_subtitle = false;
                    else
                        is_subtitle = true;
                    tt_notify_status = TT_NOTIFY_SEARCHING;
                    postInvalidate();
                    handler.sendEmptyMessageDelayed(TT_NO_SIGAL, TT_DETECT_TIMEOUT);
                    ret = native_sub_start_dtv_tt(sub_params.dtv_tt.dmx_id,
                            sub_params.dtv_tt.region_id,
                            sub_params.dtv_tt.pid,
                            sub_params.dtv_tt.page_no,
                            sub_params.dtv_tt.sub_page_no,
                            is_subtitle);
                    if (ret >= 0) {
                        play_mode = PLAY_TT;
                    }
                    break;
                case MODE_DTV_CC:
                    ret = native_sub_start_atsc_cc(
                            sub_params.vfmt,
                            sub_params.dtv_cc.caption_mode,
                            sub_params.dtv_cc.decoder_param,
                            sub_params.dtv_cc.lang,
                            sub_params.dtv_cc.fg_color,
                            sub_params.dtv_cc.fg_opacity,
                            sub_params.dtv_cc.bg_color,
                            sub_params.dtv_cc.bg_opacity,
                            sub_params.dtv_cc.font_style,
                            new Float(sub_params.dtv_cc.font_size).intValue());
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_ATV_CC:
                    ret = native_sub_start_atsc_atvcc(
                            sub_params.atv_cc.caption_mode,
                            sub_params.atv_cc.decoder_param,
                            sub_params.atv_cc.lang,
                            sub_params.atv_cc.fg_color,
                            sub_params.atv_cc.fg_opacity,
                            sub_params.atv_cc.bg_color,
                            sub_params.atv_cc.bg_opacity,
                            sub_params.atv_cc.font_style,
                            new Float(sub_params.atv_cc.font_size).intValue());
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_AV_CC:
                    ret = native_sub_start_atsc_atvcc(
                            sub_params.av_cc.caption_mode,
                            sub_params.av_cc.decoder_param,
                            sub_params.av_cc.lang,
                            sub_params.av_cc.fg_color,
                            sub_params.av_cc.fg_opacity,
                            sub_params.av_cc.bg_color,
                            sub_params.av_cc.bg_opacity,
                            sub_params.av_cc.font_style,
                            new Float(sub_params.av_cc.font_size).intValue());
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_ISDB_CC:
                    ret = native_sub_start_isdbt(
                            sub_params.isdb_cc.dmx_id,
                            sub_params.isdb_cc.pid,
                            sub_params.isdb_cc.caption_id);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                case MODE_SCTE27_SUB:
                    native_sub_start_scte27(sub_params.scte27_sub.dmx_id, sub_params.scte27_sub.pid);
                    if (ret >= 0) {
                        play_mode = PLAY_SUB;
                    }
                    break;
                default:
                    ret = -1;
                    break;
            }

            if (ret >= 0) {
                cc_is_started = true;
            }
        }
    }

    public void stop() {
        synchronized(lock) {
            Log.e(TAG, "subtitleView stop");
            if (activeView != this) {
                Log.e(TAG, "activeView not this");
                return;
            }
            stopDecoder();
        }
    }

    public void clear() {
        synchronized(lock) {
            if (activeView != this)
                return;

            handler.obtainMessage(SUB_VIEW_CLEAN).sendToTarget();
            stopDecoder();
            native_sub_clear();
            sub_params.mode = MODE_NONE;
        }
    }

    public void nextPage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            native_sub_tt_next(1);
        }
    }

    public void previousPage() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            Log.e(TAG, "previousPage");

            native_sub_tt_next(-1);
        }
    }

    public void gotoPage(int page) {
        final int TT_ANY_SUBNO = 0x3F7F;
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_goto(page, TT_ANY_SUBNO);
        }
    }
    public void gotoPage(int page, int subpg) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_goto(page, subpg);
        }
    }

    public void goHome() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_home_link();
        }
    }

    public void colorLink(int color) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            Log.e(TAG, "colorlink");

            native_sub_tt_color_link(color);
        }
    }

    public void setSearchPattern(String pattern, boolean casefold) {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_set_search_pattern(pattern, casefold);
        }
    }

    public static final int TT_DISP_NORMAL = 0;
    public static final int TT_DISP_ONLY_PGNO = 1;
    public static final int TT_DISP_ONLY_CLOCK = 2;
    public static final int TT_DISP_MIX_TRANSPARENT = 3;
    public static final int TT_DISP_MIX_RIGHT = 4;

    private static int tt_display_mode = TT_DISP_NORMAL;
    public void setTTDisplayMode(int mode) {
        synchronized(lock) {
            need_clear_canvas = true;
            //Change between display mode 0-2

            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;
            tt_display_mode = mode;
            native_sub_tt_set_display_mode(mode);
        }
    }

    public int setTTRegion(int region_id)
    {
        Log.e(TAG, "setTTRegion " + region_id);
        Settings.Global.putInt(getContext().getContentResolver(), TT_REGION_DB, region_id);
        return native_sub_tt_set_region(region_id);
    }

    public void setTTRevealMode() {
        tt_reveal_mode = !tt_reveal_mode;
        native_sub_tt_set_reveal_mode(tt_reveal_mode);
    }

    public static final int TT_LOCK_MODE_NORMAL = 0;
    public static final int TT_LOCK_MODE_LOCK = 1;
    public static final int TT_LOCK_MODE_LOCK_NO_ICON = 2;

    public void setTTSubpgLock(int mode) {
        if (mode <= TT_LOCK_MODE_LOCK_NO_ICON && mode >= TT_LOCK_MODE_NORMAL)
            native_sub_tt_lock_subpg(mode);
    }

    public static boolean tt_subpg_walk_mode = false;
    public void setTTSubpgWalk(boolean enable)
    {
        tt_subpg_walk_mode = enable;
        postInvalidate();
    }

    public int get_teletext_subpg_count()
    {
        if (tt_subs == null)
            return 0;
        else
            return tt_subs.length;
    }
    public int get_teletext_pg()
    {
        return tt_curr_pgno;
    }
    public int get_teletext_subpg()
    {
        return tt_curr_subpg;
    }

    Runnable myTimerRun=new Runnable()
    {
        @Override
        public void run() {
            draw_no_subpg_notification = false;
        }
    };
    public void notify_no_subpage()
    {
        draw_no_subpg_notification = true;
        postInvalidate();
        handler.postDelayed(myTimerRun, 2000);
        postInvalidate();
    }

    public void tt_goto_subtitle()
    {
        native_sub_tt_goto_subtitle();
    }

    private boolean tt_show_switch = true;
    public void setTTSwitch(boolean show)
    {
        tt_show_switch = show;
        if (!tt_show_switch)
            need_clear_canvas = true;
        postInvalidate();
    }

    public static int tt_subpg_mode_subno = 1;
    public int tt_subpg_updown(boolean up)
    {
        int i;
        if (tt_subs == null)
            return 0;

        for (i=0; i<tt_subs.length; i++)
        {
            if (tt_curr_subpg == tt_subs[i])
                break;
        }
        if (up) {
            i++;
            if (i >= tt_subs.length)
                i = 0;
        } else {
            i--;
            if (i < 0)
                i = tt_subs.length - 1;
        }
        Log.e(TAG, "subpg updown up " + up + " pgno " + tt_curr_pgno + " subno " + tt_curr_subpg + " tsub " + tt_subs[i]);
        native_sub_tt_goto(tt_curr_pgno, tt_subs[i]);
        return tt_subs[i];
    }

    private static int tt_mix_mode;
    public void setTTMixMode(int tt_display_mode) {
        synchronized(lock) {
//            need_clear_canvas = true;
            //Change between display mode 0 3 4
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            tt_mix_mode = tt_display_mode;
            if (tt_display_mode == TT_DISP_MIX_TRANSPARENT || tt_display_mode == TT_DISP_NORMAL) {
                handler.obtainMessage(TT_ZOOM, TT_ZOOM_NORMAL).sendToTarget();
                native_sub_tt_set_display_mode(tt_display_mode);
            } else {
                handler.obtainMessage(TT_ZOOM, TT_ZOOM_RIGHT).sendToTarget();
                native_sub_tt_set_display_mode(TT_DISP_NORMAL);
            }
        }
    }

    public static final int ATV_TT_NO_CONTENT   = 0;
    public static final int ATV_TT_HAVE_CONTENT = 1;
    public static final int ATV_TT_SEARCHING    = 2;

    public void tt_reset_status()
    {
    }

    public void tt_data_notify(int pgno)
    {
//        Log.i(TAG, "tt_data_notify pgno: " + pgno);
        notify_pgno = pgno;
        teletext_have_data = true;
        if (tt_notify_status == TT_NOTIFY_SEARCHING) {
            postInvalidate();
        }
        handler.removeMessages(TT_NO_SIGAL);
        if (pgno == 100) {
            tt_notify_status = TT_NOTIFY_CANCEL;
            postInvalidate();
        }
    }

    public boolean tt_have_data()
    {
        return teletext_have_data;
    }

    public void searchNext() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_search_next(1);
        }
    }

    public void searchPrevious() {
        synchronized(lock) {
            if (activeView != this)
                return;
            if (play_mode != PLAY_TT)
                return;

            native_sub_tt_search_next(-1);
        }
    }

    public void hide_reveal_tt()
    {
        hide_reveal_state = !hide_reveal_state;
        if (hide_reveal_state) {
            show();
        } else {
            hide();
        }
    }

    public void reset_atv_status()
    {
        synchronized (lock) {
            tt_mix_mode = TT_DISP_NORMAL;
            tt_display_mode = TT_DISP_NORMAL;
            tt_zoom_state = TT_ZOOM_NORMAL;
            tt_reveal_mode = false;
            handler.obtainMessage(TT_ZOOM, TT_ZOOM_NORMAL).sendToTarget();
            native_sub_tt_set_display_mode(TT_DISP_NORMAL);
            native_sub_tt_set_reveal_mode(false);
            native_sub_tt_lock_subpg(0);
        }
        postInvalidate();
    }

    public static final int TT_ZOOM_NORMAL = 0;
    public static final int TT_ZOOM_TOP_HALF = 1;
    public static final int TT_ZOOM_BOTTOM_HALF = 2;
    public static final int TT_ZOOM_RIGHT = 3;
    public void tt_zoom_in()
    {
        switch (tt_zoom_state)
        {
            case TT_ZOOM_NORMAL:
                tt_zoom_state = TT_ZOOM_TOP_HALF;
                break;
            case TT_ZOOM_TOP_HALF:
                tt_zoom_state = TT_ZOOM_BOTTOM_HALF;
                break;
            case TT_ZOOM_BOTTOM_HALF:
                tt_zoom_state = TT_ZOOM_NORMAL;
                break;
            default:
                tt_zoom_state = TT_ZOOM_NORMAL;
                break;
        };
        handler.obtainMessage(TT_ZOOM, tt_zoom_state).sendToTarget();
    }

    public void tt_stop_refresh()
    {
        tt_refresh_switch = !tt_refresh_switch;
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

    boolean valid_page_no(int pgno)
    {
        if (pgno < 899 && pgno >= 100)
            return true;
        else
            return false;
    }

    public void tt_set_subpn_text(String number_str)
    {
        tt_navi_subpg_string = number_str;
        while (tt_navi_subpg_string.length() < 4) {
            tt_navi_subpg_string = '0' + tt_navi_subpg_string;
        }
    }

    private static final int TT_MODE_NORMAL = 0;
    private static final int TT_MODE_SUBPG = 1;
    private void draw_navigation_bar(Canvas canvas, Paint paint,
                                     int tt_page_type, boolean tt_subpg_mode,
                                     int left, int top, int right, int bottom,
                                     int red, int green, int yellow, int blue,
                                     int pgno, byte[] subs, int curr_sub,
                                     int mode, boolean transparent)
    {
        int color_bar_width = (right - left) / 4;
        int row_height = (bottom - top) / 2;
        int red_left = left;
        int red_right = red_left + color_bar_width;
        int green_left = red_right;
        int green_right = green_left + color_bar_width;
        int yellow_left = green_right;
        int yellow_right = yellow_left + color_bar_width;
        int blue_left = yellow_right;
        int blue_right = blue_left + color_bar_width;
        float digit_width = paint.measureText("123"); //Just a three number digit
        float digit_start_in_bar = (color_bar_width - digit_width)/2;
        String red_string, blue_string, yellow_string, green_string;
        int text_bottom = top + row_height;

        paint.setTypeface(Typeface.MONOSPACE);
        paint.setTextSize((int)(row_height*1.3));

        if ((tt_page_type & 0x80000) != 0) //Not subtitle or newsflash
            return;

        if (((tt_page_type & 0x8000) != 0) && ((tt_page_type & 0x4000) == 0))
            return;

        //Draw rects
        if (!transparent) {
            paint.setColor(Color.RED);
            canvas.drawRect(red_left, top, red_right, top + row_height, paint);
            paint.setColor(Color.GREEN);
            canvas.drawRect(green_left, top, green_right, top + row_height, paint);
            if (!tt_subpg_mode) {
                paint.setColor(Color.YELLOW);
                canvas.drawRect(yellow_left, top, yellow_right, top + row_height, paint);
                paint.setColor(Color.BLUE);
                canvas.drawRect(blue_left, top, blue_right, top + row_height, paint);
            } else {
                paint.setColor(Color.YELLOW);
                canvas.drawRect(yellow_left, top, blue_right, top + row_height, paint);
            }
        }

        //Draw text
        if (tt_subpg_mode) {
            if (transparent) {
                paint.setColor(Color.RED);
                canvas.drawText("-", red_left + digit_start_in_bar, text_bottom, paint);
                paint.setColor(Color.GREEN);
                canvas.drawText("+", green_left + digit_start_in_bar, text_bottom, paint);
                paint.setColor(Color.YELLOW);
                canvas.drawText(Integer.toString(tt_curr_pgno) + " / " + tt_navi_subpg_string,
                        yellow_left + color_bar_width/2,
                        text_bottom,
                        paint);
            } else {
                paint.setColor(Color.BLACK);
                canvas.drawText("-", red_left + digit_start_in_bar, text_bottom, paint);
                canvas.drawText("+", green_left + digit_start_in_bar, text_bottom, paint);
                canvas.drawText(Integer.toString(tt_curr_pgno) + " / " + tt_navi_subpg_string,
                        yellow_left + color_bar_width/2,
                        text_bottom,
                        paint);
            }
        } else {
            if (valid_page_no(red))
                red_string = Integer.toString(red);
            else
                red_string = "---";
            if (valid_page_no(green))
                green_string = Integer.toString(green);
            else
                green_string = "---";
            if (valid_page_no(yellow))
                yellow_string = Integer.toString(yellow);
            else
                yellow_string = "---";
            if (valid_page_no(blue))
                blue_string = Integer.toString(blue);
            else
                blue_string = "---";

            if (transparent) {
                paint.setColor(Color.RED);
                canvas.drawText(red_string, red_left + digit_start_in_bar, text_bottom, paint);
                paint.setColor(Color.GREEN);
                canvas.drawText(green_string, green_left + digit_start_in_bar, text_bottom, paint);
                paint.setColor(Color.YELLOW);
                canvas.drawText(yellow_string, yellow_left + digit_start_in_bar, text_bottom, paint);
                paint.setColor(Color.BLUE);
                canvas.drawText(blue_string, blue_left + digit_start_in_bar, text_bottom, paint);
            } else {
                paint.setColor(Color.BLACK);
                canvas.drawText(red_string, red_left + digit_start_in_bar, text_bottom, paint);
                canvas.drawText(green_string, green_left + digit_start_in_bar, text_bottom, paint);
                canvas.drawText(yellow_string, yellow_left + digit_start_in_bar, text_bottom, paint);
                canvas.drawText(blue_string, blue_left + digit_start_in_bar, text_bottom, paint);
            }
        }
        if (subs != null) {
            if (subs.length > 1) {
                float string_width = paint.measureText("12 "); //Just two digits and a space
                for (int i = 0; i < subs.length; i++) {
                    if (subs[i] == curr_sub)
                        paint.setColor(Color.RED);
                    else
                        paint.setColor(Color.WHITE);
                    canvas.drawText("0" + Integer.toString(subs[i]), red_left + (i - 1) * string_width, bottom, paint);
                }
            } else if (draw_no_subpg_notification) {
                paint.setColor(Color.RED);
                canvas.drawText("No sub-page", red_left, bottom, paint);
            }
        }
    }

    @Override
    public void onDraw(Canvas canvas) {
        Rect sr, dr;
        String json_data;
        String screen_mode;
        String video_status;
        String ratio;
        int row_height = 0;
        int navi_left = 0;
        int navi_right = 0;

        if (need_clear_canvas) {
            /* Clear canvas */
            canvas.drawPaint(clear_paint);
            json_str = null;
            need_clear_canvas = false;
            return;
        }
//        Log.e(TAG, "onDraw active " + active + " visible " + visible + " pm " + play_mode);
        if (!active || !visible || (play_mode == PLAY_NONE)) {
            return;
        }

        switch (sub_params.mode)
        {
            case MODE_AV_CC:
            case MODE_DTV_CC:
            case MODE_ATV_CC:
                /* For atsc */
                if (!ci.cc_setting.is_enabled)
                    return;
                screen_mode = mSystemControlManager.readSysFs("/sys/class/video/screen_mode");
                video_status = mSystemControlManager.readSysFs("/sys/class/video/video_state");
                ratio = mSystemControlManager.readSysFs("/sys/class/video/frame_aspect_ratio");
                cw.UpdatePositioning(ratio, screen_mode, video_status);
                ci.caption_screen.updateCaptionScreen(canvas.getWidth(), canvas.getHeight());
                cw.style_use_broadcast = ci.isStyle_use_broadcast();

                cw.updateCaptionWindow(json_str);
                if (cw.init_flag)
                    decoding_status = true;
                if (decoding_status)
                    cw.draw(canvas);
                break;
            case MODE_DTV_TT:
            case MODE_ATV_TT:
                if (!tt_show_switch)
                    return;

                if (tt_notify_status == TT_NOTIFY_NOSIG) {
                    canvas.drawColor(Color.BLACK);
                    mPaint.setColor(Color.RED);
                    mPaint.setTextSize(60);
                    String notify_str = "No teletext";
                    float strlen = mPaint.measureText(notify_str);
                    canvas.drawText(notify_str, getWidth() / 2 - strlen / 2, getHeight() / 2, mPaint);
                    return;
                } else if (tt_notify_status == TT_NOTIFY_SEARCHING) {
                    canvas.drawColor(Color.BLACK);
                    mPaint.setColor(Color.RED);
                    mPaint.setTextSize(60);
                    String notify_str = "Searching teletext: ";
                    String pg_status = Integer.toString(notify_pgno);
                    if (notify_pgno == 0)
                        pg_status = "no page";
                    else
                        pg_status = Integer.toString(notify_pgno);
                    float strlen = mPaint.measureText(notify_str);
                    canvas.drawText(notify_str + pg_status,  getWidth() / 2 - strlen / 2, getHeight() / 2, mPaint);
                    return;
                }

                if (!teletext_have_data)
                    return;

                int src_tt_bottom_ori = 10*24;
                int src_tt_bottom;
                int src_tt_top;

                switch (tt_zoom_state)
                {
                    case 0:
                        src_tt_bottom = src_tt_bottom_ori;
                        src_tt_top = 0;
                        disp_left = (int)(getWidth() * 0.20);
                        disp_right = getWidth() - disp_left;
                        disp_top = (int)(getHeight() * 0.05);
                        disp_bottom = getHeight() - disp_top;
                        break;
                    case 1:
                        src_tt_bottom = src_tt_bottom_ori/2;
                        src_tt_top = 0;
                        disp_left = 0;
                        disp_right = getWidth() - disp_left;
                        disp_top = (int)(getHeight() * 0.05);
                        disp_bottom = getHeight() - disp_top;
                        break;
                    case 2:
                        src_tt_bottom = src_tt_bottom_ori;
                        src_tt_top = src_tt_bottom_ori/2;
                        disp_left = 0;
                        disp_right = getWidth() - disp_left;
                        disp_top = (int)(getHeight() * 0.05);
                        disp_bottom = getHeight() - disp_top;
                        break;
                    case 3:
                        src_tt_bottom = src_tt_bottom_ori;
                        src_tt_top = 0;
                        disp_left = (int)(getWidth() * 0.50);
                        disp_right = getWidth();
                        disp_top = (int)(getHeight() * 0.05);
                        disp_bottom = getHeight() - disp_top;
                        break;
                    default:
                        src_tt_bottom = src_tt_bottom_ori;
                        src_tt_top = 0;
                        disp_left = (int)(getWidth() * 0.20);
                        disp_right = getWidth() - disp_left;
                        disp_top = (int)(getHeight() * 0.05);
                        disp_bottom = getHeight() - disp_top;
                        break;
                }
                //bottom edge of original bitmap is not clean, so cut a little bit,
                //that is why src_tt_bottom minus one.
                row_height = (int)(disp_bottom / 26);
                sr = new Rect(1, src_tt_top + 1, 12 * 41 - 1, src_tt_bottom - 1);
                dr = new Rect(disp_left, disp_top, disp_right, row_height*25);

                canvas.setDrawFilter(paint_flag);
                mPaint.setXfermode(mXfermode);
                canvas.drawPaint(clear_paint);

                if ((tt_page_type & 0xC000 ) == 0) { //Not a subtitle or newsflash page
                    if (tt_mix_mode == TT_DISP_MIX_RIGHT) {
                        mPaint.setColor(Color.BLACK);
                        canvas.drawRect(getWidth() / 2, 0, getWidth(), getHeight(), mPaint);
                    } else if (tt_display_mode != TT_DISP_ONLY_PGNO &&
                            tt_display_mode != TT_DISP_ONLY_CLOCK &&
                            tt_display_mode != TT_DISP_MIX_TRANSPARENT &&
                            tt_mix_mode != TT_DISP_MIX_TRANSPARENT) {
                        canvas.drawColor(Color.BLACK);
                    }
                }
                native_sub_lock();
                canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                canvas.drawBitmap(bitmap, sr, dr, mPaint);
                native_sub_unlock();
                if (tt_zoom_state != TT_ZOOM_RIGHT) {
                    navi_left = (int) (getWidth() * 0.20);
                    navi_right = getWidth() - navi_left;
                } else {
                    navi_left = (int) (getWidth() / 2);
                    navi_right = (int) (getWidth() * 0.9);
                }
                if (tt_display_mode != TT_DISP_ONLY_PGNO &&
                    tt_display_mode != TT_DISP_ONLY_CLOCK)
                draw_navigation_bar(canvas, mPaint, tt_page_type, tt_subpg_walk_mode,
                        navi_left, row_height*25, navi_right, row_height*27,
                        tt_red_value, tt_green_value, tt_yellow_value, tt_blue_value,
                        tt_curr_pgno, tt_subs, tt_curr_subpg,
                        0, tt_mix_mode == TT_DISP_MIX_TRANSPARENT);
                break;
            case MODE_DVB_SUB:
                Log.e(TAG, "ondraw MODE_DVB_SUB");
                if (play_mode == PLAY_SUB) {
                    sr = new Rect(0, 0, native_get_subtitle_picture_width(), native_get_subtitle_picture_height());
                    dr = new Rect(disp_left, disp_top, getWidth() - disp_right, getHeight() - disp_bottom);
                } else {
                    sr = new Rect(0, 0, BUFFER_W, BUFFER_H);
                    dr = new Rect(disp_left, disp_top, getWidth() - disp_right, getHeight() - disp_bottom);
                }
                native_sub_lock();
                canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                canvas.drawBitmap(bitmap, sr, dr, mPaint);
                native_sub_unlock();
                break;
            case MODE_SCTE27_SUB:
                int native_w = native_get_subtitle_picture_width();
                int native_h = native_get_subtitle_picture_height();
                //int edge_l = (getWidth() - native_w * getHeight() / native_h) / 2;
                int edge_l = 0;
                sr = new Rect(0, 0, native_w, native_h);
                dr = new Rect(edge_l, 0, getWidth(), getHeight());
                native_sub_lock();
                canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
                canvas.drawBitmap(bitmap, sr, dr, mPaint);
                native_sub_unlock();
                break;
            case MODE_ISDB_CC:
                screen_mode = mSystemControlManager.readSysFs("/sys/class/video/screen_mode");
                video_status = mSystemControlManager.readSysFs("/sys/class/video/video_state");
                ratio = mSystemControlManager.readSysFs("/sys/class/video/frame_aspect_ratio");
                isdbi.updateVideoPosition(ratio, screen_mode, video_status);
                isdbi.draw(canvas, json_str);
                break;
            default:
                break;
        };
    }

    public void dispose() {
        Log.e(TAG, "Subview dispose");
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
        Log.e(TAG, "Subview finalize");
        // Resource may not be available during gc process
        dispose();
        Log.e(TAG, "Finalize");
        super.finalize();
    }

    public void setVisible(boolean value) {
        Log.d(TAG, "force set visible to:" + value);
        handler.obtainMessage(SUB_VIEW_SHOW, value).sendToTarget();
        postInvalidate();
    }

    private SubtitleDataListener mSubtitleDataListener = null;
    Handler handler = new Handler()
    {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case JSON_MSG_NORMAL:
                    json_str = (String)msg.obj;
                    if (play_mode != PLAY_NONE)
                        postInvalidate();
                    break;
                case SUB_VIEW_SHOW:
                    visible = (Boolean) msg.obj;
                    if (!visible) {
                        json_str = null;
                        need_clear_canvas = true;
                        decoding_status = false;
                    }
                    postInvalidate();
                    break;
                case SUB_VIEW_CLEAN:
                    json_str = null;
                    need_clear_canvas = true;
                    decoding_status = false;
                    postInvalidate();
                    break;
                case TT_STOP_REFRESH:
                    postInvalidate();
                    break;
                case TT_ZOOM:
                    tt_zoom_state = (int)msg.obj;
                    postInvalidate();
                    break;
                case TT_PAGE_TYPE:
                    tt_page_type = (int)msg.obj;
                    break;
                case TT_NO_SIGAL:
                    tt_notify_status = TT_NOTIFY_NOSIG;
                    postInvalidate();
                    break;
                default:
                    Log.e(TAG, "Wrong message what");
                    break;
            }
        }
    };

    public void saveJsonStr(String str) {
        if (activeView != this)
            return;

        if (!TextUtils.isEmpty(str)) {
            handler.obtainMessage(JSON_MSG_NORMAL, str).sendToTarget();
        }
    }

    public void updateData(String json) {
        if (mSubtitleDataListener != null) {
            mSubtitleDataListener.onSubtitleData(json);
        }
    }

    public String readSysFs(String name) {
        String value = null;
        if (mSubtitleDataListener != null) {
            value = mSubtitleDataListener.onReadSysFs(name);
        }
        return value;
    }

    public void writeSysFs(String name, String cmd) {
        if (mSubtitleDataListener != null) {
            mSubtitleDataListener.onWriteSysFs(name, cmd);
        }
    }

    public void setSubtitleDataListener(SubtitleDataListener l) {
        mSubtitleDataListener = l;
    }

    public interface SubtitleDataListener {
        public void onSubtitleData(String json);
        public String onReadSysFs(String node);
        public void onWriteSysFs(String node, String value);
    }

}

