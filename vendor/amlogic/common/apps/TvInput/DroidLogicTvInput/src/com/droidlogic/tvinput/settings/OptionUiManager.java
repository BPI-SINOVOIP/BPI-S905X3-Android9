/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.settings;

import android.R.integer;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.content.Intent;
import android.media.tv.TvContract.Channels;
import android.os.PowerManager;
import android.os.Bundle;
import android.provider.Settings;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnKeyListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Switch;

import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvControlDataManager;
import android.app.AlertDialog;
import com.droidlogic.app.SystemControlManager;
import android.media.tv.TvContract;
import com.droidlogic.app.tv.TvControlManager.FreqList;

import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvChannelParams;
import com.droidlogic.app.tv.TvMultilingualText;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvScanConfig;
import com.droidlogic.tvinput.R;
import com.droidlogic.tvinput.Utils;
import com.droidlogic.tvinput.settings.ContentListView;
import android.media.tv.TvInputInfo;

public class OptionUiManager implements OnClickListener, OnFocusChangeListener, OnKeyListener, TvControlManager.ScannerEventListener {
    public static final String TAG = "OptionUiManager";

    public static final int OPTION_PICTURE_MODE = 100;
    public static final int OPTION_BRIGHTNESS = 101;
    public static final int OPTION_CONTRAST = 102;
    public static final int OPTION_COLOR = 103;
    public static final int OPTION_SHARPNESS = 104;
    public static final int OPTION_BACKLIGHT = 105;
    public static final int OPTION_TINT = 106;
    public static final int OPTION_COLOR_TEMPERATURE = 107;
    public static final int OPTION_ASPECT_RATIO = 108;
    public static final int OPTION_DNR = 109;
    public static final int OPTION_3D_SETTINGS = 110;

    public static final int OPTION_SOUND_MODE = 200;
    public static final int OPTION_TREBLE = 201;
    public static final int OPTION_BASS = 202;
    public static final int OPTION_BALANCE = 203;
    public static final int OPTION_SPDIF = 204;
    public static final int OPTION_DIALOG_CLARITY = 205;
    public static final int OPTION_BASS_BOOST = 206;
    public static final int OPTION_SURROUND = 207;
    public static final int OPTION_VIRTUAL_SURROUND = 208;

    public static final int OPTION_AUDIO_TRACK = 300;
    public static final int OPTION_SOUND_CHANNEL = 301;
    public static final int OPTION_CHANNEL_INFO = 302;
    public static final int OPTION_COLOR_SYSTEM = 303;
    public static final int OPTION_SOUND_SYSTEM = 304;
    public static final int OPTION_VOLUME_COMPENSATE = 305;
    public static final int OPTION_FINE_TUNE = 306;
    public static final int OPTION_MANUAL_SEARCH = 307;
    public static final int OPTION_AUTO_SEARCH = 38;
    public static final int OPTION_CHANNEL_EDIT = 39;
    public static final int OPTION_SWITCH_CHANNEL = 310;
    public static final int OPTION_MTS = 311;

    public static final int OPTION_DTV_TYPE = 400;
    public static final int OPTION_SLEEP_TIMER = 401;
    public static final int OPTION_MENU_TIME = 402;
    public static final int OPTION_STARTUP_SETTING = 403;
    public static final int OPTION_DYNAMIC_BACKLIGHT = 404;
    public static final int OPTION_RESTORE_FACTORY = 405;
    public static final int OPTION_DEFAULT_LANGUAGE = 406;
    public static final int OPTION_SUBTITLE_SWITCH = 407;
    public static final int OPTION_HDMI20 = 408;
    public static final int OPTION_FBC_UPGRADE = 409;
    public static final int OPTION_AD_SWITCH = 410;
    public static final int OPTION_AD_MIX = 411;
    public static final int OPTION_AD_LIST = 412;

    public static final int ALPHA_NO_FOCUS = 230;
    public static final int ALPHA_FOCUSED = 255;

    public static final int ATV_MIN_KHZ = 42250;
    public static final int ATV_MAX_KHZ = 868250;

    private static final int PADDING_LEFT = 50;

    private Context mContext;
    private Resources mResources;
    private SettingsManager mSettingsManager;
    private TvControlManager mTvControlManager;
    private TvControlDataManager mTvControlDataManager = null;
    private int optionTag = OPTION_PICTURE_MODE;
    private String optionKey = null;
    private int channelNumber = 0;//for setting show searched tv channelNumber
    private int radioNumber = 0;//for setting show searched radio channelNumber
    private int tvDisplayNumber = 0;//for db store TV channel's channel displayNumber
    private int radioDisplayNumber = 0;//for db store Radio channel's channel displayNumber
    List<ChannelInfo> mChannels = new ArrayList<ChannelInfo>();

    public static final int SEARCH_STOPPED = 0;
    public static final int SEARCH_RUNNING = 1;
    public static final int SEARCH_PAUSED = 2;
    private int isSearching = SEARCH_STOPPED;

    public static final int AUTO_SEARCH = 0;
    public static final int MANUAL_SEARCH = 1;
    private int searchType = AUTO_SEARCH;

    private Toast toast = null;

    public boolean isSearching() {
        return (isSearching != SEARCH_STOPPED);
    }
    public boolean isManualSearching() {
        return (isSearching() && (searchType == MANUAL_SEARCH));
    }

    public OptionUiManager(Context context) {
        mContext = context;
        mResources = mContext.getResources();
        init(((TvSettingsActivity)mContext).getSettingsManager());
    }

    public void init (SettingsManager sm) {
        mSettingsManager = sm;
        mTvControlManager = TvControlManager.getInstance();
        mTvControlManager.setScannerListener(this);
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
    }

    public void setOptionTag(int position) {
        String item_name = null;
        if (((TvSettingsActivity) mContext).getCurrentFragment().getContentList().size() > 0) {
            item_name = ((TvSettingsActivity) mContext).getCurrentFragment().getContentList().get(position).get(ContentFragment.ITEM_NAME)
                    .toString();
        }
        if (!TextUtils.isEmpty(item_name)) {
        // Picture
            if (item_name.equals(mResources.getString(R.string.picture_mode))) {
                optionTag = OPTION_PICTURE_MODE;
                optionKey = SettingsManager.KEY_PICTURE_MODE;
            } else if (item_name.equals(mResources.getString(R.string.brightness))) {
                optionTag = OPTION_BRIGHTNESS;
                optionKey = SettingsManager.KEY_BRIGHTNESS;
            } else if (item_name.equals(mResources.getString(R.string.contrast))) {
                optionTag = OPTION_CONTRAST;
                optionKey = SettingsManager.KEY_CONTRAST;
            } else if (item_name.equals(mResources.getString(R.string.color))) {
                optionTag = OPTION_COLOR;
                optionKey = SettingsManager.KEY_COLOR;
            } else if (item_name.equals(mResources.getString(R.string.sharpness))) {
                optionTag = OPTION_SHARPNESS;
                optionKey = SettingsManager.KEY_SHARPNESS;
            } else if (item_name.equals(mResources.getString(R.string.backlight))) {
                optionTag = OPTION_BACKLIGHT;
                optionKey = SettingsManager.KEY_BACKLIGHT;
            } else if (item_name.equals(mResources.getString(R.string.tint))) {
                optionTag = OPTION_TINT;
                optionKey = SettingsManager.KEY_TINT;
            } else if (item_name.equals(mResources.getString(R.string.color_temperature))) {
                optionTag = OPTION_COLOR_TEMPERATURE;
                optionKey = SettingsManager.KEY_COLOR_TEMPERATURE;
            } else if (item_name.equals(mResources.getString(R.string.aspect_ratio))) {
                optionTag = OPTION_ASPECT_RATIO;
                optionKey = SettingsManager.KEY_ASPECT_RATIO;
            } else if (item_name.equals(mResources.getString(R.string.dnr))) {
                optionTag = OPTION_DNR;
                optionKey = SettingsManager.KEY_DNR;
            } else if (item_name.equals(mResources.getString(R.string.settings_3d))) {
                optionTag = OPTION_3D_SETTINGS;
                optionKey = SettingsManager.KEY_3D_SETTINGS;
            }
           // Sound
            else if (item_name.equals(mResources.getString(R.string.sound_mode))) {
                optionTag = OPTION_SOUND_MODE;
                optionKey = SettingsManager.KEY_SOUND_MODE;
            } else if (item_name.equals(mResources.getString(R.string.treble))) {
                optionTag = OPTION_TREBLE;
                optionKey = SettingsManager.KEY_TREBLE;
            } else if (item_name.equals(mResources.getString(R.string.bass))) {
                optionTag = OPTION_BASS;
                optionKey = SettingsManager.KEY_BASS;
            } else if (item_name.equals(mResources.getString(R.string.balance))) {
                optionTag = OPTION_BALANCE;
                optionKey = SettingsManager.KEY_BALANCE;
            } else if (item_name.equals(mResources.getString(R.string.spdif))) {
                optionTag = OPTION_SPDIF;
                optionKey = SettingsManager.KEY_SPDIF;
            } else if (item_name.equals(mResources.getString(R.string.dialog_clarity))) {
                optionTag = OPTION_DIALOG_CLARITY;
                optionKey = SettingsManager.KEY_DIALOG_CLARITY;
            } else if (item_name.equals(mResources.getString(R.string.bass_boost))) {
                optionTag = OPTION_BASS_BOOST;
                optionKey = SettingsManager.KEY_BASS_BOOST;
            } else if (item_name.equals(mResources.getString(R.string.surround))) {
                optionTag = OPTION_SURROUND;
                optionKey = SettingsManager.KEY_SURROUND;
            } else if (item_name.equals(mResources.getString(R.string.virtual_surround))) {
                optionTag = OPTION_VIRTUAL_SURROUND;
                optionKey = SettingsManager.KEY_VIRTUAL_SURROUND;
            }
        // Channel
            else if (item_name.equals(mResources.getString(R.string.audio_track))) {
                optionTag = OPTION_AUDIO_TRACK;
                optionKey = SettingsManager.KEY_AUIDO_TRACK;
            } else if (item_name.equals(mResources.getString(R.string.sound_channel))) {
                optionTag = OPTION_SOUND_CHANNEL;
                optionKey = SettingsManager.KEY_SOUND_CHANNEL;
            } else if (item_name.equals(mResources.getString(R.string.channel_info))) {
                optionTag = OPTION_CHANNEL_INFO;
                optionKey = SettingsManager.KEY_CHANNEL_INFO;
            } else if (item_name.equals(mContext.getResources().getString(R.string.defalut_lan))) {
                optionTag = OPTION_DEFAULT_LANGUAGE;
                optionKey = SettingsManager.KEY_DEFAULT_LANGUAGE;
            } else if (item_name.equals(mContext.getResources().getString(R.string.sub_switch))) {
                optionTag = OPTION_SUBTITLE_SWITCH;
                optionKey = SettingsManager.KEY_SUBTITLE_SWITCH;
            } else if (item_name.equals(mContext.getResources().getString(R.string.ad_switch))) {
                optionTag = OPTION_AD_SWITCH;
                optionKey = SettingsManager.KEY_AD_SWITCH;
            } else if (item_name.equals(mContext.getResources().getString(R.string.ad_mix))) {
                optionTag = OPTION_AD_MIX;
                optionKey = SettingsManager.KEY_AD_MIX;
            } else if (item_name.equals(mContext.getResources().getString(R.string.color_system))) {
                optionTag = OPTION_COLOR_SYSTEM;
                optionKey = SettingsManager.KEY_COLOR_SYSTEM;
            } else if (item_name.equals(mResources.getString(R.string.sound_system))) {
                optionTag = OPTION_SOUND_SYSTEM;
                optionKey = SettingsManager.KEY_SOUND_SYSTEM;
            } else if (item_name.equals(mResources.getString(R.string.volume_compensate))) {
                optionTag = OPTION_VOLUME_COMPENSATE;
                optionKey = SettingsManager.KEY_VOLUME_COMPENSATE;
            } else if (item_name.equals(mResources.getString(R.string.fine_tune))) {
                optionTag = OPTION_FINE_TUNE;
                optionKey = SettingsManager.KEY_FINE_TUNE;
            } else if (item_name.equals(mResources.getString(R.string.manual_search))) {
                optionTag = OPTION_MANUAL_SEARCH;
                optionKey = SettingsManager.KEY_MANUAL_SEARCH;
            } else if (item_name.equals(mResources.getString(R.string.auto_search))) {
                optionTag = OPTION_AUTO_SEARCH;
                optionKey = SettingsManager.KEY_AUTO_SEARCH;
            } else if (item_name.equals(mResources.getString(R.string.channel_edit))) {
                optionTag = OPTION_CHANNEL_EDIT;
                optionKey = SettingsManager.KEY_CHANNEL_EDIT;
            } else if (item_name.equals(mResources.getString(R.string.switch_channel))) {
                optionTag = OPTION_SWITCH_CHANNEL;
                optionKey = SettingsManager.KEY_SWITCH_CHANNEL;
            } else if (item_name.equals(mResources.getString(R.string.mts))) {
                optionTag = OPTION_MTS;
                optionKey = SettingsManager.KEY_MTS;
            }
        // Settings
            else if (item_name.equals(mResources.getString(R.string.dtv_type))) {
                optionTag = OPTION_DTV_TYPE;
                optionKey = SettingsManager.KEY_DTV_TYPE;
            } else if (item_name.equals(mResources.getString(R.string.sleep_timer))) {
                optionTag = OPTION_SLEEP_TIMER;
                optionKey = SettingsManager.KEY_SLEEP_TIMER;
            } else if (item_name.equals(mResources.getString(R.string.menu_time))) {
                optionTag = OPTION_MENU_TIME;
                optionKey = SettingsManager.KEY_MENU_TIME;
            } else if (item_name.equals(mResources.getString(R.string.startup_setting))) {
                optionTag = OPTION_STARTUP_SETTING;
                optionKey = SettingsManager.KEY_STARTUP_SETTING;
            } else if (item_name.equals(mResources.getString(R.string.dynamic_backlight))) {
                optionTag = OPTION_DYNAMIC_BACKLIGHT;
                optionKey = SettingsManager.KEY_DYNAMIC_BACKLIGHT;
            } else if (item_name.equals(mResources.getString(R.string.restore_factory))) {
                optionTag = OPTION_RESTORE_FACTORY;
                optionKey = SettingsManager.KEY_RESTORE_FACTORY;
            } else if (item_name.equals(mResources.getString(R.string.hdmi20))) {
                optionTag = OPTION_HDMI20;
                optionKey = SettingsManager.KEY_HDMI20;
            } else if (item_name.equals(mResources.getString(R.string.fbc_upgrade))){
                optionTag = OPTION_FBC_UPGRADE;
                optionKey = SettingsManager.KEY_FBC_UPGRADE;
            } else if (item_name.equals(mResources.getString(R.string.ad_list))){
                optionTag = OPTION_AD_LIST;
                optionKey = SettingsManager.KEY_AD_LIST;
            }
        }
    }

    public int getOptionTag() {
        return optionTag;
    }

    public int getLayoutId() {
        switch (optionTag) {
            // picture
            case OPTION_PICTURE_MODE:
                return R.layout.layout_picture_picture_mode;
            case OPTION_BRIGHTNESS:
                return R.layout.layout_picture_brightness;
            case OPTION_CONTRAST:
                return R.layout.layout_picture_contrast;
            case OPTION_COLOR:
                return R.layout.layout_picture_color;
            case OPTION_SHARPNESS:
                return R.layout.layout_picture_sharpness;
            case OPTION_BACKLIGHT:
                return R.layout.layout_picture_backlight;
            case OPTION_TINT:
                return R.layout.layout_picture_tint;
            case OPTION_COLOR_TEMPERATURE:
                return R.layout.layout_picture_color_temperature;
            case OPTION_ASPECT_RATIO:
                return R.layout.layout_picture_aspect_ratio;
            case OPTION_DNR:
                return R.layout.layout_picture_dnr;
            case OPTION_3D_SETTINGS:
                return R.layout.layout_picture_3d_settings;
            // sound
            case OPTION_SOUND_MODE:
                return R.layout.layout_sound_sound_mode;
            case OPTION_TREBLE:
                return R.layout.layout_sound_treble;
            case OPTION_BASS:
                return R.layout.layout_sound_bass;
            case OPTION_BALANCE:
                return R.layout.layout_sound_balance;
            case OPTION_SPDIF:
                return R.layout.layout_sound_spdif;
            case OPTION_VIRTUAL_SURROUND:
                return R.layout.layout_sound_virtual_surround;
            case OPTION_SURROUND:
                return R.layout.layout_sound_surround;
            case OPTION_DIALOG_CLARITY:
                return R.layout.layout_sound_dialog_clarity;
            case OPTION_BASS_BOOST:
                return R.layout.layout_sound_bass_boost;
            // channel
            case OPTION_SOUND_CHANNEL:
                return R.layout.layout_channel_sound_channel;
            case OPTION_CHANNEL_INFO:
            case OPTION_AUDIO_TRACK:
            case OPTION_DEFAULT_LANGUAGE:
                return R.layout.layout_option_list;
            case OPTION_SUBTITLE_SWITCH:
                return R.layout.layout_channel_subtitle_switch;
            case OPTION_AD_SWITCH:
                return R.layout.layout_channel_ad_switch;
            case OPTION_AD_MIX:
                return R.layout.layout_channel_ad_mix;
            case OPTION_COLOR_SYSTEM:
                return R.layout.layout_channel_color_system;
            case OPTION_SOUND_SYSTEM:
                return R.layout.layout_channel_sound_system;
            case OPTION_SWITCH_CHANNEL:
                return R.layout.layout_channel_switch_channel;
            case OPTION_MTS:
                return R.layout.layout_channel_mts;
            case OPTION_VOLUME_COMPENSATE:
                return R.layout.layout_channel_volume_compensate;
            case OPTION_FINE_TUNE:
                return R.layout.layout_channel_fine_tune;
            case OPTION_MANUAL_SEARCH:
                if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV)
                    return R.layout.layout_channel_manual_serch_dtv_for_atsc;
                else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV)
                    return R.layout.layout_channel_manual_search_atv;
                else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV)
                    return R.layout.layout_channel_manual_search_dtv;
            case OPTION_AUTO_SEARCH:
                if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV)
                    return R.layout.layout_channel_auto_search_dtv_for_atsc;
                else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV)
                    return R.layout.layout_channel_auto_search_atv;
                else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV)
                    return R.layout.layout_channel_auto_search_dtv;
            case OPTION_CHANNEL_EDIT:
                return R.layout.layout_channel_channel_edit;
            // settings
            case OPTION_DTV_TYPE:
                return R.layout.layout_settings_dtv_type;
            case OPTION_SLEEP_TIMER:
                return R.layout.layout_settings_sleep_timer;
            case OPTION_MENU_TIME:
                return R.layout.layout_settings_menu_time;
            case OPTION_STARTUP_SETTING:
                return R.layout.layout_settings_startup_setting;
            case OPTION_DYNAMIC_BACKLIGHT:
                return R.layout.layout_settings_dynamic_backlight;
            case OPTION_RESTORE_FACTORY:
                return R.layout.layout_settings_restore_factory;
            case OPTION_HDMI20:
                return R.layout.layout_settings_hdmi20;
            case OPTION_FBC_UPGRADE:
            case OPTION_AD_LIST:
                return R.layout.layout_option_list;

            default:
                break;
        }
        return 0;
    }

    public void setOptionListener(View view) {
        for (int i = 0; i < ((ViewGroup) view).getChildCount(); i++) {
            View child = ((ViewGroup) view).getChildAt(i);
            if (child != null && child.hasFocusable() && child instanceof TextView) {
                //child.setOnClickListener(this);
                child.setOnFocusChangeListener(this);
                child.setOnKeyListener(this);
            }
        }
    }

    @Override
    public void onClick(View view) {
        ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
        TextView mVirtualSurroundIn = (TextView) parent.findViewById(R.id.virtual_surround_increase);
        TextView mVirtualSurroundDe = (TextView) parent.findViewById(R.id.virtual_surround_decrease);
        RelativeLayout mVirtualSurroundProcess = (RelativeLayout) parent.findViewById(R.id.virtual_surround_persent);
        switch (view.getId()) {
            // ====Picture====
            // picture mode
            case R.id.picture_mode_standard:
                mSettingsManager.setPictureMode(SettingsManager.STATUS_STANDARD);
                break;
            case R.id.picture_mode_vivid:
                mSettingsManager.setPictureMode(SettingsManager.STATUS_VIVID);
                break;
            case R.id.picture_mode_soft:
                mSettingsManager.setPictureMode(SettingsManager.STATUS_SOFT);
                break;
            case R.id.picture_mode_monitor:
                mSettingsManager.setPictureMode(SettingsManager.STATUS_MONITOR);
                break;
            case R.id.picture_mode_user:
                mSettingsManager.setPictureMode(SettingsManager.STATUS_USER);
                break;
            // brightness
            case R.id.brightness_increase:
                mSettingsManager.setBrightness(SettingsManager.PERCENT_INCREASE);
                break;
            case R.id.brightness_decrease:
                mSettingsManager.setBrightness(SettingsManager.PERCENT_DECREASE);
                break;
            // contrast
            case R.id.contrast_increase:
                mSettingsManager.setContrast(SettingsManager.PERCENT_INCREASE);
                break;
            case R.id.contrast_decrease:
                mSettingsManager.setContrast(SettingsManager.PERCENT_DECREASE);
                break;
            // color
            case R.id.color_increase:
                mSettingsManager.setColor(SettingsManager.PERCENT_INCREASE);
                break;
            case R.id.color_decrease:
                mSettingsManager.setColor(SettingsManager.PERCENT_DECREASE);
                break;
            // sharpness
            case R.id.sharpness_increase:
                mSettingsManager.setSharpness(SettingsManager.PERCENT_INCREASE);
                break;
            case R.id.sharpness_decrease:
                mSettingsManager.setSharpness(SettingsManager.PERCENT_DECREASE);
                break;
            // backlight
            case R.id.backlight_increase:
                mSettingsManager.setBacklight(SettingsManager.PERCENT_INCREASE);
                break;
            case R.id.backlight_decrease:
                mSettingsManager.setBacklight(SettingsManager.PERCENT_DECREASE);
                break;
            // tint
            case R.id.tint_increase:
                mSettingsManager.setTint(SettingsManager.PERCENT_INCREASE);
                break;
            case R.id.tint_decrease:
                mSettingsManager.setTint(SettingsManager.PERCENT_DECREASE);
                break;
            // color temperature
            case R.id.color_temperature_standard:
                mSettingsManager.setColorTemperature(SettingsManager.STATUS_STANDARD);
                break;
            case R.id.color_temperature_warm:
                mSettingsManager.setColorTemperature(SettingsManager.STATUS_WARM);
                break;
            case R.id.color_temperature_cool:
                mSettingsManager.setColorTemperature(SettingsManager.STATUS_COOL);
                break;
            // aspect ratio
            case R.id.apect_ratio_auto:
                mSettingsManager.setAspectRatio(SettingsManager.STATUS_AUTO);
                break;
            case R.id.apect_ratio_four2three:
                mSettingsManager.setAspectRatio(SettingsManager.STATUS_4_TO_3);
                break;
            case R.id.apect_ratio_panorama:
                mSettingsManager.setAspectRatio(SettingsManager.STATUS_PANORAMA);
                break;
            case R.id.apect_ratio_full_screen:
                mSettingsManager.setAspectRatio(SettingsManager.STATUS_FULL_SCREEN);
                break;
            // dnr
            case R.id.dnr_off:
                mSettingsManager.setDnr(SettingsManager.STATUS_OFF);
                break;
            case R.id.dnr_auto:
                mSettingsManager.setDnr(SettingsManager.STATUS_AUTO);
                break;
            case R.id.dnr_medium:
                mSettingsManager.setDnr(SettingsManager.STATUS_MEDIUM);
                break;
            case R.id.dnr_high:
                mSettingsManager.setDnr(SettingsManager.STATUS_HIGH);
                break;
            case R.id.dnr_low:
                mSettingsManager.setDnr(SettingsManager.STATUS_LOW);
                break;
            // 3d settings
            case R.id.settings_3d_off:
            case R.id.settings_3d_auto:
            case R.id.settings_3d_lr_mode:
            case R.id.settings_3d_rl_mode:
            case R.id.settings_3d_ud_mode:
            case R.id.settings_3d_du_mode:
            case R.id.settings_3d_3d_to_2d:
                break;
            // ====Sound====
            // sound mode
            case R.id.sound_mode_standard:
                mSettingsManager.setSoundMode(SettingsManager.STATUS_STANDARD);
                break;
            case R.id.sound_mode_music:
                mSettingsManager.setSoundMode(SettingsManager.STATUS_MUSIC);
                break;
            case R.id.sound_mode_news:
                mSettingsManager.setSoundMode(SettingsManager.STATUS_NEWS);
                break;
            case R.id.sound_mode_movie:
                mSettingsManager.setSoundMode(SettingsManager.STATUS_MOVIE);
                break;
            case R.id.sound_mode_user:
                mSettingsManager.setSoundMode(SettingsManager.STATUS_USER);
                break;
            // Treble
            case R.id.treble_increase:
                mSettingsManager.setTreble(1);
                break;
            case R.id.treble_decrease:
                mSettingsManager.setTreble(-1);
                break;
            // Bass
            case R.id.bass_increase:
                mSettingsManager.setBass(1);
                break;
            case R.id.bass_decrease:
                mSettingsManager.setBass(-1);
                break;
            // Balance
            case R.id.balance_increase:
                mSettingsManager.setBalance(1);
                break;
            case R.id.balance_decrease:
                mSettingsManager.setBalance(-1);;
                break;
            // SPDIF
            case R.id.spdif_off:
                mSettingsManager.setSpdif(SettingsManager.STATUS_OFF);
                break;
            case R.id.spdif_pcm:
                mSettingsManager.setSpdif(SettingsManager.STATUS_PCM);
                break;
            case R.id.spdif_raw:
                mSettingsManager.setSpdif(SettingsManager.STATUS_RAW);
                break;
            // Surround
            case R.id.surround_on:
                mSettingsManager.setSurround(SettingsManager.STATUS_ON);
                break;
            case R.id.surround_off:
                mSettingsManager.setSurround(SettingsManager.STATUS_OFF);
                break;
            //Virtual Surround
            case R.id.virtual_surround_on:
                mVirtualSurroundIn.setVisibility(view.VISIBLE);
                mVirtualSurroundDe.setVisibility(view.VISIBLE);
                mVirtualSurroundProcess.setVisibility(view.VISIBLE);
                mVirtualSurroundIn.setFocusable(true);
                mVirtualSurroundDe.setFocusable(true);
                mSettingsManager.setVirtualSurround(SettingsManager.STATUS_ON);
                break;
            case R.id.virtual_surround_off:
                mVirtualSurroundIn.setVisibility(view.INVISIBLE);
                mVirtualSurroundDe.setVisibility(view.INVISIBLE);
                mVirtualSurroundProcess.setVisibility(view.INVISIBLE);
                mSettingsManager.setVirtualSurround(SettingsManager.STATUS_OFF);
                break;
            //virtual Surround increase and decrease
            case R.id.virtual_surround_increase:
                mSettingsManager.setVirtualSurroundLevel(1);
                break;
            case R.id.virtual_surround_decrease:
                mSettingsManager.setVirtualSurroundLevel(-1);
                break;
            // Dialog Clarity
            case R.id.dialog_clarity_on:
                mSettingsManager.setDialogClarity(SettingsManager.STATUS_ON);
                break;
            case R.id.dialog_clarity_off:
                mSettingsManager.setDialogClarity(SettingsManager.STATUS_OFF);
                break;
            // Bass Boost
            case R.id.bass_boost_on:
                mSettingsManager.setBassBoost(SettingsManager.STATUS_ON);
                break;
            case R.id.bass_boost_off:
                mSettingsManager.setBassBoost(SettingsManager.STATUS_OFF);
                break;
            // ====Channel====
            //sound channel
            case R.id.sound_channel_stereo:
                mSettingsManager.setSoundChannel(TvControlManager.LEFT_RIGHT_SOUND_CHANNEL.LEFT_RIGHT_SOUND_CHANNEL_STEREO.toInt());
                break;
            case R.id.sound_channel_left_channel:
                mSettingsManager.setSoundChannel(TvControlManager.LEFT_RIGHT_SOUND_CHANNEL.LEFT_RIGHT_SOUND_CHANNEL_LEFT.toInt());
                break;
            case R.id.sound_channel_right_channel:
                mSettingsManager.setSoundChannel(TvControlManager.LEFT_RIGHT_SOUND_CHANNEL.LEFT_RIGHT_SOUND_CHANNEL_RIGHT.toInt());
            // subtitle
            case R.id.sub_off:
                mSettingsManager.setSubtitleSwitch(0);
                Intent intent = new Intent(DroidLogicTvUtils.ACTION_SUBTITLE_SWITCH);
                intent.putExtra(DroidLogicTvUtils.EXTRA_SWITCH_VALUE, 0);
                mContext.sendBroadcast(intent);
                break;
            case R.id.sub_on:
                mSettingsManager.setSubtitleSwitch(1);
                intent = new Intent(DroidLogicTvUtils.ACTION_SUBTITLE_SWITCH);
                intent.putExtra(DroidLogicTvUtils.EXTRA_SWITCH_VALUE, 1);
                mContext.sendBroadcast(intent);
                break;
            case R.id.ad_off:
                mSettingsManager.setAudioADSwitch(0);
                break;
            case R.id.ad_on:
                mSettingsManager.setAudioADSwitch(1);
                intent = new Intent(DroidLogicTvUtils.ACTION_AD_SWITCH);
                intent.putExtra(DroidLogicTvUtils.EXTRA_SWITCH_VALUE, 1);
                mContext.sendBroadcast(intent);
                break;
            // ad mix
            case R.id.ad_mix_increase:
                mSettingsManager.setADMix(1);
                break;
            case R.id.ad_mix_decrease:
                mSettingsManager.setADMix(-1);
                break;
            // color system
            case R.id.color_system_pal:
                mSettingsManager.setColorSystem(TvControlManager.tvin_color_system_e.COLOR_SYSTEM_PAL.toInt());
                break;
            case R.id.color_system_ntsc:
                mSettingsManager.setColorSystem(TvControlManager.tvin_color_system_e.COLOR_SYSTEM_NTSC.toInt());
                break;
            // sound system
            case R.id.sound_system_dk:
                mSettingsManager.setSoundSystem(TvControlManager.ATV_AUDIO_STD_DK);
                break;
            case R.id.sound_system_i:
                mSettingsManager.setSoundSystem(TvControlManager.ATV_AUDIO_STD_I);
                break;
            case R.id.sound_system_bg:
                mSettingsManager.setSoundSystem(TvControlManager.ATV_AUDIO_STD_BG);
                break;
            case R.id.sound_system_m:
                mSettingsManager.setSoundSystem(TvControlManager.ATV_AUDIO_STD_M);
                break;
            // volume compensate
            case R.id.volume_compensate_increase:
                mSettingsManager.setVolumeCompensate(1);
                break;
            case R.id.volume_compensate_decrease:
                mSettingsManager.setVolumeCompensate(-1);
                break;
            // fine tune
            case R.id.fine_tune_increase:
                setFineTune(1);
                break;
            case R.id.fine_tune_decrease:
                setFineTune(-1);
                break;
            // manual search
            case R.id.manual_search_start:
            case R.id.manual_search_start_dtv:
                startManualSearch();
                break;
            case R.id.manual_search_start_dtv_manual:
                startManualSearchAccordingMode();
                break;
            // auto search
            case R.id.auto_search_start_atv:
            case R.id.auto_search_start_dtv:
                if (isSearching == SEARCH_STOPPED) {
                    if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
                        startAutosearchAccrodingTvMode();
                        ((TextView)view).setText(mContext.getResources().getString(R.string.pause_search));
                    }else {
                        startAutosearch();
                        ((TextView)view).setText(mContext.getResources().getString(R.string.pause_search));
                    }
                } else if (isSearching == SEARCH_RUNNING) {//searching running
                    pauseSearch();
                    ((TextView)view).setText(mContext.getResources().getString(R.string.resume_search));
                    return ;
                } else {//searching paused
                    resumeSearch();
                    ((TextView)view).setText(mContext.getResources().getString(R.string.pause_search));
                    return;
                }
                break;
            case R.id.audio_outmode_mono:
                mSettingsManager.setAudioOutmode(TvControlManager.AUDIO_OUTMODE_MONO);
                break;
            case R.id.audio_outmode_stereo:
                mSettingsManager.setAudioOutmode(TvControlManager.AUDIO_OUTMODE_STEREO);
                break;
            case R.id.audio_outmode_sap:
                mSettingsManager.setAudioOutmode(TvControlManager.AUDIO_OUTMODE_SAP);
                break;
            // ====Settings====
            // DTV Mode
            case R.id.dtmb:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_DTMB);
                break;
            case R.id.dvbc:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_DVB_C);
                break;
            case R.id.dvbt:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_DVB_T);
                break;
            case R.id.dvbt2:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_DVB_T2);
                break;
            case R.id.atsc_t:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_ATSC_T);
                break;
            case R.id.atsc_c:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_ATSC_C);
                break;
            case R.id.isdb_t:
                mSettingsManager.setDtvType(TvContract.Channels.TYPE_ISDB_T);
                break;
            // Sleep Timer
            case R.id.sleep_timer_off:
                mSettingsManager.setSleepTimer(0);
                break;
            case R.id.sleep_timer_15min:
                mSettingsManager.setSleepTimer(1);
                break;
            case R.id.sleep_timer_30min:
                mSettingsManager.setSleepTimer(2);
                break;
            case R.id.sleep_timer_45min:
                mSettingsManager.setSleepTimer(3);
                break;
            case R.id.sleep_timer_60min:
                mSettingsManager.setSleepTimer(4);
                break;
            case R.id.sleep_timer_90min:
                mSettingsManager.setSleepTimer(5);
                break;
            case R.id.sleep_timer_120min:
                mSettingsManager.setSleepTimer(6);
                break;
            //menu time
            case R.id.menu_time_10s:
                mSettingsManager.setMenuTime(10);
                break;
            case R.id.menu_time_20s:
                mSettingsManager.setMenuTime(20);
                break;
            case R.id.menu_time_40s:
                mSettingsManager.setMenuTime(40);
                break;
            case R.id.menu_time_60s:
                mSettingsManager.setMenuTime(60);
                break;
            // Dynamic Backlight
            case R.id.dynamic_backlight_on:
                mTvControlManager.startAutoBacklight();
                break;
            case R.id.dynamic_backlight_off:
                mTvControlManager.stopAutoBacklight();
                break;
            // Switch Channel
            case R.id.switch_channel_static_frame:
                mTvControlManager.setBlackoutEnable(0, 1);
                break;
            case R.id.switch_channel_black_frame:
                mTvControlManager.setBlackoutEnable(1, 1);
                break;
            // startup app
            case R.id.startup_setting_launcher:
                mSettingsManager.setStartupSetting(0);
                break;
            case R.id.startup_setting_tv:
                mSettingsManager.setStartupSetting(1);
                break;
            // Restore Factory Settings
            case R.id.restore_factory:
                createFactoryResetUi();
                break;
            // HDMI 2.0 ON-OFF
            case R.id.hdmi20_on:
                mSettingsManager.setHdmi20Mode(SettingsManager.STATUS_ON);
                break;
            case R.id.hdmi20_off:
                mSettingsManager.setHdmi20Mode(SettingsManager.STATUS_OFF);
                break;
            default:
                break;
        }
        setChoosedIcon();
        setProgressStatus();
        ((TvSettingsActivity) mContext).getCurrentFragment().refreshList();
    }

    public void setChoosedIcon() {
        ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);

        switch (optionTag) {
            // picture
            case OPTION_PICTURE_MODE:
                if (TextUtils.equals(mSettingsManager.getPictureModeStatus(), mResources.getString(R.string.standard))) {
                    setIcon(parent, R.id.picture_mode_standard);
                } else if (TextUtils.equals(mSettingsManager.getPictureModeStatus(), mResources.getString(R.string.vivid))) {
                    setIcon(parent, R.id.picture_mode_vivid);
                } else if (TextUtils.equals(mSettingsManager.getPictureModeStatus(), mResources.getString(R.string.soft))) {
                    setIcon(parent, R.id.picture_mode_soft);
                } else if (TextUtils.equals(mSettingsManager.getPictureModeStatus(), mResources.getString(R.string.monitor))) {
                    setIcon(parent, R.id.picture_mode_monitor);
                } else if (TextUtils.equals(mSettingsManager.getPictureModeStatus(), mResources.getString(R.string.user))) {
                    setIcon(parent, R.id.picture_mode_user);
                }
                break;
            case OPTION_COLOR_TEMPERATURE:
                if (TextUtils.equals(mSettingsManager.getColorTemperatureStatus(), mResources.getString(R.string.standard))) {
                    setIcon(parent, R.id.color_temperature_standard);
                } else if (TextUtils.equals(mSettingsManager.getColorTemperatureStatus(), mResources.getString(R.string.warm))) {
                    setIcon(parent, R.id.color_temperature_warm);
                } else if (TextUtils.equals(mSettingsManager.getColorTemperatureStatus(), mResources.getString(R.string.cool))) {
                    setIcon(parent, R.id.color_temperature_cool);
                }
                break;
            case OPTION_ASPECT_RATIO:
                if (TextUtils.equals(mSettingsManager.getAspectRatioStatus(), mResources.getString(R.string.full_screen))) {
                    setIcon(parent, R.id.apect_ratio_full_screen);
                } else if (TextUtils.equals(mSettingsManager.getAspectRatioStatus(), mResources.getString(R.string.four2three))) {
                    setIcon(parent, R.id.apect_ratio_four2three);
                } else if (TextUtils.equals(mSettingsManager.getAspectRatioStatus(), mResources.getString(R.string.auto))) {
                    setIcon(parent, R.id.apect_ratio_auto);
                } else if (TextUtils.equals(mSettingsManager.getAspectRatioStatus(), mResources.getString(R.string.panorama))) {
                    setIcon(parent, R.id.apect_ratio_panorama);
                }
                break;
            case OPTION_DNR:
                if (TextUtils.equals(mSettingsManager.getDnrStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.dnr_off);
                } else if (TextUtils.equals(mSettingsManager.getDnrStatus(), mResources.getString(R.string.low))) {
                    setIcon(parent, R.id.dnr_low);
                } else if (TextUtils.equals(mSettingsManager.getDnrStatus(), mResources.getString(R.string.medium))) {
                    setIcon(parent, R.id.dnr_medium);
                } else if (TextUtils.equals(mSettingsManager.getDnrStatus(), mResources.getString(R.string.high))) {
                    setIcon(parent, R.id.dnr_high);
                } else if (TextUtils.equals(mSettingsManager.getDnrStatus(), mResources.getString(R.string.auto))) {
                    setIcon(parent, R.id.dnr_auto);
                }
                break;
            // sound
            case OPTION_SOUND_MODE:
                if (TextUtils.equals(mSettingsManager.getSoundModeStatus(), mResources.getString(R.string.standard))) {
                    setIcon(parent, R.id.sound_mode_standard);
                } else if (TextUtils.equals(mSettingsManager.getSoundModeStatus(), mResources.getString(R.string.music))) {
                    setIcon(parent, R.id.sound_mode_music);
                } else if (TextUtils.equals(mSettingsManager.getSoundModeStatus(), mResources.getString(R.string.news))) {
                    setIcon(parent, R.id.sound_mode_news);
                } else if (TextUtils.equals(mSettingsManager.getSoundModeStatus(), mResources.getString(R.string.movie))) {
                    setIcon(parent, R.id.sound_mode_movie);
                } else if (TextUtils.equals(mSettingsManager.getSoundModeStatus(), mResources.getString(R.string.user))) {
                    setIcon(parent, R.id.sound_mode_user);
                }
                break;
            case OPTION_SPDIF:
                if (TextUtils.equals(mSettingsManager.getSpdifStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.spdif_off);
                } else if (TextUtils.equals(mSettingsManager.getSpdifStatus(), mResources.getString(R.string.raw))) {
                    setIcon(parent, R.id.spdif_raw);
                } else if (TextUtils.equals(mSettingsManager.getSpdifStatus(), mResources.getString(R.string.pcm))) {
                    setIcon(parent, R.id.spdif_pcm);
                }
                break;
            case OPTION_SURROUND:
                if (TextUtils.equals(mSettingsManager.getSurroundStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.surround_off);
                } else if (TextUtils.equals(mSettingsManager.getSurroundStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.surround_on);
                }
                break;
            case OPTION_VIRTUAL_SURROUND:
                if (TextUtils.equals(mSettingsManager.getVirtualSurroundStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.virtual_surround_off);
                } else if (TextUtils.equals(mSettingsManager.getVirtualSurroundStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.virtual_surround_on);
                }
                break;
            case OPTION_DIALOG_CLARITY:
                if (TextUtils.equals(mSettingsManager.getDialogClarityStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.dialog_clarity_off);
                } else if (TextUtils.equals(mSettingsManager.getDialogClarityStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.dialog_clarity_on);
                }
                break;
            case OPTION_BASS_BOOST:
                if (TextUtils.equals(mSettingsManager.getBassBoostStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.bass_boost_off);
                } else if (TextUtils.equals(mSettingsManager.getBassBoostStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.bass_boost_on);
                }
                break;
            // channel
            /*
            case OPTION_DEFAULT_LANGUAGE:
            if (TextUtils.equals(mSettingsManager.getDefaultLanStatus(), mResources.getString(R.string.on))) {
                 setIcon(parent, R.id.sub_on);
             } else if (TextUtils.equals(mSettingsManager.getStartupSettingStatus(), mResources.getString(R.string.off))) {
                 setIcon(parent, R.id.sub_off);
             }
             break;
             */
            case OPTION_SUBTITLE_SWITCH:
                if (TextUtils.equals(mSettingsManager.getSubtitleSwitchStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.sub_on);
                } else if (TextUtils.equals(mSettingsManager.getSubtitleSwitchStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.sub_off);
                }
                break;
            case OPTION_AD_SWITCH:
                if (TextUtils.equals(mSettingsManager.getADSwitchStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.ad_on);
                } else if (TextUtils.equals(mSettingsManager.getADSwitchStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.ad_off);
                }
                break;
            case OPTION_SOUND_CHANNEL:
                if (TextUtils.equals(mSettingsManager.getSoundChannelStatus(), mResources.getString(R.string.stereo))) {
                    setIcon(parent, R.id.sound_channel_stereo);
                } else if (TextUtils.equals(mSettingsManager.getSoundChannelStatus(), mResources.getString(R.string.left_channel))) {
                    setIcon(parent, R.id.sound_channel_left_channel);
                } else if (TextUtils.equals(mSettingsManager.getSoundChannelStatus(), mResources.getString(R.string.right_channel))) {
                    setIcon(parent, R.id.sound_channel_right_channel);
                }
                break;
            case OPTION_COLOR_SYSTEM:
                if (TextUtils.equals(mSettingsManager.getColorSystemStatus(), mResources.getString(R.string.pal))) {
                    setIcon(parent, R.id.color_system_pal);
                } else if (TextUtils.equals(mSettingsManager.getColorSystemStatus(), mResources.getString(R.string.ntsc))) {
                    setIcon(parent, R.id.color_system_ntsc);
                }
                break;
            case OPTION_SOUND_SYSTEM:
                if (TextUtils.equals(mSettingsManager.getSoundSystemStatus(), mResources.getString(R.string.sound_system_dk))) {
                    setIcon(parent, R.id.sound_system_dk);
                } else if (TextUtils.equals(mSettingsManager.getSoundSystemStatus(), mResources.getString(R.string.sound_system_i))) {
                    setIcon(parent, R.id.sound_system_i);
                } else if (TextUtils.equals(mSettingsManager.getSoundSystemStatus(), mResources.getString(R.string.sound_system_bg))) {
                    setIcon(parent, R.id.sound_system_bg);
                } else if (TextUtils.equals(mSettingsManager.getSoundSystemStatus(), mResources.getString(R.string.sound_system_m))) {
                    setIcon(parent, R.id.sound_system_m);
                }
                break;
            case OPTION_SWITCH_CHANNEL:
                if (TextUtils.equals(mSettingsManager.getSwitchChannelStatus(), mResources.getString(R.string.static_frame))) {
                    setIcon(parent, R.id.switch_channel_static_frame);
                } else if (TextUtils.equals(mSettingsManager.getSwitchChannelStatus(), mResources.getString(R.string.black_frame))) {
                    setIcon(parent, R.id.switch_channel_black_frame);
                }
                break;
            case OPTION_MTS:
                if (TextUtils.equals(mSettingsManager.getAudioOutmode(),mResources.getString(R.string.audio_outmode_mono))) {
                    setIcon(parent, R.id.audio_outmode_mono);
                } else if (TextUtils.equals(mSettingsManager.getAudioOutmode(),mResources.getString(R.string.audio_outmode_stereo))) {
                    setIcon(parent, R.id.audio_outmode_stereo);
                } else if (TextUtils.equals(mSettingsManager.getAudioOutmode(),mResources.getString(R.string.audio_outmode_sap))) {
                    setIcon(parent, R.id.audio_outmode_sap);
                }
                break;
            // settings
            case OPTION_DTV_TYPE: {
                String status  = mSettingsManager.getDtvTypeStatus(mSettingsManager.getDtvType());
                if (TextUtils.equals(status, mResources.getString(R.string.dtmb))) {
                    setIcon(parent, R.id.dtmb);
                } else if (TextUtils.equals(status, mResources.getString(R.string.dvbc))) {
                    setIcon(parent, R.id.dvbc);
                } else if (TextUtils.equals(status, mResources.getString(R.string.dvbt))) {
                    setIcon(parent, R.id.dvbt);
                } else if (TextUtils.equals(status, mResources.getString(R.string.dvbt2))) {
                    setIcon(parent, R.id.dvbt2);
                } else if (TextUtils.equals(status, mResources.getString(R.string.atsc_t))) {
                    setIcon(parent, R.id.atsc_t);
                } else if (TextUtils.equals(status, mResources.getString(R.string.atsc_c))) {
                    setIcon(parent, R.id.atsc_c);
                } else if (TextUtils.equals(status, mResources.getString(R.string.isdb_t))) {
                    setIcon(parent, R.id.isdb_t);
                }
                }break;
            case OPTION_SLEEP_TIMER:
                if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.sleep_timer_off);
                } else if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.time_15min))) {
                    setIcon(parent, R.id.sleep_timer_15min);
                } else if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.time_30min))) {
                    setIcon(parent, R.id.sleep_timer_30min);
                } else if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.time_45min))) {
                    setIcon(parent, R.id.sleep_timer_45min);
                } else if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.time_60min))) {
                    setIcon(parent, R.id.sleep_timer_60min);
                } else if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.time_90min))) {
                    setIcon(parent, R.id.sleep_timer_90min);
                } else if (TextUtils.equals(mSettingsManager.getSleepTimerStatus(), mResources.getString(R.string.time_120min))) {
                    setIcon(parent, R.id.sleep_timer_120min);
                }
                break;
            case OPTION_MENU_TIME:
                if (TextUtils.equals(mSettingsManager.getMenuTimeStatus(), mResources.getString(R.string.time_10s))) {
                    setIcon(parent, R.id.menu_time_10s);
                } else if (TextUtils.equals(mSettingsManager.getMenuTimeStatus(), mResources.getString(R.string.time_20s))) {
                    setIcon(parent, R.id.menu_time_20s);
                } else if (TextUtils.equals(mSettingsManager.getMenuTimeStatus(), mResources.getString(R.string.time_40s))) {
                    setIcon(parent, R.id.menu_time_40s);
                } else if (TextUtils.equals(mSettingsManager.getMenuTimeStatus(), mResources.getString(R.string.time_60s))) {
                    setIcon(parent, R.id.menu_time_60s);
                }
                break;
            case OPTION_STARTUP_SETTING:
                if (TextUtils.equals(mSettingsManager.getStartupSettingStatus(), mResources.getString(R.string.launcher))) {
                    setIcon(parent, R.id.startup_setting_launcher);
                } else if (TextUtils.equals(mSettingsManager.getStartupSettingStatus(), mResources.getString(R.string.tv))) {
                    setIcon(parent, R.id.startup_setting_tv);
                }
                break;
            case OPTION_DYNAMIC_BACKLIGHT:
                if (TextUtils.equals(mSettingsManager.getDynamicBacklightStatus(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.dynamic_backlight_off);
                } else if (TextUtils.equals(mSettingsManager.getDynamicBacklightStatus(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.dynamic_backlight_on);
                }
                break;
            case OPTION_HDMI20:
                if (TextUtils.equals(mSettingsManager.getHdmi20Status(), mResources.getString(R.string.off))) {
                    setIcon(parent, R.id.hdmi20_off);
                } else if (TextUtils.equals(mSettingsManager.getHdmi20Status(), mResources.getString(R.string.on))) {
                    setIcon(parent, R.id.hdmi20_on);
                }
                break;

            default:
                break;
        }
    }

    private void setIcon(ViewGroup parent, int id) {
        TextView choosed = (TextView)parent.findViewById(id);
        setIconStyle(choosed);
        if (parent.findViewById(R.id.virtual_surround_increase) != null) {
            TextView mVirtualSurroundIn = (TextView) parent.findViewById(R.id.virtual_surround_increase);
            TextView mVirtualSurroundDe = (TextView) parent.findViewById(R.id.virtual_surround_decrease);
            if (choosed != mVirtualSurroundIn && choosed != mVirtualSurroundDe) {
               for (int i = 1; i < 3; i++) {
                    View child = parent.getChildAt(i);
                    if (child != null && child instanceof TextView) {
                        if (child.getId() != id) {
                            ((TextView) child).setCompoundDrawables(null, null, null, null);
                            ((TextView) child).setPaddingRelative(
                                    ContentListView.dipToPx(mContext, (float) PADDING_LEFT), 0, 0, 0);
                        }
                    }
                }
            }
        }else {
            for (int i = 1; i < parent.getChildCount(); i++) {
                View child = parent.getChildAt(i);
                if (child != null && child instanceof TextView) {
                    if (child.getId() != id) {
                        ((TextView) child).setCompoundDrawables(null, null, null, null);
                        ((TextView) child).setPaddingRelative(
                                ContentListView.dipToPx(mContext, (float) PADDING_LEFT), 0, 0, 0);
                    }
                }
            }
        }
    }

    private void setIconStyle(TextView text) {
        Drawable drawable = mResources.getDrawable(R.drawable.icon_option_choosed);
        drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());

        int padding = 15;
        text.setCompoundDrawablePadding(padding);
        text.setCompoundDrawables(drawable, null , null , null );
        text.setPaddingRelative(ContentListView.dipToPx(mContext, (float)PADDING_LEFT)
                                - (drawable.getMinimumWidth() + padding), 0, 0, 0);
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        if (v instanceof TextView) {
            if (hasFocus) {
                ((TextView) v).setTextColor(mResources.getColor(R.color.color_text_focused));
            } else
                ((TextView) v).setTextColor(mResources.getColor(R.color.color_text_item));
        }
    }


    @Override
    public  boolean onKey (View v, int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (event.getKeyCode()) {
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_ENTER:
                    if (!isManualSearching())
                        onClick(v);
                    break;
            }
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
        }
        return false;
    }


    public void setProgressStatus() {
        int progress;

        switch (optionTag) {
            case OPTION_FINE_TUNE:
                progress = mSettingsManager.getFineTuneProgress();
                setProgress(progress);

                int freq = mSettingsManager.getFrequency();
                if (freq != -1) {
                    setFineTuneFrequency(freq + mSettingsManager.getFineTune());
                }
                break;
            case OPTION_MANUAL_SEARCH:
                progress = mSettingsManager.getManualSearchProgress();
                setProgress(progress);
                setManualSearchInfo(null);
                break;
            case OPTION_AUTO_SEARCH:
                setProgress(0);
                break;
            case OPTION_CHANNEL_INFO:
                break;
            default:
                progress = getIntegerFromString(mSettingsManager.getStatus(optionKey));
                setProgress(progress);
                break;
        }
    }

    public void setProgress(int progress) {
        RelativeLayout progressLayout = null;
        ViewGroup optionView = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
        for (int i = 0; i < optionView.getChildCount(); i++) {
            View view = optionView.getChildAt(i);
            if (view instanceof RelativeLayout) {
                progressLayout = (RelativeLayout) view;
                break;
            }
        }

        if (progressLayout != null) {
            for (int i = 0; i < progressLayout.getChildCount(); i++) {
                View child = progressLayout.getChildAt(i);
                if (child instanceof TextView) {
                    switch (optionTag) {
                        case OPTION_FINE_TUNE:
                            ((TextView) child).setText(mSettingsManager.getFineTuneStatus());
                            break;
                        default:
                            ((TextView) child).setText(Integer.toString(progress) + "%");
                            break;
                    }
                } else if (child instanceof ImageView) {
                    ((ImageView) child).setImageResource(getProgressResourceId(progress));
                }
            }
        }
    }

    public void setProgressOffset(int progress) {
        RelativeLayout progressLayout = null;
        ViewGroup optionView = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);

        for (int i = 0; i < optionView.getChildCount(); i++) {
            View view = optionView.getChildAt(i);
            if (view instanceof RelativeLayout) {
                progressLayout = (RelativeLayout) view;
                break;
            }
        }

        if (progressLayout != null) {
            for (int i = 0; i < progressLayout.getChildCount(); i++) {
                View child = progressLayout.getChildAt(i);
                if (child instanceof TextView) {
                    ((TextView) child).setText((progress >= 50 ? "+" : "") + Integer.toString(2 * (progress - 50)) + "%");
                } else if (child instanceof ImageView) {
                    ((ImageView) child).setImageResource(getProgressResourceId(progress));
                }
            }
        }
    }

    public int getProgressResourceId(int progress) {
        if (progress == 0)
            return R.drawable.progress_0;

        switch (progress / 5) {
            case 0:
                if (progress > 0)
                    return R.drawable.progress_1;
                else
                    return R.drawable.progress_m1;
            case 1:
                return R.drawable.progress_2;
            case 2:
                return R.drawable.progress_3;
            case 3:
                return R.drawable.progress_4;
            case 4:
                return R.drawable.progress_5;
            case 5:
                return R.drawable.progress_6;
            case 6:
                return R.drawable.progress_7;
            case 7:
                return R.drawable.progress_8;
            case 8:
                return R.drawable.progress_9;
            case 9:
                return R.drawable.progress_10;
            case 10:
                return R.drawable.progress_11;
            case 11:
                return R.drawable.progress_12;
            case 12:
                return R.drawable.progress_13;
            case 13:
                return R.drawable.progress_14;
            case 14:
                return R.drawable.progress_15;
            case 15:
                return R.drawable.progress_16;
            case 16:
                return R.drawable.progress_17;
            case 17:
                return R.drawable.progress_18;
            case 18:
                return R.drawable.progress_19;
            case 19:
                return R.drawable.progress_20;
            case 20:
                return R.drawable.progress_21;
            // minus progress
            case -1:
                return R.drawable.progress_m2;
            case -2:
                return R.drawable.progress_m3;
            case -3:
                return R.drawable.progress_m4;
            case -4:
                return R.drawable.progress_m5;
            case -5:
                return R.drawable.progress_m6;
            case -6:
                return R.drawable.progress_m7;
            case -7:
                return R.drawable.progress_m8;
            case -8:
                return R.drawable.progress_m9;
            case -9:
                return R.drawable.progress_m10;
            case -10:
                return R.drawable.progress_m11;
            case -11:
                return R.drawable.progress_m12;
            case -12:
                return R.drawable.progress_m13;
            case -13:
                return R.drawable.progress_m14;
            case -14:
                return R.drawable.progress_m15;
            case -15:
                return R.drawable.progress_m16;
            case -16:
                return R.drawable.progress_m17;
            case -17:
                return R.drawable.progress_m18;
            case -18:
                return R.drawable.progress_m19;
            case -19:
                return R.drawable.progress_m20;
            case -20:
                return R.drawable.progress_m21;

            default:
                break;
        }
        return R.drawable.progress_10;
    }

    private void setFineTune(int step) {
        mSettingsManager.setFineTuneStep(step);
        //setProgressOffset(mSettingsManager.getFineTuneProgress());
    }

    private void setFineTuneFrequency(int freq) {
        ViewGroup optionView = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);

        TextView frequency = (TextView) optionView.findViewById(R.id.fine_tune_frequency);
        TextView frequency_band = (TextView) optionView.findViewById(R.id.fine_tune_frequency_band);
        if (frequency != null && frequency_band != null) {
            double f = freq;
            f /= 1000 * 1000;
            frequency.setText(Double.toString(f) + mContext.getResources().getString(R.string.mhz));
            frequency_band.setText(parseFrequencyBand(f));
        }
    }

    private void stopSearch() {
        Log.d(TAG, "stopSearch");
        doScanCmd(DroidLogicTvUtils.ACTION_STOP_SCAN, null);
    }

    private void pauseSearch() {
        Log.d(TAG, "pauseSearch");
        doScanCmd(DroidLogicTvUtils.ACTION_ATV_PAUSE_SCAN, null);
        isSearching = SEARCH_PAUSED;
    }

    private void resumeSearch() {
        Log.d(TAG, "resumeSearch");
        doScanCmd(DroidLogicTvUtils.ACTION_ATV_RESUME_SCAN, null);
        isSearching = SEARCH_RUNNING;
    }
    private void startManualSearchAccordingMode() {
        Log.d(TAG, "startManualSearchAccordingMode");
        ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
        OptionEditText edit = (OptionEditText) parent.findViewById(R.id.manual_search_dtv_channel_manual);
        String channel = edit.getText().toString();
        if (channel == null || channel.length() == 0) {
            channel = (String)edit.getHint();
        }
        Bundle bundle = new Bundle();
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        int frequency = getDvbFrequencyByPd(Integer.valueOf(channel));
        Log.d(TAG, "frequency :" + frequency);
        bundle.putInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, frequency);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
        switch (((TvSettingsActivity)mContext).mManualScanEdit.checkAutoScanMode()) {
        case ManualScanEdit.SCAN_ATV_DTV:
            Log.d(TAG, "MANUAL_SCAN_ATV_DTV");
            if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                mode.setList(1);
            }
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_MANUAL);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
        break;
        case ManualScanEdit.SCAN_ONLY_ATV:
            Log.d(TAG, "MANUAL_SCAN_ONLY_ATV");
            if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                mode.setList(4);
            } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                mode.setList(5);
            }
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
        break;
        case ManualScanEdit.SCAN_ONLY_DTV:
            Log.d(TAG, "MANUAL_SCAN_ONLY_DTV");
            if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                mode.setList(1);
            }
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_MANUAL);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE);
        break;
        }
        bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
        mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
        doScanCmdForAtscManual(bundle);
        isSearching = SEARCH_RUNNING;
        mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
    }

    private void doScanCmdForAtscManual (Bundle bundle) {
        mTvControlManager.DtvSetTextCoding("GB2312");
        int dtvMode = (bundle == null ? TvChannelParams.MODE_DTMB
                : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB));
        TvControlManager.TvMode tvMode = TvControlManager.TvMode.fromMode(dtvMode);
        if ((tvMode.getExt() & 1) != 0) {
            int atvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_ATV_NONE
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE));
            int dtvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_DTV_NONE
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE));
            int atvFreq1 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0));
            int atvFreq2 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0));
            int dtvFreq = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0));
            int atvVideoStd = (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()));
            int atvAudioStd = (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));
            TvControlManager.FEParas fe = new TvControlManager.FEParas();
            fe.setMode(tvMode);
            fe.setVideoStd(atvVideoStd);
            fe.setAudioStd(atvAudioStd);
            TvControlManager.ScanParas scan = new TvControlManager.ScanParas();
            if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                atvFreq1 = dtvFreq;// - 5750000;
                atvFreq2 = dtvFreq;// + 5250000;
                scan.setAtvFrequency1(atvFreq1);
                scan.setAtvFrequency2(atvFreq2);
                scan.setMode(TvControlManager.ScanParas.MODE_ATV_DTV);
            } else {
                scan.setMode(TvControlManager.ScanParas.MODE_DTV_ATV);
            }
            scan.setAtvMode(atvScanType);
            scan.setDtvMode(dtvScanType);
            scan.setDtvFrequency1(dtvFreq);
            scan.setDtvFrequency2(dtvFreq);
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
            mTvControlManager.TvScan(fe, scan);
            }
    }

    private void startManualSearch() {
        Log.d(TAG, "startManualSearch");
        ViewGroup parent = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            Log.d(TAG, "ADTV");
            OptionEditText edit = (OptionEditText) parent.findViewById(R.id.manual_search_dtv_channel);
            String channel = edit.getText().toString();
            if (channel == null || channel.length() == 0)
                channel = (String)edit.getHint();
            Bundle bundle = new Bundle();
            TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
            int frequency = getDvbFrequencyByPd(Integer.valueOf(channel));
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, frequency);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_MANUAL);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_MANUAL);
            //bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0);
            //bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
            isSearching = SEARCH_RUNNING;
            mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            OptionEditText edit_from = (OptionEditText) parent.findViewById(R.id.manual_search_edit_from);
            OptionEditText edit_to = (OptionEditText) parent.findViewById(R.id.manual_search_edit_to);

            String str_begin = edit_from.getText().toString();
            if (str_begin == null || str_begin.length() == 0)
                str_begin = (String) edit_from.getHint();

            String str_end = edit_to.getText().toString();
            if (str_end == null || str_end.length() == 0)
                str_end = (String) edit_to.getHint();

            int from = Integer.valueOf(str_begin);
            int to =  Integer.valueOf(str_end);

            if (from < ATV_MIN_KHZ || from > ATV_MAX_KHZ || to < ATV_MIN_KHZ || to > ATV_MAX_KHZ)
                showToast(mResources.getString(R.string.error_atv_error_1));
            //else if (from > to)
            //    showToast(mResources.getString(R.string.error_atv_error_2));
            else if (Math.abs(to - from) < 1000)
                showToast(mResources.getString(R.string.error_atv_error_3));
            else {
                //mSettingsManager.sendBroadcastToTvapp("search_channel");
                mSettingsManager.setManualSearchProgress(0);
                mSettingsManager.setManualSearchSearchedNumber(0);
                //int ret = mTvControlManager.AtvManualScan(from * 1000, to * 1000,
                //                                TvControlManager.ATV_VIDEO_STD_PAL, TvControlManager.ATV_AUDIO_STD_DK);
                //Log.d(TAG, "mTvControlManager.AtvManualScan return " + ret);
                //if (ret < 0) {
                //    showToast(mResources.getString(R.string.error_atv_startSearch));
                //    return;
                //}
                Bundle bundle = new Bundle();
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, from * 1000);
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, to * 1000);
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
                bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
                mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_ATV_MANUAL_SCAN, bundle);
                doScanCmd(DroidLogicTvUtils.ACTION_ATV_MANUAL_SCAN, bundle); //ww
                isSearching = SEARCH_RUNNING;
                mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
            }
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            //mSettingsManager.sendBroadcastToTvapp("search_channel");
            OptionEditText edit = (OptionEditText) parent.findViewById(R.id.manual_search_dtv_channel);
            String channel = edit.getText().toString();
            if (channel == null || channel.length() == 0)
                channel = (String)edit.getHint();
            //mTvControlManager.DtvSetTextCoding("GB2312");
            //mTvControlManager.DtvManualScan(new TvControlManager.TvMode(mSettingsManager.getDtvType()).getMode(),
            //    getDvbFrequencyByPd(Integer.valueOf(channel)));
            //Intent intent = new Intent(DroidLogicTvUtils.ACTION_SUBTITLE_SWITCH);
            //intent.putExtra(DroidLogicTvUtils.EXTRA_SWITCH_VALUE, 0);
            //mContext.sendBroadcast(intent);
            Bundle bundle = new Bundle();
            TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, getDvbFrequencyByPd(Integer.valueOf(channel)));
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN, bundle);
            isSearching = SEARCH_RUNNING;
            mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
        }
        searchType = MANUAL_SEARCH;
    }

    public void setManualSearchEditStyle(View view) {
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            final OptionEditText edit = (OptionEditText) view.findViewById(R.id.manual_search_dtv_channel_manual);
            edit.setNextFocusLeftId(R.id.content_list);
            edit.setNextFocusRightId(edit.getId());
            edit.setNextFocusUpId(edit.getId());
            final TextView start_frequency = (TextView)view.findViewById(R.id.manual_search_dtv_start_frequency_manual);

            TextWatcher textWatcher = new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                public void onTextChanged(CharSequence s, int start, int before, int count) {
                }

                public void afterTextChanged(Editable s) {
                    if (s.length() > 0) {
                        int pos = s.length() - 1;
                        char c = s.charAt(pos);
                        if (c < '0' || c > '9') {
                            s.delete(pos, pos + 1);
                            showToast(mResources.getString(R.string.error_not_number));
                        } else {
                            String number = edit.getText().toString();
                            if (number == null || number.length() == 0)
                                number = (String)edit.getHint();

                            if (number.matches("[0-9]+"))
                                start_frequency.setText(parseChannelFrequency(getDvbFrequencyByPd(Integer.valueOf(number))));
                        }
                    }
                }
            };
            edit.addTextChangedListener(textWatcher);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            final OptionEditText edit = (OptionEditText) view.findViewById(R.id.manual_search_dtv_channel);
            edit.setNextFocusLeftId(R.id.content_list);
            edit.setNextFocusRightId(edit.getId());
            edit.setNextFocusUpId(edit.getId());
            final TextView start_frequency = (TextView)view.findViewById(R.id.manual_search_dtv_start_frequency);

            TextWatcher textWatcher = new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                public void onTextChanged(CharSequence s, int start, int before, int count) {
                }

                public void afterTextChanged(Editable s) {
                    if (s.length() > 0) {
                        int pos = s.length() - 1;
                        char c = s.charAt(pos);
                        if (c < '0' || c > '9') {
                            s.delete(pos, pos + 1);
                            showToast(mResources.getString(R.string.error_not_number));
                        } else {
                            String number = edit.getText().toString();
                            if (number == null || number.length() == 0)
                                number = (String)edit.getHint();

                            if (number.matches("[0-9]+"))
                                start_frequency.setText(parseChannelFrequency(getDvbFrequencyByPd(Integer.valueOf(number))));
                        }
                    }
                }
            };
            edit.addTextChangedListener(textWatcher);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            OptionEditText edit_from = (OptionEditText) view.findViewById(R.id.manual_search_edit_from);
            OptionEditText edit_to = (OptionEditText) view.findViewById(R.id.manual_search_edit_to);
            edit_from.setNextFocusLeftId(R.id.content_list);
            edit_from.setNextFocusRightId(edit_from.getId());
            edit_from.setNextFocusUpId(edit_from.getId());
            edit_to.setNextFocusLeftId(R.id.content_list);
            edit_to.setNextFocusRightId(edit_to.getId());

            TextWatcher textWatcher = new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                public void onTextChanged(CharSequence s, int start, int before, int count) {
                }

                public void afterTextChanged(Editable s) {
                    if (s.length() > 0) {
                        int pos = s.length() - 1;
                        char c = s.charAt(pos);
                        if (c < '0' || c > '9') {
                            s.delete(pos, pos + 1);
                            showToast(mResources.getString(R.string.error_not_number));
                        }
                    }
                }
            };
            edit_from.addTextChangedListener(textWatcher);
            edit_to.addTextChangedListener(textWatcher);
        }
    }

    private String parseChannelFrequency(double freq) {
        String frequency = mResources.getString(R.string.start_frequency);
        frequency += Double.toString(freq / (1000 * 1000)) + mResources.getString(R.string.mhz);
        return frequency;
    }

    private int getList() {
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)
                || mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                boolean isCable = mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C);
                int cableMode = ((TvSettingsActivity)mContext).mManualScanEdit.checkCableMode();

                switch (((TvSettingsActivity)mContext).mManualScanEdit.checkAutoScanMode()) {
                    case ScanEdit.SCAN_ATV_DTV:
                    case ScanEdit.SCAN_ONLY_DTV:
                    if (isCable) {
                        switch (cableMode) {
                            case ScanEdit.CABLE_MODE_STANDARD: return 1;
                            case ScanEdit.CABLE_MODE_LRC:      return 2;
                            case ScanEdit.CABLE_MODE_HRC:      return 3;
                        }
                        return 1;
                    }
                    return 0;
                    case ScanEdit.SCAN_ONLY_ATV:
                    if (isCable) {
                         switch (cableMode) {
                            case ScanEdit.CABLE_MODE_STANDARD: return 5;
                            case ScanEdit.CABLE_MODE_LRC:      return 6;
                            case ScanEdit.CABLE_MODE_HRC:      return 7;
                        }
                        return 5;
                    }
                    return 4;
                }
            }
        }
        return 0;
    }

    private int getDvbFrequencyByPd(int pd_number) {
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        mode.setList(getList());
        Log.d(TAG, "[get freq]type:"+mode.toType()+" use list:"+mode.getList());
        return getDvbFrequencyByPd(mode.getMode(), pd_number);
    }

    private int getDvbFrequencyByPd(int tvMode, int pdNumber) {
        ArrayList<FreqList> m_fList = mTvControlManager.DTVGetScanFreqList(tvMode);
        String type = TvControlManager.TvMode.fromMode(tvMode).toType();
        int size = m_fList.size();
        int the_freq = -1;

        if (type.equals(TvContract.Channels.TYPE_ATSC_T)
            || type.equals(TvContract.Channels.TYPE_ATSC_C))
            pdNumber = pdNumber - 1;

        if (pdNumber < 1)
            pdNumber = 1;

        for (int i = 0; i < size; i++) {
            if (pdNumber == m_fList.get(i).ID) {
                the_freq = m_fList.get(i).freq;
                break;
            }
        }
        return (the_freq < 0)? m_fList.get(0).freq : the_freq;
    }

    private void setManualSearchInfo(TvControlManager.ScannerEvent event) {
        Log.d(TAG, "mSettingsManager.getCurentVirtualTvSource()="+mSettingsManager.getCurentVirtualTvSource());
        ViewGroup optionView = (ViewGroup)((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
        if (event == null)
            return;
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            OptionListView listView = (OptionListView)optionView.findViewById(R.id.manual_search_dtv_info_manual);
            ArrayList<HashMap<String, Object>> dataList = getSearchedDtvInfo(event);
            SimpleAdapter adapter = new SimpleAdapter(mContext, dataList,
                    R.layout.layout_option_double_text,
                    new String[] {SettingsManager.STRING_NAME, SettingsManager.STRING_STATUS},
                    new int[] {R.id.text_name, R.id.text_status});
            listView.setAdapter(adapter);
        }else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            OptionListView listView = (OptionListView)optionView.findViewById(R.id.manual_search_dtv_info);
            ArrayList<HashMap<String, Object>> dataList = getSearchedDtvInfo(event);
            SimpleAdapter adapter = new SimpleAdapter(mContext, dataList,
                    R.layout.layout_option_double_text,
                    new String[] {SettingsManager.STRING_NAME, SettingsManager.STRING_STATUS},
                    new int[] {R.id.text_name, R.id.text_status});
            listView.setAdapter(adapter);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            OptionEditText begin = (OptionEditText)optionView.findViewById(R.id.manual_search_edit_from);
            TextView frequency = (TextView)optionView.findViewById(R.id.manual_search_frequency);
            TextView frequency_band = (TextView)optionView.findViewById(R.id.manual_search_frequency_band);
            TextView searched_number = (TextView)optionView.findViewById(R.id.manual_search_searched_number);
            double freq = 0.0;

            if (event == null) {
                freq = Double.valueOf((String)begin.getHint()).doubleValue();
                freq /= 1000;// KHZ to MHZ
            } else {
                freq = event.freq;
                freq /= 1000 * 1000;// HZ to MHZ
            }

            if (frequency != null && frequency_band != null && searched_number != null) {
                frequency.setText(Double.toString(freq) + mResources.getString(R.string.mhz));
                frequency_band.setText(parseFrequencyBand(freq));
                searched_number.setText(mResources.getString(R.string.searched_number) + ": " + channelNumber);
            }
        }
    }

    public ArrayList<HashMap<String, Object>> getSearchedDtvInfo (TvControlManager.ScannerEvent event) {
        ArrayList<HashMap<String, Object>> list =  new ArrayList<HashMap<String, Object>>();

        HashMap<String, Object> item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.frequency_l) + ":");
        item.put(SettingsManager.STRING_STATUS, Double.toString(event.freq / (1000 * 1000)) +
                 mResources.getString(R.string.mhz));
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.quality) + ":");
        item.put(SettingsManager.STRING_STATUS, event.quality + mResources.getString(R.string.db));
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.strength) + ":");
        item.put(SettingsManager.STRING_STATUS, event.strength + "%");
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.tv_channel) + ":");
        item.put(SettingsManager.STRING_STATUS, channelNumber);
        list.add(item);

        item = new HashMap<String, Object>();
        item.put(SettingsManager.STRING_NAME, mResources.getString(R.string.radio_channel) + ":");
        item.put(SettingsManager.STRING_STATUS, radioNumber);
        list.add(item);

        return list;
    }

    //add by ww
    private void startAutosearch() {
        Log.d(TAG, "startAutoSearch");
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        //mSettingsManager.sendBroadcastToTvapp("search_channel");
        if (mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            Log.d(TAG, "ADTV");

            deleteChannels(mode, true, true);

            Bundle bundle = new Bundle();
            int[] freqPair = new int[2];
            TvScanConfig.GetTvAtvMinMaxFreq(DroidLogicTvUtils.getCountry(mContext), freqPair);
            mode.setExt(mode.getExt() | 1);//mixed adtv
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_ALLBAND);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_AUTO);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, freqPair[0]);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, freqPair[1]);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);  //ww
            mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_DTV_CHANNEL_INDEX, -1);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            Log.d(TAG, "SOURCE_TYPE_TV");

            deleteChannels(mode, true, false);

            Bundle bundle = new Bundle();
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0);
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_ATV_AUTO_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_ATV_AUTO_SCAN, bundle);
            mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_ATV_CHANNEL_INDEX, -1);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV) {
            Log.d(TAG, "SOURCE_TYPE_DTV");

            deleteChannels(mode, true, false);

            Bundle bundle = new Bundle();
            bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
            bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
            mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);
            doScanCmd(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);
            mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_DTV_CHANNEL_INDEX, -1);
        }
        isSearching = SEARCH_RUNNING;
        searchType = AUTO_SEARCH;
        mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
    }

    private void deleteChannels(TvControlManager.TvMode mode, boolean deleteAtv, boolean deleteDtv)
    {
        Log.d(TAG, " delete mode:"+mode.getBase()+" atv:"+deleteAtv+" dtv:"+deleteDtv);
        if (deleteAtv) {
            if (mode.getBase() == TvChannelParams.MODE_ATSC)
                mSettingsManager.deleteChannels(TvContract.Channels.TYPE_NTSC);
            //else
                mSettingsManager.deleteChannels(TvContract.Channels.TYPE_PAL);
        }
        if (deleteDtv)
            mSettingsManager.deleteChannels(mode.toType());
    }

    private void startAutosearchAccrodingTvMode() {
        Log.d(TAG, "startAutosearchAccrodingTvMode");
        TvControlManager.TvMode mode = new TvControlManager.TvMode(mSettingsManager.getDtvType());
        //mSettingsManager.sendBroadcastToTvapp("search_channel");
        Bundle bundle = new Bundle();

        int[] freqPair = new int[2];
        TvScanConfig.GetTvAtvMinMaxFreq(DroidLogicTvUtils.getCountry(mContext), freqPair);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA1, freqPair[0]);
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA2, freqPair[1]);

        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys());
        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys());
        switch (((TvSettingsActivity)mContext).mScanEdit.checkAutoScanMode()) {
            case ScanEdit.SCAN_ATV_DTV:
                Log.d(TAG, "ADTV");

                deleteChannels(mode, true, true);

                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_STANDARD) {
                        mode.setList(1);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 5);
                    } else if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_LRC) {
                        mode.setList(2);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 6);
                    } else if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_HRC) {
                        mode.setList(3);
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 7);
                    }
                } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 4);
                }
                mode.setExt(mode.getExt() | 1);//mixed adtv
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_ALLBAND);
                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_DTMB))
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_AUTO);
                else
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
            break;
            case ScanEdit.SCAN_ONLY_ATV:
                Log.d(TAG, "SOURCE_TYPE_TV");

                deleteChannels(mode, true, false);

                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_STANDARD) {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 5);
                    } else if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_LRC) {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 6);
                    } else if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_HRC) {
                        bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 7);
                    }
                } else if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_T)) {
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_PARA5, 4);
                }

                mode.setExt(mode.getExt() | 1);//mixed adtv
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE);
                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_DTMB))
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_AUTO);
                else
                    bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_FREQ);
            break;
            case ScanEdit.SCAN_ONLY_DTV:
                Log.d(TAG, "SOURCE_TYPE_DTV");

                deleteChannels(mode, false, true);

                if (mSettingsManager.getDtvType().equals(TvContract.Channels.TYPE_ATSC_C)) {
                    if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_STANDARD) {
                        mode.setList(1);
                    } else if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_LRC) {
                        mode.setList(2);
                    } else if (((TvSettingsActivity)mContext).mScanEdit.checkCableMode() == ScanEdit.CABLE_MODE_HRC) {
                        mode.setList(3);
                    }
                }
                mode.setExt(mode.getExt() | 1);//mixed adtv
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_MODE, mode.getMode());
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_ALLBAND);
                bundle.putInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE);
            break;
        }
        bundle.putString(TvInputInfo.EXTRA_INPUT_ID,mSettingsManager.getInputId());
        mSettingsManager.sendBroadcastToTvapp(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);
        doScanCmd(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN, bundle);  //ww
        mTvControlDataManager.putInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_DTV_CHANNEL_INDEX, -1);
        isSearching = SEARCH_RUNNING;
        searchType = AUTO_SEARCH;
        mSettingsManager.setActivityResult(DroidLogicTvUtils.RESULT_UPDATE);
    }

    private void doScanCmd(String action, Bundle bundle) {
        Log.d(TAG, "doScanCmd:"+action);
        if (DroidLogicTvUtils.ACTION_ATV_AUTO_SCAN.equals(action)) {
            //ww//
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_ATV);
            mTvControlManager.AtvAutoScan(
                (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, mSettingsManager.getTvSearchTypeSys())),
                (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, mSettingsManager.getTvSearchSoundSys())),
                (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, 0)),
                (bundle == null ? 1 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, 1)));
        } else if (DroidLogicTvUtils.ACTION_ATV_MANUAL_SCAN.equals(action)) {
            if (bundle != null) {
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_ATV);
                mTvControlManager.AtvManualScan(
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0),
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0),
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()),
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));
            }
        } else if (DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN.equals(action)) {
            mTvControlManager.DtvSetTextCoding("GB2312");
            int dtvMode = (bundle == null ? TvChannelParams.MODE_DTMB
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB));
            TvControlManager.TvMode tvMode = TvControlManager.TvMode.fromMode(dtvMode);
            if ((tvMode.getExt() & 1) != 0) {//ADTV
                int atvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_ATV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE));
                int dtvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_DTV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE));
                int atvFreq1 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0));
                int atvFreq2 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0));
                int dtvFreq = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0));
                int atvVideoStd = (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()));
                int atvAudioStd = (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));
                int atvList = (bundle == null ? tvMode.getList()
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA5, tvMode.getList()));
                TvControlManager.FEParas fe = new TvControlManager.FEParas();
                fe.setMode(tvMode);
                fe.setVideoStd(atvVideoStd);
                fe.setAudioStd(atvAudioStd);
                TvControlManager.ScanParas scan = new TvControlManager.ScanParas();
                if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                    scan.setMode(TvControlManager.ScanParas.MODE_ATV_DTV);
                } else {
                    scan.setMode(TvControlManager.ScanParas.MODE_DTV_ATV);
                }
                scan.setAtvMode(atvScanType);
                scan.setDtvMode(dtvScanType);
                scan.setAtvFrequency1(atvFreq1);
                scan.setAtvFrequency2(atvFreq2);
                scan.setDtvFrequency1(dtvFreq);
                scan.setDtvFrequency2(dtvFreq);
                scan.setAtvModifier(TvControlManager.FEParas.K_MODE,
                    TvControlManager.TvMode.fromMode(dtvMode).setList(atvList).getMode());
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.TvScan(fe, scan);
            } else {
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.DtvAutoScan(dtvMode);
            }
        } else if (DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN.equals(action)) {
            mTvControlManager.DtvSetTextCoding("GB2312");
            int dtvMode = (bundle == null ? TvChannelParams.MODE_DTMB
                    : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB));
            TvControlManager.TvMode tvMode = TvControlManager.TvMode.fromMode(dtvMode);
            if ((tvMode.getExt() & 1) != 0) {//ADTV
                int atvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_ATV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_ATV, TvControlManager.ScanType.SCAN_ATV_NONE));
                int dtvScanType = (bundle == null ? TvControlManager.ScanType.SCAN_DTV_NONE
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_TYPE_DTV, TvControlManager.ScanType.SCAN_DTV_NONE));
                int atvFreq1 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA1, 0));
                int atvFreq2 = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA2, 0));
                int dtvFreq = (bundle == null ? 0 : bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0));
                int atvVideoStd = (bundle == null ? TvControlManager.ATV_VIDEO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA3, mSettingsManager.getTvSearchTypeSys()));
                int atvAudioStd = (bundle == null ? TvControlManager.ATV_AUDIO_STD_AUTO
                        : bundle.getInt(DroidLogicTvUtils.PARA_SCAN_PARA4, mSettingsManager.getTvSearchSoundSys()));

                if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                    atvFreq1 = dtvFreq - 9750000;
                    atvFreq2 = dtvFreq + 1250000;
                }
                TvControlManager.FEParas fe = new TvControlManager.FEParas();
                fe.setMode(tvMode);
                fe.setVideoStd(atvVideoStd);
                fe.setAudioStd(atvAudioStd);
                TvControlManager.ScanParas scan = new TvControlManager.ScanParas();
                if (atvScanType != TvControlManager.ScanType.SCAN_ATV_NONE) {
                    scan.setMode(TvControlManager.ScanParas.MODE_ATV_DTV);
                } else {
                    scan.setMode(TvControlManager.ScanParas.MODE_DTV_ATV);
                }
                scan.setAtvMode(atvScanType);
                scan.setDtvMode(dtvScanType);
                scan.setAtvFrequency1(atvFreq1);
                scan.setAtvFrequency2(atvFreq2);
                scan.setDtvFrequency1(dtvFreq);
                scan.setDtvFrequency2(dtvFreq);
                //scan.setDtvStandard(TvControlManager.ScanParas.DTVSTD_ATSC);
                //ww//
                mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.TvScan(fe, scan);
            } else {
            //ww//
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.OPEN_DEV_FOR_SCAN_DTV);
                mTvControlManager.DtvManualScan(
                    bundle.getInt(DroidLogicTvUtils.PARA_SCAN_MODE, TvChannelParams.MODE_DTMB),
                    bundle.getInt(DroidLogicTvUtils.PARA_MANUAL_SCAN, 0)
                );
            }
        } else if (DroidLogicTvUtils.ACTION_STOP_SCAN.equals(action)) {
            //ww//
            mTvControlManager.OpenDevForScan(DroidLogicTvUtils.CLOSE_DEV_FOR_SCAN);
            mTvControlManager.DtvStopScan();
        } else if (DroidLogicTvUtils.ACTION_ATV_PAUSE_SCAN.equals(action)) {
            mTvControlManager.AtvDtvPauseScan();
        } else if (DroidLogicTvUtils.ACTION_ATV_RESUME_SCAN.equals(action)) {
            mTvControlManager.AtvDtvResumeScan();
        }
    }

    private void setAutoSearchFrequency(TvControlManager.ScannerEvent event) {
        ViewGroup optionView = (ViewGroup) ((TvSettingsActivity) mContext).mOptionLayout.getChildAt(0);
        if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV
            || mSettingsManager.getCurentVirtualTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV) {
            OptionListView listView = (OptionListView)optionView.findViewById(R.id.auto_search_dtv_info);
            ArrayList<HashMap<String, Object>> dataList = getSearchedDtvInfo(event);
            SimpleAdapter adapter = new SimpleAdapter(mContext, dataList,
                    R.layout.layout_option_double_text,
                    new String[] {SettingsManager.STRING_NAME, SettingsManager.STRING_STATUS},
                    new int[] {R.id.text_name, R.id.text_status});
            listView.setAdapter(adapter);
        } else if (mSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            TextView frequency = (TextView) optionView.findViewById(R.id.auto_search_frequency_atv);
            TextView frequency_band = (TextView) optionView.findViewById(R.id.auto_search_frequency_band_atv);
            TextView searched_number = (TextView) optionView.findViewById(R.id.auto_search_searched_number_atv);
            if (frequency != null && frequency_band != null && searched_number != null) {
                double freq = event.freq;
                freq /= 1000 * 1000;
                frequency.setText(Double.toString(freq) + mResources.getString(R.string.mhz));
                frequency_band.setText(parseFrequencyBand(freq));
                searched_number.setText(mResources.getString(R.string.searched_number) + ": " + channelNumber);
            }
        }
    }



    private String parseFrequencyBand(double freq) {
        String band = "";
        if (freq > 44.25 && freq < 143.25) {
            band = "VHFL";
        } else if (freq >= 143.25 && freq < 426.25) {
            band = "VHFH";
        } else if (freq >= 426.25 && freq < 868.25) {
            band = "UHF";
        }
        return band;
    }

    private int getIntegerFromString(String str) {
        if (str != null && str.contains("%"))
            return Integer.valueOf(str.replaceAll("%", ""));
        else
            return -1;
    }

    @Override
    public void onEvent(TvControlManager.ScannerEvent event) {
        ChannelInfo channel = null;
        String name = null;

        if (!isSearching())
            return;

        switch (event.type) {
            case TvControlManager.EVENT_SCAN_PROGRESS:
                int isNewProgram = 0;
                Log.d(TAG, "onEvent:"+event.precent + "%\tfreq[" + event.freq + "] lock[" + event.lock + "] strength[" + event.strength + "] quality[" + event.quality + "]");

                if ((event.mode == TvChannelParams.MODE_ANALOG) && (event.lock == 0x11)) { //trick here
                    isNewProgram = 1;
                    Log.d(TAG, "Resume Scanning");
                    if ((mTvControlManager.AtvDtvGetScanStatus() & TvControlManager.ATV_DTV_SCAN_STATUS_PAUSED)
                            == TvControlManager.ATV_DTV_SCAN_STATUS_PAUSED)
                        resumeSearch();
                } else if ((event.mode != TvChannelParams.MODE_ANALOG) && (event.programName.length() != 0)) {
                    try {
                        name = TvMultilingualText.getText(event.programName);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    isNewProgram = 2;
                }

                if (isNewProgram == 1) {
                    channelNumber++;
                    Log.d(TAG, "New ATV Program");
                } else if (isNewProgram == 2) {
                    if (event.srvType == 1)
                        channelNumber++;
                    else if (event.srvType == 2)
                        radioNumber++;
                    Log.d(TAG, "New DTV Program : [" + name + "] type[" + event.srvType + "]");
                }

                setProgress(event.precent);
                if (optionTag == OPTION_MANUAL_SEARCH)
                    setManualSearchInfo(event);
                else
                    setAutoSearchFrequency(event);

                if ((event.mode == TvChannelParams.MODE_ANALOG) && (optionTag == OPTION_MANUAL_SEARCH)
                    && event.precent == 100)
                    stopSearch();
                break;

            case TvControlManager.EVENT_STORE_BEGIN:
                Log.d(TAG, "onEvent:Store begin");
                break;

            case TvControlManager.EVENT_STORE_END:
                Log.d(TAG, "onEvent:Store end");
                String prompt = mResources.getString(R.string.searched);
                if (channelNumber != 0) {
                    prompt += " " + channelNumber + " " + mResources.getString(R.string.tv_channel);
                    if (radioNumber != 0) {
                        prompt += ",";
                    }
                }
                if (radioNumber != 0) {
                    prompt += " " + radioNumber + " " + mResources.getString(R.string.radio_channel);
                }
                showToast(prompt);
                break;

            case TvControlManager.EVENT_SCAN_END:
                Log.d(TAG, "onEvent:Scan end");
                stopSearch();
                break;

            case TvControlManager.EVENT_SCAN_EXIT:
                Log.d(TAG, "onEvent:Scan exit.");
                SystemControlManager scm = SystemControlManager.getInstance();
                scm.setProperty("tv.channels.count", ""+(channelNumber+radioNumber));
                isSearching = SEARCH_STOPPED;
                ((TvSettingsActivity) mContext).finish();
                if (channelNumber == 0 && radioNumber == 0) {
                    showToast(mResources.getString(R.string.searched) + " 0 " + mResources.getString(R.string.channel));
                }
                break;
            default:
                break;
        }
    }

    private void createFactoryResetUi () {
        LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.layout_dialog, null);

        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        final AlertDialog mAlertDialog = builder.create();
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        //mAlertDialog.getWindow().setLayout(150, 320);

        TextView button_cancel = (TextView)view.findViewById(R.id.dialog_cancel);
        button_cancel.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mAlertDialog != null)
                    mAlertDialog.dismiss();
            }
        });
        button_cancel.setOnFocusChangeListener(this);
        TextView button_ok = (TextView)view.findViewById(R.id.dialog_ok);
        button_ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                mSettingsManager.doFactoryReset();
                ((PowerManager) mContext.getSystemService(Context.POWER_SERVICE)).reboot("");
            }
        });
        button_ok.setOnFocusChangeListener(this);
    }

    private void showToast(String text) {
        LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View layout = inflater.inflate(R.layout.layout_toast, null);

        TextView propmt = (TextView)layout.findViewById(R.id.toast_content);
        propmt.setText(text);

        toast = new Toast(mContext);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.setView(layout);
        toast.show();
    }

    public void release() {
        mTvControlManager.setScannerListener(null);
    }

}
