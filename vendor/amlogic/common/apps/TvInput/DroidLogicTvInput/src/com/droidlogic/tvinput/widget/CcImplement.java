/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.widget;

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

/**
 * Created by daniel on 16/10/2017.
 */
public class CcImplement {
    public final String TAG = "CcImplement";
    CaptionScreen caption_screen;
    CcSetting cc_setting;
    private Paint background_paint;
    private Paint text_paint;
    private Paint window_paint;
    private Paint fade_paint;
    private Paint wipe_paint;
    private Paint shadow_paint;
    private boolean style_broadcast_use_database;

    private Path path1;
    private Path path2;

    private final int EDGE_SIZE_PERCENT = 15;
    private Context context;

    private Typeface undef_tf;
    private Typeface undef_it_tf;
    private Typeface mono_serif_tf;
    private Typeface mono_serif_it_tf;
    private Typeface serif_tf;
    private Typeface serif_it_tf;
    private Typeface mono_no_serif_tf;
    private Typeface mono_no_serif_it_tf;
    private Typeface no_serif_tf;
    private Typeface no_serif_it_tf;
    private Typeface casual_tf;
    private Typeface casual_it_tf;
    private Typeface cursive_tf;
    private Typeface cursive_it_tf;
    private Typeface small_capital_tf;
    private Typeface small_capital_it_tf;
    private Typeface prop_sans_tf;
    private Typeface prop_sans_it_tf;

    PorterDuffXfermode porter_src;
    PorterDuffXfermode porter_add;
    PorterDuffXfermode porter_clear;
    PorterDuffXfermode porter_screen;

    CcImplement(Context context, CustomFonts cf) {
        /* TODO: how to fetch this setting? No trigger in tv input now */
        this.context = context;
        cc_setting = new CcSetting();
        caption_screen = new CaptionScreen();
        window_paint = new Paint();
        background_paint = new Paint();
        text_paint = new Paint();
        fade_paint = new Paint();
        wipe_paint = new Paint();
        shadow_paint = new Paint();
        path1 = new Path();
        path2 = new Path();
        style_broadcast_use_database = true;

        porter_add = new PorterDuffXfermode(PorterDuff.Mode.ADD);
        porter_clear = new PorterDuffXfermode(PorterDuff.Mode.CLEAR);
        porter_src = new PorterDuffXfermode(PorterDuff.Mode.SRC);
        porter_screen = new PorterDuffXfermode(PorterDuff.Mode.SCREEN);

        if (cf != null) {
            mono_serif_tf = cf.mono_serif_tf;
            mono_serif_it_tf = cf.mono_serif_it_tf;
            casual_tf = cf.casual_tf;
            casual_it_tf = cf.casual_it_tf;
            prop_sans_tf = cf.prop_sans_tf;
            prop_sans_it_tf = cf.prop_sans_it_tf;
            small_capital_tf = cf.small_capital_tf;
            small_capital_it_tf = cf.small_capital_it_tf;
            cursive_tf = cf.cursive_tf;
            cursive_it_tf = cf.cursive_it_tf;
        }
    }

    void resetCi()
    {
        window_paint.reset();
        background_paint.reset();
        text_paint.reset();
        fade_paint.reset();
        wipe_paint.reset();
        shadow_paint.reset();
    }

    boolean isStyle_use_broadcast()
    {
        int style_setting = 0;
        if (style_broadcast_use_database = true) {
            try {
                style_setting = Settings.Secure.getInt(context.getContentResolver(), "accessibility_captioning_style_enabled");
            } catch (Settings.SettingNotFoundException e) {
                Log.w(TAG, e.toString());
                style_setting = 0;
                style_broadcast_use_database = false;
            }
        }
        /* 0 for broadcast 1 for using caption manager */
        if (style_setting == 1)
            return false;
        else
            return true;
    }

    private int convertCcColor(int CcColor)
    {
        int convert_color;
        convert_color = (CcColor&0x3)*85 |
            (((CcColor&0xc)>>2)*85) << 8 |
            (((CcColor&0x30)>>4)*85) << 16 |
            0xff<<24;
        return convert_color;
    }

    private Typeface getTypefaceFromString(String font_face, boolean italics) {
        Typeface convert_face;
        //Log.e(TAG, "font_face " + font_face);
        if (font_face.equalsIgnoreCase("default")) {
            if (italics)
                convert_face = mono_serif_it_tf;
            else
                convert_face = mono_serif_tf;
        } else if (font_face.equalsIgnoreCase("mono_serif")) {
            if (italics)
                convert_face = mono_serif_it_tf;
            else
                convert_face = mono_serif_tf;
        } else if (font_face.equalsIgnoreCase("prop_serif")) {
            if (italics)
                convert_face = prop_sans_it_tf;
            else
                convert_face = prop_sans_tf;

        } else if (font_face.equalsIgnoreCase("mono_sans")) {
            if (italics)
                convert_face = mono_serif_it_tf;
            else
                convert_face = mono_serif_tf;
        } else if (font_face.equalsIgnoreCase("prop_sans")) {
            if (italics)
                convert_face = prop_sans_it_tf;
            else
                convert_face = prop_sans_tf;
        } else if (font_face.equalsIgnoreCase("casual")) {
            if (italics)
                convert_face = casual_it_tf;
            else
                convert_face = casual_tf;
        } else if (font_face.equalsIgnoreCase("cursive")) {
            if (italics)
                convert_face = cursive_it_tf;
            else
                convert_face = cursive_tf;
        } else if (font_face.equalsIgnoreCase("small_caps")) {
            if (italics)
                convert_face = small_capital_it_tf;
            else
                convert_face = small_capital_tf;
        }
        /* For caption manager convert */
        else if (font_face.equalsIgnoreCase("sans-serif")) {
            convert_face = prop_sans_tf;
        } else if (font_face.equalsIgnoreCase("sans-serif-condensed")) {
            convert_face = prop_sans_tf;
        } else if (font_face.equalsIgnoreCase("sans-serif-monospace")) {
            convert_face = mono_serif_tf;
        } else if (font_face.equalsIgnoreCase("serif")) {
            convert_face = prop_sans_tf;
        } else if (font_face.equalsIgnoreCase("serif-monospace")) {
            convert_face = mono_serif_tf;
        } else if (font_face.equalsIgnoreCase("casual")) {
            convert_face = casual_tf;
        } else if (font_face.equalsIgnoreCase("cursive")) {
            convert_face = mono_serif_tf;
        } else if (font_face.equalsIgnoreCase("small-capitals")) {
            convert_face = small_capital_tf;
        } else {
            Log.w(TAG, "font face exception " + font_face);
            if (italics)
                convert_face = mono_serif_it_tf;
            else
                convert_face = mono_serif_tf;
        }
        return convert_face;
    }

    private boolean isFontfaceMono(String font_face) {
        if (font_face.equalsIgnoreCase("default")) {
            return true;
        } else if (font_face.equalsIgnoreCase("mono_serif")) {
            return true;
        } else if (font_face.equalsIgnoreCase("prop_serif")) {
            return true;
        } else if (font_face.equalsIgnoreCase("mono_sans")) {
            return true;
        } else if (font_face.equalsIgnoreCase("prop_sans")) {
            return false;
        } else if (font_face.equalsIgnoreCase("casual")) {
            return false;
        } else if (font_face.equalsIgnoreCase("cursive")) {
            return false;
        } else if (font_face.equalsIgnoreCase("small_caps")) {
            return false;
        }
        /* For caption manager convert */
        else if (font_face.equalsIgnoreCase("sans-serif")) {
            return false;
        } else if (font_face.equalsIgnoreCase("sans-serif-condensed")) {
            return false;
        } else if (font_face.equalsIgnoreCase("sans-serif-monospace")) {
            return true;
        } else if (font_face.equalsIgnoreCase("serif")) {
            return false;
        } else if (font_face.equalsIgnoreCase("serif-monospace")) {
            return true;
        } else if (font_face.equalsIgnoreCase("casual")) {
            return false;
        } else if (font_face.equalsIgnoreCase("cursive")) {
            return true;
        } else if (font_face.equalsIgnoreCase("small-capitals")) {
            return false;
        } else {
            return true;
        }
    }


    public class CaptionScreen
    {
        int width;
        int height;
        double safe_title_left;
        double safe_title_right;
        double safe_title_top;
        double safe_title_buttom;
        double safe_title_width;
        double safe_title_height;
        final double safe_title_w_percent = 0.80;
        final double safe_title_h_percent = 0.85;

        int video_left;
        int video_right;
        int video_top;
        int video_bottom;
        int video_height;
        int video_width;

        double video_h_v_rate_on_screen;
        int video_h_v_rate_origin;
        double h_v_rate;

        double screen_left;
        double screen_right;

        int anchor_vertical;
        int anchor_horizon;
        double anchor_vertical_density;
        double anchor_horizon_density;
        double max_font_height;
        double max_font_width;
        double max_font_size;
        double fixed_char_width;
        float window_border_width;
        final int cc_row_count = 15;
        final int cc_col_count = 32;

        void updateCaptionScreen(int w, int h)
        {
            width = w;
            height = h;
            h_v_rate = (double)width/(double)height;
            updateLayout();
        }

        //Must called before calling updateCaptionScreen
        void updateVideoPosition(String ratio, String screen_mode, String video_status)
        {
            try {
                String hs_str = video_status.split("VPP_hsc_startp 0x")[1].split("\\.")[0];
                String he_str = video_status.split("VPP_hsc_endp 0x")[1].split("\\.")[0];
                String vs_str = video_status.split("VPP_vsc_startp 0x")[1].split("\\.")[0];
                String ve_str = video_status.split("VPP_vsc_endp 0x")[1].split("\\.")[0];

                video_left = Integer.valueOf(hs_str, 16);
                video_right = Integer.valueOf(he_str, 16);
                video_top = Integer.valueOf(vs_str, 16);
                video_bottom = Integer.valueOf(ve_str, 16);
                video_height = video_bottom - video_top;
                video_width = video_right - video_left;

                video_h_v_rate_on_screen = (double)(video_right - video_left) / (double)(video_bottom - video_top);
                Log.i(TAG, "position: "+ video_left + " " + video_right + " " + video_top + " " + video_bottom + " " + video_h_v_rate_on_screen);
            } catch (Exception e) {
                Log.d(TAG, "position exception " + e.toString());
            }
            try {
                video_h_v_rate_origin = Integer.valueOf(ratio.split("0x")[1], 16);
//                Log.d(TAG, "ratio: " + video_h_v_rate_origin);
            } catch (Exception e) {
                //0x90 means 16:9 a fallback value
                video_h_v_rate_origin = 0x90;
                Log.d(TAG, "ratio exception " + e.toString());
            }

        }

        void updateLayout()
        {
            //Safe title must be calculated using video width and height.
            safe_title_height = height * safe_title_h_percent;
            if (video_h_v_rate_on_screen > 1.7)
                safe_title_width = width * safe_title_w_percent;
            else
                safe_title_width = width * 12 / 16 * safe_title_w_percent;

            //Font height is relative with safe title height, but now ratio of width and height can not be changed.
            max_font_height = safe_title_height / cc_row_count;
            if (video_h_v_rate_on_screen > 1.7)
                max_font_width = max_font_height * 0.8;
            else
                max_font_width = max_font_height * 0.6;

            max_font_size = max_font_height;

            //This is used for postioning character in 608 mode.
            fixed_char_width = safe_title_width / (cc_col_count + 1);

            anchor_horizon = ((video_h_v_rate_origin & 1) == 0)?210:160; //16:9 or 4:3
            anchor_vertical = 75;

            //This is used for calculate coordinate in non-relative anchor mode
            anchor_horizon_density = safe_title_width / anchor_horizon;
            anchor_vertical_density = safe_title_height / anchor_vertical;

            window_border_width = (float)(max_font_height/6);
            safe_title_left = (width - safe_title_width)/2;
            safe_title_right = safe_title_left + safe_title_width;
            safe_title_top = (height - safe_title_height)/2;
            safe_title_buttom = safe_title_top + safe_title_height;
            screen_left = caption_screen.getWindowLeftTopX(true, 0, 0, 0);
            screen_right = caption_screen.getWindowLeftTopY(true, 0, 0, 0);
//            Log.i(TAG, "updateCaptionScreen " + " " + screen_left+ " " + screen_right
//                    +video_h_v_rate_on_screen + ' '+anchor_horizon+' '+anchor_horizon_density);
        }

        double getWindowLeftTopX(boolean anchor_relative, int anchor_h, int anchor_point, double row_length)
        {
            double offset;
            /* Get anchor coordinate x */
            if (!anchor_relative)
                /* anchor_h is horizontal steps */
                offset = safe_title_width * anchor_h / anchor_horizon + safe_title_left;
            else
                /* anchor_h is percentage */
                offset = safe_title_width * anchor_h / 100 + safe_title_left;
//            Log.i(TAG,
//                    "Window anchor relative " + anchor_relative +
//                            " horizon density " + anchor_horizon_density +
//                            " h " + anchor_h + " point " + anchor_point + " " + width + " safe width " + safe_title_width +
//                            " row_length " + row_length + " offset " + offset);
            switch (anchor_point)
            {
                case 0:
                case 3:
                case 6:
                    return offset;
                case 1:
                case 4:
                case 7:
                    return offset - row_length/2;
                case 2:
                case 5:
                case 8:
                    return offset - row_length;
                default:
                    return -1;
            }
        }

        double getWindowLeftTopY(boolean anchor_relative, int anchor_v, int anchor_point, int row_count)
        {
            double offset;
            double position;

            if (!anchor_relative)
                /* anchor_v is vertical steps */
                offset = safe_title_height * anchor_v / anchor_vertical + safe_title_top;
            else
                /* anchor_v is percentage */
                offset = safe_title_height * anchor_v / 100 + safe_title_top;

//             Log.i(TAG,
//                    "Window anchor relative " + anchor_relative +
//                    " vertical density "+anchor_horizon_density +
//                    " v "+anchor_v+
//                    " point " + anchor_point +
//                    " " + height);

            switch (anchor_point)
            {
                case 0:
                case 1:
                case 2:
                    position = offset;
                    break;
                case 3:
                case 4:
                case 5:
                    position = offset - (row_count * max_font_height)/2;
                    break;
                case 6:
                case 7:
                case 8:
                    position = offset - row_count * max_font_height;
                    break;
                default:
                    position = safe_title_top - row_count * max_font_height;
                    break;
            }

            if ((position) < safe_title_top)
                position = safe_title_top;
            else if ((position + row_count * max_font_height) > safe_title_buttom)
                position = safe_title_buttom - row_count * max_font_height;
            return position;
        }
    }

    class CcSetting {
        Locale cc_locale;
        float font_scale;
        boolean is_enabled;
        Typeface type_face;
        boolean has_background_color;
        boolean has_edge_color;
        boolean has_edge_type;
        boolean has_foreground_color;
        boolean has_window_color;
        int foreground_color;
        int foreground_opacity;
        int window_color;
        int window_opacity;
        int background_color;
        int background_opacity;
        int edge_color;
        int edge_type;
        int stroke_width;
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
                    cc_locale = cm.getLocale();
                    font_scale = cm.getFontScale();
                    is_enabled = cm.isEnabled();
                    // Log.e(TAG, "Caption manager enable: " + is_enabled);
                    stroke_width = 0;
                    /* When use preset setting of cc, such as black on white,
                     * font face will be null */
                    //type_face = cs.getTypeface();
                    /* We need to find out the selected fontface name ourself,
                     * and load the face from local area */
                    // Log.e(TAG, "typeface name " + Settings.Secure.getString(context.getContentResolver(),
                    //            "accessibility_captioning_typeface"));
                    type_face = cs.getTypeface();
                    has_background_color = cs.hasBackgroundColor();
                    has_edge_color = cs.hasEdgeColor();
                    has_edge_type = cs.hasEdgeType();
                    has_foreground_color = cs.hasForegroundColor();
                    has_window_color = cs.hasWindowColor();
                    foreground_color = cs.foregroundColor;
                    foreground_opacity = foreground_color >>> 24;
                    window_color = cs.windowColor;
                    window_opacity = window_color >>> 24;
                    background_color = cs.backgroundColor;
                    background_opacity = background_color >>> 24;
                    edge_color = cs.edgeColor;
                    edge_type = cs.edgeType;
                }
            }
            // dump();
        }
        void dump()
        {
            Log.i(TAG, "enable "+ is_enabled +
                    " locale " + cc_locale +
                    " font_scale " + font_scale +
                    " stroke_width " + stroke_width +
                    " type_face " + type_face +
                    " has_background_color " + has_background_color +
                    " has_edge_color " + has_edge_color +
                    " has_edge_type " + has_edge_color +
                    " has_foreground_color " + has_foreground_color +
                    " has_window_color " + has_window_color +
                    " foreground_color " + Integer.toHexString(foreground_color) +
                    " foreground_opacity " + foreground_opacity +
                    " window_color " + Integer.toHexString(window_color) +
                    " window_opacity " + window_opacity +
                    " background_color " + Integer.toHexString(background_color) +
                    " background_opacity " + background_opacity +
                    " edge_color " + Integer.toHexString(edge_color) +
                    " edge_type " + edge_type);
        }
    }

    class CaptionWindow
    {
        JSONObject ccObj = null;
        JSONArray windowArr = null;
        String ccVersion;
        int windows_count;
        Window windows[];
        boolean init_flag;
        boolean style_use_broadcast;
        String ratio;
        String screen_mode;
        String video_status;
        void UpdatePositioning(String in_ratio, String in_screen_mode, String in_video_status)
        {
            ratio = in_ratio;
            screen_mode = in_screen_mode;
            video_status = in_video_status;
            Log.e(TAG, "UpdateWindowPosition ratio: " + in_ratio + " screen_mode: " + in_screen_mode + " " +
                    "video_status: " + in_video_status);
        }

        CaptionWindow() {
            windows = new Window[8];
            for (int i=0; i<8; i++)
                windows[i] = new Window();
        }

        void updateCaptionWindow(String jsonStr)
        {
            caption_screen.updateVideoPosition(ratio, screen_mode, video_status);
            caption_screen.updateLayout();
            init_flag = false;
            try {
                if (!TextUtils.isEmpty(jsonStr))
                    ccObj = new JSONObject(jsonStr);
                else {
                    init_flag = false;
                    return;
                }
                ccVersion = ccObj.getString("type");
                if (ccVersion.matches("cea608"))
                {
                    windowArr = ccObj.getJSONArray("windows");
                    windows_count = windowArr.length();
                    //Log.e(TAG, "ccType 608" + " window number: " + windows_count);
                    for (int i=0; i<windows_count; i++)
                        windows[i].updateWindow(windowArr.getJSONObject(i));
                }
                else if (ccVersion.matches("cea708"))
                {
                    windowArr = ccObj.getJSONArray("windows");
                    windows_count = windowArr.length();
                    //Log.e(TAG, "ccType 708" + " window number: " + windows_count);
                    for (int i=0; i<windows_count; i++)
                        windows[i].updateWindow(windowArr.getJSONObject(i));
                }
                else {
                    init_flag = false;
                    Log.d(TAG, "ccType unknown");
                    return;
                }

                if (windows_count == 0) {
                    init_flag = false;
                    return;
                }
            }
            catch (JSONException e)
            {
                Log.e(TAG, "Window init failed, exception: " + e.toString());
                init_flag = false;
                return;
            }
            init_flag = true;
        }

        class Window
        {
            int anchor_point;
            int anchor_v;
            int anchor_h;
            boolean anchor_relative;
            int row_count;
            int col_count;

            boolean row_lock;
            boolean column_lock;
            String justify;
            String print_direction;
            String scroll_direction;
            boolean wordwrap;
            int effect_speed;
            String fill_opacity;
            int fill_color;
            String border_type;
            int border_color;
            double pensize_window_depend;

            String display_effect;
            String effect_direction;
            String effect_status;
            int effect_percent;

            final double window_edge_rate = 0.15;
            double window_edge_width;

            double window_width;
            double window_left_most;
            double window_start_x;
            double window_start_y;

            double window_left;
            double window_top;
            double window_bottom;
            double window_right;
            double row_length;
            int heart_beat;

            double window_max_font_size;

            JSONArray json_rows;
            int fill_opacity_int;

            Rows rows[];
            //Temp use system property

            Window() {
                rows = new Rows[16];
                for (int i=0; i<16; i++)
                    rows[i] = new Rows();
            }
            void windowReset()
            {
                row_count = 0;
                col_count = 0;
                effect_percent = 0;
                window_edge_width = 0;
                window_width = 0;
                window_left_most = 10000;
                window_start_x = 0;
                window_start_y = 0;
                window_left = 0;
                window_top = 0;
                window_bottom = 0;
                window_right = 0;
                row_length = 0;
                window_max_font_size = 0;
                pensize_window_depend = 0;
            }
            void updateWindow(JSONObject windowStr) {
                windowReset();
                window_edge_width = (float)(caption_screen.max_font_height * window_edge_rate);
                try {
                    anchor_point = windowStr.getInt("anchor_point");
                    anchor_v = windowStr.getInt("anchor_vertical");
                    anchor_h = windowStr.getInt("anchor_horizontal");
                    anchor_relative = windowStr.getBoolean("anchor_relative");
                    row_count = windowStr.getInt("row_count") ;
                    col_count = windowStr.getInt("column_count");
                    row_lock = windowStr.getBoolean("row_lock");
                    column_lock = windowStr.getBoolean("column_lock");
                    justify = windowStr.getString("justify");
                    print_direction = windowStr.getString("print_direction");
                    scroll_direction = windowStr.getString("scroll_direction");
                    wordwrap = windowStr.getBoolean("wordwrap");
                    display_effect = windowStr.getString("display_effect");
                    effect_direction = windowStr.getString("effect_direction");
                    effect_speed = windowStr.getInt("effect_speed");
                    heart_beat = windowStr.getInt("heart_beat");
                } catch (Exception e) {
                    Log.e(TAG, "window property exception " + e.toString());
                    init_flag = false;
                }

                if (!style_use_broadcast && ccVersion.matches("cea708"))
                {
                    fill_opacity_int = cc_setting.window_opacity;
                    fill_color = cc_setting.window_color;
                    border_type = "none";

                } else {
                    if (ccVersion.matches("cea708")) {
                        try {
                            effect_percent = windowStr.getInt("effect_percent");
                            effect_status = windowStr.getString("effect_status");
                        } catch (Exception e) {
                            effect_percent = 100;
                            effect_status = null;
                            Log.e(TAG, "window effect attr exception: " + e.toString());
                        }
                    }
                    try {
                        fill_opacity = windowStr.getString("fill_opacity");
                        fill_color = windowStr.getInt("fill_color");
                        border_type = windowStr.getString("border_type");
                        border_color = windowStr.getInt("border_color");
                    } catch (Exception e) {
                        Log.e(TAG, "window style exception " + e.toString());
                    }

                        /* Value from stream need to be converted */
                    fill_color = convertCcColor(fill_color);
                    border_color = convertCcColor(border_color);

                    if (fill_opacity.equalsIgnoreCase("solid")) {
                        fill_opacity_int = 0xff;
                    } else if (fill_opacity.equalsIgnoreCase("transparent")){
                        fill_opacity_int = 0;
                    } else if (fill_opacity.equalsIgnoreCase("translucent")){
                        fill_opacity_int = 0x80;
                    } else if (fill_opacity.equalsIgnoreCase("flash")) {
                        if (heart_beat == 0)
                            fill_opacity_int = 0;
                        else
                            fill_opacity_int = 0xff;
                    } else
                        fill_opacity_int = 0xff;
                }
                try {
                    json_rows = windowStr.getJSONArray("rows");
                } catch (Exception e) {
                    Log.e(TAG, "json fatal exception "+ e.toString());
                    init_flag = false;
                }
                //Log.e(TAG, "json_rows: " + json_rows.toString());

                if (row_count > json_rows.length())
                    Log.i(TAG, "window loses "+ (row_count - json_rows.length()) + " rows");

                    window_max_font_size = caption_screen.safe_title_width / 32;
                    window_width = col_count * window_max_font_size;
                    /* ugly repeat */
                    for (int i=0; i<row_count; i++) {
                        try {
                            rows[i].updateRows(new JSONObject(json_rows.optString(i)));
                        } catch (Exception e) {
                            Log.e(TAG, "json rows construct exception " + e.toString());
                            init_flag = false;
                        }
                        rows[i].row_number_in_window = i;
                        row_length = rows[i].row_length_on_paint;
                        //Log.e(TAG, "Row right most: " + i + " " + row_length);
                        /* window_width = (window_width > row_length)
                           ? window_width : row_length; */
                        //window_width = col_count * caption_screen.max_font_width;
                        window_left_most = rows[i].row_start_x < window_left_most ?
                            rows[i].row_start_x : window_left_most;
                    }

                    //window_left_most *= caption_screen.max_font_width;
                    window_left_most *= pensize_window_depend;
                    //max_row_str_length = col_count * caption_screen.max_font_width;
                    //Log.e(TAG, "Max row length "+ window_width);
                    window_start_x = caption_screen.getWindowLeftTopX(anchor_relative, anchor_h, anchor_point, window_width);

                    window_start_y = caption_screen.getWindowLeftTopY(anchor_relative, anchor_v, anchor_point, row_count);

//                dump();
            }

            void draw(Canvas canvas)
            {
                /* Draw window */
                if (ccVersion.equalsIgnoreCase("cea708")) {
                    double columns_width;
                    columns_width = col_count * caption_screen.max_font_width;
                    //window_left = window_start_x - window_edge_width + window_left_most;
                    window_left = window_start_x;
                    window_top = window_start_y;
                    /* Use columns count to get window right margin */
                    window_right = window_start_x + window_width;
                    window_bottom = window_start_y + caption_screen.max_font_height * row_count;

                    window_paint.setXfermode(porter_src);
                    /* Draw border */
                    /* Draw border color */
                    draw_border(canvas, window_paint, shadow_paint, border_type,
                            (float)window_left, (float)window_top, (float)window_right, (float)window_bottom,
                            border_color);

                    /* Draw window */
                    window_paint.setColor(fill_color);
                    window_paint.setAlpha(fill_opacity_int);
                    //Log.e(TAG, "window rect: "+ " color " + fill_color + " opacity "+fill_opacity_int);
                }
                /* This is only for 608 text mode, and the window is background */
                else {
                    window_left = window_start_x;
                    window_right = window_start_x + caption_screen.safe_title_width;
                    window_top = window_start_y;
                    window_bottom = window_start_y + caption_screen.safe_title_height;
                    window_paint.setColor(fill_color);
                    window_paint.setAlpha(fill_opacity_int);
                }
                canvas.drawRect((float) window_left, (float) window_top, (float) window_right, (float) window_bottom, window_paint);
//                Log.e(TAG, "window rect " + window_left + " " + window_right + " " + window_top + " " + window_bottom);


                /* Draw rows */
                for (int i=0; i<row_count; i++) {
                    if (rows[i].rowArray.length() != 0)
                        rows[i].draw(canvas);
                }

                if (ccVersion.equalsIgnoreCase("cea708")) {
                    double rect_left, rect_right, rect_top, rect_bottom;
                    float border = caption_screen.window_border_width;
                    if (display_effect.equalsIgnoreCase("fade")) {
                        fade_paint.setColor(Color.WHITE);
                        fade_paint.setAlpha(effect_percent*255/100);
                        fade_paint.setXfermode(porter_screen);
                        canvas.drawRect((float)window_left - border,
                                (float)window_top - border,
                                (float)window_right + border,
                                (float)window_bottom + border,
                                fade_paint);

                    } else if (display_effect.equalsIgnoreCase("wipe")) {
                        if (effect_direction.equalsIgnoreCase("left_right")) {
                            rect_left = window_start_x - border;
                            rect_right = window_start_x + window_width * effect_percent /100 + border;
                            rect_top = window_top - border;
                            rect_bottom = window_bottom + border;
                        } else if (effect_direction.equalsIgnoreCase("right_left")) {
                            rect_left = window_start_x + window_width * (100 - effect_percent)/100 - border;
                            rect_right = window_start_x + window_width + border;
                            rect_top = window_top - border;
                            rect_bottom = window_bottom + border;
                        } else if (effect_direction.equalsIgnoreCase("top_bottom")) {
                            rect_left = window_start_x - border;
                            rect_right = window_start_x + window_width + border;
                            rect_top = window_top - border;
                            rect_bottom = window_top + (window_bottom - window_top) * effect_percent/100 + border;
                        } else if (effect_direction.equalsIgnoreCase("bottom_top")) {
                            rect_left = window_start_x - border;
                            rect_right = window_start_x + window_width + border;
                            rect_top = window_top + (window_bottom - window_top) * (100 - effect_percent)/100 - border;
                            rect_bottom = window_bottom + border;
                        } else {
                            rect_left = 0;
                            rect_bottom = 0;
                            rect_top = 0;
                            rect_right = 0;
                        }
                        wipe_paint.setColor(Color.WHITE);
                        wipe_paint.setAlpha(0);
                        wipe_paint.setXfermode(porter_clear);
                        canvas.drawRect((float)rect_left,
                                (float)rect_top,
                                (float)rect_right,
                                (float)rect_bottom,
                                wipe_paint);
                    }
                }
            }
            void draw_border(Canvas canvas, Paint border_paint, Paint shadow_paint, String border_type,
                             float l, float t, float r, float b,
                             int border_color)
            {
                float gap = (float)(caption_screen.window_border_width);
                //Log.e(TAG, "Border type " + border_type + " left " + l + " right " + r + " top " + t + " bottom " + b);
                shadow_paint.setColor(Color.GRAY);
                shadow_paint.setAlpha(0x90);
                if (border_type.equalsIgnoreCase("none")) {
//                     We do nothing??
                }
                if (border_type.equalsIgnoreCase("raised") ||
                        border_type.equalsIgnoreCase("depressed")) {

                    float og = (float)(gap * 0.6);
                    int left_top_color, right_bottom_color;
                    border_paint.setStyle(Paint.Style.FILL);
                    path1.reset();
                    path2.reset();

                    // Left top
                    border_paint.setColor(border_color);
                    if (border_type.equalsIgnoreCase("raised")) {
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

                        canvas.drawPath(path1, border_paint);
                        canvas.drawPath(path2, border_paint);
                    //Top
                        canvas.drawRect(l - og, t - og, r + og, t, border_paint);

                    } else if (border_type.equalsIgnoreCase("depressed")) {
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

                        canvas.drawPath(path1, border_paint);
                        canvas.drawPath(path2, border_paint);
                        canvas.drawRect(l - og, b, r + og, b + og, border_paint);
                    }
                }
                if (border_type.equalsIgnoreCase("uniform")) {
                    window_paint.setColor(border_color);
                    canvas.drawRect(l-gap, t-gap, r+gap, b+gap, window_paint);
                }
                if (border_type.equalsIgnoreCase("shadow_left")) {
                    window_paint.setColor(border_color);
                    canvas.drawRect(l-gap, t+gap, r-gap, b+gap, window_paint);
                }
                if (border_type.equalsIgnoreCase("shadow_right"))
                {
                    window_paint.setColor(border_color);
                    canvas.drawRect(l+gap, t+gap, r+gap, b+gap, window_paint);
                }
            }

            void dump()
            {
                Log.i(TAG, "Window attr: " +
                        " anchor_point " + anchor_point +
                        " anchor_v " + anchor_v +
                        " anchor_h " + anchor_h +
                        " anchor_relative " + anchor_relative +
                        " row_count " + row_count +
                        " col_count " + col_count +
                        " row_lock " + row_lock +
                        " column_lock " + column_lock +
                        " justify " + justify +
                        " print_direction " + print_direction +
                        " scroll_direction " + scroll_direction +
                        " wordwrap " + wordwrap +
                        " display_effect " + display_effect +
                        " effect_direction " + effect_direction +
                        " effect_speed " + effect_speed +
                        " effect_percent " + effect_percent +
                        " fill_opacity " + fill_opacity +
                        " fill_color " + fill_color +
                        " border_type " + border_type +
                        " border_color " + border_color +
                        " window_length " + window_width +
                        " window_start_x " + window_start_x +
                        " window_start_y " + window_start_y +
                        " width " + caption_screen.width +
                        " height " + caption_screen.height);
            }

            class Rows
            {
                int str_count;
                RowStr rowStrs[];
                JSONArray rowArray;
                /* Row length is sum of each string */
                double row_length_on_paint;
                double row_start_x;
                double row_start_y;
                int row_number_in_window;
                int row_characters_count;
                double prior_str_position_for_draw;
                /* This is for full justification use */
                double character_gap;
                double row_max_font_size;

                Rows() {
                    rowStrs = new RowStr[6];
                    for (int i=0; i<6; i++)
                        rowStrs[i] = new RowStr();
                }
                void updateRows(JSONObject rows)
                {
                    prior_str_position_for_draw = -1;
                    row_characters_count = 0;
                    row_max_font_size = 0;
                    row_length_on_paint = 0;
                    row_start_x = 0;
                    row_start_y = 0;
                    try {
                        rowArray = rows.optJSONArray("content");
                        row_start_x = rows.optInt("row_start");
                        str_count = rowArray.length();
                        double single_char_width = ccVersion.matches("cea708") ?
                                window_max_font_size : caption_screen.fixed_char_width;
                        for (int i=0; i<str_count; i++) {
                            rowStrs[i].updateRowStr(rowArray.getJSONObject(i));
                            //Every string starts at prior string's tail
                            rowStrs[i].str_start_x = row_length_on_paint + row_start_x * single_char_width;
                            row_characters_count += rowStrs[i].str_characters_count;
                            row_length_on_paint += rowStrs[i].string_length_on_paint;
                            double str_max_font_size = rowStrs[i].max_single_font_width;
                            row_max_font_size = (str_max_font_size > row_max_font_size)
                                ?str_max_font_size:row_max_font_size;
                        }
                        character_gap = window_width/row_characters_count;
                    } catch (JSONException e) {
                        Log.w(TAG, "Str exception: " + e.toString());
                        row_length_on_paint = 0;
                        init_flag = false;
                    }
                }

                void draw(Canvas canvas)
                {
                    if (row_length_on_paint == 0 || str_count == 0)
                        return;
                    for (int i=0; i<str_count; i++)
                        rowStrs[i].draw(canvas);
                }

                class RowStr{
                    /* For parse json use */
                    boolean italics;
                    boolean underline;
                    int edge_color;
                    int fg_color;
                    int bg_color;
                    String pen_size;
                    String font_style;
                    String offset;
                    String edge_type;
                    String fg_opacity;
                    String bg_opacity;
                    String data;
                    double string_length_on_paint;
                    /* TODO: maybe there is more efficient way to do this */
                    double max_single_font_width;
                    double str_start_x;
                    double str_left;
                    double str_top;
                    double str_right;
                    double str_bottom;
                    double font_size;
                    int str_characters_count;
                    Paint.FontMetricsInt fontMetrics;


                    /* below is the actual parameters we used */
                    int fg_opacity_int = 0xff;
                    int bg_opacity_int = 0xff;
                    double font_scale;
                    Typeface font_face;
                    double edge_width;
                    boolean use_caption_manager_style;

                    RowStr(){}
                    void updateRowStr(JSONObject rowStr)
                    {
                        string_length_on_paint = 0;
                        /* Get parameters from json */
                        if (!style_use_broadcast && ccVersion.matches("cea708")) {
                            use_caption_manager_style = true;
                            /* Get parameters from Caption manager */
                            /* Retreat font scale: 0.5 1 1.5 2 */
                            //font_scale = (cc_setting.font_scale - 2) / 5 + 1;
                            font_scale = cc_setting.font_scale/(2/0.8);
                            font_size = caption_screen.max_font_size;

                            switch (cc_setting.edge_type)
                            {
                                case 0:
                                    edge_type = "none";
                                    break;
                                case 1:
                                    /* Uniform is outline */
                                    edge_type = "uniform";
                                    break;
                                case 2:
                                    edge_type = "shadow_right";
                                    break;
                                case 3:
                                    edge_type = "raised";
                                    break;
                                case 4:
                                    edge_type = "depressed";
                                    break;
                                default:
                                    Log.w(TAG, "Edge not supported: " + cc_setting.edge_type);
                            }
                            edge_color = cc_setting.edge_color;
                            fg_color = cc_setting.foreground_color;
                            fg_opacity_int = cc_setting.foreground_opacity;
                            bg_color = cc_setting.background_color;
                            bg_opacity_int = cc_setting.background_opacity;
                            font_style = "mono_serif";
                            offset = "normal";
                            /* Add italic and underline support */
                            italics = false;
                            underline = false;

                            try {
                                data = rowStr.getString("data");
                            } catch (JSONException e) {
                                Log.e(TAG, "Get string failed: " + e.toString());
                                init_flag = false;
                                return;
                            }
                            setDrawerConfig(data, null, font_size, font_scale,
                                    offset,
                                    fg_color, fg_opacity_int,
                                    bg_color, bg_opacity_int,
                                    edge_color, edge_width, edge_type,
                                    italics, underline, use_caption_manager_style);
                        } else {
                            use_caption_manager_style = false;
                            try {
                                pen_size = rowStr.getString("pen_size");
                            } catch (Exception e) {
                                pen_size = "standard";
                            }
                            if (pen_size.equalsIgnoreCase("small")) {
                                this.font_scale = 0.5;
                            } else if (pen_size.equalsIgnoreCase("large")) {
                                this.font_scale = 0.8;
                            } else if (pen_size.equalsIgnoreCase("standard")) {
                                this.font_scale = 0.65;
                            } else {
                                Log.w(TAG, "Font scale not supported: " + pen_size);
                                this.font_scale = 0.8;
                            }

                            try {
                                font_style = rowStr.getString("font_style");
                            } catch (Exception e) {
                                font_style = "default";
                            }

                            try {
                                offset = rowStr.getString("offset");
                            } catch (Exception e) {
                                offset = "normal";
                            }

                            try {
                                italics = rowStr.getBoolean("italics");
                                underline = rowStr.getBoolean("underline");
                                fg_color = rowStr.getInt("fg_color");
                                fg_opacity = rowStr.getString("fg_opacity");
                                bg_color = rowStr.getInt("bg_color");
                                bg_opacity = rowStr.getString("bg_opacity");
                                data = rowStr.getString("data");
                            } catch (Exception e) {
                                italics = false;
                                underline = false;
                                fg_color = 0;
                                fg_opacity = "transparent";
                                bg_color = 0;
                                bg_opacity = "transparent";
                                data = "";
                                Log.e(TAG, "Row str exception: " + e.toString());
                            }

                            if (ccVersion.equalsIgnoreCase("cea708")) {
                                try {
                                    edge_type = rowStr.getString("edge_type");
                                } catch (Exception e) {
                                    edge_type = "none";
                                }

                                try {
                                    edge_color = rowStr.getInt("edge_color");
                                } catch (Exception e) {
                                    edge_color = 0;
                                }
                            }

                            font_size = caption_screen.max_font_size;// * font_scale;

                            fg_color = convertCcColor(fg_color);
                            bg_color = convertCcColor(bg_color);
                            edge_color = convertCcColor(edge_color);

//                              1. Solid --> opacity = 100
//                              2. Transparent --> opacity = 0
//                              3. Translucent --> opacity = 50
//                              4. flashing
                            if (fg_opacity.equalsIgnoreCase("solid")) {
                                fg_opacity_int = 0xff;
                            } else if (fg_opacity.equalsIgnoreCase("transparent")) {
                                fg_opacity_int = 0;
                            } else if (fg_opacity.equalsIgnoreCase("translucent")) {
                                fg_opacity_int = 0x80;
                            } else {
                                Log.w(TAG, "Fg opacity Not supported yet " + fg_opacity);
                            }

                            /* --------------------Background----------------- */
                            if (bg_opacity.equalsIgnoreCase("solid")) {
                                bg_opacity_int = 0xff;
                            } else if (bg_opacity.equalsIgnoreCase("transparent")){
                                bg_opacity_int = 0x0;
                            } else if (bg_opacity.equalsIgnoreCase("translucent")){
                                bg_opacity_int = 0x80;
                            } else if (bg_opacity.equalsIgnoreCase("flash")) {
                                if (heart_beat == 0)
                                    bg_opacity_int = 0;
                                else
                                    bg_opacity_int = 0xff;
                            }
                            setDrawerConfig(data, font_style, font_size, font_scale,
                                    offset,
                                    fg_color, fg_opacity_int,
                                    bg_color, bg_opacity_int,
                                    edge_color, edge_width, edge_type,
                                    italics, underline, use_caption_manager_style);
                        }


                        /* Set parameters */
                        window_paint.setTypeface(font_face);
                        /* Get a largest metric to get the baseline */
                        window_paint.setTextSize((float)caption_screen.max_font_height);
                        fontMetrics = window_paint.getFontMetricsInt();
                        /* Return to normal */
                        window_paint.setTextSize((float)(font_size * font_scale));

                        //Log.e(TAG, "str on paint " + string_length_on_paint + " " + data);
                        edge_width = font_size/EDGE_SIZE_PERCENT;
                        if (pensize_window_depend == 0)
                            pensize_window_depend = window_paint.measureText("H");

                        str_characters_count = data.length();

                    }
                    void setDrawerConfig(String data, String font_face, double font_size, double font_scale,
                            String offset,
                            int fg_color, int fg_opacity,
                            int bg_color, int bg_opacity,
                            int edge_color, double edge_width, String edge_type,
                            boolean italics, boolean underline, boolean use_caption_manager_style)
                    {
                        this.data = data;
                        /* Convert font scale to a logical range */
                        this.font_size = font_size * font_scale;
                        boolean is_monospace = false;

                        if (font_face == null)
                            font_face = "not set";
                        /* Typeface handle:
                         * Temporarily leave caption manager's config, although it is lack of some characters
                         * Now, only switch typeface for stream
                         */
                        if (!use_caption_manager_style) {
                            this.font_face = getTypefaceFromString(font_face, italics);
                            is_monospace = isFontfaceMono(font_face);
                        } else {
                            String cm_fontface_name = Settings.Secure.getString(context.getContentResolver(),
                                    "accessibility_captioning_typeface");
                            if (cm_fontface_name == null)
                                cm_fontface_name = "not set";
                            this.font_face = getTypefaceFromString(cm_fontface_name, false);
                            is_monospace = isFontfaceMono(cm_fontface_name);
                        }

                        this.fg_color = fg_color;
                        this.fg_opacity_int = fg_opacity;
                        this.bg_color = bg_color;
                        this.bg_opacity_int = bg_opacity;
                        this.edge_color = edge_color;
                        this.edge_width = edge_width;
                        this.edge_type = edge_type;
                        this.italics = italics;
                        this.underline = underline;
                        this.offset = offset;

                        window_paint.setTypeface(this.font_face);
                        window_paint.setTextSize((float)this.font_size);
                        // window_paint.setLetterSpacing((float) 0.05);
                        max_single_font_width = window_paint.measureText("_");
                        if (ccVersion.matches("cea708")) {
                            if (is_monospace) {
//                                string_length_on_paint = (data.length() + 1) * max_single_font_width;
                                string_length_on_paint = window_paint.measureText(data);
                            }
                            else
                                string_length_on_paint = window_paint.measureText(data);
//                                string_length_on_paint = window_paint.measureText(data) + max_single_font_width;
//                             string_length_on_paint = (data.length()+1) * max_single_font_width;
                        } else {
//                            string_length_on_paint = text_paint.measureText(data);
//                            string_length_on_paint = data.length() * caption_screen.fixed_char_width;
                            string_length_on_paint = window_paint.measureText(data);
                        }
                        /* Convert */
                        /*
                           Log.e(TAG, "str attr: " +
                           " use_user_style " + use_caption_manager_style +
                           " max_font_height " + caption_screen.max_font_size +
                           " font_size " + this.font_size +
                           " font_scale " + font_scale +
                           " font_style " + font_face +
                           " offset " + this.offset +
                           " italics " + this.italics +
                           " underline " + this.underline +
                           " edge_type " + this.edge_type +
                           " edge_color " + this.edge_color +
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
                        str_top = window_start_y + row_number_in_window * caption_screen.max_font_height;
                        str_bottom = window_start_y + (row_number_in_window + 1) * caption_screen.max_font_height;
                        /* Handle justify here */
                        if (justify.equalsIgnoreCase("left")) {
                            if (prior_str_position_for_draw == -1)
                                prior_str_position_for_draw = window_start_x + str_start_x;
                            str_left = prior_str_position_for_draw;
                            str_right = str_left + string_length_on_paint;
                            prior_str_position_for_draw = str_right;
                        } else if (justify.equalsIgnoreCase("right")) {
                            if (prior_str_position_for_draw == -1)
                                prior_str_position_for_draw = window_start_x + window_width;
                            str_right = prior_str_position_for_draw;
                            str_left = str_right - string_length_on_paint;
                            prior_str_position_for_draw = str_left;
                        } else if (justify.equalsIgnoreCase("full")) {
                            if (prior_str_position_for_draw == -1)
                                prior_str_position_for_draw = window_start_x;
                            str_left = prior_str_position_for_draw;
                            str_right = str_left + character_gap * str_characters_count;
                            prior_str_position_for_draw = str_right;
                            /*
                               Log.e(TAG, "prior " + prior_str_position_for_draw +
                               " character_gap " + character_gap +
                               " str_count " + str_characters_count +
                               " str_left " + str_left +
                               " str_right " + str_right);
                               */
                        } else if (justify.equalsIgnoreCase("center")) {
                            if (prior_str_position_for_draw == -1)
                                prior_str_position_for_draw = (window_width - row_length_on_paint)/2 + window_start_x;
                            str_left = prior_str_position_for_draw;
                            str_right = str_left + string_length_on_paint;
                            prior_str_position_for_draw = str_right;
                        } else {
                            /* default using left justfication */
                            if (prior_str_position_for_draw == -1)
                                prior_str_position_for_draw = window_start_x + str_start_x;
                            str_left = prior_str_position_for_draw;
                            str_right = str_left + string_length_on_paint;
                            prior_str_position_for_draw = str_right;
                        }

                        /* Draw background, a rect, if opacity == 0, skip it */
                        if (str_bottom < caption_screen.safe_title_top + caption_screen.max_font_height)
                            str_bottom = caption_screen.safe_title_top + caption_screen.max_font_height;
                        if (str_top < caption_screen.safe_title_top)
                            str_top = caption_screen.safe_title_top;
                        if (bg_opacity_int != 0) {
                            background_paint.setColor(bg_color);
                            background_paint.setAlpha(bg_opacity_int);
                            if (fill_opacity_int != 0xff)
                                background_paint.setXfermode(porter_src);
                            canvas.drawRect((float) str_left, (float) str_top, (float) str_right, (float) str_bottom, background_paint);
//                            canvas.drawRect((float) str_left, (float) str_top, (float) str_left + text_paint.measureText(data), (float) str_bottom, background_paint);
                        }
                        if (!justify.equalsIgnoreCase("full")) {
                            draw_text(canvas, data, font_face, font_size,
                                    (float) str_left, (float) (str_bottom - fontMetrics.descent),
                                    fg_color, fg_opacity, fg_opacity_int,
                                    underline,
                                    edge_color, (float) edge_width, edge_type, text_paint);
                        } else {
                            double prior_character_position = str_left;
                            for (int i=0; i<data.length(); i++) {
                                draw_text(canvas, "" + data.charAt(i), font_face, font_size,
                                        (float) prior_character_position, (float) (str_bottom - fontMetrics.descent),
                                        fg_color, fg_opacity, fg_opacity_int,
                                        underline,
                                        edge_color, (float) edge_width, edge_type, text_paint);

                                prior_character_position += character_gap;
                            }
                        }

                        /* Draw text */
                        /*Log.e(TAG, "Draw str, " + data +
                          " start x,y: "+(str_start_x+window_start_x) +
                          " " + (row_start_y+window_start_y));
                          */
                    }

                    void draw_str(Canvas canvas, String str, float left, float bottom, Paint paint)
                    {
                        paint.setXfermode(porter_clear);
//                        if (ccVersion.matches("cea708")) {
                            canvas.drawText(str, left, bottom, paint);
//                        } else {
//                            int i, l = str.length();
//                            float x = left;
//
//                            for (i = 0; i < l; i++) {
//                                String sub = str.substring(i, i + 1);
//                                canvas.drawText(sub, x, bottom, paint);
//                                x += caption_screen.fixed_char_width;
//                            }
//                        }
                        paint.setXfermode(porter_add);
//                        if (ccVersion.matches("cea708")) {
                            canvas.drawText(str, left, bottom, paint);
//                        } else {
//                            int i, l = str.length();
//                            float x = left;
//
//                            for (i = 0; i < l; i++) {
//                                String sub = str.substring(i, i + 1);
//                                canvas.drawText(sub, x, bottom, paint);
//                                x += caption_screen.fixed_char_width;
//                            }
//                        }
                    }

                    void draw_text(Canvas canvas, String data,
                                   Typeface face, double font_size,
                                   float left, float bottom, int fg_color, String opacity, int opacity_int,
                                   boolean underline,
                                   int edge_color, float edge_width, String edge_type, Paint paint) {

                        //Log.e(TAG, "draw_text "+data + " fg_color: "+ fg_color +" opa:"+ opacity + edge_type + "edge color: "+ edge_color);
                        paint.reset();
                        if (style_use_broadcast) {
                            if (opacity.equalsIgnoreCase("flash")) {
                                if (heart_beat == 0)
                                    return;
                                else
                                    opacity_int = 0xff;
                            }
                        }
                        paint.setSubpixelText(true);
                        paint.setTypeface(face);
                        if (opacity_int != 0xff)
                            paint.setXfermode(porter_src);

                        paint.setAntiAlias(true);
                        paint.setTextSize((float)font_size);

                        if (edge_type == null || edge_type.equalsIgnoreCase("none")) {
                            paint.setColor(fg_color);
                            paint.setAlpha(opacity_int);
                            draw_str(canvas, data, left, bottom, paint);
                        } else if (edge_type.equalsIgnoreCase("uniform")) {
                            //paint.setStrokeJoin(Paint.Join.ROUND);
                            paint.setStrokeWidth((float)(edge_width*1.5));
                            paint.setColor(edge_color);
                            paint.setStyle(Paint.Style.STROKE);
                            draw_str(canvas, data, left, bottom, paint);
                            paint.setColor(fg_color);
                            paint.setAlpha(opacity_int);
                            paint.setStyle(Paint.Style.FILL);
                            draw_str(canvas, data, left, bottom, paint);
                        } else if (edge_type.equalsIgnoreCase("shadow_right")) {
                            paint.setShadowLayer((float)edge_width, (float) edge_width, (float) edge_width, edge_color);
                            paint.setColor(fg_color);
                            paint.setAlpha(opacity_int);
                            draw_str(canvas, data, left, bottom, paint);
                        } else if (edge_type.equalsIgnoreCase("shadow_left")) {
                            paint.setShadowLayer((float)edge_width, (float) -edge_width, (float) -edge_width, edge_color);
                            paint.setColor(fg_color);
                            paint.setAlpha(opacity_int);
                            draw_str(canvas, data, left, bottom, paint);
                        } else if (edge_type.equalsIgnoreCase("raised") ||
                                edge_type.equalsIgnoreCase("depressed")) {

                            boolean raised;
                            if (edge_type.equalsIgnoreCase("depressed"))
                                raised = false;
                            else
                                raised = true;
                            int colorUp = raised ? fg_color : edge_color;
                            int colorDown = raised ? edge_color : fg_color;
                            float offset = (float)edge_width;
                            paint.setColor(fg_color);
                            paint.setStyle(Paint.Style.FILL);
                            paint.setShadowLayer(edge_width, -offset, -offset, colorUp);
                            draw_str(canvas, data, left, bottom, paint);
                            paint.setShadowLayer(edge_width, offset, offset, colorDown);
                            draw_str(canvas, data, left, bottom, paint);
                        } else if (edge_type.equalsIgnoreCase("none")) {
                            paint.setColor(fg_color);
                            paint.setAlpha(opacity_int);
                            draw_str(canvas, data, left, bottom, paint);
                        }
                        if (underline) {
                            paint.setStrokeWidth((float)(2.0));
                            paint.setColor(fg_color);
                            canvas.drawLine(left, (float)(bottom + edge_width * 2),
                                    (float) (left + string_length_on_paint),
                                    (float)(bottom + edge_width * 2), paint);
                        }
                    }
                }
            }
        }

        void draw(Canvas canvas)
        {
            /* Windows come in rising queue,
             * so we need to revert the draw sequence */
            if (!init_flag) {
                Log.e(TAG, "Init failed, skip draw");
                return;
            }
            for (int i = windows_count - 1; i >= 0; i--)
                windows[i].draw(canvas);
        }
    }
}
