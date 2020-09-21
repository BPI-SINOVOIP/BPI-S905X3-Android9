package com.droidlogic.settings;

import android.util.Log;
import android.media.tv.TvTrackInfo;

import java.lang.reflect.Method;
import java.util.Map;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;


import com.droidlogic.app.SystemControlManager;

public class ConstantManager {

    private static final String TAG = "ConstantManager";
    private static final boolean DEBUG = true;

    public static final String PI_FORMAT_KEY = "pi_format";
    public static final String KEY_AUDIO_CODES_DES = "audio_codes";
    public static final String KEY_TRACK_PID = "pid";
    public static final String KEY_INFO = "info";
    public static final String KEY_SIGNAL_STRENGTH = "signal_strength";
    public static final String KEY_SIGNAL_QUALITY = "signal_quality";

    public static final String EVENT_STREAM_PI_FORMAT = "event_pi_format";
    public static final String EVENT_RESOURCE_BUSY = "event_resource_busy";
    public static final String EVENT_SIGNAL_INFO = "event_signal_info";
    public static final String EVENT_RECORD_PROGRAM_URI = "event_record_program_uri";
    public static final String EVENT_RECORD_DATA_URI = "event_record_data_uri";

    //show or hide overlay
    public static final String ACTION_TIF_CONTROL_OVERLAY = "tif_control_overlay";
    public static final String KEY_TIF_OVERLAY_SHOW_STATUS = "tif_overlay_show_status";
    //surface related
    public static final String KEY_SURFACE = "surface";
    public static final String KEY_TV_STREAM_CONFIG = "config";

    //add for pvr delete control
    public static final String ACTION_REMOVE_ALL_DVR_RECORDS = "droidlogic.intent.action.remove_all_dvr_records";
    public static final String ACTION_DVR_RESPONSE = "droidlogic.intent.action.dvr_response";
    public static final String KEY_DVR_DELETE_RESULT_LIST = "dvr_delete_result_list";
    public static final String KEY_DTVKIT_SEARCH_TYPE = "dtvkit_search_type";//auto or manual
    public static final String KEY_DTVKIT_SEARCH_TYPE_AUTO = "dtvkit_auto_search";//auto
    public static final String KEY_DTVKIT_SEARCH_TYPE_MANUAL = "dtvkit_manual_search_type";//manual
    public static final String KEY_LIVETV_PVR_STATUS = "livetv_pvr_status";//manual
    public static final String VALUE_LIVETV_PVR_SCHEDULE_AVAILABLE = "livetv_pvr_schedule";
    public static final String VALUE_LIVETV_PVR_RECORD_PROGRAM_AVAILABLE = "livetv_pvr_program";

    public static final String CONSTANT_QAA = "qaa";//Original Audio flag
    public static final String CONSTANT_ORIGINAL_AUDIO = "Original Audio";
    public static final String CONSTANT_UND_FLAG = "und";//undefined flag
    public static final String CONSTANT_UND_VALUE = "Undefined";
    public static final String CONSTANT_QAD = "qad";
    public static final String CONSTANT_FRENCH = "french";

    //add for trackinfo
    public static final String SYS_VIDEO_DECODE_PATH = "/sys/class/vdec/vdec_status";
    public static final String SYS_VIDEO_DECODE_VIDEO_WIDTH_PREFIX = "frame width :";
    public static final String SYS_VIDEO_DECODE_VIDEO_WIDTH_SUFFIX = "frame height";
    public static final String SYS_VIDEO_DECODE_VIDEO_HEIGHT_PREFIX = "frame height :";
    public static final String SYS_VIDEO_DECODE_VIDEO_HEIGHT_SUFFIX = "frame rate";
    public static final String SYS_VIDEO_DECODE_VIDEO_FRAME_RATE_PREFIX = "frame rate :";
    public static final String SYS_VIDEO_DECODE_VIDEO_FRAME_RATE_SUFFIX = "fps";
    //video
    public static final String KEY_TVINPUTINFO_VIDEO_WIDTH = "video_width";
    public static final String KEY_TVINPUTINFO_VIDEO_HEIGHT = "video_height";
    public static final String KEY_TVINPUTINFO_VIDEO_FRAME_RATE = "video_frame_rate";
    public static final String KEY_TVINPUTINFO_VIDEO_FORMAT = "video_format";
    public static final String KEY_TVINPUTINFO_VIDEO_FRAME_FORMAT = "video_frame_format";
    public static final String KEY_TVINPUTINFO_VIDEO_CODEC = "video_codec";
    public static final String VALUE_TVINPUTINFO_VIDEO_INTERLACE = "I";
    public static final String VALUE_TVINPUTINFO_VIDEO_PROGRESSIVE = "P";
    //audio
    public static final String KEY_TVINPUTINFO_AUDIO_INDEX = "audio_index";
    public static final String KEY_TVINPUTINFO_AUDIO_AD = "audio_ad";
    public static final String KEY_TVINPUTINFO_AUDIO_CODEC = "audio_codec";
    public static final String KEY_TVINPUTINFO_AUDIO_SAMPLING_RATE = "audio_sampling_rate";
    public static final String KEY_TVINPUTINFO_AUDIO_CHANNEL = "audio_channel";
    public static final String AUDIO_PATCH_COMMAND_GET_SAMPLING_RATE = "sample_rate";
    public static final String AUDIO_PATCH_COMMAND_GET_AUDIO_CHANNEL = "channel_nums";
    public static final String AUDIO_PATCH_COMMAND_GET_AUDIO_CHANNEL_CONFIGURE = "channel_configuration";
    //subtitle
    public static final String KEY_TVINPUTINFO_SUBTITLE_INDEX = "subtitle_index";
    public static final String KEY_TVINPUTINFO_SUBTITLE_IS_TELETEXT = "is_teletext";
    public static final String KEY_TVINPUTINFO_SUBTITLE_IS_HARD_HEARING = "is_hard_hearing";

    public static final Map<String, String> PI_TO_VIDEO_FORMAT_MAP = new HashMap<>();
    static {
        PI_TO_VIDEO_FORMAT_MAP.put("interlace", "I");
        PI_TO_VIDEO_FORMAT_MAP.put("progressive", "P");
        PI_TO_VIDEO_FORMAT_MAP.put("interlace-top", "I");
        PI_TO_VIDEO_FORMAT_MAP.put("interlace-bottom", "I");
        PI_TO_VIDEO_FORMAT_MAP.put("Compressed", "P");
    }

    public static final String CONSTANT_FORMAT_PROGRESSIVE = "progressive";
    public static final String CONSTANT_FORMAT_INTERLACE = "interlace";
    public static final String CONSTANT_FORMAT_COMRPESSED = "Compressed";

    public static final String SYS_WIDTH_PATH = "/sys/class/video/frame_width";
    public static final String SYS_HEIGHT_PATH = "/sys/class/video/frame_height";
    public static final String SYS_PI_PATH = "/sys/class/deinterlace/di0/frame_format";//"/sys/class/video/frame_format";

    //add define for subtitle type
    public static final int ADB_SUBTITLE_TYPE_DVB  = 0x10;
    public static final int ADB_SUBTITLE_TYPE_DVB_4_3 = 0x11;
    public static final int ADB_SUBTITLE_TYPE_DVB_16_9 = 0x12;
    public static final int ADB_SUBTITLE_TYPE_DVB_221_1 = 0x13;
    public static final int ADB_SUBTITLE_TYPE_DVB_HD = 0x14;
    public static final int ADB_SUBTITLE_TYPE_DVB_HARD_HEARING = 0x20;
    public static final int ADB_SUBTITLE_TYPE_DVB_HARD_HEARING_4_3 = 0x21;
    public static final int ADB_SUBTITLE_TYPE_DVB_HARD_HEARING_16_9 = 0x22;
    public static final int ADB_SUBTITLE_TYPE_DVB_HARD_HEARING_221_1 = 0x23;
    public static final int ADB_SUBTITLE_TYPE_DVB_HARD_HEARING_HD = 0x24;

    //add define for telextet type
    public static final int ADB_TELETEXT_TYPE_INITIAL = 0x01;
    public static final int ADB_TELETEXT_TYPE_SUBTITLE = 0x02;
    public static final int ADB_TELETEXT_TYPE_ADDITIONAL_INFO = 0x03;
    public static final int ADB_TELETEXT_TYPE_SCHEDULE = 0x04;
    public static final int ADB_TELETEXT_TYPE_SUBTITLE_HARD_HEARING = 0x05;

    //add dtvkit satellite
    public static final String DTVKIT_SATELLITE_DATA = "/vendor/etc/tvconfig/dtvkit/satellite.json";

    public static void ascendTrackInfoOderByPid(List<TvTrackInfo> list) {
        if (list != null) {
            Collections.sort(list, new PidAscendComparator());
        }
    }

    public static class PidAscendComparator implements Comparator<TvTrackInfo> {
        public int compare(TvTrackInfo o1, TvTrackInfo o2) {
            Integer pid1 = new Integer(o1.getExtra().getInt("pid", 0));
            Integer pid2 = new Integer(o2.getExtra().getInt("pid", 0));
            return pid1.compareTo(pid2);
        }
    }

    public static class PidDscendComparator implements Comparator<TvTrackInfo> {
        public int compare(TvTrackInfo o1, TvTrackInfo o2) {
            Integer pid1 = new Integer(o1.getExtra().getInt("pid", 0));
            Integer pid2 = new Integer(o2.getExtra().getInt("pid", 0));
            return pid2.compareTo(pid1);
        }
    }
}
