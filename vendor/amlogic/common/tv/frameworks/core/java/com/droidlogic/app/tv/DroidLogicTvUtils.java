/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.content.UriMatcher;
import android.media.tv.TvContract;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvContentRating;
import android.net.Uri;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.SparseArray;

import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;
import java.util.HashMap;
import java.util.Arrays;
import java.net.URLEncoder;
import java.net.URLDecoder;
import java.io.UnsupportedEncodingException;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;
import com.droidlogic.app.tv.TvScanConfig;
import android.util.Log;

public class DroidLogicTvUtils
{
    public static final String TAG = "DroidLogicTvUtils";
    private static final boolean DEBUG = false;
    /**
     * final parameters for {@link TvInpuptService.Session.notifySessionEvent}
     */
    public static final String SIG_INFO_EVENT = "sig_info_event";
    public static final String SIG_INFO_TYPE  = "sig_info_type";
    public static final String SIG_INFO_LABEL  = "sig_info_label";
    public static final String SIG_INFO_ARGS  = "sig_info_args";
    public static final String AV_SIG_SCRAMBLED  = "av_sig_scambled";

    public static final String SIG_INFO_C_DISPLAYNUM_EVENT = "sig_info_c_displaynum_event";
    public static final String SIG_INFO_C_DISPLAYNUM = "sig_info_c_displaynum";

    public static final String SIG_INFO_C_PROCESS_EVENT = "sig_info_c_process_event";
    public static final String SIG_INFO_C_STORE_BEGIN_EVENT = "sig_info_c_store_begin_event";
    public static final String SIG_INFO_C_STORE_END_EVENT = "sig_info_c_store_end_event";
    public static final String SIG_INFO_C_SCAN_BEGIN_EVENT = "sig_info_c_scan_begin_event";
    public static final String SIG_INFO_C_SCAN_END_EVENT = "sig_info_c_scan_end_event";
    public static final String SIG_INFO_C_SCAN_EXIT_EVENT = "sig_info_c_scan_exit_event";
    public static final String SIG_INFO_C_SCANNING_FRAME_STABLE_EVENT = "sig_info_c_scanning_frame_stable_event";

    public static final String SIG_INFO_C_TYPE = "type";
    public static final String SIG_INFO_C_PRECENT = "precent";
    public static final String SIG_INFO_C_TOTALCOUNT = "totalcount";
    public static final String SIG_INFO_C_LOCK = "lock";
    public static final String SIG_INFO_C_CNUM = "cnum";
    public static final String SIG_INFO_C_FREQ = "freq";
    public static final String SIG_INFO_C_PROGRAMNAME = "programName";
    public static final String SIG_INFO_C_SRVTYPE = "srvType";
    public static final String SIG_INFO_C_MSG = "msg";
    public static final String SIG_INFO_C_STRENGTH = "strength";
    public static final String SIG_INFO_C_QUALITY = "quality";
    //ATV
    public static final String SIG_INFO_C_VIDEOSTD = "videoStd";
    public static final String SIG_INFO_C_AUDIOSTD = "audioStd";
    public static final String SIG_INFO_C_ISAUTOSTD = "isAutoStd";
    public static final String SIG_INFO_C_FINETUNE = "fineTune";

    //DTV
    public static final String SIG_INFO_C_MODE = "mode";
    public static final String SIG_INFO_C_SR = "sr";
    public static final String SIG_INFO_C_MOD = "mod";
    public static final String SIG_INFO_C_BANDWIDTH = "bandwidth";
    public static final String SIG_INFO_C_OFM_MODE = "ofdm_mode";
    public static final String SIG_INFO_C_PARAS = "paras";
    public static final String SIG_INFO_C_TS_ID = "ts_id";
    public static final String SIG_INFO_C_ORIG_NET_ID = "orig_net_id";

    public static final String SIG_INFO_C_SERVICEiD = "serviceID";
    public static final String SIG_INFO_C_VID = "vid";
    public static final String SIG_INFO_C_VFMT = "vfmt";
    public static final String SIG_INFO_C_AIDS = "aids";
    public static final String SIG_INFO_C_AFMTS = "afmts";
    public static final String SIG_INFO_C_ALANGS = "alangs";
    public static final String SIG_INFO_C_ATYPES = "atypes";
    public static final String SIG_INFO_C_AEXTS = "aexts";
    public static final String SIG_INFO_C_PCR = "pcr";

    public static final String SIG_INFO_C_STYPES = "stypes";
    public static final String SIG_INFO_C_SIDS = "sids";
    public static final String SIG_INFO_C_SSTYPES = "sstypes";
    public static final String SIG_INFO_C_SID1S = "sid1s";
    public static final String SIG_INFO_C_SID2S = "sid2s";
    public static final String SIG_INFO_C_SLANGS = "slangs";

    public static final int SIG_INFO_TYPE_ATV    = 0;
    public static final int SIG_INFO_TYPE_DTV    = 1;
    public static final int SIG_INFO_TYPE_HDMI   = 2;
    public static final int SIG_INFO_TYPE_AV     = 3;
    public static final int SIG_INFO_TYPE_SPDIF  = 4;
    public static final int SIG_INFO_TYPE_ADTV  = 5;
    public static final int SIG_INFO_TYPE_AUX  = 6;
    public static final int SIG_INFO_TYPE_ARC  = 7;
    public static final int SIG_INFO_TYPE_OTHER  = 8;

    //AV
    public static final String SIG_INFO_AV_VCHIP = "av_vchip";
    public static final String SIG_INFO_AV_VCHIP_KEY = "av_aratings";

    //CC
    public static final String SIG_INFO_CC_DATA_INFO = "av_data_info";
    public static final String SIG_INFO_CC_DATA_INFO_KEY = "cc_data_mask";

    public static final String SIG_INFO_EAS_EVENT = "dtv_eas_event";
    public static final String SIG_INFO_EAS_STATUS = "eas_status";

    public static final String TV_SEARCHING_STATE = "searching_state";
    public static final String IS_NUMBER_SEARCH = "is_number_search";
    public static final String ACTIVITY_RESULT = "activity_result";

    public static final String EVENT_FREQUENCY = "frequency";
    public static final String EVENT_QUALITY = "quality";
    public static final String EVENT_STRENGTH = "strength";
    public static final String EVENT_CHANNEL_NUMBER = "channel_number";
    public static final String EVENT_RADIO_NUMBER = "radio_number";

    //menu display time
    public static final String KEY_MENU_TIME = "menu_time";
    public static final int DEFUALT_MENU_TIME = 0;

    /**
     * source input type need to switch
     */
    private static final int SOURCE_TYPE_START  = 0;
    private static final int SOURCE_TYPE_END    = 12;

    public static final int SOURCE_TYPE_ATV     = SOURCE_TYPE_START;
    public static final int SOURCE_TYPE_DTV     = SOURCE_TYPE_START + 1;
    public static final int SOURCE_TYPE_AV1     = SOURCE_TYPE_START + 2;
    public static final int SOURCE_TYPE_AV2     = SOURCE_TYPE_START + 3;
    public static final int SOURCE_TYPE_HDMI1   = SOURCE_TYPE_START + 4;
    public static final int SOURCE_TYPE_HDMI2   = SOURCE_TYPE_START + 5;
    public static final int SOURCE_TYPE_HDMI3   = SOURCE_TYPE_START + 6;
    public static final int SOURCE_TYPE_HDMI4   = SOURCE_TYPE_START + 7;
    public static final int SOURCE_TYPE_SPDIF   = SOURCE_TYPE_START + 8;
    public static final int SOURCE_TYPE_ADTV    = SOURCE_TYPE_START + 9;
    public static final int SOURCE_TYPE_AUX    = SOURCE_TYPE_START + 10;
    public static final int SOURCE_TYPE_ARC   = SOURCE_TYPE_START + 11;
    public static final int SOURCE_TYPE_OTHER   = SOURCE_TYPE_END;

    /**
     * source input id sync with {@link CTvin.h}
     */
    public static final int DEVICE_ID_ATV        = 0;
    public static final int DEVICE_ID_AV1        = 1;
    public static final int DEVICE_ID_AV2        = 2;
    public static final int DEVICE_ID_HDMI1      = 5;
    public static final int DEVICE_ID_HDMI2      = 6;
    public static final int DEVICE_ID_HDMI3      = 7;
    public static final int DEVICE_ID_HDMI4      = 8;
    public static final int DEVICE_ID_DTV        = 11;
    public static final int DEVICE_ID_AUX      = 17;
    public static final int DEVICE_ID_SPDIF      = 15;

    /*virtual device*/
    public static final int DEVICE_ID_ADTV       = 16;
    public static final int DEVICE_ID_ARC      = 18;
    public static final int DEVICE_ID_HDMIEXTEND = 19;

    public static final int DEVICE_ID_OFFSET = 0x1F;

    public static final int RESULT_OK = 1;
    public static final int RESULT_UPDATE = 2;
    public static final int RESULT_FAILED = 3;

    /**
     * action for {@link TvInputService.Session.onAppPrivateCommand}
     */
    public static final String ACTION_STOP_TV = "stop_tv";

    public static final String ACTION_STOP_PLAY = "stop_play";

    public static final String ACTION_TIMEOUT_SUSPEND = "droidlogic.intent.action.suspend";

    public static final String ACTION_UPDATE_TV_PLAY = "android.intent.action.update_tv";

    public static final String ACTION_DELETE_CHANNEL = "android.intent.action.tv_delete_channel";

    public static final String ACTION_SWITCH_CHANNEL = "android.intent.action.tv_switch_channel";

    public static final String ACTION_SUBTITLE_SWITCH = "android.intent.action.subtitle_switch";

    public static final String ACTION_AD_SWITCH = "android.intent.action.ad_switch";

    public static final String ACTION_AD_MIXING_LEVEL = "android.intent.action.ad_mixing_level";

    public static final String ACTION_AD_TRACK = "android.intent.action.ad_track";

    public static final String ACTION_CHANNEL_CHANGED = "android.intent.action.tv_channel_changed";

    public static final String ACTION_PROGRAM_APPOINTED = "droidlogic.intent.action.tv_appointed_program";
    public static final String ACTION_LIVETV_PROGRAM_APPOINTED = "droidlogic.intent.action.livetv_appointed_program";
    public static final String ACTION_DROID_PROGRAM_WATCH_APPOINTED = "droidlogic.intent.action.droid_appointed_program";
    public static final String EXTRA_APPOINTED_DELAY = "droidlogic.intent.action.droid_appointed_delay";
    public static final String EXTRA_APPOINTED_SETTING = "droidlogic.intent.action.droid_appointed_setting";
    public static final String EXTRA_APPOINTED_ACTION = "droidlogic.intent.action.droid_appointed_action";
    public static final String ACTION_DROID_PROGRAM_RECORD_APPOINTED = "droidlogic.intent.action.droid_appointed_record_program";
    public static final String ACTION_DROID_PROGRAM_RECORD_OPERATION = "droidlogic.intent.action.droid_appointed_record_operation";
    public static final String ACTION_DROID_PROGRAM_RECORD_STATUS = "droidlogic.intent.action.REQUEST_RECORD_STATUS";
    public static final String ACTION_DROID_PROGRAM_RECORD_STATUS_JSON = "droidlogic.intent.action.REQUEST_RECORD_STATUS_JSON";
    public static final String KEY_LIVETV_PROGRAM_APPOINTED = "livetv_appointed_program";
    public static final String KEY_ENABLE_NOSIGNAL_TIMEOUT = "enable_nosignal_timeout";
    public static final String PROP_DISABLE_NOSIGNAL_TIMEOUT_FUNCTION = "persist.tv.disable.nosig.timeout";
    public static final String KEY_ENABLE_SUSPEND_TIMEOUT = "enable_suspend_timeout";
    public static final String ACTION_STOP_EPG_ACTIVITY = "droidlogic.intent.action.stop_epg_activity";
    public static final String ACTION_REMOVE_ALL_DVR_RECORDS = "droidlogic.intent.action.remove_all_dvr_records";
    public static final String ACTION_DVR_RESPONSE = "droidlogic.intent.action.dvr_response";
    public static final String KEY_DVR_DELETE_RESULT_LIST = "dvr_delete_result_list";
    public static final String KEY_DTVKIT_SEARCH_TYPE = "dtvkit_search_type";//auto or manual
    public static final String KEY_DTVKIT_SEARCH_TYPE_AUTO = "dtvkit_auto_search";//auto
    public static final String KEY_DTVKIT_SEARCH_TYPE_MANUAL = "dtvkit_manual_search_type";//manual
    public static final String KEY_LIVETV_PVR_STATUS = "livetv_pvr_status";//manual
    public static final String VALUE_LIVETV_PVR_SCHEDULE_AVAILABLE = "livetv_pvr_schedule";
    public static final String VALUE_LIVETV_PVR_RECORD_PROGRAM_AVAILABLE = "livetv_pvr_program";

    public static final String ACTION_ATV_AUTO_SCAN = "atv_auto_scan";
    public static final String ACTION_DTV_AUTO_SCAN = "dtv_auto_scan";
    public static final String ACTION_ATV_MANUAL_SCAN = "atv_manual_scan";
    public static final String ACTION_DTV_MANUAL_SCAN = "dtv_manual_scan";
    public static final String ACTION_ATV_PAUSE_SCAN = "atv_pause_scan";
    public static final String ACTION_ATV_RESUME_SCAN = "atv_resume_scan";
    public static final String ACTION_STOP_SCAN = "stop_scan";
    public static final String PARA_MANUAL_SCAN = "scan_freq";
    public static final String PARA_MANUAL_SCAN_END = "scan_freq_end";
    public static final String PARA_SCAN_MODE = "scan_mode";
    public static final String PARA_SCAN_TYPE_DTV = "scan_type_dtv";
    public static final String PARA_SCAN_TYPE_ATV = "scan_type_atv";
    public static final String PARA_SCAN_PARA1 = "scan_para1";
    public static final String PARA_SCAN_PARA2 = "scan_para2";
    public static final String PARA_SCAN_PARA3 = "scan_para3";
    public static final String PARA_SCAN_PARA4 = "scan_para4";
    public static final String PARA_SCAN_PARA5 = "scan_para5";
    public static final String PARA_SCAN_PARA6 = "scan_para6";

    /*define number search program type*/
    public static final String ATVPROGRAM = "atv_program";
    public static final String DTVPROGRAM = "dtv_program";
    public static final String RADIOPROGRAM = "radio_program";
    public static final String TOTALNUMBER = "total_number";
    public static final String ATVNUMBER = "atv_number";
    public static final String DTVNUMBER = "dtv_number";
    public static final String RADIONUMBER = "radio_number";
    public static final String FIRSTAUTOFOUNDFREQUENCY = "first_found_frequency";
    public static final String AUTO_SEARCH_MODE = "auto_search_mode";
    public static final String TV_SEARCH_DTMB_NUMBER = "search_dtmb_number";

    /*auto tracks call*/
    public static final String ACTION_DTV_AUTO_TRACKS = "dtv_auto_tracks";

   /*when it's on ,block all program without rating*/
   public static final String ACTION_BLOCK_NORATING = "block_norating";
   public static final String PARAM_NORATING_ENABLE = "enable_norating";
   public static final String ACTION_BLOCK_CHANNEL= "block_channel";
   public static final String PARAM_CHANNEL_BLOCK_ENABLE= "enable_channel_block";
   public static final int NORATING_OFF = 0;
   public static final int NORATING_ON = 1;
   public static final int NORATING_UNLOCK_CURRENT = 2;

    /*set type call*/
    public static final String ACTION_DTV_SET_TYPE = "dtv_set_type";
    public static final String PARA_TYPE = "dtv_type";

    /*ad audio call*/
    public static final String ACTION_DTV_ENABLE_AUDIO_AD = "dtv_enable_audio_ad";
    public static final String PARA_ENABLE = "enable";
    public static final String PARA_VALUE1 = "value1";

    /**
     * Other extra names for {@link TvInputInfo.createSetupIntent#intent} except for input id.
     */
    public static final String EXTRA_CHANNEL_ID = "tv_channel_id";

    public static final String EXTRA_CHANNEL_DEVICE_ID = "channel_device_id";

    public static final String EXTRA_PROGRAM_ID = "tv_program_id";

    public static final String EXTRA_KEY_CODE = "key_code";

    public static final String EXTRA_CHANNEL_NUMBER = "channel_number";

    public static final String EXTRA_IS_RADIO_CHANNEL = "is_radio_channel";

    public static final String EXTRA_SWITCH_VALUE = "switch_val";

    public static final String EXTRA_MORE = "bundle";

    public static final int OPEN_DEV_FOR_SCAN_ATV = 1;
    public static final int OPEN_DEV_FOR_SCAN_DTV = 2;
    public static final int CLOSE_DEV_FOR_SCAN = 3;

    public static final int SEARCH_TYPE_ATV_INDEX = 0;
    public static final int SEARCH_TYPE_DTV_INDEX1 = 1;
    public static final int SEARCH_TYPE_DTV_INDEX2 = 2;
    public static final int SEARCH_TYPE_DTV_INDEX3 = 3;

    //tif private command action
    public static final String ACTION_TIF_BEFORE_TUNE = "tif_before_tune";
    public static final String ACTION_TIF_AFTER_TUNE = "tif_after_tune";

    public static final String SYSTEM_CAPTION_STYLE_ENABLE = "accessibility_captioning_style_enabled";

    /**
     * used for TvSettings to switch hdmi source
     * {@link #SOURCE_NAME}
     */
    public static final String SOURCE_INPUT_ID = "id";

    /**
     * used for table {@link Settings#System}.
     * {@link #TV_START_UP_ENTER_APP} indicates whether enter into TV but not home activity,
     * and enter into it when value is {@value >0}.
     * {@link #TV_START_UP_APP_NAME} indicates the class name of TV, format is pkg_name/.class_name
     */
    public static final String TV_START_UP_ENTER_APP = "tv_start_up_enter_app";
    public static final String TV_START_UP_APP_NAME = "tv_start_up_app_name";
    public static final String TV_KEY_DEFAULT_LANGUAGE = "default_language";
    public static final String TV_KEY_SUBTITLE_SWITCH = "sub_switch";
    public static final String TV_KEY_AD_SWITCH = "ad_switch";
    public static final String TV_KEY_AD_MIX = "ad_mix_level";
    public static final String TV_KEY_AD_TRACK = "ad_track";

    public static final String TV_CURRENT_DEVICE_ID = "tv_current_device_id";
    public static final String TV_ATV_CHANNEL_INDEX = "tv_atv_channel_index";
    public static final String TV_DTV_CHANNEL_INDEX  = "tv_dtv_channel_index";
    public static final String TV_CURRENT_CHANNEL_IS_RADIO = "tv_current_channel_is_radio";
    public static final String BLOCK_NORATING = "block_norating";
    public static final String TV_CURRENT_BLOCK_STATUS = "tv_current_channel_blocked";
    public static final String TV_CURRENT_CHANNELBLOCK_STATUS = "tv_current_channelblock_blocked";

    public static final String TV_KEY_DTV_NUMBER_MODE = "tv_dtv_number_mode";
    public static final String TV_KEY_DTV_TYPE = "tv_dtv_type";

    public static final String SIGNAL_TYPE_ERROR = "error";
    public static final String ALL_CHANNELS_NUMBER = "all_channels_number";
    public static final String DTV_TYPE_SWITCHED = "dtv_type_switched";
    public static final String TV_SEARCH_COUNTRY = "tv_country";
    public static final String TV_SEARCH_MODE = "tv_search_mode";
    public static final String TV_SEARCH_MODE_MANUAL = "manual";
    public static final String TV_SEARCH_MODE_AUTO = "auto";
    public static final String TV_SEARCH_MODE_NIT = "nit";
    public static final String TV_SEARCH_TYPE = "tv_search_type";
    public static final String TV_SEARCH_ORDER = "tv_search_order";
    public static final String TV_SEARCH_ATV_DTV_MODE= "search_atv_dtv_flag";
    public static final String TV_SEARCH_TYPE_CHANGED = "tv_search_type_changed";
    public static final String TV_SEARCH_INPUTID = "tv_search_inputid";
    public static final String TV_CURRENT_INPUTID = "tv_current_inputid";
    public static final String TV_SEARCH_INPUTID_CHANGED = "tv_search_inputid_changed";
    public static final String TV_DROIDLOGIC_PACKAGE = "com.droidlogic.tvinput";
    public static final String ATV_CHANNEL_INDEX = "atv_channel_index";
    public static final String DTV_CHANNEL_INDEX = "dtv_channel_index";
    public static final String TV_INPUT_ID = "tv_input_id";
    public static final String DB_ID_TV_SOURCE_TYPE = "db_id_tv_source_type";
    public static final String TV_SEARCH_ATSC_CLIST = "atsc_c_list_mode";
    public static final String TV_NUMBER_SEARCH_MODE = "number_search_mode";
    public static final String TV_NUMBER_SEARCH_NUMBER = "number_search_number";
    public static final String TV_SEARCH_DVBC_QAM = "search_dvbc_qam";
    public static final String ATSC_TV_SEARCH_SYS = "atsc_tv_search_sys";
    public static final String ATSC_TV_SEARCH_SOUND_SYS = "atsc_tv_search_sound_sys";

    public static final int TV_SEARCH_DVBC_QAM16 = 1;
    public static final int TV_SEARCH_DVBC_QAM32 = 2;
    public static final int TV_SEARCH_DVBC_QAM64 = 3;
    public static final int TV_SEARCH_DVBC_QAM128 = 4;
    public static final int TV_SEARCH_DVBC_QAM256 = 5;
    public static final int TV_SEARCH_DVBC_QAMAUTO = 6;
    public static final String TV_SEARCH_DVBC_SYMBOL_RATE = "search_dvbc_symbol_rate";
    public static final int TV_SEARCH_DVBC_SYMBOL_RATE_DEFAULT = 6875000;

    public static final int TV_SEARCH_ORDER_LOW = 0;
    public static final int TV_SEARCH_ORDER_HIGH = 1;
    public static final String[] SEARCH_ORDER_LIST = {"low_to_high", "high_to_low"};

    public static final int TV_SEARCH_ATV = 0;
    public static final int TV_SEARCH_DTV = 1;
    public static final int TV_SEARCH_ATV_DTV = 2;

    public static final int TV_ATSC_MODE_STANDARD = 0;
    public static final int TV_ATSC_MODE_LRC = 1;
    public static final int TV_ATSC_MODE_HRC = 2;
    public static final int TV_ATSC_MODE_AUTO = 3;

    public static final int VIDEO_UNAVAILABLE_REASON_NODATA = 5;

    private static final UriMatcher sUriMatcher;
    public static final int NO_MATCH = UriMatcher.NO_MATCH;
    public static final int MATCH_CHANNEL = 1;
    public static final int MATCH_CHANNEL_ID = 2;
    public static final int MATCH_CHANNEL_ID_LOGO = 3;
    public static final int MATCH_PASSTHROUGH_ID = 4;
    public static final int MATCH_PROGRAM = 5;
    public static final int MATCH_PROGRAM_ID = 6;
    public static final int MATCH_WATCHED_PROGRAM = 7;
    public static final int MATCH_WATCHED_PROGRAM_ID = 8;
    static {
        sUriMatcher = new UriMatcher(NO_MATCH);
        sUriMatcher.addURI(TvContract.AUTHORITY, "channel", MATCH_CHANNEL);
        sUriMatcher.addURI(TvContract.AUTHORITY, "channel/#", MATCH_CHANNEL_ID);
        sUriMatcher.addURI(TvContract.AUTHORITY, "channel/#/logo", MATCH_CHANNEL_ID_LOGO);
        sUriMatcher.addURI(TvContract.AUTHORITY, "passthrough/*", MATCH_PASSTHROUGH_ID);
        sUriMatcher.addURI(TvContract.AUTHORITY, "program", MATCH_PROGRAM);
        sUriMatcher.addURI(TvContract.AUTHORITY, "program/#", MATCH_PROGRAM_ID);
        sUriMatcher.addURI(TvContract.AUTHORITY, "watched_program", MATCH_WATCHED_PROGRAM);
        sUriMatcher.addURI(TvContract.AUTHORITY, "watched_program/#", MATCH_WATCHED_PROGRAM_ID);
    }

    public static boolean isChina(Context mContext) {
        return TextUtils.equals(getCountry(mContext), "CN");
    }

    public static int getCurrentDtmbPhysicalNumber(Context context) {
        int mode = TvControlDataManager.getInstance(context).getInt(context.getContentResolver(), TV_SEARCH_DTMB_NUMBER, 0);
        Log.d(TAG, "getCurrentDtmbPhysicalNumber = " + mode);
        return mode;
    }

    public static void setCurrentDtmbPhysicalNumber(Context context, int number) {
        Log.d(TAG, "setCurrentDtmbPhysicalNumber = " + number);
        TvControlDataManager.getInstance(context).putInt(context.getContentResolver(), TV_SEARCH_DTMB_NUMBER, number);
    }

    public static boolean isAtscCountry(Context mContext) {
        return TextUtils.equals(getCountry(mContext), "US") || TextUtils.equals(getCountry(mContext), "MX");
    }

    public static void setCountry(Context mContext, String country) {
        Log.d(TAG, "setCountry = " + country);
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_COUNTRY, country);
    }

    public static String getCountry(Context mContext) {
        String country = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), TV_SEARCH_COUNTRY);
        if (TextUtils.isEmpty(country) || !TvScanConfig.TV_COUNTRY.contains(country)) {
            String defaultCountry = TvScanConfig.getTvDefaultCountry();
            Log.i(TAG, "getCountry error, country: " + country + " not support, set default country:" + defaultCountry);
            country = defaultCountry;
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_COUNTRY, country);
        }
        Log.d(TAG, "getCountry = " + country);
        return country;
    }

    public static void setSearchType(Context mContext, String value) {
        Log.d(TAG, "setSearchType = " + value);
        if (getSearchType(mContext) != value) {
            Log.d(TAG, "tv_search_type was changed");
            TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_TYPE_CHANGED, 1);
        }
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_TYPE, value);
    }

    public static String getSearchType(Context mContext) {
        String type = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), TV_SEARCH_TYPE);
        if (type == null) {
            String countryId = getCountry(mContext);
            ArrayList<String> searchTypeList = TvScanConfig.GetTvDtvSystemList(countryId);
            if (TvScanConfig.GetTvAtvSupport(countryId)) {
                int i = TvScanConfig.TV_SEARCH_TYPE_ATSC_C_AUTO_INDEX;
                for (; i<=TvScanConfig.TV_SEARCH_TYPE_ATSC_T_INDEX; i++) {
                    if (searchTypeList.contains(TvScanConfig.TV_SEARCH_TYPE.get(i))) {
                        break;
                    }
                }
                // ATSC need be show first
                if (i <= TvScanConfig.TV_SEARCH_TYPE_ATSC_T_INDEX) {
                    type = TvScanConfig.TV_SEARCH_TYPE.get(i);
                } else {
                    type = TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX);
                }
            } else {
                type = searchTypeList.get(0);
            }
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_TYPE, type);
            Log.i(TAG, "getSearchType null, set default search type:" + type);
        }
        Log.d(TAG, "getSearchType = " + type);
        return type;
    }

    public static void setSearchMode(Context mContext, String mode) {
        Log.d(TAG, "setSearchMode = " + mode);
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_MODE, mode);
    }

    public static String getSearchMode(Context mContext) {
        String mode = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), TV_SEARCH_MODE);
        if (mode == null) {
            ArrayList<String> searchMode = TvScanConfig.GetTvSearchModeList(getCountry(mContext));
            mode = searchMode.get(0);
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_MODE, mode);
            Log.i(TAG, "getSearchMode null, set default search mode:" + mode);
        }
        Log.d(TAG, "getSearchMode = " + mode);
        return mode;
    }

    public static void setDtvType(Context mContext, String type) {
        Log.d(TAG, "setDtvType = " + type);
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_KEY_DTV_TYPE, type);
    }

    public static String getDtvType(Context mContext) {
        String type = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), TV_KEY_DTV_TYPE);
        if (type == null) {
            String searchType  = getSearchType(mContext);
            type = DTV_SEARCH_TYPE_LIST.get(searchType);
            Log.i(TAG, "getDtvType null, set default dtv type:" + type);
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_KEY_DTV_TYPE, type);
        }
        Log.d(TAG, "getDtvType = " + type);
        return type;
    }

    public static void setAtsccListMode(Context mContext, int mode) {
        Log.d(TAG, "setAtsccListMode = " + mode);
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_ATSC_CLIST, mode);
    }

    public static int getAtsccListMode(Context mContext) {
        int mode = TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_ATSC_CLIST, TV_ATSC_MODE_STANDARD);
        Log.d(TAG, "getAtsccListMode = " + mode);
        return mode;
    }

    public static void setTvSearchTypeSys(Context mContext, String colorSys) {
        Log.d(TAG, "setTvSearchTypeSys = " + colorSys);
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), ATSC_TV_SEARCH_SYS, colorSys);
    }

    public static String getTvSearchTypeSys(Context mContext) {
        String colorSys = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), ATSC_TV_SEARCH_SYS);
        if (TextUtils.isEmpty(colorSys) || !TvScanConfig.TV_COLOR_SYS.contains(colorSys)) {
            ArrayList<String> clrSysList = TvScanConfig.GetTvAtvColorSystemList(getCountry(mContext));
            colorSys = clrSysList.get(0);
            Log.i(TAG, "getTvSearchTypeSys null, set default color system:" + colorSys);
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), ATSC_TV_SEARCH_SYS, colorSys);
        }
        Log.d(TAG, "getTvSearchTypeSys = " + colorSys);
        return colorSys;
    }

    public static void setTvSearchSoundSys(Context mContext, String soundSys) {
        Log.d(TAG, "setTvSearchSoundSys = " + soundSys);
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), ATSC_TV_SEARCH_SOUND_SYS, soundSys);
    }

    public static String getTvSearchSoundSys(Context mContext) {
        String soundSys = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), ATSC_TV_SEARCH_SOUND_SYS);
        if (TextUtils.isEmpty(soundSys) || !TvScanConfig.TV_SOUND_SYS.contains(soundSys)) {
            ArrayList<String> audSysList = TvScanConfig.GetTvAtvColorSystemList(getCountry(mContext));
            soundSys = audSysList.get(0);
            Log.i(TAG, "getTvSearchSoundSys null, set default audio system:" + soundSys);
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), ATSC_TV_SEARCH_SOUND_SYS, soundSys);
        }
        Log.d(TAG, "getTvSearchSoundSys = " + soundSys);
        return soundSys;
    }

    public static int getSearchStatus (Context mContext) {
        return TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCHING_STATE, 0);
    }

    public static void resetSearchStatus (Context mContext) {
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCHING_STATE, 0);
    }

    public static void resetSearchTypeChanged(Context mContext) {
        int searchTypeChanged = TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_TYPE_CHANGED, 0);
        if (searchTypeChanged == 1) {
            TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_TYPE_CHANGED, 0);
        }
    }

    public static void setSearchOrder(Context mContext, int mode) {
        Log.d(TAG, "setSearchOrder = " + mode);
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_ORDER, mode);
    }

    public static int getSearchOrder(Context mContext) {
        int mode = TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_INPUTID_CHANGED, TV_SEARCH_ORDER_LOW);
        Log.d(TAG, "getSearchOrder = " + mode);
        return mode;
    }

    public static void setAtvDtvModeFlag(Context mContext, int mode) {
        Log.d(TAG, "setAtvDtvModeFlag = " + mode);
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_ATV_DTV_MODE, mode);
    }

    public static int getAtvDtvModeFlag(Context mContext) {
        int mode = TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_ATV_DTV_MODE, TV_SEARCH_ATV);
        Log.d(TAG, "getAtvDtvModeFlag = " + mode);
        return mode;
    }

    public static void setDvbcQamMode(Context mContext, int mode) {
        Log.d(TAG, "setDvbcQamMode = " + mode);
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_DVBC_QAM, mode);
    }

    public static int getDvbcQamMode(Context mContext) {
        int mode = TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_DVBC_QAM, TV_SEARCH_DVBC_QAM16);
        Log.d(TAG, "getDvbcQamMode = " + mode);
        return mode;
    }

    public static void setDvbcSymbolRate(Context mContext, int rate) {
        Log.d(TAG, "setDvbcSymbolRate = " + rate);
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_DVBC_SYMBOL_RATE, rate);
    }

    public static int getDvbcSymbolRate(Context mContext) {
        int rate = TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_DVBC_SYMBOL_RATE, TV_SEARCH_DVBC_SYMBOL_RATE_DEFAULT);
        Log.d(TAG, "getDvbcSymbolRate = " + rate);
        return rate;
    }

    public static void setSearchInputId(Context mContext, String value, boolean ispassthrough) {
        Log.d(TAG, "setSearchInputId = " + value);
        if (!ispassthrough && !TextUtils.equals(getSearchInputId(mContext), value)) {
            Log.d(TAG, "setSearchInputId inputid was changed");
            TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), TV_SEARCH_INPUTID_CHANGED, 1);
        }
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_SEARCH_INPUTID, value);
    }

    public static int getSearchInputIdChangeStatus(Context mContext) {
        return TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), TV_SEARCH_INPUTID_CHANGED, 0);
    }

    public static void setCurrentInputId(Context mContext, String value) {
        Log.d(TAG, "setCurrentInputId = " + value);
        TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), TV_CURRENT_INPUTID, value);
    }

    public static String getCurrentInputId(Context mContext) {
        return TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), TV_CURRENT_INPUTID);
    }

    public static boolean isDroidLogicInput(String inputid) {
        if (inputid != null && inputid.startsWith("com.droidlogic.tvinput")) {
            return true;
        }
        return false;
    }

    public static String getSearchInputId(Context mContext) {
        String inputid = TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), TV_SEARCH_INPUTID);
        return inputid;
    }

    public static boolean isATV(Context mContext) {
        return (getSearchType(mContext).equals(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX)));
    }

    public static boolean isDTV(Context mContext) {
        return (!getSearchType(mContext).equals(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX)));
    }

    public static void saveInputId(Context mContext, String inputId) {
        if (inputId != null) {
            TvControlDataManager.getInstance(mContext).putString(mContext.getContentResolver(), DroidLogicTvUtils.TV_INPUT_ID, inputId);
        }
    }

    public static String getInputId(Context mContext) {
        return TvControlDataManager.getInstance(mContext).getString(mContext.getContentResolver(), DroidLogicTvUtils.TV_INPUT_ID);
    }


    public static void setTvSourceType(Context mContext, int sourceType) {
        TvControlDataManager.getInstance(mContext).putInt(mContext.getContentResolver(), DB_ID_TV_SOURCE_TYPE, sourceType);
    }

    public static int getTvSourceType(Context mContext) {
        return TvControlDataManager.getInstance(mContext).getInt(mContext.getContentResolver(), DB_ID_TV_SOURCE_TYPE, DEVICE_ID_ADTV);
    }

    public static String getCurrentSignalType(Context context) {
        String dtvType = TvControlDataManager.getInstance(context).getString(context.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_TYPE);
        if (dtvType == null) {
            TvControlManager mTvControlManager = TvControlManager.getInstance();
            String config = mTvControlManager.GetTVSupportCountries();
            String[] supportcountry = config.split(",");
            switch (supportcountry[0]) {
                case "US":
                case "MX":
                    dtvType = TvContract.Channels.TYPE_ATSC_T;
                    break;
                case "CN":
                    dtvType = TvContract.Channels.TYPE_DTMB;
                    break;
                case "IN":
                case "ID":
                case "DE":
                    dtvType = DroidLogicTvUtils.SIGNAL_TYPE_ERROR;
                    break;
            }
        }
        Log.d(TAG, "getCurrentSignalType = " + dtvType);
        return dtvType;
    }

    public static int matchsWhich(Uri uri) {
        return sUriMatcher.match(uri);
    }

    public static int getChannelId(Uri uri) {
        if (sUriMatcher.match(uri) == MATCH_CHANNEL_ID)
            return Integer.parseInt(uri.getLastPathSegment());
        return -1;
    }

    public static boolean isHardwareInput(TvInputInfo info) {
        if (info == null)
            return false;

        String[] temp = info.getId().split("/");
        return temp.length==3 ? true : false;
    }

    public static boolean isHardwareInput(String input_id) {
        if (TextUtils.isEmpty(input_id))
            return false;
        String[] temp = input_id.split("/");
        return temp.length==3 ? true : false;
    }

    public static int getHardwareDeviceId(TvInputInfo info) {
        if (info == null)
            return -1;

        String[] temp = info.getId().split("/");
        return temp.length==3 ? Integer.parseInt(temp[2].substring(2)) : -1;
    }

    public static int getHardwareDeviceId(String input_id) {
        if (TextUtils.isEmpty(input_id))
            return -1;
        String[] temp = input_id.split("/");
        return temp.length==3 ? Integer.parseInt(temp[2].substring(2)) : -1;
    }

    public static int getSourceType(int device_id) {
        int ret = SOURCE_TYPE_OTHER;
        switch (device_id) {
            case DEVICE_ID_ATV:
                ret = SOURCE_TYPE_ATV;
                break;
            case DEVICE_ID_DTV:
                ret = SOURCE_TYPE_DTV;
                break;
            case DEVICE_ID_AV1:
                ret = SOURCE_TYPE_AV1;
                break;
            case DEVICE_ID_AV2:
                ret = SOURCE_TYPE_AV2;
                break;
            case DEVICE_ID_HDMI1:
                ret = SOURCE_TYPE_HDMI1;
                break;
            case DEVICE_ID_HDMI2:
                ret = SOURCE_TYPE_HDMI2;
                break;
            case DEVICE_ID_HDMI3:
                ret = SOURCE_TYPE_HDMI3;
                break;
            case DEVICE_ID_HDMI4:
                ret = SOURCE_TYPE_HDMI4;
                break;
            case DEVICE_ID_SPDIF:
                ret = SOURCE_TYPE_SPDIF;
                break;
            case DEVICE_ID_ADTV:
                ret = SOURCE_TYPE_ADTV;
                break;
            case DEVICE_ID_AUX:
                ret = SOURCE_TYPE_AUX;
                break;
            case DEVICE_ID_ARC:
                ret = SOURCE_TYPE_ARC;
                break;
            default:
                break;
        }
        return ret;
    }

    public static int getSigType(int source_type) {
        int ret = 0;
        switch (source_type) {
            case SOURCE_TYPE_ATV:
                ret = SIG_INFO_TYPE_ATV;
                break;
            case SOURCE_TYPE_DTV:
                ret = SIG_INFO_TYPE_DTV;
                break;
            case SOURCE_TYPE_AV1:
            case SOURCE_TYPE_AV2:
                ret = SIG_INFO_TYPE_AV;
                break;
            case SOURCE_TYPE_HDMI1:
            case SOURCE_TYPE_HDMI2:
            case SOURCE_TYPE_HDMI3:
            case SOURCE_TYPE_HDMI4:
                ret = SIG_INFO_TYPE_HDMI;
                break;
            case SOURCE_TYPE_SPDIF:
                ret = SIG_INFO_TYPE_SPDIF;
                break;
            case SOURCE_TYPE_ADTV:
                ret = SIG_INFO_TYPE_ADTV;
                break;
            case SOURCE_TYPE_AUX:
                ret = SIG_INFO_TYPE_AUX;
                break;
            case SOURCE_TYPE_ARC:
                ret = SIG_INFO_TYPE_ARC;
                break;
            default:
                ret = SIG_INFO_TYPE_OTHER;
                break;
        }
        return ret;
    }

    public static int getSigType(ChannelInfo info) {
        if (info.isAnalogChannel())
            return SIG_INFO_TYPE_ATV;
        return SIG_INFO_TYPE_DTV;
    }
    public static final HashMap<String, String> DTV_SEARCH_TYPE_LIST = new HashMap<String, String>() {
        {
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATSC_C_AUTO_INDEX), TvContract.Channels.TYPE_ATSC_C);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATSC_C_HRC_INDEX), TvContract.Channels.TYPE_ATSC_C);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATSC_C_LRC_INDEX), TvContract.Channels.TYPE_ATSC_C);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATSC_C_STD_INDEX), TvContract.Channels.TYPE_ATSC_C);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATSC_T_INDEX), TvContract.Channels.TYPE_ATSC_T);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX), TvContract.Channels.TYPE_PAL);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_DTMB_INDEX), TvContract.Channels.TYPE_DTMB);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_DVBC_INDEX), TvContract.Channels.TYPE_DVB_C);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_DVBS_INDEX), TvContract.Channels.TYPE_DVB_S);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_DVBT_INDEX), TvContract.Channels.TYPE_DVB_T);
            put(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ISDBT_INDEX), TvContract.Channels.TYPE_ISDB_T);
        }
    };

    private static final Map<Integer, TvControlManager.SourceInput_Type> DeviceIdToTvSourceType = new HashMap<Integer, TvControlManager.SourceInput_Type>();
    static {
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_ATV, TvControlManager.SourceInput_Type.SOURCE_TYPE_TV);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_AV1, TvControlManager.SourceInput_Type.SOURCE_TYPE_AV);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_AV2, TvControlManager.SourceInput_Type.SOURCE_TYPE_AV);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_HDMI1, TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_HDMI2, TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_HDMI3, TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_HDMI4, TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_DTV, TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_ADTV, TvControlManager.SourceInput_Type.SOURCE_TYPE_ADTV);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_AUX, TvControlManager.SourceInput_Type.SOURCE_TYPE_AUX);
        DeviceIdToTvSourceType.put(DroidLogicTvUtils.DEVICE_ID_ARC, TvControlManager.SourceInput_Type.SOURCE_TYPE_ARC);
    }

    public static TvControlManager.SourceInput_Type parseTvSourceTypeFromDeviceId (int deviceId) {
        return DeviceIdToTvSourceType.get(deviceId);
    }

    private static final Map<Integer, TvControlManager.SourceInput> DeviceIdToTvSourceInput = new HashMap<Integer, TvControlManager.SourceInput>();
    static {
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_ATV, TvControlManager.SourceInput.TV);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_AV1, TvControlManager.SourceInput.AV1);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_AV2, TvControlManager.SourceInput.AV2);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_HDMI1, TvControlManager.SourceInput.HDMI1);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_HDMI2, TvControlManager.SourceInput.HDMI2);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_HDMI3, TvControlManager.SourceInput.HDMI3);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_HDMI4, TvControlManager.SourceInput.HDMI4);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_DTV, TvControlManager.SourceInput.DTV);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_ADTV, TvControlManager.SourceInput.ADTV);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_SPDIF, TvControlManager.SourceInput.SOURCE_SPDIF);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_AUX, TvControlManager.SourceInput.AUX);
        DeviceIdToTvSourceInput.put(DroidLogicTvUtils.DEVICE_ID_ARC, TvControlManager.SourceInput.ARC);
    }

    public static TvControlManager.SourceInput parseTvSourceInputFromDeviceId (int deviceId) {
        return DeviceIdToTvSourceInput.get(deviceId);
    }

    private static final Map<Integer, TvControlManager.SourceInput_Type> SigTypeToTvSourceType = new HashMap<Integer, TvControlManager.SourceInput_Type>();
    static {
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_ATV, TvControlManager.SourceInput_Type.SOURCE_TYPE_TV);
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_AV, TvControlManager.SourceInput_Type.SOURCE_TYPE_AV);
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_HDMI, TvControlManager.SourceInput_Type.SOURCE_TYPE_HDMI);
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_DTV, TvControlManager.SourceInput_Type.SOURCE_TYPE_DTV);
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_SPDIF, TvControlManager.SourceInput_Type.SOURCE_TYPE_SPDIF);
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_AUX, TvControlManager.SourceInput_Type.SOURCE_TYPE_AUX);
        SigTypeToTvSourceType.put(DroidLogicTvUtils.SIG_INFO_TYPE_ARC, TvControlManager.SourceInput_Type.SOURCE_TYPE_ARC);
    }

    public static TvControlManager.SourceInput_Type parseTvSourceTypeFromSigType (int sigType) {
        return SigTypeToTvSourceType.get(sigType);
    }

    private static final Map<Integer, TvControlManager.SourceInput> SigTypeToTvSourceInput = new HashMap<Integer, TvControlManager.SourceInput>();
    static {
        SigTypeToTvSourceInput.put(DroidLogicTvUtils.SIG_INFO_TYPE_ATV, TvControlManager.SourceInput.TV);
        SigTypeToTvSourceInput.put(DroidLogicTvUtils.SIG_INFO_TYPE_DTV, TvControlManager.SourceInput.DTV);
    }

    public static TvControlManager.SourceInput parseTvSourceInputFromSigType (int sigType) {
        return SigTypeToTvSourceInput.get(sigType);
    }

    private static final int AUDIO_AD_MAIN = 0x1000000;
    private static final int AUDIO_AD_ASVC = 0x2000000;
    private static final int AUDIO_AD_ID_MASK = 0xFF0000;

    public static int[] getAudioADTracks(ChannelInfo info, int mainTrackIndex) {
        String[] trackLangArray = info.getAudioLangs();
        if (trackLangArray == null)
            return null;

        if (mainTrackIndex < 0 || mainTrackIndex >= trackLangArray.length)
            return null;

        /*audio_exten is 32 bit,31:24 bit: exten type,the value is enum AM_Audio_Exten vaue*/
        /*23:16:mainid or asvc id,8 bit*/
        /*15:0 bit:no use*/
        int mainAudioExt = info.getAudioExts()[mainTrackIndex];

        /*only main track need ad*/
        if ((mainAudioExt & AUDIO_AD_MAIN) != AUDIO_AD_MAIN)
            return null;

        int mainAudioId = (mainAudioExt & AUDIO_AD_ID_MASK) >>> 16;

        int[] trackExtArray = info.getAudioExts();

        ArrayList<Integer> a = new ArrayList();
        for (int trackIdx = 0; trackIdx < trackExtArray.length; trackIdx++) {
            if ((trackExtArray[trackIdx] & AUDIO_AD_ASVC) != AUDIO_AD_ASVC)
                continue;
            int audioId = (trackExtArray[trackIdx] & AUDIO_AD_ID_MASK) >>> 16;
            if ((audioId & (1<<mainAudioId)) == (1<<mainAudioId)) {
                a.add(trackIdx);
            }
        }

        int[] ret = null;
        int s = a.size();
        if (s != 0) {
            ret = new int[s];
            Iterator<Integer> it = a.iterator();
            for (int i = 0; i < ret.length; i++)
                ret[i] = it.next().intValue();
        }
        return ret;
    }

    public static boolean hasAudioADTracks(ChannelInfo info) {
        String[] trackLangArray = info.getAudioLangs();
        if (trackLangArray == null)
            return false;

        /*audio_exten is 32 bit,31:24 bit: exten type,the value is enum AM_Audio_Exten vaue*/
        /*23:16:mainid or asvc id,8 bit*/
        /*15:0 bit:no use*/
        int[] trackExtArray = info.getAudioExts();
        for (int trackIdx = 0; trackIdx < trackExtArray.length; trackIdx++) {
            if ((trackExtArray[trackIdx] & AUDIO_AD_ASVC) == AUDIO_AD_ASVC)
                return true;
        }
        return false;
    }

    private static final Map<String, String> HEIGHT_TO_VIDEO_FORMAT_MAP = new HashMap<>();

    static {
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("240", "VIDEO_FORMAT_240");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("320", "VIDEO_FORMAT_320");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("480", "VIDEO_FORMAT_480");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("576", "VIDEO_FORMAT_576");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("720", "VIDEO_FORMAT_720");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("1080", "VIDEO_FORMAT_1080");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("1088", "VIDEO_FORMAT_1080");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("2160", "VIDEO_FORMAT_2160");
        HEIGHT_TO_VIDEO_FORMAT_MAP.put("4320", "VIDEO_FORMAT_4320");
    }

    private static final Map<String, String> PI_TO_VIDEO_FORMAT_MAP = new HashMap<>();

    static {
        PI_TO_VIDEO_FORMAT_MAP.put("interlace", "I");
        PI_TO_VIDEO_FORMAT_MAP.put("progressive", "P");
        PI_TO_VIDEO_FORMAT_MAP.put("interlace-top", "I");
        PI_TO_VIDEO_FORMAT_MAP.put("interlace-bottom", "I");
        PI_TO_VIDEO_FORMAT_MAP.put("Compressed", "P");
    }

    public static String convertVideoFormat(String height, String pi) {
        String result = "";
        if (!TextUtils.isEmpty(height) && !"NA".equals(height) && !TextUtils.isEmpty(pi) && !"null".equals(pi) && !"NA".equals(pi)) {
            height = HEIGHT_TO_VIDEO_FORMAT_MAP.get(height);
            if (pi.startsWith("interlace")) {
                result = height + PI_TO_VIDEO_FORMAT_MAP.get("interlace");
            } else if (pi.startsWith("progressive")) {
                result = height + PI_TO_VIDEO_FORMAT_MAP.get("progressive");
            } else if (pi.startsWith("Compressed")) {
                result = height + PI_TO_VIDEO_FORMAT_MAP.get("progressive");//Compressed may exist with progressive or interlace
            } else {
                result = height + PI_TO_VIDEO_FORMAT_MAP.get("progressive");
            }
        } else if ("NA".equals(height) && "NA".equals(pi)) {
            result = "";
        } else {
            height = HEIGHT_TO_VIDEO_FORMAT_MAP.get(height);
            result = height + PI_TO_VIDEO_FORMAT_MAP.get("progressive");
        }
        return result;
    }

    public static boolean isHardwareExisted(Context context, int deviceId) {
        TvInputManager tim = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
        List<TvInputHardwareInfo> hardwareList = tim.getHardwareList();
        if (hardwareList == null || hardwareList.size() == 0)
            return false;

        for (TvInputHardwareInfo hardwareInfo : hardwareList) {
            if (deviceId == hardwareInfo.getDeviceId())
                return true;
        }
        return false;
    }

    public static String mapToString(Map<String, String> map) {
        StringBuilder stringBuilder = new StringBuilder();

        for (String key : map.keySet()) {
            if (stringBuilder.length() > 0) {
                stringBuilder.append("&");
            }
            String value = map.get(key);
            try {
                stringBuilder.append((key != null ? URLEncoder.encode(key, "UTF-8") : ""));
                stringBuilder.append("=");
                stringBuilder.append(value != null ? URLEncoder.encode(value, "UTF-8") : "");
            } catch (UnsupportedEncodingException e) {
                throw new RuntimeException("This method requires UTF-8 encoding support", e);
            }
        }

        return stringBuilder.toString();
    }

    public static Map<String, String> stringToMap(String input) {
        Map<String, String> map = new HashMap<String, String>();

        String[] nameValuePairs = input.split("&");
        for (String nameValuePair : nameValuePairs) {
            String[] nameValue = nameValuePair.split("=");
            try {
                map.put(URLDecoder.decode(nameValue[0], "UTF-8"), nameValue.length > 1 ? URLDecoder.decode(
                            nameValue[1], "UTF-8") : "");
            } catch (UnsupportedEncodingException e) {
                throw new RuntimeException("This method requires UTF-8 encoding support", e);
            }
        }

        return map;
    }

    public static String mapToJson(String name, Map<String, String> map) {
        StringBuilder stringBuilder = new StringBuilder();
        boolean has_member = false;

        if (!map.isEmpty()) {
            if ((name != null) && !name.isEmpty())
                stringBuilder.append("\"").append(name).append("\":");

            stringBuilder.append("{");
            for (String key : map.keySet()) {
                if (has_member)
                    stringBuilder.append(",");

                String value = map.get(key);
                stringBuilder.append("\"")
                    .append((key != null ? key : ""))
                    .append("\":")
                    .append(value != null ? value : "");
                has_member = true;
            }
            stringBuilder.append("}");
        }
        return stringBuilder.toString();
    }
    public static String mapToJson(Map<String, String> map) {
        return mapToJson(null, map);
    }
    public static Map<String, String> jsonToMap(String jsonString) {
        if (jsonString == null || jsonString.length() == 0)
            return null;
        Map<String, String> map = new HashMap<String, String>();

        JSONObject jsonObject;
        try {
            jsonObject = new JSONObject(jsonString);
        } catch (JSONException e) {
            Log.e(TAG, "Json parse fail: ["+jsonString+"]" + e.getMessage());
            return null;
        }
        Iterator it = jsonObject.keys();
        while (it.hasNext()) {
            String k = (String)it.next();
            String v;
            try {
                map.put(k, jsonObject.get(k).toString());
            } catch (JSONException e) {
                Log.e(TAG, "Json get fail: ["+ k +"]" + e.getMessage());
            }
        }
        return map;
    }

    public static Map<String, String> multiJsonToMap(String jsonString) {
        if (jsonString == null || jsonString.length() == 0)
            return null;
        Map<String, String> map = new HashMap<String, String>();

        JSONObject jsonObject;
        try {
            jsonObject = new JSONObject(jsonString);
        } catch (JSONException e) {
            Log.e(TAG, "multiJsonToMap parse fail: ["+jsonString+"]" + e.getMessage());
            return null;
        }
        Iterator it = jsonObject.keys();
        while (it.hasNext()) {
            String k = (String)it.next();
            String v;
            try {
                String childStr = jsonObject.get(k).toString();
                map.put(k, childStr);
                JSONObject childJsonObject = null;
                Object childObject = jsonObject.get(k);
                if (childObject instanceof JSONObject) {
                    childJsonObject = (JSONObject)childObject;
                }
                if (childJsonObject != null) {
                    Map<String, String> childMap = multiJsonToMap(childJsonObject.toString());
                    if (childMap != null) {
                        map.putAll(childMap);
                    }
                }
            } catch (JSONException e) {
                //Log.e(TAG, "multiJsonToMap get fail: ["+ k +"]" + e.getMessage());
            }
        }
        return map;
    }

    public static class TvString {
        public static String toString(String[] array) {
            StringBuilder sb = new StringBuilder();
            boolean first = true;
            sb.append("[");
            if (array != null && array.length != 0) {
                for (String s : array) {
                    if (!first)
                        sb.append(",");
                    sb.append("\"").append(s).append("\"");
                    first = false;
                }
            }
            sb.append("]");
            return sb.toString();
        }
        public static String toString(String s) {
            return "\""+((s == null)? "" : s.toString())+"\"";
        }
        public static String fromString(String s) {
            return (s == null)? null : s.replace("\"", "");
        }
        public static String[] fromArrayString(String s) {
            return (s == null)? null : s.replace("[", "").replace("]", "").replace("\"", "").split(",");
        }
    }

    public static TvInputInfo getTvInputInfo(Context context, String inputId) {
        TvInputManager tim = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
        return tim.getTvInputInfo(inputId);
    }
    public static String CA_ENContentRatingDimensions[] = { "CA_TV_EN_EXEMPT", "CA_TV_EN_C", "CA_TV_EN_C8",     "CA_TV_EN_G", "CA_TV_EN_PG", "CA_TV_EN_14", "CA_TV_EN_18"};
    public static String CA_FRContentRatingDimensions[] = { "CA_TV_FR_E", "CA_TV_FR_G", "CA_TV_FR_8", "CA_TV_FR_13", "CA_TV_FR_16", "CA_TV_FR_18", };
    public static String US_MvContentRatingDimensions[] = { "US_MV_G", "US_MV_PG", "US_MV_PG13", "US_MV_R", "US_MV_NC17", "US_MV_X", "US_MV_NR" };
    public static String US_TvContentRatingDimensions[][] =
    {
        {"US_TV_Y",  "",            "",           "",           "",              ""},
        {"US_TV_Y7", "US_TV_FV",    "",           "",           "",              ""},
        {"US_TV_G",  "",            "",           "",           "",              ""},
        {"US_TV_PG", "",            "US_TV_L",    "US_TV_S",    "US_TV_V",       "US_TV_D"},
        {"US_TV_14", "",            "US_TV_L",    "US_TV_S",    "US_TV_V",       "US_TV_D"},
        {"US_TV_MA", "",            "US_TV_L",    "US_TV_S",    "US_TV_V",       ""},
    };
    public static String RatingDomain = "com.android.tv";
    public static int  RowNum = 6;
    public static int  ColumnNum = 6;
    public static TvContentRating[] unblockUSTVLowRating(String mrating, List<String> subRatings) {
        int mrow = 0;
        int mcolumn = 0;
        int mRatingrowPos = 0;
        boolean has_subrating = false;
        if (subRatings != null && subRatings.size() > 0) {
            has_subrating = true;
        }
        ArrayList<TvContentRating> RatingList = new ArrayList<TvContentRating>();

        for (mrow = 0; mrow < RowNum; mrow ++) {
            if (mrating.equals(US_TvContentRatingDimensions[mrow][0])) {
                    mRatingrowPos = mrow;
                break;
            }
        }
        if (DEBUG) Log.d("unblockLowRating", "mRatingrowPos: " + mRatingrowPos +"  has_subrating:" + has_subrating +"  mrating:" + mrating);
        for (mrow = 0; mrow <= mRatingrowPos; mrow ++) {
            for (mcolumn = 0; mcolumn < ColumnNum; mcolumn ++) {
                    if (US_TvContentRatingDimensions[mrow][mcolumn].length() > 0 && (!has_subrating || mrow <= 2 || mcolumn == 0)) {
                        if (DEBUG) Log.d("unblockLowRating", "high rating: " + mrating + " low rating[" + mrow + "][" + mcolumn + "]: " + US_TvContentRatingDimensions[mrow][mcolumn]);
                        ArrayList<String> subRats = new ArrayList<String>();
                        TvContentRating subrating;
                        if (mcolumn != 0) {
                            subRats.add(US_TvContentRatingDimensions[mrow][mcolumn]);
                            subrating = TvContentRating.createRating(RatingDomain, "US_TV",
                               US_TvContentRatingDimensions[mrow][0],  subRats.toArray(new String[subRats.size()]));
                         } else {
                            subrating = TvContentRating.createRating(RatingDomain, "US_TV",
                               US_TvContentRatingDimensions[mrow][0]);
                         }
                         if (subrating != null)
                          RatingList.add(subrating);
                    } else if (US_TvContentRatingDimensions[mrow][mcolumn].length() > 0
                             && subRatings.contains(US_TvContentRatingDimensions[mrow][mcolumn])) {
                        if (DEBUG) Log.d("unblockLowRating", "--high rating: " + mrating + " low rating[" + mrow + "][" + mcolumn + "]: " + US_TvContentRatingDimensions[mrow][mcolumn]);
                        ArrayList<String> subRats = new ArrayList<String>();
                        TvContentRating subrating;
                        subRats.add(US_TvContentRatingDimensions[mrow][mcolumn]);
                        subrating = TvContentRating.createRating(RatingDomain, "US_TV",
                               US_TvContentRatingDimensions[mrow][0],  subRats.toArray(new String[subRats.size()]));
                        if (subrating != null)
                        RatingList.add(subrating);
                    }
            }
        }

        return (RatingList.size() == 0 ? null : RatingList.toArray(new TvContentRating[RatingList.size()]));
    }

     public static TvContentRating[] unblockUSMVLowRating(String mrating) {
      int index = -1;
      int pos = 0;
      ArrayList<TvContentRating> RatingList = new ArrayList<TvContentRating>();
      for (index = 0; index < US_MvContentRatingDimensions.length; index ++) {
        if (mrating.equals(US_MvContentRatingDimensions[index])) {
            break;
        }
      }
      pos = index;
      if (pos < US_MvContentRatingDimensions.length) {
        for (index = 0; index < pos; index ++) {
            TvContentRating subrating = TvContentRating.createRating(RatingDomain, "US_MV",
               US_MvContentRatingDimensions[index]);
            if (subrating != null)
            RatingList.add(subrating);
        }
      }
      return (RatingList.size() == 0 ? null : RatingList.toArray(new TvContentRating[RatingList.size()]));
    }

    public static TvContentRating[] unblockCAENLowRating(String mrating) {
      int index = -1;
      int pos = 0;
      ArrayList<TvContentRating> RatingList = new ArrayList<TvContentRating>();
      for (index = 0; index < CA_ENContentRatingDimensions.length; index ++) {
        if (mrating.equals(CA_ENContentRatingDimensions[index])) {
            break;
        }
      }
      pos = index;
      if (pos < CA_ENContentRatingDimensions.length) {
        for (index = 0; index < pos; index ++) {
            TvContentRating subrating = TvContentRating.createRating(RatingDomain, "CA_TV_EN",
               CA_ENContentRatingDimensions[index]);
            if (subrating != null)
            RatingList.add(subrating);
        }
      }
      return (RatingList.size() == 0 ? null : RatingList.toArray(new TvContentRating[RatingList.size()]));
    }

    public static TvContentRating[] unblockCAFRLowRating(String mrating) {
      int index = -1;
      int pos = 0;
      ArrayList<TvContentRating> RatingList = new ArrayList<TvContentRating>();
      for (index = 0; index < CA_FRContentRatingDimensions.length; index ++) {
        if (mrating.equals(CA_FRContentRatingDimensions[index])) {
            break;
        }
      }
      if (DEBUG) Log.d(TAG, "unblockCAFRLowRating:-:" + index);
      pos = index;
      if (pos < CA_FRContentRatingDimensions.length) {
        for (index = 0; index < pos; index ++) {
            TvContentRating subrating = TvContentRating.createRating(RatingDomain, "CA_TV_FR",
               CA_FRContentRatingDimensions[index]);
            if (subrating != null)
            RatingList.add(subrating);
        }
      }
      return (RatingList.size() == 0 ? null : RatingList.toArray(new TvContentRating[RatingList.size()]));
    }

    public static TvContentRating[] unblockLowRating(TvContentRating rating) {
       if (rating == null) {
            return null;
        }
        if (DEBUG) Log.d(TAG, "unblockLowRating:" + rating.flattenToString());
        String mregain = rating.getRatingSystem();
        String mrating = rating.getMainRating();
        List<String> subRatings = rating.getSubRatings();
        if (mregain.equals("US_MV")) {
            //mv rating case
            return unblockUSMVLowRating(mrating);
        } else if (mregain.equals("US_TV")) {
            return unblockUSTVLowRating(mrating, subRatings);
        } else if (mregain.equals("CA_TV_EN")) {
            return unblockCAENLowRating(mrating);
        } else if (mregain.equals("CA_TV_FR")) {
            return unblockCAFRLowRating(mrating);
        }
        return null;
    }

    public static String US_ContentRatingDimensions[][] =
        {
            {"", "None", "US_TV_G", "US_TV_PG", "US_TV_14", "US_TV_MA"},
            {"", "US_TV_D"},
            {"", "US_TV_L"},
            {"", "US_TV_S"},
            {"", "US_TV_V"},
            {"", "US_TV_Y", "US_TV_Y7"},
            {"", "US_TV_FV"},
            {"", "NA", "US_MV_G", "US_MV_PG", "US_MV_PG13", "US_MV_R", "US_MV_NC17", "US_MV_X", "US_MV_NR"}
        };
    public static String CA_EN_ContentRatingDimensions[] =
        {
            "CA_TV_EN_EXEMPT", "CA_TV_EN_C", "CA_TV_EN_C8", "CA_TV_EN_G", "CA_TV_EN_PG", "CA_TV_EN_14", "CA_TV_EN_18"
        };
    public static String CA_FR_ContentRatingDimensions[] =
        {
            "CA_TV_FR_E", "CA_TV_FR_G", "CA_TV_FR_8", "CA_TV_FR_13", "CA_TV_FR_16", "CA_TV_FR_18"
        };


    public static TvContentRating[] parseDRatings(String jsonString) {
        String RatingDomain = "com.android.tv";
        Log.d(TAG, "parseDRatings:"+jsonString);
        if (jsonString == null || jsonString.isEmpty())
            return null;

        ArrayList<TvContentRating> RatingList = new ArrayList<TvContentRating>();
        ArrayList<String> tvDimList = new ArrayList<String>();
        ArrayList<String> tvSubRatingList = new ArrayList<String>();

        /*
        [
            //g=region, rx=ratings, d=dimension, r=rating value, rs:rating string
            {g:0,rx:[{d:0,r:3},{d:2,r:1},{d:4,r:1}],rs:[{lng:"eng",txt:"TV-PG-L-V"}]},
            {g:1,rx:[{d:7,r:3},rs:[{lng:"eng",txt:"MPAA-PG"}]}
            ...
        ]
        */
        JSONArray regionArray;
        JSONObject obj = null;
        try {
            obj = new JSONObject(jsonString);
        } catch (JSONException e) {
            throw new RuntimeException("Json parse fail: ("+jsonString+")", e);
        }

        try {
            regionArray = obj.getJSONArray("Dratings");
        } catch (JSONException e) {
            return null;
        }

        Log.d(TAG, "D rating:"+regionArray.toString());

        int ArraySize = regionArray.length();
        for (int i = 0; i < regionArray.length(); i++) {
            JSONObject g = regionArray.optJSONObject(i);
            if (g == null)
                continue;

            String ratingDescription = TvMultilingualText.getTextJ(g.optString("rs"));
            int region = g.optInt("g", -1);

            JSONArray ratings = g.optJSONArray("rx");
            if (ratings != null) {
                JSONObject ratingValues = ratings.optJSONObject(0);
                if (null == ratingValues)
                    continue;
                int dimension = ratingValues.optInt("d", -1);
                int value = ratingValues.optInt("r", -1);
                if (dimension == -1 || value == -1)
                    continue;
                if (region == 1) {//US ratings
                    for (int j = 0; j < ratings.length(); j++) {
                        JSONObject subRatingValues = ratings.optJSONObject(j);
                        int subDimension = subRatingValues.optInt("d", -1);
                        int subValue = subRatingValues.optInt("r", -1);

                        if (subDimension == -1 || subValue == -1)
                            continue;
                        if (subDimension > 7 ||
                            subValue >= DroidLogicTvUtils.US_ContentRatingDimensions[subDimension].length ||
                            TextUtils.isEmpty(DroidLogicTvUtils.US_ContentRatingDimensions[subDimension][subValue])) {
                            continue;
                        }
                        if (subDimension == 7) {
                            TvContentRating r = TvContentRating.createRating(RatingDomain, "US_MV",
                                    DroidLogicTvUtils.US_ContentRatingDimensions[subDimension][subValue]);
                            RatingList.add(r);
                            Log.d(TAG, "mv add rating:"+r.flattenToString());
                        } else {
                        /*
                            if (subDimension == 255)
                                subDimension = 0;
                            TvContentRating r = TvContentRating.createRating(RatingDomain, "US_TV",
                                    DroidLogicTvUtils.US_ContentRatingDimensions[subDimension][subValue]);
                            RatingList.add(r);
                            Log.d(TAG, "tv add rating:"+r.flattenToString());
                         */
                            String name = "";
                            if ((subDimension == 1) || (subDimension == 2) || (subDimension == 3) || (subDimension == 4) || (subDimension == 6)) {
                                        if (DroidLogicTvUtils.US_ContentRatingDimensions[subDimension].length > subValue)
                                             name = DroidLogicTvUtils.US_ContentRatingDimensions[subDimension][subValue];
                                        if (!TextUtils.isEmpty(name))
                                             tvSubRatingList.add(name);
                            } else if ((subDimension == 0) || (subDimension == 5)){
                                        if (DroidLogicTvUtils.US_ContentRatingDimensions[subDimension].length > subValue)
                                             name = DroidLogicTvUtils.US_ContentRatingDimensions[subDimension][subValue];
                                        if (!TextUtils.isEmpty(name))
                                             tvDimList.add(name);
                            }
                        }
                    }
                } else if (region == 2) {//Canadian ratings
                    for (int j = 0; j < ratings.length(); j++) {
                        JSONObject RatingValues = ratings.optJSONObject(j);
                        int Dimension = RatingValues.optInt("d", -1);
                        int Value = RatingValues.optInt("r", -1);
                        if (Dimension == -1 || Value == -1)
                             continue;
                        if (Dimension == 0) {
                        if (Value >= DroidLogicTvUtils.CA_EN_ContentRatingDimensions.length ||
                             TextUtils.isEmpty(DroidLogicTvUtils.CA_EN_ContentRatingDimensions[Value])) {
                             continue;
                        }
                            //canadian english language rating
                            TvContentRating r = TvContentRating.createRating(RatingDomain, "CA_TV_EN",
                                    DroidLogicTvUtils.CA_EN_ContentRatingDimensions[Value]);
                            RatingList.add(r);
                            Log.d(TAG, "add rating:"+r.flattenToString());
                        } else if (Dimension == 1) {
                            if (Value >= DroidLogicTvUtils.CA_FR_ContentRatingDimensions.length ||
                                 TextUtils.isEmpty(DroidLogicTvUtils.CA_FR_ContentRatingDimensions[Value])) {
                                 continue;
                            }
                            //canadian frech language rating
                            TvContentRating r = TvContentRating.createRating(RatingDomain, "CA_TV_FR",
                                    DroidLogicTvUtils.CA_FR_ContentRatingDimensions[Value]);
                            RatingList.add(r);
                            Log.d(TAG, "add rating:"+r.flattenToString());
                        }
                    }
                }
            }
        }

        if (tvDimList.size() != 0) {
            String[] subRatings = tvSubRatingList.toArray(new String[tvSubRatingList.size()]);

            for (String dim : tvDimList) {
                TvContentRating r = TvContentRating.createRating(RatingDomain, "US_TV", dim, subRatings);
                RatingList.add(r);
            }
        }

        return (RatingList.size() == 0 ? null : RatingList.toArray(new TvContentRating[RatingList.size()]));
    }

    public static TvContentRating getARating(int auth, int id, int dlsv) {
        final String RatingDomain = "com.android.tv";
        final String ratings[][] = {
            { "", "", "", "", "", "", "", "" },
            { "NA", "US_MV_G", "US_MV_PG", "US_MV_PG13", "US_MV_R", "US_MV_NC17", "US_MV_X", "US_MV_NR" },
            { "None", "US_TV_Y", "US_TV_Y7", "US_TV_G", "US_TV_PG", "US_TV_14", "US_TV_MA", "None" },
            { "CA_TV_EN_EXEMPT", "CA_TV_EN_C", "CA_TV_EN_C8", "CA_TV_EN_G", "CA_TV_EN_PG", "CA_TV_EN_14", "CA_TV_EN_18", "" },
            { "CA_TV_FR_E", "CA_TV_FR_G", "CA_TV_FR_8", "CA_TV_FR_13", "CA_TV_FR_16", "CA_TV_FR_18", "", "" }
        };
        final String region[] = { "", "US_MV", "US_TV", "CA_TV_EN", "CA_TV_FR" };

        final int VBI_RATING_AUTH_NONE = 0;
        final int VBI_RATING_AUTH_MPAA = 1;
        final int VBI_RATING_AUTH_TV_US = 2;
        final int VBI_RATING_AUTH_TV_CA_EN = 3;
        final int VBI_RATING_AUTH_TV_CA_FR = 4;

        final int VBI_RATING_D = 0x08;
        final int VBI_RATING_L = 0x04;
        final int VBI_RATING_S = 0x02;
        final int VBI_RATING_V = 0x01;

        if (auth < 0 || auth > 4)
            return null;

        if (id < 0 || id > 7)
            return null;

        ArrayList<String> subRatings = new ArrayList<String>();
        if ((dlsv & VBI_RATING_D) == VBI_RATING_D)
            subRatings.add("US_TV_D");
        if ((dlsv & VBI_RATING_L) == VBI_RATING_L)
            subRatings.add("US_TV_L");
        if ((dlsv & VBI_RATING_S) == VBI_RATING_S)
            subRatings.add("US_TV_S");
        if ((dlsv & VBI_RATING_V) == VBI_RATING_V)
            subRatings.add(TextUtils.equals(ratings[auth][id], "US_TV_Y7")? "US_TV_FV" : "US_TV_V");

        return TvContentRating.createRating(RatingDomain, region[auth],
                   ratings[auth][id],
                   (subRatings.size() == 0 ? null : subRatings.toArray(new String[subRatings.size()]))
               );
    }

    public static TvContentRating[] parseARatings(String jsonString) {

        if (jsonString == null || jsonString.isEmpty())
            return null;

        ArrayList<TvContentRating> RatingList = new ArrayList<TvContentRating>();

        /*
            //g=region, i=id, dlsv=dlsv
            Aratings:{g:0,i:0,dlsv:3}
        */
        JSONObject obj = null;
        try {
            obj = new JSONObject(jsonString);
        } catch (JSONException e) {
           return null;
        }
        JSONObject ratingObj = null;
        try {
            ratingObj = obj.getJSONObject("Aratings");
        } catch (JSONException e) {
            return null;
        }

        Log.d(TAG, "A rating:"+ratingObj.toString());

        int region = ratingObj.optInt("g", -1);
        int id = ratingObj.optInt("i", -1);
        int dlsv = ratingObj.optInt("dlsv", 0);
        if (region == -1 || id == -1)
            return null;

        TvContentRating r = getARating(region, id, dlsv);
        if (r != null) {
            RatingList.add(r);
            Log.d(TAG, "add rating:"+r.flattenToString());
        }
        return (RatingList.size() == 0 ? null : RatingList.toArray(new TvContentRating[RatingList.size()]));
    }

    public static List<ChannelInfo.Subtitle> parseAtscCaptions(String jsonString) {

        if (jsonString == null || jsonString.isEmpty())
            return null;

        ArrayList<ChannelInfo.Subtitle> SubtitleList = new ArrayList<ChannelInfo.Subtitle>();
        /*
            [
                //bdig=b_digtal_cc, sn=caption_service_number, lng=language,
                //beasy=b_easy_reader, bwar=b_wide_aspect_ratio
                {bdig:1,sn:0x2,lng:"eng",beasy:0,bwar:1},
                {bdig:0,l21:1},
                ...
            ]
        */

        JSONArray captionArray = null;
        JSONObject obj = null;
        try {
            obj = new JSONObject(jsonString);
        } catch (JSONException e) {
            throw new RuntimeException("Json parse fail: ("+jsonString+")", e);
        }

        try {
            captionArray = obj.getJSONArray("captions");
        } catch (JSONException e) {
            return null;
        }

        Log.d(TAG, "captions:"+captionArray.toString());

        int ArraySize = captionArray.length();
        int count = 0;
        for (int i = 0; i < captionArray.length(); i++) {
            JSONObject c = captionArray.optJSONObject(i);
            if (c == null)
                continue;

            int isDigitalCC = c.optInt("bdig", -1);
            int decoder_param = c.optInt("private_data", -1);
            if (isDigitalCC == 1) {
                int serviceNumber = c.optInt("sn", -1);
                int easyReader = c.optInt("bwasy", 0);
                int wideAspectRatio = c.optInt("bwar", 0);
                ChannelInfo.Subtitle s
                    = new ChannelInfo.Subtitle(ChannelInfo.Subtitle.TYPE_DTV_CC,
                                        ChannelInfo.Subtitle.CC_CAPTION_SERVICE1 + serviceNumber - 1,
                                        ChannelInfo.Subtitle.TYPE_DTV_CC,
                                        decoder_param,
                                        (easyReader == 0 ? 0x80 : 0) | (wideAspectRatio == 0 ? 0x40 : 0),
                                        c.optString("lng", ""),
                                        count++);
                SubtitleList.add(s);
            } else if (isDigitalCC == 0) {
                int line21 = c.optInt("l21", 0);
                ChannelInfo.Subtitle s
                    = new ChannelInfo.Subtitle(ChannelInfo.Subtitle.TYPE_ATV_CC,
                                        ChannelInfo.Subtitle.CC_CAPTION_CC1,
                                        ChannelInfo.Subtitle.TYPE_ATV_CC,
                                        0,
                                        0,
                                        "CC1",
                                        count++);
                SubtitleList.add(s);
            }
         }

        return (SubtitleList.size() == 0 ? null : SubtitleList);
    }

    public static String getObjectString(String jsonString, String objName) {
        if (jsonString == null || jsonString.isEmpty()
            ||objName == null || objName.isEmpty())
            return null;

        /*
            title:{a:0,b:0,c:1}
        */
        JSONObject obj = null;
        try {
            obj = new JSONObject(jsonString);
        } catch (JSONException e) {
            throw new RuntimeException("Json parse fail: ("+jsonString+")", e);
        }
        JSONObject tObj = null;
        try {
            tObj = obj.getJSONObject(objName);
        } catch (JSONException e) {
            return null;
        }

        Log.d(TAG, objName+":"+tObj.toString());

        return tObj.toString();
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

    public static byte[] serializeInternalProviderData(String data) {
        if (data == null) {
            return null;
        }
        byte[] result = null;
        try {
            result = data.getBytes();
        } catch (Exception e) {
            Log.e(TAG, "serializeInternalProviderData Exception = " + e.getMessage());
        }
        return result;
    }

    public static String deserializeInternalProviderData(byte[] bytes) {
        if (bytes == null || bytes.length == 0) {
            return null;
        }
        String result = null;
        try {
            result = new String(bytes);
        } catch (Exception e) {
            Log.e(TAG, "deserializeInternalProviderData Exception = " + e.getMessage());
        }
        return result;
    }
}
