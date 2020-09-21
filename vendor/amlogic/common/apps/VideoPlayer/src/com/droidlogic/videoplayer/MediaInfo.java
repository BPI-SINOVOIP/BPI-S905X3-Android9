/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
package com.droidlogic.videoplayer;

import android.content.Context;
import android.media.MediaPlayer;
import android.util.Log;
import android.os.PersistableBundle;
import android.os.Parcel;
import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Set;


import com.droidlogic.app.MediaPlayerExt;

public class MediaInfo {
        private static final String TAG = "MediaInfo";
        private static final boolean DEBUG = false;
        private static Context mContext = null;
        private MediaPlayerExt mp = null;
        private MediaPlayerExt.MediaInfo mInfo = null;

        private int mAudioTotal = 0;
        private int mVideoTotal = 0;
        private int mVideoWidth = 0;
        private int mVideoHeight = 0;

        public MediaInfo (MediaPlayerExt mediaPlayer, Context context) {
            mp = mediaPlayer;
            mContext = context;
        }

        public void initMediaInfo(MediaPlayerExt mp) {
            if (mp != null) {
                mInfo = mp.getMediaInfo(mp);
            }
            if (DEBUG) { printMediaInfo(); }
        }

        public void setDefaultData(MediaPlayer.TrackInfo[] trackInfo) {
            //prepare audio and video total number
            for (int i = 0; i < trackInfo.length; i++) {
                int trackType = trackInfo[i].getTrackType();
                if (trackType == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_AUDIO) {
                    mAudioTotal++;
                }
                else if (trackType == MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_VIDEO) {
                    mVideoTotal++;
                }
            }

            //prepare video width and height
            Class<?> metadataClazz = null;
            Class<?> mediaPlayerClazz = null;
            Object metadataInstance = null;
            Method has = null;
            Method getInt = null;
            Method getMetadata = null;
            Field parcelField = null;
            Field keyField = null;
            int key = -1;
            boolean isGetInt = false;
            try {
                mediaPlayerClazz = Class.forName("android.media.MediaPlayer");
                metadataClazz = Class.forName("android.media.Metadata");
                has = metadataClazz.getMethod("has", Integer.TYPE);
                getInt = metadataClazz.getMethod("getInt", Integer.TYPE);
                getMetadata = mediaPlayerClazz.getMethod("getMetadata", Boolean.TYPE, Boolean.TYPE);
                metadataInstance = getMetadata.invoke(mp, false, false);

                keyField = metadataClazz.getDeclaredField("VIDEO_WIDTH");
                keyField.setAccessible(true);
                key = (int)keyField.get(metadataInstance);
                if ((boolean)has.invoke(metadataInstance, key)) {
                    mVideoWidth = (int)getInt.invoke(metadataInstance, key);
                    isGetInt = true;
                }

                keyField = metadataClazz.getDeclaredField("VIDEO_HEIGHT");
                keyField.setAccessible(true);
                key = (int)keyField.get(metadataInstance);
                if ((boolean)has.invoke(metadataInstance, key)) {
                    mVideoHeight = (int)getInt.invoke(metadataInstance, key);
                    isGetInt = true;
                }

                if (isGetInt) {
                    parcelField = metadataClazz.getDeclaredField("mParcel");
                    parcelField.setAccessible(true);
                    Parcel p = (Parcel)parcelField.get(metadataInstance);
                    p.recycle();
                }
            }catch (Exception ex) {
                ex.printStackTrace();
            }
        }

        //@@--------this part for video info-------------------------------------------------------
        public static final int VFORMAT_UNKNOWN = -1;
        public static final int VFORMAT_MPEG12 = 0;
        public static final int VFORMAT_MPEG4 = 1;
        public static final int VFORMAT_H264 = 2;
        public static final int VFORMAT_MJPEG =3;
        public static final int VFORMAT_REAL = 4;
        public static final int VFORMAT_JPEG = 5;
        public static final int VFORMAT_VC1 = 6;
        public static final int VFORMAT_AVS = 7;
        public static final int VFORMAT_SW = 8;
        public static final int VFORMAT_H264MVC = 9;
        public static final int VFORMAT_H264_4K2K = 10;
        public static final int VFORMAT_HEVC = 11;
        public static final int VFORMAT_UNSUPPORT = 12;
        public static final int VFORMAT_MAX = 13;
        public String getFileName (String path) {
            String filename = null;
            if (path != null && path.startsWith ("content")) {
                filename = "null";
            }
            else {
                File f = new File (path);
                filename = f.getName();
                filename = filename.substring (0, filename.lastIndexOf ("."));
            }
            return filename;
        }

        public String getFileType (String path) {
            String ext = "unknown";
            String filename = null;
            //check file start with "content"
            if (path != null && path.startsWith ("content")) {
                ext = "unknown";
            }
            else {
                int idx = path.lastIndexOf (".");
                if (idx < 0) {
                    ext = "unknown";
                }
                else {
                    ext = path.substring (path.lastIndexOf (".") + 1, path.length()).toLowerCase();
                }
            }
            return ext;
        }

        public String getFileType() {
            String str_type = "UNKNOWN";
            if (mInfo != null) {
                switch (mInfo.type) {
                    case 0:
                        break;
                    case 1:
                        str_type = "AVI";
                        break;
                    case 2:
                        str_type = "MPEG";
                        break;
                    case 3:
                        str_type = "WAV";
                        break;
                    case 4:
                        str_type = "MP3";
                        break;
                    case 5:
                        str_type = "AAC";
                        break;
                    case 6:
                        str_type = "AC3";
                        break;
                    case 7:
                        str_type = "RM";
                        break;
                    case 8:
                        str_type = "DTS";
                        break;
                    case 9:
                        str_type = "MKV";
                        break;
                    case 10:
                        str_type = "MOV";
                        break;
                    case 11:
                        str_type = "MP4";
                        break;
                    case 12:
                        str_type = "FLAC";
                        break;
                    case 13:
                        str_type = "H264";
                        break;
                    case 14:
                        str_type = "M2V";
                        break;
                    case 15:
                        str_type = "FLV";
                        break;
                    case 16:
                        str_type = "P2P";
                        break;
                    case 17:
                        str_type = "ASF";
                        break;
                    default:
                        break;
                }
            }
            return str_type;
        }

        /*public String getFileSize() {
            long fs = mInfo.file_size;
            String str_size = "0";
            if (fs <= 1024)
                str_size = "1KB";
            else if (fs <= 1024 * 1024) {
                fs /= 1024;
                fs += 1;
                str_size = fs + "KB";
            }
            else if (fs > 1024 * 1024) {
                fs /= 1024*1024;
                fs += 1;
                str_size = fs + "MB";
            }
            return str_size;
        }*/

        public static final long SIZE_GB = 1024 * 1024 * 1024;
        public static final long SIZE_MB = 1024 * 1024;
        public static final long SIZE_KB = 1024;
        public String getFileSizeStr(long length) {
            int sub_index = 0;
            String sizeStr = "";
            if (length >= SIZE_GB) {
                sub_index = (String.valueOf((float)length / SIZE_GB)).indexOf(".");
                sizeStr = ((float)length / SIZE_GB + "000").substring(0, sub_index + 3) + " GB";
            }
            else if (length >= SIZE_MB) {
                sub_index = (String.valueOf((float)length / SIZE_MB)).indexOf(".");
                sizeStr =((float)length / SIZE_MB + "000").substring(0, sub_index + 3) + " MB";
            }
            else if (length >= SIZE_KB) {
                sub_index = (String.valueOf((float)length / SIZE_KB)).indexOf(".");
                sizeStr = ((float)length/SIZE_KB + "000").substring(0, sub_index + 3) + " KB";
            }
            else if (length < SIZE_KB) {
                sizeStr = String.valueOf(length) + " B";
            }
            return sizeStr;
        }

        public String getFileSize(String path) {
            String sizeStr = "";
            if (mInfo.file_size == null || mInfo.file_size.equals("null") || mInfo.file_size.equals("NULL")) {
                File file = new File (path);
                sizeStr = getFileSizeStr(file.length());
            }
            else {
                sizeStr = mInfo.file_size;
            }
            return sizeStr;
        }

        public String getResolution() {
            return getVideoWidth() + "*" + getVideoHeight();
        }

        public int getVideoTotalNum() {
            int num = -1;
            if (mInfo != null) {
                num = mInfo.total_video_num;
                if (num <= 0) {
                    num = mVideoTotal;
                }
            }
            return num;
        }

        public int getCurVideoIdx() {
            if (mInfo != null && mInfo.total_video_num > 0) {
                for (int i = 0; i < mInfo.total_video_num; i++) {
                    if (mInfo.cur_video_index == mInfo.videoInfo[i].index) {// current video track index, should tranfer to list index
                        return i; // current list index
                    }
                }
            }

            return -1;
        }

        public int getVideoIdx(int listIdx) {
            if (mInfo != null && mInfo.total_video_num > 0) {
                return mInfo.videoInfo[listIdx].index;
            }

            return -1;
        }

        public String getVideoFormat(int i) {
            if (mInfo != null && mInfo.total_video_num > 0) {
                return mInfo.videoInfo[i].vformat;
            }

            return null;
        }

        public int getTsTotalNum() {
            if (mInfo != null) {
                return mInfo.total_ts_num;
            }

            return 0;
        }

        public String getTsTitle(int i) {
            if (mInfo != null && mInfo.total_ts_num > 0) {
                for (int m = 0; m < mInfo.total_ts_num; m++) {
                    if (mInfo.videoInfo[i].id == mInfo.tsprogrameInfo[m].v_pid) {
                        return mInfo.tsprogrameInfo[m].title;
                    }
                }
            }

            return null;
        }

        public String geVideoFormatStr(int vFormat) {
            String type = null;
            switch (vFormat) {
                case VFORMAT_UNKNOWN:
                    type = "UNKNOWN";
                    break;
                case VFORMAT_MPEG12:
                    type = "MPEG12";
                    break;
                case VFORMAT_MPEG4:
                    type = "MPEG4";
                    break;
                case VFORMAT_H264:
                    type = "H264";
                    break;
                case VFORMAT_MJPEG:
                    type = "MJPEG";
                    break;
                case VFORMAT_REAL:
                    type = "REAL";
                    break;
                case VFORMAT_JPEG:
                    type = "JPEG";
                    break;
                case VFORMAT_VC1:
                    type = "VC1";
                    break;
                case VFORMAT_AVS:
                    type = "AVS";
                    break;
                case VFORMAT_SW:
                    type = "SW";
                    break;
                case VFORMAT_H264MVC:
                    type = "H264MVC";
                    break;
                case VFORMAT_H264_4K2K:
                    type = "H264_4K2K";
                    break;
                case VFORMAT_HEVC:
                    type = "HEVC";
                    break;
                case AFORMAT_UNSUPPORT:
                    type = "UNSUPPORT";
                    break;
                case AFORMAT_MAX:
                    type = "MAX";
                    break;
                default:
                    type = "UNKNOWN";
                    break;
            }
            return type;
            }

        public int getVideoWidth() {
            int width = -1;
            if (mInfo != null && mInfo.total_video_num > 0) {
                width = mInfo.videoInfo[0].width;
            }
            if (width <= 0) {
                width = mVideoWidth;
            }
            return width;
        }

        public int getVideoHeight() {
            int height = -1;
            if (mInfo != null && mInfo.total_video_num > 0) {
                height = mInfo.videoInfo[0].height;
            }
            if (height <= 0) {
                height = mVideoHeight;
            }
            return height;
        }

        public int getDuration() {
            int ret = -1;
            if (mInfo != null) {
                ret = mInfo.duration;
            }
            return ret;
        }

        public String getVFormat() {
            String vformatStr = "UNKNOWN";
            if (mInfo != null) {
                if (mInfo.total_video_num > 0) {
                    vformatStr = mInfo.videoInfo[0].vformat;
                }
            }
            return vformatStr;
        }

        public int getBitrate() {
            int bitrate = -1;
            if (mInfo != null) {
                bitrate = mInfo.bitrate;
            }
            return bitrate;
        }

        public int getFps() {
            int fps = -1;
            if (mInfo != null) {
                //fps = mInfo.fps;//wxl shield 20150925 need MediaPlayer.java code merge
            }
            return fps;
        }

        public String getSFormat(int i) {
            String sformat = "";
            if (mInfo != null/* && mSubtitleManager != null*/) {
                //sformat = mSubtitleManager.subtitleGetSubTypeStr(); //wxl shield 20150925 need MediaPlayer.java code merge, no api
                if (sformat.equals("INSUB")) {
                    if (i < mInfo.total_sub_num) {
                        sformat = "INSUB"; // TODO: //mInfo.subtitleInfo[i].sub_type; // insub + external sub
                    }
                }
            }
            return sformat;
        }

        //@@--------this part for audio info-------------------------------------------------------
        //audio format
        public static final int AFORMAT_UNKNOWN = -1;
        public static final int AFORMAT_MPEG   = 0;
        public static final int AFORMAT_PCM_S16LE = 1;
        public static final int AFORMAT_AAC   = 2;
        public static final int AFORMAT_AC3   = 3;
        public static final int AFORMAT_ALAW = 4;
        public static final int AFORMAT_MULAW = 5;
        public static final int AFORMAT_DTS = 6;
        public static final int AFORMAT_PCM_S16BE = 7;
        public static final int AFORMAT_FLAC = 8;
        public static final int AFORMAT_COOK = 9;
        public static final int AFORMAT_PCM_U8 = 10;
        public static final int AFORMAT_ADPCM = 11;
        public static final int AFORMAT_AMR  = 12;
        public static final int AFORMAT_RAAC  = 13;
        public static final int AFORMAT_WMA  = 14;
        public static final int AFORMAT_WMAPRO   = 15;
        public static final int AFORMAT_PCM_BLURAY = 16;
        public static final int AFORMAT_ALAC = 17;
        public static final int AFORMAT_VORBIS = 18;
        public static final int AFORMAT_AAC_LATM = 19;
        public static final int AFORMAT_APE   = 20;
        public static final int AFORMAT_EAC3   = 21;
        public static final int AFORMAT_PCM_WIFIDISPLAY = 22;
        public static final int AFORMAT_DRA = 23;
        public static final int AFORMAT_SIPR = 24;
        public static final int AFORMAT_TRUEHD = 25;
        public static final int AFORMAT_MPEG1 = 26;
        public static final int AFORMAT_MPEG2 = 27;
        public static final int AFORMAT_WMAVOI = 28;
        public static final int AFORMAT_UNSUPPORT = 29;
        public static final int AFORMAT_MAX = 30;

        public int getCurAudioIdx() {
            int ret = -1;
            int aIdx = -1;
            if (mInfo != null && mInfo.total_audio_num > 0) {
                aIdx = mInfo.cur_audio_index;// current audio track index, should tranfer to list index
                for (int i = 0; i < mInfo.total_audio_num; i++) {
                    if (mInfo.cur_audio_index == mInfo.audioInfo[i].index) {
                        ret = i; // current list index
                    }
                }
            }
            return ret;
        }

        public int getAudioTotalNum() {
            int ret = -1;
            if (mInfo != null) {
                ret = mInfo.total_audio_num;
                if (ret <= 0) {
                    ret = mAudioTotal;
                }
            }
            return ret;
        }

        public int getAudioIdx (int listIdx) {
            int ret = -1;
            if (mInfo != null && mInfo.total_audio_num > 0) {
                ret = mInfo.audioInfo[listIdx].index;
            }
            return ret;
        }

        public int getAudioSampleRate(int i) {
            int ret = -1;
            if (mInfo != null && mInfo.total_audio_num > 0) {
                ret = mInfo.audioInfo[i].sample_rate;
            }
            return ret;
        }

        public int getAudioFormat (int i) {
            int ret = -1;
            if (mInfo != null && mInfo.total_audio_num > 0) {
                ret = mInfo.audioInfo[i].aformat;
            }
            return ret;
        }

        public String getAudioMime (int i) {
            String mime;
            if (mInfo != null && mInfo.total_audio_num > 0) {
                mime = mInfo.audioInfo[i].audioMime;
                Log.i(TAG,"amime:"+mime);
                return mime;
            }
            return null;
        }

        public String getAudioFormatByMetrics() {
            PersistableBundle metrics = mp.getMetrics();
            if (metrics != null && !metrics.isEmpty()) {
                String aMime = metrics.getString(MediaPlayer.MetricsConstants.MIME_TYPE_AUDIO, null);
                Log.i(TAG,"amime:"+aMime);
                return aMime;
            }
            return null;
        }
        public String getVideoFormatByMetrics() {
            PersistableBundle metrics = mp.getMetrics();
            if (metrics != null && !metrics.isEmpty()) {
                String vMime = metrics.getString(MediaPlayer.MetricsConstants.MIME_TYPE_VIDEO, null);
                Log.i(TAG,"vmime:"+vMime);
                return vMime;
            }
            return null;
        }

        public String getAudioFormatStr (int aFormat) {
            String type = null;
            switch (aFormat) {
                case AFORMAT_UNKNOWN:
                    type = "UNKNOWN";
                    break;
                case AFORMAT_MPEG:
                    type = "MP3";
                    break;
                case AFORMAT_PCM_S16LE:
                    type = "PCM";
                    break;
                case AFORMAT_AAC:
                    type = "AAC";
                    break;
                case AFORMAT_AC3:
                    type = "AC3";
                    break;
                case AFORMAT_ALAW:
                    type = "ALAW";
                    break;
                case AFORMAT_MULAW:
                    type = "MULAW";
                    break;
                case AFORMAT_DTS:
                    type = "DTS";
                    break;
                case AFORMAT_PCM_S16BE:
                    type = "PCM_S16BE";
                    break;
                case AFORMAT_FLAC:
                    type = "FLAC";
                    break;
                case AFORMAT_COOK:
                    type = "COOK";
                    break;
                case AFORMAT_PCM_U8:
                    type = "PCM_U8";
                    break;
                case AFORMAT_ADPCM:
                    type = "ADPCM";
                    break;
                case AFORMAT_AMR:
                    type = "AMR";
                    break;
                case AFORMAT_RAAC:
                    type = "RAAC";
                    break;
                case AFORMAT_WMA:
                    type = "WMA";
                    break;
                case AFORMAT_WMAPRO:
                    type = "WMAPRO";
                    break;
                case AFORMAT_PCM_BLURAY:
                    type = "PCM_BLURAY";
                    break;
                case AFORMAT_ALAC:
                    type = "ALAC";
                    break;
                case AFORMAT_VORBIS:
                    type = "VORBIS";
                    break;
                case AFORMAT_AAC_LATM:
                    type = "AAC_LATM";
                    break;
                case AFORMAT_APE:
                    type = "APE";
                    break;
                case AFORMAT_EAC3:
                    type = "EAC3";
                    break;
                case AFORMAT_PCM_WIFIDISPLAY:
                    type = "PCM_WIFIDISPLAY";
                    break;
                case AFORMAT_DRA:
                    type = "DRA";
                    break;
                case AFORMAT_SIPR:
                    type = "SIPR";
                    break;
                case AFORMAT_TRUEHD:
                    type = "TRUEHD";
                    break;
                case AFORMAT_MPEG1:
                    type = "MP1";
                    break;
                case AFORMAT_MPEG2:
                    type = "MP2";
                    break;
                case AFORMAT_WMAVOI:
                    type = "WMAVOI";
                    break;
                case AFORMAT_UNSUPPORT:
                    type = "UNSUPPORT";
                    break;
                case AFORMAT_MAX:
                    type = "MAX";
                    break;
                default:
                    type = "UNKNOWN";
                    break;
            }
            return type;
        }
        public String getAudioCodecStrByMetrics (String aMime, String vMime) {
            String type = "UNKNOWN";
            switch (aMime) {
                case "audio/mpeg":
                    type = "MP3";
                    break;
                case "audio/dtshd":
                    type = "DTS";
                    break;
                case "audio/ac3":
                    type = "AC3";
                    break;
                case "audio/eac3":
                    type = "EAC3";
                    break;
                case "audio/adpcm":
                    type = "ADPCM";
                    break;
                case "audio/x-aac":
                    type = "AAC";
                    break;
                case "audio/vnd.dra":
                    type = "DRA";
                    break;
                case "audio/mp1":
                    type = "MP1";
                    break;
                case "audio/mp2":
                    type = "MP2";
                    break;
                case "audio/mp3":
                    type = "MP3";
                    break;
                case "audio/x-ms-wma":
                    type = "WMA";
                    break;
                case "audio/flac":
                    type = "FLAC";
                    break;
                case "audio/raw":
                    type = "PCM";
                    break;
                case "audio/mp4a-latm":
                    type = "AAC_LATM";
                    break;
                case "audio/ffmpeg":
                    if (vMime.equals("video/mpeg2"))
                        type = "MP2";
                    else if (vMime.equals("video/mp4v-es"))
                        type = "MP3";
                    else if (vMime.equals("video/hevc"))
                        type = "AAC";
                    else if (vMime.equals("video/avs"))
                        type = "AVS";
                    else if (vMime.equals("video/x-vnd.on2.vp8"))
                        type = "FLAC";
                    else if (vMime.equals("video/x-vnd.on2.vp9"))
                        type = "OPUS";
                    else if (vMime.equals("video/avs2"))
                        type = "MPEG";
                    else if (vMime.equals("video/rm40")
                           || vMime.equals("video/rm30")
                           || vMime.equals("video/rm20")
                           || vMime.equals("video/rm10"))
                        type = "COOKER";
                    else
                        type = "WMA";
                    break;
                case "audio/g711-mlaw":
                    type = "MULAW";
                    break;
                case "audio/vorbis":
                    type = "VORBIS";
                    break;
                case "audio/opus":
                    type = "OPUS";
                    break;
                case "audio/3gpp":
                    if (vMime.equals("video/mp4v-es"))
                        type = "MP3";
                    break;
            }
            return type;
        }


        //@@--------this part for DTS Asset check -------------------------------------------------------------
        public boolean checkAudioisDTS (int aFormat) {
            boolean ret = false;
            if (aFormat == AFORMAT_DTS) {
                ret = true;
            }
            return ret;
        }

        //@@--------this part for certification check-------------------------------------------------------------
        public static final int CERTIFI_Dolby  = 1;
        public static final int CERTIFI_Dolby_Plus  = 2;
        public static final int CERTIFI_DTS  = 3;
        public int checkAudioCertification (String aMime) {
            int ret = -1;
            if (aMime != null) {
                if (aMime.equals("audio/ac3")) {
                    ret = CERTIFI_Dolby;
                }
                else if (aMime.equals("audio/eac3")) {
                    ret = CERTIFI_Dolby_Plus;
                }
                else if (aMime.equals("audio/dtshd")) {
                    ret = CERTIFI_DTS;
                }
            }
            // add more ...
            return ret;
        }

        //@@--------this part for media info show on OSD--------------------------------------------------------
        //media info no, must sync with media_info_type in MediaPlayer.h
        public static final int MEDIA_INFO_UNKNOWN = 1;
        public static final int MEDIA_INFO_STARTED_AS_NEXT = 2;
        public static final int MEDIA_INFO_RENDERING_START = 3;
        public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;
        public static final int MEDIA_INFO_BUFFERING_START = 701;
        public static final int MEDIA_INFO_BUFFERING_END = 702;
        public static final int MEDIA_INFO_NETWORK_BANDWIDTH = 703;
        public static final int MEDIA_INFO_BAD_INTERLEAVING = 800;
        public static final int MEDIA_INFO_NOT_SEEKABLE = 801;
        public static final int MEDIA_INFO_METADATA_UPDATE = 802;
        public static final int MEDIA_INFO_AUDIO_NOT_PLAYING = 804;
        public static final int MEDIA_INFO_VIDEO_NOT_PLAYING = 805;
        public static final int MEDIA_INFO_TIMED_TEXT_ERROR = 900;
        public static final int MEDIA_INFO_AMLOGIC_BASE = 8000;
        public static final int MEDIA_INFO_AMLOGIC_VIDEO_NOT_SUPPORT = MEDIA_INFO_AMLOGIC_BASE + 1;
        public static final int MEDIA_INFO_AMLOGIC_AUDIO_NOT_SUPPORT = MEDIA_INFO_AMLOGIC_BASE + 2;
        public static final int MEDIA_INFO_AMLOGIC_NO_VIDEO = MEDIA_INFO_AMLOGIC_BASE + 3;
        public static final int MEDIA_INFO_AMLOGIC_NO_AUDIO = MEDIA_INFO_AMLOGIC_BASE + 4;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_DTS_ASSET = MEDIA_INFO_AMLOGIC_BASE + 5;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_DTS_EXPRESS = MEDIA_INFO_AMLOGIC_BASE + 6;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_DTS_HD_MASTER_AUDIO = MEDIA_INFO_AMLOGIC_BASE + 7;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_AUDIO_LIMITED = MEDIA_INFO_AMLOGIC_BASE+8;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_DTS_MULASSETHINT = MEDIA_INFO_AMLOGIC_BASE+9;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_DTS_HPS_NOTSUPPORT = MEDIA_INFO_AMLOGIC_BASE+10;
        public static final int MEDIA_INFO_AMLOGIC_BLURAY_STREAM_PATH = MEDIA_INFO_AMLOGIC_BASE + 11;
        public static final int MEDIA_INFO_AMLOGIC_SHOW_DOLBY_VISION = MEDIA_INFO_AMLOGIC_BASE + 12;

        public static final int BLURAY_STREAM_TYPE_VIDEO = 'V';
        public static final int BLURAY_STREAM_TYPE_AUDIO = 'A';
        public static final int BLURAY_STREAM_TYPE_SUB = 'S';

        public static boolean needShowOnUI (int info) {
            boolean ret = false;
            if ( (info == MEDIA_INFO_AMLOGIC_VIDEO_NOT_SUPPORT)
                    || (info == MEDIA_INFO_AMLOGIC_AUDIO_NOT_SUPPORT)
                    || (info == MEDIA_INFO_AMLOGIC_NO_VIDEO)
                    || (info == MEDIA_INFO_AMLOGIC_NO_AUDIO)
                    || (info == MEDIA_INFO_VIDEO_NOT_PLAYING)
                    || (info == MEDIA_INFO_AUDIO_NOT_PLAYING)
                    || (info == MEDIA_INFO_TIMED_TEXT_ERROR)) {
                ret = true;
            }
            return ret;
        }

        public static String getInfo (int info, Context context) {
            String infoStr = null;
            switch (info) {
                case MEDIA_INFO_AMLOGIC_VIDEO_NOT_SUPPORT:
                    infoStr = context.getResources().getString (R.string.unsupport_video_format); //"Unsupport Video format";
                    break;
                case MEDIA_INFO_AMLOGIC_AUDIO_NOT_SUPPORT:
                    infoStr = context.getResources().getString (R.string.unsupport_audio_format); //"Unsupport Audio format";
                    break;
                case MEDIA_INFO_AMLOGIC_NO_VIDEO:
                    infoStr = context.getResources().getString (R.string.file_have_no_video); //"file have no video";
                    break;
                case MEDIA_INFO_AMLOGIC_NO_AUDIO:
                    infoStr = context.getResources().getString (R.string.file_have_no_audio); //"file have no audio";
                    break;
                case MEDIA_INFO_VIDEO_NOT_PLAYING:
                    infoStr = context.getResources().getString (R.string.unsupport_video_format); //"video not playing";
                    break;
                case MEDIA_INFO_AUDIO_NOT_PLAYING:
                    infoStr = context.getResources().getString (R.string.unsupport_audio_format); //"audio not playing";
                    break;
                case MEDIA_INFO_TIMED_TEXT_ERROR:
                    infoStr = context.getResources().getString (R.string.play_subtitle_error); //"subtitle file error";
                    break;
                default:
                    break;
            }
            return infoStr;
        }

        private void printMediaInfo() {
            Log.i (TAG, "[printMediaInfo]mInfo:" + mInfo);
            if (mInfo != null) {
                Log.i (TAG, "[printMediaInfo]filename:" + mInfo.filename + ",duration:" + mInfo.duration + ",file_size:" + mInfo.file_size + ",bitrate:" + mInfo.bitrate + ",type:" + mInfo.type);
                Log.i (TAG, "[printMediaInfo]cur_video_index:" + mInfo.cur_video_index + ",cur_audio_index:" + mInfo.cur_audio_index + ",cur_sub_index:" + mInfo.cur_sub_index);
                //----video info----
                Log.i (TAG, "[printMediaInfo]total_video_num:" + mInfo.total_video_num);
                for (int i = 0; i < mInfo.total_video_num; i++) {
                    Log.i (TAG, "[printMediaInfo]videoInfo i:" + i + ",index:" + mInfo.videoInfo[i].index + ",id:" + mInfo.videoInfo[i].id);
                    Log.i (TAG, "[printMediaInfo]videoInfo i:" + i + ",vformat:" + mInfo.videoInfo[i].vformat);
                    Log.i (TAG, "[printMediaInfo]videoInfo i:" + i + ",width:" + mInfo.videoInfo[i].width + ",height:" + mInfo.videoInfo[i].height);
                }
                //----audio info----
                Log.i (TAG, "[printMediaInfo]total_audio_num:" + mInfo.total_audio_num);
                for (int j = 0; j < mInfo.total_audio_num; j++) {
                    Log.i (TAG, "[printMediaInfo]audioInfo j:" + j + ",index:" + mInfo.audioInfo[j].index + ",id:" + mInfo.audioInfo[j].id); //mInfo.audioInfo[j].id is useless for application
                    Log.i (TAG, "[printMediaInfo]audioInfo j:" + j + ",aformat:" + mInfo.audioInfo[j].aformat);
                    Log.i (TAG, "[printMediaInfo]audioInfo j:" + j + ",channel:" + mInfo.audioInfo[j].channel + ",sample_rate:" + mInfo.audioInfo[j].sample_rate);
                }
                //----subtitle info got from google standard flow----
                Log.i (TAG, "[printMediaInfo]total_sub_num:" + mInfo.total_sub_num);
                for (int k = 0; k < mInfo.total_sub_num; k++) {
                    Log.i (TAG, "[printMediaInfo]subtitleInfo k:" + k + ",index:" + mInfo.subtitleInfo[k].index + ",id:" + mInfo.subtitleInfo[k].id + ",sub_type:" + mInfo.subtitleInfo[k].sub_type);
                    Log.i (TAG, "[printMediaInfo]subtitleInfo k:" + k + ",sub_language:" + mInfo.subtitleInfo[k].sub_language);
                }
            }
        }

}

