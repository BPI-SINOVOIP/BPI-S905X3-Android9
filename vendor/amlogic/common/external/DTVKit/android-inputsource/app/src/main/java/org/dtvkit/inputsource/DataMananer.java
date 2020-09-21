package org.dtvkit.inputsource;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.util.Log;
import android.provider.Settings;

public class DataMananer {

    private static final String TAG = "DataMananer";
    private Context mContext;

    public static final String KEY_SATALLITE = "key_satallite";
    public static final String KEY_TRANSPONDER = "key_transponder";
    public static final String KEY_CURRENT_TYPE = "key_current_type";
    public static final String KEY_LNB_TYPE = "key_lnb_type";
    //unicable
    public static final String KEY_UNICABLE = "key_unicable";
    public static final String KEY_UNICABLE_SWITCH = "key_unicable_switch";
    public static final String KEY_USER_BAND = "key_user_band";
    public static final String KEY_UB_FREQUENCY = "key_ub_frequency";
    public static final String KEY_POSITION = "key_position";

    public static final String KEY_LNB_POWER = "key_lnb_power";
    public static final String KEY_22_KHZ = "key_22_khz";
    public static final String KEY_TONE_BURST = "key_tone_burst";
    public static final String KEY_DISEQC1_0 = "key_diseqc1_0";
    public static final String KEY_DISEQC1_1 = "key_diseqc1_1";
    public static final String KEY_MOTOR = "key_motor";
    public static final String KEY_DISEQC1_2 = "key_diseqc1_2";
    public static final String KEY_DISEQC1_2_DISH_LIMITS_STATUS = "key_dish_limit_status";
    public static final String KEY_DISEQC1_2_DISH_EAST_LIMITS = "key_dish_east_limit";
    public static final String KEY_DISEQC1_2_DISH_WEST_LIMITS = "key_dish_west_limit";
    public static final String KEY_DISEQC1_2_DISH_MOVE_DIRECTION = "key_dish_move_direction";
    public static final String KEY_DISEQC1_2_DISH_MOVE_STEP = "key_dish_move_step";
    public static final String KEY_DISEQC1_2_DISH_CURRENT_POSITION = "key_dish_current_position";
    public static final String KEY_DISEQC1_2_DISH_SAVE_POSITION = "key_dish_save_to_position";
    public static final String KEY_DISEQC1_2_DISH_MOVE_TO_POSITION = "key_dish_move_to_position";

    public static final String[] ID_DIALOG_KEY_COLLECTOR = {KEY_SATALLITE, KEY_TRANSPONDER, KEY_LNB_TYPE, KEY_UNICABLE, KEY_LNB_POWER,
            KEY_22_KHZ, KEY_TONE_BURST, KEY_DISEQC1_0, KEY_DISEQC1_1, KEY_MOTOR};
    public static final String KEY_LNB_CUSTOM = "key_lnb_custom";
    public static final String KEY_LNB_CUSTOM_SINGLE_DOUBLE = "key_lnb_custom_single_double";
    public static final int DEFAULT_LNB_CUSTOM_SINGLE_DOUBLE = 0;//SINGLE
    public static final String KEY_LNB_CUSTOM_LOW_MIN = "key_lnb_low_band_min";
    public static final String KEY_LNB_CUSTOM_LOW_MAX = "key_lnb_low_band_max";
    public static final String KEY_LNB_CUSTOM_HIGH_MIN = "key_lnb_high_band_min";
    public static final String KEY_LNB_CUSTOM_HIGH_MAX = "key_lnb_high_band_max";
    //default value
    public static final String KEY_SATALLITE_DEFAULT_VALUE = "null";
    public static final String KEY_TRANSPONDER_DEFAULT_VALUE = "null";
    public static final String KEY_LNB_TYPE_DEFAULT_VALUE = "9750/10600";
    //unicable
    public static final String KEY_UNICABLE_DEFAULT_VALUE = "off";
    public static final String KEY_UNICABLE_SWITCH_DEFAULT_VALUE = "off";
    public static final String KEY_USER_BAND_DEFAULT_VALUE = "0";
    public static final String KEY_UB_FREQUENCY_DEFAULT_VALUE = "0";
    public static final String KEY_POSITION_DEFAULT_VALUE = "false";

    public static final String KEY_LNB_POWER_DEFAULT_VALUE = "18V";
    public static final String KEY_22_KHZ_DEFAULT_VALUE = "auto";
    public static final String KEY_TONE_BURST_DEFAULT_VALUE = "none";
    public static final String KEY_DISEQC1_0_DEFAULT_VALUE = "none";
    public static final String KEY_DISEQC1_1_DEFAULT_VALUE = "none";
    public static final String KEY_MOTOR_DEFAULT_VALUE = "none";

    public static final String KEY_FUNCTION = "function";
    public static final String KEY_ADD_SATELLITE = "add_satellite";
    public static final String KEY_EDIT_SATELLITE = "edit_satellite";
    public static final String KEY_REMOVE_SATELLITE = "remove_satellite";
    public static final String KEY_ADD_TRANSPONDER = "add_transponder";
    public static final String KEY_EDIT_TRANSPONDER = "edit_transponder";
    public static final String KEY_REMOVE_TRANSPONDER = "remove_transponder";

    //default value that is save by index
    public static final int KEY_SATALLITE_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_TRANSPONDER_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_LNB_TYPE_DEFAULT_INDEX_INDEX = 1;
    //unicable
    public static final int KEY_UNICABLE_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_UNICABLE_SWITCH_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_USER_BAND_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_UB_FREQUENCY_DEFAULT_VALUE_INDEX = 1284;
    public static final int KEY_POSITION_DEFAULT_VALUE_INDEX = 0;

    public static final int KEY_LNB_POWER_DEFAULT_VALUE_INDEX = 1;
    public static final int KEY_22_KHZ_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_TONE_BURST_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_DISEQC1_0_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_DISEQC1_1_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_MOTOR_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_DISEQC1_2_DISH_LIMITS_STATUS_DEFAULT_VALUE_INDEX = 0;
    public static final int KEY_FEC_DEFAULT_VALUE = 0;
    public static final int KEY_MODULATION_DEFAULT_VALUE = 0;

    public static final String[] DIALOG_SET_ITEM_UNICABLE_KEY_LIST = {KEY_UNICABLE_SWITCH, KEY_USER_BAND, KEY_UB_FREQUENCY, KEY_POSITION};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_TONE_BURST_LIST = {"none", "a", "b"};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_0_LIST = {"none", "1/4", "2/4", "3/4", "4/4"};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_1_LIST = {"none", "1/16", "2/16", "3/16", "4/16", "5/16", "6/16", "7/16", "8/16", "9/16", "10/16", "11/16", "12/16", "13/16", "14/16", "15/16", "16/16"};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER_LIST = {"13v", "18v", "off"/*, "13/18V"*/};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ_LIST = {"on", "off"/*, "auto"*/};

    public static final String KEY_NIT = "network_search";
    public static final String KEY_DVBS_NIT = "dvbs_network_search";
    public static final String KEY_CLEAR = "clear_old_search";
    public static final String KEY_DVBS2 = "dvbs_enable";
    public static final String KEY_SEARCH_MODE = "search_mode";
    public static final int VALUE_BLIND_DEFAULT_START_FREQUENCY = 950;//MHZ
    public static final int VALUE_BLIND_DEFAULT_END_FREQUENCY = 2150;//MHZ
    public static final int VALUE_SEARCH_NIT_DISABLE = 0;
    public static final int VALUE_SEARCH_NIT_ENABLE = 1;
    public static final int VALUE_SEARCH_MODE_BLIND = 0;
    public static final int VALUE_SEARCH_MODE_SATELLITE = 1;
    public static final String KEY_FEC_MODE = "forward_error_correction";
    public static final String KEY_MODULATION_MODE = "modulation";
    public static final String[] KEY_SEARCH_MODE_LIST = {"blind", "satellite", "transponder"};
    public static final String[] KEY_FEC_ARRAY_VALUE = {"auto", "1/2", "2/3", "3/4", "5/6", "7/8", "1/4", "1/3", "2/5", "8/9", "9/10", "3/5", "4/5"};
    public static final String[] KEY_MODULATION_ARRAY_VALUE = {"auto", "qpsk", "8psk", "16qam"};

    public static final String KEY_SELECT_SEARCH_ACTIVITY = "search_activity";
    public static final String KEY_IS_DVBT = "is_dvbt";
    public static final String KEY_PACKAGE_NAME = "org.dtvkit.inputsource";
    public static final String KEY_ACTIVITY_DVBT = "org.dtvkit.inputsource.DtvkitDvbtSetup";
    public static final String KEY_ACTIVITY_DVBS = "org.dtvkit.inputsource.DtvkitDvbsSetup";
    public static final String KEY_ACTIVITY_SETTINGS = "com.droidlogic.settings.DtvkitDvbSettings";
    public static final int SELECT_DVBC = 0;
    public static final int SELECT_DVBT = 1;
    public static final int SELECT_DVBS = 2;
    public static final int SELECT_SETTINGS = 3;
    public static final String KEY_DTVKIT_COUNTRY = "dtvkit_country";
    public static final String KEY_DTVKIT_MAIN_AUDIO_LANG = "main_audio_lang";
    public static final String KEY_DTVKIT_ASSIST_AUDIO_LANG = "assist_audio_lang";
    public static final String KEY_DTVKIT_MAIN_SUBTITLE_LANG = "main_subtitle_lang";
    public static final String KEY_DTVKIT_ASSIST_SUBTITLE_LANG = "assist_subtitle_lang";

    //manual search
    public static final String KEY_IS_FREQUENCY = "frequency_mode";
    public static final int VALUE_FREQUENCY_MODE = 0;
    public static final String KEY_DVBT_BANDWIDTH = "dvbt_band_width";
    public static final String[] VALUE_DVBT_BANDWIDTH_LIST = {"5MHZ", "6MHZ", "7MHZ", "8MHZ", "10MHZ"};
    public static final int VALUE_DVBT_BANDWIDTH_5MHZ = 0;
    public static final String KEY_DVBT_MODE = "dvbt_mode";
    public static final String[] VALUE_DVBT_MODE_LIST = {"1K", "2K", "4K", "8K", "16K", "32K"};
    public static final int VALUE_DVBT_MODE_1K = 0;
    public static final String KEY_DVBT_TYPE = "dvbt_type";
    public static final String[] VALUE_DVBT_TYPE_LIST = {"DVB-T", "DVB-T2"};
    public static final int VALUE_DVBT_MODE_DVBT = 0;
    public static final String KEY_DVBC_MODE = "dvbc_mode";
    public static final String[] VALUE_DVBC_MODE_LIST = {/*"QAM4", "QAM8", */"QAM16", "QAM32", "QAM64", "QAM128", "QAM256", "AUTO"};
    public static final int VALUE_DVBC_MODE_QAM16 = 2;
    public static final String KEY_DVBC_SYMBOL_RATE = "dvbc_symbol_rate";
    public static final int VALUE_DVBC_SYMBOL_RATE = 6900;
    public static final String KEY_PUBLIC_SEARCH_MODE = "public_search_mode";
    public static final int VALUE_PUBLIC_SEARCH_MODE_AUTO = 1;
    public static final String KEY_SEARCH_DVBC_CHANNEL_NAME = "dvbc_channel_name";
    public static final String KEY_SEARCH_DVBT_CHANNEL_NAME = "dvbt_channel_name";
    public static final int VALUE_SEARCH_CHANNEL_NAME_INDEX_DEFAULT = 0;

    //add for pvr record path setting
    public static final String KEY_PVR_RECORD_PATH = "pvr_record_path";
    public static final String PVR_DEFAULT_PATH = "/data/data/org.dtvkit.inputsource";

    //audio ad
    public static final String TV_KEY_AD_SWITCH = "ad_switch";
    public static final String TV_KEY_AD_MIX = "ad_mix_level";
    public static final String ACTION_AD_MIXING_LEVEL = "android.intent.action.ad_mixing_level";
    public static final String PARA_VALUE1 = "value1";
    public static final int VALUE_AD_MIX_LEVEL = 50;

    //dtvkit satellite data import flag
    public static final String DTVKIT_IMPORT_SATELLITE_FLAG = "has_dtvkit_import_satellite";

    public DataMananer(Context context) {
        this.mContext = context;
    }

    public void saveIntParameters(String key, int value) {
        Settings.System.putInt(mContext.getContentResolver(), key, value);
    }

    public int getIntParameters(String key) {
        int defValue = 0;
        switch (key) {
            case KEY_LNB_TYPE:
                defValue = 1;
                break;
            case KEY_UNICABLE_SWITCH:
                defValue = KEY_UNICABLE_SWITCH_DEFAULT_VALUE_INDEX;
                break;
            case KEY_USER_BAND:
                defValue = KEY_USER_BAND_DEFAULT_VALUE_INDEX;
                break;
            case KEY_UB_FREQUENCY:
                defValue = KEY_UB_FREQUENCY_DEFAULT_VALUE_INDEX;
                break;
            case KEY_POSITION:
                defValue = KEY_POSITION_DEFAULT_VALUE_INDEX;
                break;
            case KEY_UNICABLE:
                defValue = KEY_UNICABLE_SWITCH_DEFAULT_VALUE_INDEX;
                break;
            case KEY_LNB_POWER:
                defValue = KEY_LNB_POWER_DEFAULT_VALUE_INDEX;
                break;
            case KEY_22_KHZ:
                defValue = KEY_22_KHZ_DEFAULT_VALUE_INDEX;
                break;
            case KEY_TONE_BURST:
                defValue = KEY_TONE_BURST_DEFAULT_VALUE_INDEX;
                break;
            case KEY_DISEQC1_0:
                defValue = KEY_DISEQC1_0_DEFAULT_VALUE_INDEX;
                break;
            case KEY_DISEQC1_1:
                defValue = KEY_DISEQC1_1_DEFAULT_VALUE_INDEX;
                break;
            case KEY_MOTOR:
                defValue = KEY_MOTOR_DEFAULT_VALUE_INDEX;
                break;
            case KEY_DISEQC1_2_DISH_LIMITS_STATUS:
                defValue = KEY_MOTOR_DEFAULT_VALUE_INDEX;
                break;
            case KEY_DISEQC1_2_DISH_EAST_LIMITS:
                defValue = 180;
                break;
            case KEY_DISEQC1_2_DISH_WEST_LIMITS:
                defValue = 180;
                break;
            case KEY_DISEQC1_2_DISH_MOVE_DIRECTION:
                defValue = 0;
                break;
            case KEY_DISEQC1_2_DISH_MOVE_STEP:
                defValue = 1;
                break;
            case KEY_DISEQC1_2_DISH_CURRENT_POSITION:
                defValue = 0;
                break;
            case KEY_NIT:
                defValue = VALUE_SEARCH_NIT_DISABLE;
                break;
            case KEY_CLEAR:
                defValue = 1;
                break;
            case KEY_DVBS2:
                defValue = 0;
                break;
            case KEY_FEC_MODE:
                defValue = 0;
                break;
            case KEY_MODULATION_MODE:
                defValue = 0;
                break;
            case KEY_SELECT_SEARCH_ACTIVITY:
                defValue = SELECT_DVBC;
                break;
            case KEY_IS_FREQUENCY:
                defValue = VALUE_FREQUENCY_MODE;
                break;
            case KEY_DVBT_BANDWIDTH:
                defValue = VALUE_DVBT_BANDWIDTH_5MHZ;
                break;
            case KEY_DVBT_MODE:
                defValue = VALUE_DVBT_BANDWIDTH_5MHZ;
                break;
            case KEY_DVBT_TYPE:
                defValue = VALUE_DVBT_MODE_DVBT;
                break;
            case KEY_DVBC_MODE:
                defValue = VALUE_DVBC_MODE_QAM16;
                break;
            case KEY_DVBC_SYMBOL_RATE:
                defValue = VALUE_DVBC_SYMBOL_RATE;
                break;
            case KEY_PUBLIC_SEARCH_MODE:
                defValue = VALUE_PUBLIC_SEARCH_MODE_AUTO;
                break;
            case KEY_DVBS_NIT:
                defValue = VALUE_SEARCH_NIT_ENABLE;
                break;
            case KEY_SEARCH_MODE:
                defValue = VALUE_SEARCH_MODE_SATELLITE;
                break;
            case TV_KEY_AD_MIX:
                defValue = VALUE_AD_MIX_LEVEL;
                break;
            default:
                defValue = 0;
                break;
        }
        int result = Settings.System.getInt(mContext.getContentResolver(), key, defValue);
        //Log.d(TAG, "getIntParameters key = " + key + ", defValue = " + defValue);
        return result;
    }

    public void saveStringParameters(String key, String value) {
        Settings.System.putString(mContext.getContentResolver(), key, value);
    }

    public String getStringParameters(String key) {
        String defValue = null;
        if (key == null) {
            return null;
        }
        switch (key) {
            case KEY_CURRENT_TYPE:
                defValue = "";
                break;
            case KEY_SATALLITE:
                defValue = KEY_SATALLITE_DEFAULT_VALUE;
                break;
            case KEY_TRANSPONDER:
                defValue = KEY_TRANSPONDER_DEFAULT_VALUE;
                break;
            case KEY_LNB_TYPE:
                defValue = KEY_LNB_TYPE_DEFAULT_VALUE;
                break;
            case KEY_UNICABLE_SWITCH:
                defValue = KEY_UNICABLE_SWITCH_DEFAULT_VALUE;
                break;
            case KEY_USER_BAND:
                defValue = KEY_USER_BAND_DEFAULT_VALUE;
                break;
            case KEY_UB_FREQUENCY:
                defValue = KEY_UB_FREQUENCY_DEFAULT_VALUE;
                break;
            case KEY_POSITION:
                defValue = KEY_POSITION_DEFAULT_VALUE;
                break;
            case KEY_UNICABLE:
                defValue = KEY_UNICABLE_SWITCH_DEFAULT_VALUE;
                break;
            case KEY_LNB_POWER:
                defValue = KEY_LNB_POWER_DEFAULT_VALUE;
                break;
            case KEY_22_KHZ:
                defValue = KEY_22_KHZ_DEFAULT_VALUE;
                break;
            case KEY_TONE_BURST:
                defValue = KEY_TONE_BURST_DEFAULT_VALUE;
                break;
            case KEY_DISEQC1_0:
                defValue = KEY_DISEQC1_0_DEFAULT_VALUE;
                break;
            case KEY_DISEQC1_1:
                defValue = KEY_DISEQC1_1_DEFAULT_VALUE;
                break;
            case KEY_MOTOR:
                defValue = KEY_MOTOR_DEFAULT_VALUE;
                break;
            case KEY_LNB_CUSTOM:
                defValue = "";
                break;
            case KEY_PVR_RECORD_PATH:
                defValue = PVR_DEFAULT_PATH;
                break;
            default:
                defValue = "";
                break;
        }
        String result = Settings.System.getString(mContext.getContentResolver(), key);
        Log.d(TAG, "getStringParameters key = " + key + ", result = " + result);
        if (TextUtils.isEmpty(result)) {
            result = defValue;
        }
        return result;
    }
}
