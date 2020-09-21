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
import android.graphics.Color;
import android.os.SystemProperties;
import android.util.Log;
import android.view.View;
import android.view.accessibility.CaptioningManager;
import android.view.accessibility.CaptioningManager.CaptionStyle;
import com.droidlogic.SubTitleService.R;
import com.tv.DTVSubtitleView;
import com.droidlogic.app.ISubTitleServiceCallback;


public class TvSubtitle {
    private static final String TAG = "TvSubtitle";
    private Context mContext;
    private boolean mDebug = SystemProperties.getBoolean("vendor.sys.subtitleService.tv.debug", true);

    private DTVSubtitleView mTvSubtitleView = null;
    private CaptioningManager mCaptioningManager = null;

    private static final int TV_SUB_MPEG2 = 0;  //close caption mpeg2
    private static final int TV_SUB_H264 = 2;    //close caption h264
    private static final int TV_SUB_SCTE27 = 3;
    private static final int TV_SUB_DVB = 4;
    public TvSubtitle(Context context, View view) {
        mContext = context;
        mTvSubtitleView = (DTVSubtitleView)view.findViewById(R.id.tvsubtitle);
        mCaptioningManager = (CaptioningManager) mContext.getSystemService(Context.CAPTIONING_SERVICE);

        LOGI("[TvSubtitle] mTvSubtitleView:" + mTvSubtitleView);
    }

    public void init() {
    }

    private void LOGI(String msg) {
        if (mDebug) Log.i(TAG, msg);
    }

    public void hide() {
        if (mTvSubtitleView != null) {
            mTvSubtitleView.hide();
        }
    }

    public static void setCallback(ISubTitleServiceCallback cb)  {
        //DTVSubtitleView.setCallback(cb);
    }

    public int getSubHeight() {
        if (mTvSubtitleView != null) {
            return mTvSubtitleView.getSubHeight();
        }
        return -1;
    }

    public int getSubWidth() {
        if (mTvSubtitleView != null) {
            return mTvSubtitleView.getSubWidth();
        }
        return -1;
    }


    public void show() {
        if (mTvSubtitleView != null) {
            mTvSubtitleView.show();
        }
    }

    public void switchCloseCaption(int tvType, int vfmt, int channelId) {
        LOGI("[start]tvType:" + tvType + ",vfmt:" + vfmt + ",channelId:" + channelId);
        if (mTvSubtitleView != null) {
            setParam(-1, vfmt, channelId, 0, 0, 0);
            mTvSubtitleView.switchCloseCaption(vfmt, channelId);
        }
        else {
            LOGI("[start]tv subtitle view is null");
        }
    }

    public void start(int tvType, int vfmt, int channelId, int pid, int page_id, int anc_page_id) {
        LOGI("[start] tvType:" + tvType + ",vfmt:" + vfmt + ",channelId:" + channelId + ",pid:" + pid + ",page_id:" + page_id + ",anc_page_id:" + anc_page_id + ", mTvSubtitleView:" + mTvSubtitleView);
        if (mTvSubtitleView != null) {
            enableSubtitleShow(true);
            mTvSubtitleView.stopAll();
            setParam(tvType, vfmt, channelId, pid, page_id, anc_page_id);
            mTvSubtitleView.setActive(true);
            mTvSubtitleView.startSub();
        }
        else {
            LOGI("[start]tv subtitle view is null");
        }
    }

    public void stop() {
        LOGI("[stop]");
        if (mTvSubtitleView != null) {
            enableSubtitleShow(false);
            mTvSubtitleView.stop();
        }
        else {
            LOGI("[stop]tv subtitle view is null");
        }
    }

    public void stopAll() {
        LOGI("[stopAll]");
        if (mTvSubtitleView != null) {
            enableSubtitleShow(false);
            mTvSubtitleView.stopAll();
        }
        else {
            LOGI("[stopAll]tv subtitle view is null");
        }
    }

    private void enableSubtitleShow(boolean enable) {
        if (mTvSubtitleView != null) {
            mTvSubtitleView.setVisible(enable);
            if (enable)
                mTvSubtitleView.show();
            else
                mTvSubtitleView.hide();
        }
    }


    /**
     * set subtitle param
     * @param tvType: 0:mpeg,2:h264, 3:scte27, 4:dvb subtitle
     * @param vfmt 0:mpeg, 2:h264
     * @param channelId: cc channelId
     * @param pid: subtitle pid, scte27 & dvb subtitle need
     * @param page_id: dvb subtitle need
     * @param anc_page_id: dvb subtitle need
     */
    private void setParam(int tvType, int vfmt, int channelId, int pid, int page_id, int anc_page_id) {
        //if (vfmt == TV_SUB_MPEG2 || vfmt == TV_SUB_H264) {
            CCStyleParams ccParam = getCaptionStyle();
            DTVSubtitleView.DTVCCParams params =
                new DTVSubtitleView.DTVCCParams(vfmt, channelId,
                    ccParam.fg_color,
                    ccParam.fg_opacity,
                    ccParam.bg_color,
                    ccParam.bg_opacity,
                    ccParam.font_style,
                    ccParam.font_size);
            mTvSubtitleView.setSubParams(params);
            LOGI("[setParam]pid=" + pid +
                ",vfmt=" + vfmt +
                ",channelId=" + channelId +
                ",fg_color=" + ccParam.fg_color +
                ", fg_op=" + ccParam.fg_opacity +
                ", bg_color=" + ccParam.bg_color +
                ", bg_op=" + ccParam.bg_opacity);
        //}

        if (tvType == TV_SUB_SCTE27) {
            DTVSubtitleView.Scte27Params sub_params = new DTVSubtitleView.Scte27Params(1, pid);
            mTvSubtitleView.setSubParams(sub_params);
        } else if (tvType == TV_SUB_DVB) {
            DTVSubtitleView.DVBSubParams sub_params = new DTVSubtitleView.DVBSubParams(1, pid, page_id, anc_page_id);
            mTvSubtitleView.setSubParams(sub_params);
        }
        mTvSubtitleView.setMargin(225, 128, 225, 128);

    }

    private class CCStyleParams {
        protected int fg_color;
        protected int fg_opacity;
        protected int bg_color;
        protected int bg_opacity;
        protected int font_style;
        protected float font_size;

        public CCStyleParams(int fg_color, int fg_opacity,
            int bg_color, int bg_opacity, int font_style, float font_size) {
            this.fg_color = fg_color;
            this.fg_opacity = fg_opacity;
            this.bg_color = bg_color;
            this.bg_opacity = bg_opacity;
            this.font_style = font_style;
            this.font_size = font_size;
        }
    }

    private static final int DTV_COLOR_WHITE = 1;
    private static final int DTV_COLOR_BLACK = 2;
    private static final int DTV_COLOR_RED = 3;
    private static final int DTV_COLOR_GREEN = 4;
    private static final int DTV_COLOR_BLUE = 5;
    private static final int DTV_COLOR_YELLOW = 6;
    private static final int DTV_COLOR_MAGENTA = 7;
    private static final int DTV_COLOR_CYAN = 8;

    private static final int DTV_OPACITY_TRANSPARENT = 1;
    private static final int DTV_OPACITY_TRANSLUCENT = 2;
    private static final int DTV_OPACITY_SOLID = 3;

    private static final int DTV_CC_STYLE_WHITE_ON_BLACK = 0;
    private static final int DTV_CC_STYLE_BLACK_ON_WHITE = 1;
    private static final int DTV_CC_STYLE_YELLOW_ON_BLACK = 2;
    private static final int DTV_CC_STYLE_YELLOW_ON_BLUE = 3;
    private static final int DTV_CC_STYLE_USE_DEFAULT = 4;
    private static final int DTV_CC_STYLE_USE_CUSTOM = -1;

    private CCStyleParams getCaptionStyle() {
        boolean USE_NEW_CCVIEW = true;
        CCStyleParams params;

        String[] typeface = mContext.getResources().getStringArray(R.array.captioning_typeface_selector_values);
        CaptioningManager.CaptionStyle userStyle = mCaptioningManager.getUserStyle();
        int style = 0; //mCaptioningManager.getRawUserStyle();
        float textSize = mCaptioningManager.getFontScale();
        int fg_color = userStyle.foregroundColor & 0x00ffffff;
        int fg_opacity = userStyle.foregroundColor & 0xff000000;
        int bg_color = userStyle.backgroundColor & 0x00ffffff;
        int bg_opacity = userStyle.backgroundColor & 0xff000000;
        int fontStyle = DTVSubtitleView.CC_FONTSTYLE_DEFAULT;

        /*for (int i = 0; i < typeface.length; ++i) {
            if (typeface[i].equals(userStyle.mRawTypeface)) {
                fontStyle = i;
                break;
            }
        }*/
        //LOGI("[getCaptionStyle]get style: " + style + ", fontStyle" + fontStyle + ", typeface: " + userStyle.mRawTypeface);

        int fg = userStyle.foregroundColor;
        int bg = userStyle.backgroundColor;
        int convert_fg_color = USE_NEW_CCVIEW ? fg_color : getColor(fg_color);
        int convert_fg_opacity = USE_NEW_CCVIEW ? fg_opacity : getOpacity(fg_opacity);
        int convert_bg_color = USE_NEW_CCVIEW ? bg_color : getColor(bg_color);
        int convert_bg_opacity = USE_NEW_CCVIEW ? bg_opacity : getOpacity(bg_opacity);
        float convert_font_size = USE_NEW_CCVIEW ? textSize: getFontSize(textSize);
        LOGI("[getCaptionStyle]Caption font size:" + convert_font_size +
            " ,fg_color:" + Integer.toHexString(fg) +
            ", fg_opacity:" + Integer.toHexString(fg_opacity) +
            " ,bg_color:" + Integer.toHexString(bg) +
            ", @fg_color:" + convert_fg_color +
            ", @bg_color:" + convert_bg_color +
            ", @fg_opacity:" + convert_fg_opacity +
            ", @bg_opacity:" + convert_bg_opacity);

        switch (style) {
            case DTV_CC_STYLE_WHITE_ON_BLACK:
                convert_fg_color = USE_NEW_CCVIEW ? Color.WHITE : DTV_COLOR_WHITE;
                convert_fg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                convert_bg_color = USE_NEW_CCVIEW ? Color.BLACK : DTV_COLOR_BLACK;
                convert_bg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                break;

            case DTV_CC_STYLE_BLACK_ON_WHITE:
                convert_fg_color = USE_NEW_CCVIEW ? Color.BLACK : DTV_COLOR_BLACK;
                convert_fg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                convert_bg_color = USE_NEW_CCVIEW ? Color.WHITE : DTV_COLOR_WHITE;
                convert_bg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                break;

            case DTV_CC_STYLE_YELLOW_ON_BLACK:
                convert_fg_color = USE_NEW_CCVIEW ? Color.YELLOW : DTV_COLOR_YELLOW;
                convert_fg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                convert_bg_color = USE_NEW_CCVIEW ? Color.BLACK : DTV_COLOR_BLACK;
                convert_bg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                break;

            case DTV_CC_STYLE_YELLOW_ON_BLUE:
                convert_fg_color = USE_NEW_CCVIEW ? Color.YELLOW : DTV_COLOR_YELLOW;
                convert_fg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTV_OPACITY_SOLID;
                convert_bg_color = USE_NEW_CCVIEW ? Color.BLUE : DTV_COLOR_BLUE;
                convert_bg_opacity = USE_NEW_CCVIEW ? Color.BLUE : DTV_OPACITY_SOLID;
                break;

            case DTV_CC_STYLE_USE_DEFAULT:
                convert_fg_color = USE_NEW_CCVIEW ? Color.WHITE : DTVSubtitleView.CC_COLOR_DEFAULT;
                convert_fg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTVSubtitleView.CC_OPACITY_DEFAULT;
                convert_bg_color = USE_NEW_CCVIEW ? Color.BLACK : DTVSubtitleView.CC_COLOR_DEFAULT;
                convert_bg_opacity = USE_NEW_CCVIEW ? Color.BLACK : DTVSubtitleView.CC_OPACITY_DEFAULT;
                break;

            case DTV_CC_STYLE_USE_CUSTOM:
                break;
        }
        params = new CCStyleParams(convert_fg_color,
            convert_fg_opacity,
            convert_bg_color,
            convert_bg_opacity,
            fontStyle,
            convert_font_size);

        return params;
    }

    private int getColor(int color) {
        switch (color) {
            case 0xFFFFFF:
                return DTV_COLOR_WHITE;
            case 0x0:
                return DTV_COLOR_BLACK;
            case 0xFF0000:
                return DTV_COLOR_RED;
            case 0x00FF00:
                return DTV_COLOR_GREEN;
            case 0x0000FF:
                return DTV_COLOR_BLUE;
            case 0xFFFF00:
                return DTV_COLOR_YELLOW;
            case 0xFF00FF:
                return DTV_COLOR_MAGENTA;
            case 0x00FFFF:
                return DTV_COLOR_CYAN;
        }
        return DTV_COLOR_WHITE;
    }

    private int getOpacity(int opacity) {
        LOGI("[getOpacity] opacity:" + Integer.toHexString(opacity));
        switch (opacity) {
            case 0:
                return DTV_OPACITY_TRANSPARENT;
            case 0x80000000:
                return DTV_OPACITY_TRANSLUCENT;
            case 0xFF000000:
                return DTV_OPACITY_SOLID;
        }
        return DTV_OPACITY_TRANSPARENT;
    }
    public String getSubLanguage() {
        if (mTvSubtitleView != null) {
            return mTvSubtitleView.getSubLanguage();
        }
        return null;
    }

    private float getFontSize(float textSize) {
        if (0 <= textSize && textSize < .375) {
            return 1.0f;//AM_CC_FONTSIZE_SMALL
        } else if (textSize < .75) {
            return 1.0f;//AM_CC_FONTSIZE_SMALL
        } else if (textSize < 1.25) {
            return 2.0f;//AM_CC_FONTSIZE_DEFAULT
        } else if (textSize < 1.75) {
            return 3.0f;//AM_CC_FONTSIZE_BIG
        } else if (textSize < 2.5) {
            return 4.0f;//AM_CC_FONTSIZE_MAX
        }else {
            return 2.0f;//AM_CC_FONTSIZE_DEFAULT
        }
    }

}
