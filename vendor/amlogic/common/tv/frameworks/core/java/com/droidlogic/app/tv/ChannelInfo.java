/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.net.Uri;
import android.util.Log;
import android.text.TextUtils;

import java.util.Map;
import java.util.Arrays;
import android.database.sqlite.SQLiteException;
import java.net.URLEncoder;
import java.net.URLDecoder;
import java.util.HashMap;
import java.io.UnsupportedEncodingException;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class ChannelInfo {
    private static final String TAG = "ChannelInfo";
    private static final boolean DEBUG = false;

    public static final String COLUMN_LCN = Channels.COLUMN_INTERNAL_PROVIDER_FLAG2;
    public static final String COLUMN_LCN1 = Channels.COLUMN_INTERNAL_PROVIDER_FLAG3;
    public static final String COLUMN_LCN2 = Channels.COLUMN_INTERNAL_PROVIDER_FLAG4;

    public static final String[] COMMON_PROJECTION = {
        Channels._ID,
        Channels.COLUMN_INPUT_ID,
        Channels.COLUMN_TYPE,
        Channels.COLUMN_SERVICE_TYPE,
        Channels.COLUMN_SERVICE_ID,
        Channels.COLUMN_DISPLAY_NUMBER,
        Channels.COLUMN_DISPLAY_NAME,
        Channels.COLUMN_ORIGINAL_NETWORK_ID,
        Channels.COLUMN_TRANSPORT_STREAM_ID,
        Channels.COLUMN_VIDEO_FORMAT,
        Channels.COLUMN_INTERNAL_PROVIDER_DATA,
        Channels.COLUMN_BROWSABLE,
        TvContract.Channels.COLUMN_LOCKED,
        COLUMN_LCN,
        COLUMN_LCN1,
        COLUMN_LCN2
    };

    public static final String[] SIMPLE_PROJECTION = {
        TvContract.Channels._ID,
        TvContract.Channels.COLUMN_DISPLAY_NUMBER,
        TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID,
        TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID,
        TvContract.Channels.COLUMN_SERVICE_ID
    };

    public static final String KEY_DISPLAY_NUMBER = "display_number";

    public static final String KEY_TYPE = "type";
    public static final String KEY_SERVICE_TYPE = "service_type";

    //need sync with TvContract, different from KEY_VFMT
    public static final String KEY_VIDEO_FORMAT = "video_format";
    public static final String KEY_VFMT = "vfmt";//used for play programs
    public static final String KEY_VIDEO_PID = "video_pid";
    public static final String KEY_VIDEO_STD = "video_std";

    public static final String KEY_AUDIO_PIDS = "audio_pids";
    public static final String KEY_AUDIO_FORMATS = "AUDIO_FORMATS";
    public static final String KEY_AUDIO_LANGS = "audio_langs";
    public static final String KEY_AUDIO_EXTS = "audio_exts";
    public static final String KEY_AUDIO_TRACK_INDEX = "audio_track_index";
    public static final String KEY_AUDIO_OUTPUT_MODE = "audio_out_mode";
    public static final String KEY_AUDIO_COMPENSATION = "audio_compensation";
    public static final String KEY_AUDIO_CHANNEL = "audio_channel";
    public static final String KEY_AUDIO_STD = "audio_std";
    public static final String KEY_IS_AUTO_STD = "is_auto_std";

    public static final String KEY_PCR_ID = "pcr_id";

    public static final String KEY_SUBT_TYPES = "subt_types";
    public static final String KEY_SUBT_PIDS = "subt_pids";
    public static final String KEY_SUBT_STYPES = "subt_stypes";
    public static final String KEY_SUBT_ID1S = "subt_id1s";
    public static final String KEY_SUBT_ID2S = "subt_id2s";
    public static final String KEY_SUBT_LANGS = "subt_langs";
    public static final String KEY_SUBT_TRACK_INDEX = "subt_track_index";

    public static final String KEY_MULTI_NAME = "multi_name";

    public static final String KEY_FREE_CA = "free_ca";
    public static final String KEY_SCRAMBLED = "scrambled";

    public static final String KEY_SDT_VERSION = "sdt_version";

    public static final String KEY_FREQUENCY = "frequency";
    public static final String KEY_BAND_WIDTH = "band_width";
    public static final String KEY_SYMBOL_RATE = "symbol_rate";
    public static final String KEY_MODULATION = "modulation";
    public static final String KEY_FE_PARAS = "fe";
    public static final String KEY_FINE_TUNE = "fine_tune";
    public static final String KEY_IS_FAVOURITE = "is_favourite";
    public static final String KEY_SET_FAVOURITE = "set_favourite";
    public static final String KEY_SET_DISPLAYNAME = "set_displayname";
    public static final String KEY_SET_DISPLAYNUMBER = "set_displaynumber";
    public static final String KEY_FAVOURITE_INFO = "favourite_info";
    public static final String KEY_SATELLITE_INFO = "satellite_info";
    public static final String KEY_SATELLITE_INFO_NAME = "satellite_info_name";
    public static final String KEY_TRANSPONDER_INFO = "transponder_info";
    public static final String KEY_TRANSPONDER_INFO_DISPLAY_NAME = "transponder_info_display_name";
    public static final String KEY_TRANSPONDER_INFO_SATELLITE_NAME = "transponder_info_satellite_name";
    public static final String KEY_TRANSPONDER_INFO_TRANSPONDER_FREQUENCY = "transponder_info_frequency";
    public static final String KEY_TRANSPONDER_INFO_TRANSPONDER_POLARITY = "transponder_info_polarity";
    public static final String KEY_TRANSPONDER_INFO_TRANSPONDER_SYMBOL = "transponder_info_symbol";
    public static final String KEY_CHANNEL_SIGNAL_TYPE = "channel_signal_type";

    public static final String KEY_MAJOR_NUM = "majorNum";
    public static final String KEY_MINOR_NUM = "minorNum";
    public static final String KEY_SOURCE_ID = "srcId";
    public static final String KEY_ACCESS_CONTROL = "access";
    public static final String KEY_HIDDEN = "hidden";
    public static final String KEY_SET_HIDDEN = "set_hidden";
    public static final String KEY_HIDE_GUIDE = "hideGuide";
    public static final String KEY_VCT = "vct";
    public static final String KEY_EITV = "eitv";
    public static final String KEY_PROGRAMS_IN_PAT = "programs_in_pat";
    public static final String KEY_PAT_TS_ID = "pat_ts_id";

    public static final String EXTRA_CHANNEL_INFO = "extra_channel_info";
    public static final String KEY_CONTENT_RATINGS = "content_ratings";
    public static final String KEY_SIGNAL_TYPE = "signal_type";

    public static final String KEY_OTHER_CUSTOM = "custom";//for other type channel

    public static final String LABEL_ATV = "ATV";
    public static final String LABEL_DTV = "DTV";
    public static final String LABEL_AV1 = "AV1";
    public static final String LABEL_AV2 = "AV2";
    public static final String LABEL_HDMI1 = "HDMI1";
    public static final String LABEL_HDMI2 = "HDMI2";
    public static final String LABEL_HDMI3 = "HDMI3";
    public static final String LABEL_HDMI4 = "HDMI4";
    public static final String LABEL_SPDIF = "SPDIF";
    public static final String LABEL_AUX = "AUX";
    public static final String LABEL_ARC = "ARC";

    private long mId;
    private String mInputId;
    private String mType;
    private String mServiceType;
    private int mServiceId;
    private String mDisplayNumber;
    private int mNumber;
    private String mDisplayName;
    private String mVideoFormat;
    private String mLogoUrl;
    private int mOriginalNetworkId;
    private int mTransportStreamId;

    private int mVideoPid;
    private int mVideoStd;
    private int mVfmt;
    private int mVideoWidth;
    private int mVideoHeight;

    private int mAudioPids[];
    private int mAudioFormats[];
    private String mAudioLangs[];
    private int mAudioExts[];
    private int mAudioStd;
    private int mIsAutoStd;
    private int mAudioTrackIndex; //-1:not select, -2:off, >=0:index
    private int mAudioCompensation;
    private int mAudioChannel;
    private int mAudioOutPutMode;//-1:not set 0:mono 1:stereo 2:sap

    private int mPcrPid;

    private int mSubtitleTypes[];
    private int mSubtitlePids[];
    private int mSubtitleStypes[];
    private int mSubtitleId1s[];
    private int mSubtitleId2s[];
    private String mSubtitleLangs[];
    private int mSubtitleTrackIndex;//-1:not select, -2:off, >=0:index

    private int mFrequency;
    private int mBandwidth;
    private int mSymbolRate;
    private int mModulation;
    private int mFineTune;
    private String mFEParas;

    private boolean mBrowsable;
    private boolean mIsFavourite;
    private boolean mLocked;
    private boolean mIsPassthrough;
    private String mSatelliteInfo;
    private String mTransponderInfo;
    private String mFavInfo;
    private String mChannelSignalType;

    private String mDisplayNameMulti;//multi-language

    private int mFreeCa;
    private int mScrambled;

    private int mSdtVersion;

    private int mLCN;
    private int mLCN1;
    private int mLCN2;

    private int mMajorChannelNumber;
    private int mMinorChannelNumber;
    private int mSourceId;
    private int mAccessControlled;
    private int mHidden;
    private int mHideGuide;
    private String mVct;
    private int[] mEitVersions;
    private int   mProgramsInPat;
    private int   mPatTsId;

    private String mContentRatings;
    private String mSignalType;

    private ChannelInfo() {}

    public static ChannelInfo createPassthroughChannel(Uri paramUri) {
        if (!TvContract.isChannelUriForPassthroughInput(paramUri))
            throw new IllegalArgumentException("URI is not a passthrough channel URI");
        return createPassthroughChannel((String)paramUri.getPathSegments().get(1));
    }

    public static ChannelInfo createPassthroughChannel(String input_id) {
        return new Builder().setInputId(input_id).setPassthrough(true).build();
    }

    public static ChannelInfo fromCommonCursor(Cursor cursor) {
        Builder builder = new Builder();

        int index = cursor.getColumnIndex(Channels._ID);
        if (index >= 0)
            builder.setId(cursor.getLong(index));
        index = cursor.getColumnIndex(Channels.COLUMN_INPUT_ID);
        if (index >= 0)
            builder.setInputId(cursor.getString(index));
        index = cursor.getColumnIndex(Channels.COLUMN_TYPE);
        if (index >= 0)
            builder.setType(cursor.getString(index));
        index = cursor.getColumnIndex(Channels.COLUMN_SERVICE_TYPE);
        if (index >= 0)
            builder.setServiceType(cursor.getString(index));
        index = cursor.getColumnIndex(Channels.COLUMN_SERVICE_ID);
        if (index >= 0)
            builder.setServiceId(cursor.getInt(index));
        index = cursor.getColumnIndex(Channels.COLUMN_DISPLAY_NUMBER);
        if (index >= 0)
            builder.setDisplayNumber(cursor.getString(index));
        index = cursor.getColumnIndex(Channels.COLUMN_DISPLAY_NAME);
        if (index >= 0)
            builder.setDisplayName(cursor.getString(index));
        index = cursor.getColumnIndex(Channels.COLUMN_ORIGINAL_NETWORK_ID);
        if (index >= 0)
            builder.setOriginalNetworkId(cursor.getInt(index));
        index = cursor.getColumnIndex(Channels.COLUMN_TRANSPORT_STREAM_ID);
        if (index >= 0)
            builder.setTransportStreamId(cursor.getInt(index));
        index = cursor.getColumnIndex(Channels.COLUMN_VIDEO_FORMAT);
        if (index >= 0)
            builder.setVideoFormat(cursor.getString(index));
        index = cursor.getColumnIndex(Channels.COLUMN_INTERNAL_PROVIDER_DATA);
        if (index >= 0) {
            String value = null;
            int type = 0;
            try{
                type = cursor.getType(index);
                if (type == Cursor.FIELD_TYPE_BLOB) {
                    //youtube iptv database in this column is blob. Add it for the future.
                    byte[] data = cursor.getBlob(index);
                    //Log.d(TAG,"cursor is blob, return null");
                    value = DroidLogicTvUtils.deserializeInternalProviderData(data);
                    if (DEBUG) Log.i(TAG,"cursor is blob, set value = " + value);
                } else if (type == Cursor.FIELD_TYPE_STRING)
                    value = cursor.getString(index);
                else
                    value = null;//return null;
            } catch (SQLiteException e) {
                if (DEBUG) Log.d(TAG,"SQLiteException:"+e);
                return null;
            }
            Map<String, String> parsedMap = null;
            if (type == Cursor.FIELD_TYPE_BLOB) {
                parsedMap = DroidLogicTvUtils.multiJsonToMap(value);
            } else {
                parsedMap = DroidLogicTvUtils.jsonToMap(value);
            }
            if (parsedMap != null && parsedMap.size() > 0 && parsedMap.get(KEY_AUDIO_PIDS) != null) {
                String[] str_audioPids = parsedMap.get(KEY_AUDIO_PIDS).replace("[", "").replace("]", "").split(",");
                String[] str_audioFormats = parsedMap.get(KEY_AUDIO_FORMATS).replace("[", "").replace("]", "").split(",");
                String[] str_audioExts = parsedMap.get(KEY_AUDIO_EXTS).replace("[", "").replace("]", "").split(",");
                int number = (str_audioPids[0].compareTo("null") == 0)? 0 : str_audioPids.length;
                int[] audioPids = null;
                int[] audioFormats = null;
                int[] audioExts = null;
                String[] audioLangs = null;
                if (number > 0) {
                    audioPids = new int[number];
                    audioFormats = new int[number];
                    audioExts = new int[number];
                    audioLangs = new String[number];
                    for (int i=0; i < str_audioPids.length; i++) {
                        audioPids[i] = Integer.parseInt(str_audioPids[i]);
                        audioFormats[i] = Integer.parseInt(str_audioFormats[i]);
                        audioExts[i] = Integer.parseInt(str_audioExts[i]);
                    }
                    audioLangs = DroidLogicTvUtils.TvString.fromArrayString(parsedMap.get(KEY_AUDIO_LANGS));
                    builder.setAudioPids(audioPids);
                    builder.setAudioFormats(audioFormats);
                    builder.setAudioExts(audioExts);
                    builder.setAudioLangs(audioLangs);
                }
            }else {
                builder.setAudioPids(null);
                builder.setAudioFormats(null);
                builder.setAudioExts(null);
                builder.setAudioLangs(null);
            }
            if (parsedMap != null && parsedMap.get(KEY_VFMT) != null)
                builder.setVfmt(Integer.parseInt(parsedMap.get(KEY_VFMT)));
            if (parsedMap != null && parsedMap.get(KEY_FREQUENCY) != null)
                builder.setFrequency(Integer.parseInt(parsedMap.get(KEY_FREQUENCY)));
            if (parsedMap != null && parsedMap.get(KEY_BAND_WIDTH) != null)
                builder.setBandwidth(Integer.parseInt(parsedMap.get(KEY_BAND_WIDTH)));
            if (parsedMap != null && parsedMap.get(KEY_SYMBOL_RATE) != null)
                builder.setSymbolRate(Integer.parseInt(parsedMap.get(KEY_SYMBOL_RATE)));
            if (parsedMap != null && parsedMap.get(KEY_MODULATION) != null)
                builder.setModulation(Integer.parseInt(parsedMap.get(KEY_MODULATION)));
            if (parsedMap != null && parsedMap.get(KEY_FE_PARAS) != null)
                builder.setFEParas(parsedMap.get(KEY_FE_PARAS));
            if (parsedMap != null && parsedMap.get(KEY_VIDEO_PID) != null)
                builder.setVideoPid(Integer.parseInt(parsedMap.get(KEY_VIDEO_PID)));
            if (parsedMap != null && parsedMap.get(KEY_PCR_ID) != null)
                builder.setPcrPid(Integer.parseInt(parsedMap.get(KEY_PCR_ID)));
            if (parsedMap != null && parsedMap.get(KEY_CONTENT_RATINGS) != null)
                builder.setContentRatings(DroidLogicTvUtils.TvString.fromString(parsedMap.get(KEY_CONTENT_RATINGS)));
            if (parsedMap != null && parsedMap.get(KEY_SIGNAL_TYPE) != null)
                builder.setSignalType(DroidLogicTvUtils.TvString.fromString(parsedMap.get(KEY_SIGNAL_TYPE)));
            if (parsedMap != null && parsedMap.get(KEY_AUDIO_TRACK_INDEX) != null)
                builder.setAudioTrackIndex(Integer.parseInt(parsedMap.get(KEY_AUDIO_TRACK_INDEX)));
            if (parsedMap != null && parsedMap.get(KEY_AUDIO_OUTPUT_MODE) != null)
                builder.setAudioOutPutMode(Integer.parseInt(parsedMap.get(KEY_AUDIO_OUTPUT_MODE)));
            if (parsedMap != null && parsedMap.get(KEY_AUDIO_COMPENSATION) != null)
                builder.setAudioCompensation(Integer.parseInt(parsedMap.get(KEY_AUDIO_COMPENSATION)));
            if (parsedMap != null && parsedMap.get(KEY_AUDIO_CHANNEL) != null)
                builder.setAudioChannel(Integer.parseInt(parsedMap.get(KEY_AUDIO_CHANNEL)));
            if (parsedMap != null && parsedMap.get(KEY_IS_FAVOURITE) != null)
                builder.setIsFavourite(parsedMap.get(KEY_IS_FAVOURITE).equals("1") ? true : false);
            if (parsedMap != null && parsedMap.get(KEY_FAVOURITE_INFO) != null)
                builder.setFavouriteInfo(parsedMap.get(KEY_FAVOURITE_INFO));
            if (parsedMap != null && parsedMap.get(KEY_TRANSPONDER_INFO) != null)
                builder.setTransponderInfo(parsedMap.get(KEY_TRANSPONDER_INFO));
            if (parsedMap != null && parsedMap.get(KEY_SATELLITE_INFO) != null)
                builder.setSatelliteInfo(parsedMap.get(KEY_SATELLITE_INFO));
            if (parsedMap != null && parsedMap.get(KEY_CHANNEL_SIGNAL_TYPE) != null)
                builder.setChannelSignalType(parsedMap.get(KEY_CHANNEL_SIGNAL_TYPE));
            if (parsedMap != null && parsedMap.get(KEY_VIDEO_STD) != null)
                builder.setVideoStd(Integer.parseInt(parsedMap.get(KEY_VIDEO_STD)));
            if (parsedMap != null && parsedMap.get(KEY_AUDIO_STD) != null)
                builder.setAudioStd(Integer.parseInt(parsedMap.get(KEY_AUDIO_STD)));
            if (parsedMap != null && parsedMap.get(KEY_IS_AUTO_STD) != null)
                builder.setIsAutoStd(Integer.parseInt(parsedMap.get(KEY_IS_AUTO_STD)));
            if (parsedMap != null && parsedMap.get(KEY_FINE_TUNE) != null)
                builder.setFineTune(Integer.parseInt(parsedMap.get(KEY_FINE_TUNE)));

            if (parsedMap != null && parsedMap.get(KEY_SUBT_PIDS) != null) {
                String[] str_subtPids = parsedMap.get(KEY_SUBT_PIDS).replace("[", "").replace("]", "").split(",");
                int subtNumber = (str_subtPids[0].compareTo("null") == 0)? 0 : str_subtPids.length;
                if (subtNumber > 0) {
                    String[] str_subtTypes = parsedMap.get(KEY_SUBT_TYPES).replace("[", "").replace("]", "").split(",");
                    String[] str_subtStypes = parsedMap.get(KEY_SUBT_STYPES).replace("[", "").replace("]", "").split(",");
                    String[] str_subtId1s = parsedMap.get(KEY_SUBT_ID1S).replace("[", "").replace("]", "").split(",");
                    String[] str_subtId2s = parsedMap.get(KEY_SUBT_ID2S).replace("[", "").replace("]", "").split(",");
                    int[] subtTypes = new int[subtNumber];
                    int[] subtStypes = new int[subtNumber];
                    int[] subtPids = new int[subtNumber];
                    int[] subtId1s = new int[subtNumber];
                    int[] subtId2s = new int[subtNumber];


                    for (int i=0; i < subtNumber; i++) {
                        subtTypes[i] = Integer.parseInt(str_subtTypes[i]);
                        subtStypes[i] = Integer.parseInt(str_subtStypes[i]);
                        subtPids[i] = Integer.parseInt(str_subtPids[i]);
                        subtId1s[i] = Integer.parseInt(str_subtId1s[i]);
                        subtId2s[i] = Integer.parseInt(str_subtId2s[i]);
                    }
                    String[] subtLangs = DroidLogicTvUtils.TvString.fromArrayString(parsedMap.get(KEY_SUBT_LANGS));

                    builder.setSubtitlePids(subtPids)
                            .setSubtitleId1s(subtId1s)
                            .setSubtitleId2s(subtId2s)
                            .setSubtitleTypes(subtTypes)
                            .setSubtitleStypes(subtStypes)
                            .setSubtitleLangs(subtLangs);
                }
            }

            if (parsedMap != null && parsedMap.get(KEY_SUBT_TRACK_INDEX) != null)
                builder.setSubtitleTrackIndex(Integer.parseInt(parsedMap.get(KEY_SUBT_TRACK_INDEX)));

            if (parsedMap != null && parsedMap.get(KEY_MULTI_NAME) != null)
                builder.setDisplayNameMulti(DroidLogicTvUtils.TvString.fromString(parsedMap.get(KEY_MULTI_NAME)));

            if (parsedMap != null && parsedMap.get(KEY_FREE_CA) != null)
                builder.setFreeCa(Integer.parseInt(parsedMap.get(KEY_FREE_CA)));
            if (parsedMap != null && parsedMap.get(KEY_SCRAMBLED) != null)
                builder.setScrambled(Integer.parseInt(parsedMap.get(KEY_SCRAMBLED)));
            if (parsedMap != null && parsedMap.get(KEY_SDT_VERSION) != null)
                builder.setSdtVersion(Integer.parseInt(parsedMap.get(KEY_SDT_VERSION)));

            if (parsedMap != null && parsedMap.get(KEY_MAJOR_NUM) != null)
                builder.setMajorChannelNumber(Integer.parseInt(parsedMap.get(KEY_MAJOR_NUM)));
            if (parsedMap != null && parsedMap.get(KEY_MINOR_NUM) != null)
                builder.setMinorChannelNumber(Integer.parseInt(parsedMap.get(KEY_MINOR_NUM)));
            if (parsedMap != null && parsedMap.get(KEY_SOURCE_ID) != null)
                builder.setSourceId(Integer.parseInt(parsedMap.get(KEY_SOURCE_ID)));
            if (parsedMap != null && parsedMap.get(KEY_ACCESS_CONTROL) != null)
                builder.setAccessControled(Integer.parseInt(parsedMap.get(KEY_ACCESS_CONTROL)));
            if (parsedMap != null && parsedMap.get(KEY_HIDDEN) != null)
                builder.setHidden("true".equals(parsedMap.get(KEY_HIDDEN)) ? 1 : 0);
            if (parsedMap != null && parsedMap.get(KEY_HIDE_GUIDE) != null)
                builder.setHideGuide(Integer.parseInt(parsedMap.get(KEY_HIDE_GUIDE)));
            if (parsedMap != null && parsedMap.get(KEY_VCT) != null)
                builder.setVct(parsedMap.get(KEY_VCT));
            if (parsedMap != null && parsedMap.get(KEY_EITV) != null) {
                String[] svs = parsedMap.get(KEY_EITV).replace("[", "").replace("]", "").split(",");
                int vn = (svs[0].compareTo("null") == 0)? 0 : svs.length;
                if (vn > 0) {
                    int[] vs = new int[vn];
                    for (int i = 0; i < vn; i++)
                        vs[i] = Integer.parseInt(svs[i]);
                    builder.setEitVersions(vs);
                }
            }
            if (parsedMap != null && parsedMap.get(KEY_PROGRAMS_IN_PAT) != null)
                builder.setProgramsInPat(Integer.parseInt(parsedMap.get(KEY_PROGRAMS_IN_PAT)));
            if (parsedMap != null && parsedMap.get(KEY_PAT_TS_ID) != null)
                builder.setPatTsId(Integer.parseInt(parsedMap.get(KEY_PAT_TS_ID)));
        }

        index = cursor.getColumnIndex(Channels.COLUMN_BROWSABLE);
        if (index >= 0)
            builder.setBrowsable(cursor.getInt(index)==1 ? true : false);

        index = cursor.getColumnIndex(Channels.COLUMN_LOCKED);
        if (index >= 0)
            builder.setLocked(cursor.getInt(index)==1 ? true : false);

        index = cursor.getColumnIndex(COLUMN_LCN);
        if (index >= 0)
            builder.setLCN(cursor.getInt(index));

        index = cursor.getColumnIndex(COLUMN_LCN1);
        if (index >= 0)
            builder.setLCN1(cursor.getInt(index));

        index = cursor.getColumnIndex(COLUMN_LCN2);
        if (index >= 0)
            builder.setLCN2(cursor.getInt(index));

        return builder.build();
    }

    public long getId() {
        return mId;
    }

    public String getInputId() {
        return mInputId;
    }

    public String getType() {
        return mType;
    }

    public String getServiceType() {
        return mServiceType;
    }

    public int getServiceId() {
        return mServiceId;
    }

    public String getDisplayNumber() {
        return mDisplayNumber;
    }

    public int getNumber() {
        return mNumber;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public String getLogoUrl() {
        return mLogoUrl;
    }

    public int getOriginalNetworkId() {
        return mOriginalNetworkId;
    }

    public int getTransportStreamId() {
        return mTransportStreamId;
    }

    public int getVfmt() {
        return mVfmt;
    }

    public int getVideoPid() {
        return mVideoPid;
    }

    public int getVideoStd() {
        return mVideoStd;
    }

    public String getVideoFormat() {
        return mVideoFormat;
    }

    public int getVideoWidth() {
        return mVideoWidth;
    }

    public int getVideoHeight() {
        return mVideoHeight;
    }

    public int[] getAudioPids() {
        return mAudioPids;
    }

    public int[] getAudioFormats() {
        return mAudioFormats;
    }

    public String[] getAudioLangs() {
        return mAudioLangs;
    }

    public int[] getAudioExts() {
        return mAudioExts;
    }

    public int getAudioStd() {
        return mAudioStd;
    }

    public int getIsAutoStd() {
        return mIsAutoStd;
    }

    public int getAudioTrackIndex() {
        return mAudioTrackIndex;
    }

    public int getAudioOutPutMode() {
        return mAudioOutPutMode;
    }

    public int getAudioCompensation() {
        return mAudioCompensation;
    }

    public int getAudioChannel() {
        return mAudioChannel;
    }

    public int getPcrPid() {
        return mPcrPid;
    }

    public int getFrequency() {
        return mFrequency;
    }

    public String getContentRatings() {
        return mContentRatings;
    }

    public String getSignalType() {
        return mSignalType;
    }

    public void setSignalType(String signalType) {
        mSignalType = signalType;
    }

    public void setContentRatings(String contentRatings) {
        mContentRatings = contentRatings;
    }

    public int getBandwidth() {
        return mBandwidth;
    }

    public int getSymbolRate() {
        return mSymbolRate;
    }

    public int getModulation() {
        return mModulation;
    }

    public String getFEParas() {
        return mFEParas;
    }

    public int getFineTune() {
        return mFineTune;
    }

    public int[] getSubtitleTypes() {
        return mSubtitleTypes;
    }

    public int[] getSubtitlePids() {
        return mSubtitlePids;
    }

    public int[] getSubtitleStypes() {
        return mSubtitleStypes;
    }

    public int[] getSubtitleId1s() {
        return mSubtitleId1s;
    }

    public int[] getSubtitleId2s() {
        return mSubtitleId2s;
    }

    public String[] getSubtitleLangs() {
        return mSubtitleLangs;
    }

    public int getSubtitleTrackIndex() {
        return mSubtitleTrackIndex;
    }

    public String getDisplayNameMulti() {
        return mDisplayNameMulti;
    }

    public String getDisplayNameLocal() {
        return TvMultilingualText.getText(getDisplayNameMulti());
    }

    public String getDisplayName(String local) {
        return TvMultilingualText.getText(getDisplayNameMulti(), local);
    }

    public int getFreeCa() {
        return mFreeCa;
    }

    public int getScrambled() {
        return mScrambled;
    }

    public int getSdtVersion() {
        return mSdtVersion;
    }

    public int getLCN() {
        return mLCN;
    }

    public int getLCN1() {
        return mLCN1;
    }

    public int getLCN2() {
        return mLCN2;
    }

    public int getMajorChannelNumber() {
        return mMajorChannelNumber;
    }

    public int getMinorChannelNumber() {
        return mMinorChannelNumber;
    }

    public int getSourceId() {
        return mSourceId;
    }

    public int getAccessControled() {
        return mAccessControlled;
    }

    public int getHidden() {
        return mHidden;
    }

    public int getHideGuide() {
        return mHideGuide;
    }

    public String getVct() {
        return mVct;
    }

    public int[] getEitVersions() {
        return mEitVersions;
    }

    public int getProgramsInPat() {
        return mProgramsInPat;
    }

    public int getPatTsId() {
        return mPatTsId;
    }

    public boolean isBrowsable() {
        return this.mBrowsable;
    }

    public boolean isFavourite() {
        return mIsFavourite;
    }

    public boolean hasSetFavourite() {
        boolean result = false;
        if (mSatelliteInfo == null) {
            return result;
        }
        try {
            JSONObject obj = new JSONObject(mFavInfo);
            if (obj != null && obj.length() > 0) {
                result = true;
            }
        } catch (Exception e) {
            Log.i(TAG, "hasSetFavourite Exception = " + e.getMessage());
        }
        return result;
    }

    public String getFavouriteInfo() {
        return mFavInfo;
    }

    public String getSatelliteInfo() {
        return mSatelliteInfo;
    }

    public String getSatelliteName() {
        String result = null;
        if (mSatelliteInfo == null) {
            return result;
        }
        try {
            JSONObject obj = new JSONObject(mSatelliteInfo);
            if (obj != null && obj.length() > 0) {
                result = obj.getString(KEY_SATELLITE_INFO_NAME);
            }
        } catch (Exception e) {
            Log.i(TAG, "getSatelliteName Exception = " + e.getMessage());
        }
        return result;
    }

    public String getTransponderInfo() {
        return mTransponderInfo;
    }

    public String getTransponderDisplay() {
        String result = null;
        if (mTransponderInfo == null) {
            return result;
        }
        try {
            JSONObject obj = new JSONObject(mTransponderInfo);
            if (obj != null && obj.length() > 0) {
                result = obj.getString(KEY_TRANSPONDER_INFO_TRANSPONDER_FREQUENCY);
                result += "/" + obj.getString(KEY_TRANSPONDER_INFO_TRANSPONDER_POLARITY);
                result += "/" + obj.getString(KEY_TRANSPONDER_INFO_TRANSPONDER_SYMBOL);
            }
        } catch (Exception e) {
            Log.i(TAG, "getTransponderDisplay Exception = " + e.getMessage());
        }
        return result;
    }

    public String getChannelSignalType() {
        return mChannelSignalType;
    }

    public boolean isPassthrough() {
        return mIsPassthrough;
    }

    public boolean isLocked() {
        return mLocked;
    }

    public Uri getUri() {
        if (isPassthrough()) {
            return TvContract.buildChannelUriForPassthroughInput(this.mInputId);
        }
        return TvContract.buildChannelUri(this.mId);
    }

    public void setType(String type) {
        mType = type;
    }

    public void setId(long value) {
        mId = value;
    }

    public void setDisplayNumber(String number) {
        mDisplayNumber = number;
        mNumber = stringToInteger(number);
    }

    public void setDisplayName(String name) {
        mDisplayName = name;
    }

    public void setDisplayNameLocal(String name) {
        if (mDisplayNameMulti != null) {
            mDisplayNameMulti = mDisplayNameMulti.replace(getDisplayNameLocal(), name);
        }
    }

    public void setDisplayNameMulti(String name) {
        mDisplayNameMulti = name;
    }

    public void setVideoStd(int std) {
        mVideoStd = std;
    }

    public void setVideoPid(int pid) {
        mVideoPid = pid;
    }

    public void setVfmt(int vfmt) {
        mVfmt = vfmt;
    }

    public void setVideoFormat(String format) {
        mVideoFormat = format;
    }

    public void setAudioPids(int[] pids) {
        mAudioPids = pids;
    }

    public void setAudioFormats(int[] formats) {
        mAudioFormats = formats;
    }

    public void setAudioLangs(String[] langs) {
        mAudioLangs = langs;
    }

    public void setAudioExts(int[] exts) {
        mAudioExts = exts;
    }

    public void setAudioStd(int std) {
        mAudioStd = std;
    }

    public void setAudioTrackIndex(int index) {
        mAudioTrackIndex = index;
    }

    public void setAudioOutPutMode(int mode) {
        mAudioOutPutMode = mode;
    }

    public void setAudioCompensation(int value) {
        mAudioCompensation = value;
    }

    public void setAudioChannel(int channel) {
        mAudioChannel = channel;
    }

    public void setPcrPid(int pid) {
        mPcrPid = pid;
    }

    public void setBrowsable(boolean enable) {
        mBrowsable = enable;
    }

    public void setLocked(boolean enable) {
        mLocked = enable;
    }

    public void setFavourite(boolean enable) {
        mIsFavourite = enable;
    }

    public void setFavouriteInfo(String value) {
        mFavInfo = value;
    }

    public void setSatelliteInfo(String value) {
        mSatelliteInfo = value;
    }

    public void setTransponderInfo(String value) {
        mTransponderInfo = value;
    }

    public void setChannelSignalType(String value) {
        mChannelSignalType = value;
    }

    public void setSubtitleTypes(int[] types) {
        mSubtitleTypes = types;
    }

    public void setSubtitlePids(int[] pids) {
        mSubtitlePids = pids;
    }

    public void setSubtitleStypes(int[] types) {
        mSubtitleStypes = types;
    }

    public void setSubtitleId1s(int[] id1s) {
        mSubtitleId1s = id1s;
    }

    public void setSubtitleId2s(int[] id2s) {
        mSubtitleId2s = id2s;
    }

    public void setSubtitleLangs(String[] langs) {
        mSubtitleLangs = langs;
    }

    public void setSubtitleTrackIndex(int index) {
        mSubtitleTrackIndex = index;
    }

    public void setFinetune(int finetune) {
        mFineTune = finetune;
    }

    public void setFreeCa(int free_ca) {
        mFreeCa = free_ca;
    }

    public void setScrambled(int scrambled) {
        mScrambled = scrambled;
    }

    public void setSdtVersion(int version) {
        mSdtVersion = version;
    }

    public void setOriginalNetworkId(int id) {
        mOriginalNetworkId = id;
    }

    public void setLCN(int lcn) {
        mLCN = lcn;
    }

    public void setLCN1(int lcn) {
        mLCN1 = lcn;
    }

    public void setLCN2(int lcn) {
        mLCN2 = lcn;
    }

    public void setEitVersions(int[] versions) {
        mEitVersions = versions;
    }

    public void setProgramsInPat(int n) {
        mProgramsInPat = n;
    }

    public void setPatTsId(int tsid) {
        mPatTsId = tsid;
    }

    public void copyFrom(ChannelInfo channel) {
        if (this == channel)
            return;
        this.mId = channel.mId;
        this.mInputId = channel.mInputId;
        this.mType = channel.mType;
        this.mServiceType = channel.mServiceType;
        this.mServiceId = channel.mServiceId;
        this.mDisplayNumber = channel.mDisplayNumber;
        this.mDisplayName = channel.mDisplayName;
        this.mVideoFormat = channel.mVideoFormat;
        this.mBrowsable = channel.mBrowsable;
        this.mLocked = channel.mLocked;
        this.mIsPassthrough = channel.mIsPassthrough;
    }

    public static final class Builder {
        private final ChannelInfo mChannel = new ChannelInfo();

        public Builder() {
            mChannel.mId = -1L;
            mChannel.mInputId = "";
            mChannel.mType = "";
            mChannel.mServiceType= "";
            mChannel.mServiceId = -1;
            mChannel.mDisplayNumber = "-1";
            mChannel.mNumber = -1;
            mChannel.mDisplayName = "";
            mChannel.mVideoFormat = "";
            mChannel.mLogoUrl = "";
            mChannel.mOriginalNetworkId = -1;
            mChannel.mTransportStreamId = -1;

            mChannel.mVideoPid = -1;
            mChannel.mVideoStd = -1;
            mChannel.mVfmt = -1;
            mChannel.mVideoWidth = -1;
            mChannel.mVideoHeight = -1;

            mChannel.mAudioPids = null;
            mChannel.mAudioFormats = null;
            mChannel.mAudioLangs = null;
            mChannel.mAudioExts = null;
            mChannel.mAudioStd = -1;
            mChannel.mIsAutoStd = -1;
            mChannel.mAudioTrackIndex = -1;
            mChannel.mAudioOutPutMode = -1;
            mChannel.mAudioCompensation = -1;
            mChannel.mAudioChannel = 0;

            mChannel.mPcrPid = -1;
            mChannel.mFrequency = -1;
            mChannel.mContentRatings = "";
            mChannel.mSignalType = "";
            mChannel.mBandwidth = -1;
            mChannel.mSymbolRate = -1;
            mChannel.mModulation = -1;
            mChannel.mFineTune = 0;
            mChannel.mFEParas = "";

            mChannel.mBrowsable = false;
            mChannel.mIsFavourite = false;
            mChannel.mLocked = false;
            mChannel.mIsPassthrough = false;
            mChannel.mFavInfo = null;
            mChannel.mSatelliteInfo = null;
            mChannel.mTransponderInfo = null;
            mChannel.mChannelSignalType = null;

            mChannel.mSubtitlePids = null;
            mChannel.mSubtitleTypes = null;
            mChannel.mSubtitleStypes = null;
            mChannel.mSubtitleLangs = null;
            mChannel.mSubtitleId1s = null;
            mChannel.mSubtitleId2s = null;
            mChannel.mSubtitleTrackIndex = -1;

            mChannel.mFreeCa = 0;
            mChannel.mScrambled = 0;

            mChannel.mSdtVersion = 0xff;

            mChannel.mLCN = -1;
            mChannel.mLCN1 = -1;
            mChannel.mLCN2 = -1;

            mChannel.mMajorChannelNumber = 0;
            mChannel.mMinorChannelNumber = 0;
            mChannel.mSourceId = -1;
            mChannel.mAccessControlled = 0;
            mChannel.mHidden = 0;
            mChannel.mHideGuide = 0;
            mChannel.mVct = null;
            mChannel.mEitVersions = null;
            mChannel.mProgramsInPat = 0;
            mChannel.mPatTsId = -1;
        }

        public Builder setId(long id) {
            mChannel.mId = id;
            return this;
        }

        public Builder setInputId(String input_id) {
            mChannel.mInputId = input_id;
            return this;
        }

        public Builder setType(String type) {
            mChannel.mType = type;
            return this;
        }

        public Builder setServiceType(String type) {
            mChannel.mServiceType = type;
            return this;
        }

        public Builder setServiceId(int id) {
            mChannel.mServiceId = id;
            return this;
        }

        public Builder setDisplayNumber(String number) {
            mChannel.mDisplayNumber = number;
            mChannel.mNumber = stringToInteger(number);
            return this;
        }

        public Builder setDisplayName(String name) {
            mChannel.mDisplayName = name;
            return this;
        }

        public Builder setLogoUrl(String url) {
            mChannel.mLogoUrl = url;
            return this;
        }

        public Builder setOriginalNetworkId(int id) {
            mChannel.mOriginalNetworkId = id;
            return this;
        }

        public Builder setTransportStreamId(int id) {
            mChannel.mTransportStreamId = id;
            return this;
        }

        public Builder setVfmt(int value) {
            mChannel.mVfmt = value;
            return this;
        }

        public Builder setVideoPid(int id) {
            mChannel.mVideoPid = id;
            return this;
        }

        public Builder setVideoStd(int std) {
            mChannel.mVideoStd = std;
            return this;
        }

        public Builder setVideoFormat(String format) {
            mChannel.mVideoFormat = format;
            return this;
        }

        public Builder setVideoWidth(int w) {
            mChannel.mVideoWidth = w;
            return this;
        }

        public Builder setVideoHeight(int h) {
            mChannel.mVideoWidth = h;
            return this;
        }

        public Builder setAudioPids(int[] pids) {
            mChannel.mAudioPids = pids;
            return this;
        }

        public Builder setAudioFormats(int[] formats) {
            mChannel.mAudioFormats = formats;
            return this;
        }

        public Builder setAudioLangs(String[] langs) {
            mChannel.mAudioLangs = langs;
            return this;
        }

        public Builder setAudioExts(int[] exts) {
            mChannel.mAudioExts = exts;
            return this;
        }

        public Builder setAudioStd(int std) {
            mChannel.mAudioStd = std;
            return this;
        }

        public Builder setIsAutoStd(int isStd) {
            mChannel.mIsAutoStd = isStd;
            return this;
        }

        public Builder setAudioTrackIndex(int index) {
            mChannel.mAudioTrackIndex = index;
            return this;
        }

        public Builder setAudioOutPutMode(int mode) {
            mChannel.mAudioOutPutMode = mode;
            return this;
        }

        public Builder setAudioCompensation(int c) {
            mChannel.mAudioCompensation = c;
            return this;
        }

        public Builder setAudioChannel(int c) {
            mChannel.mAudioChannel = c;
            return this;
        }

        public Builder setPcrPid(int pid) {
            mChannel.mPcrPid = pid;
            return this;
        }

        public Builder setFrequency(int freq) {
            mChannel.mFrequency = freq;
            return this;
        }

        public Builder setContentRatings(String contentRatings) {
            mChannel.mContentRatings = contentRatings;
            return this;
        }

        public Builder setSignalType(String signalType) {
            mChannel.mSignalType = signalType;
            return this;
        }

        public Builder setBandwidth(int w) {
            mChannel.mBandwidth = w;
            return this;
        }

        public Builder setSymbolRate(int s) {
            mChannel.mSymbolRate = s;
            return this;
        }

        public Builder setModulation(int m) {
            mChannel.mModulation = m;
            return this;
        }

        public Builder setFineTune(int f) {
            mChannel.mFineTune = f;
            return this;
        }

        public Builder setFEParas(String p) {
            mChannel.mFEParas = p;
            return this;
        }

        public Builder setBrowsable(boolean flag) {
            mChannel.mBrowsable = flag;
            return this;
        }

        public Builder setIsFavourite(boolean fav) {
            mChannel.mIsFavourite = fav;
            return this;
        }

        public Builder setFavouriteInfo(String value) {
            mChannel.mFavInfo = value;
            return this;
        }

        public Builder setSatelliteInfo(String value) {
            mChannel.mSatelliteInfo = value;
            return this;
        }

        public Builder setTransponderInfo(String value) {
            mChannel.mTransponderInfo = value;
            return this;
        }

        public Builder setChannelSignalType(String value) {
            mChannel.mChannelSignalType = value;
            return this;
        }

        public Builder setPassthrough(boolean flag) {
            mChannel.mIsPassthrough = flag;
            return this;
        }

        public Builder setLocked(boolean flag) {
            mChannel.mLocked = flag;
            return this;
        }

        public Builder setSubtitleTypes(int[] types) {
            mChannel.mSubtitleTypes = types;
            return this;
        }

        public Builder setSubtitlePids(int[] pids) {
            mChannel.mSubtitlePids = pids;
            return this;
        }

        public Builder setSubtitleStypes(int[] types) {
            mChannel.mSubtitleStypes = types;
            return this;
        }

        public Builder setSubtitleId1s(int[] id1s) {
            mChannel.mSubtitleId1s = id1s;
            return this;
        }

        public Builder setSubtitleId2s(int[] id2s) {
            mChannel.mSubtitleId2s = id2s;
            return this;
        }

        public Builder setSubtitleLangs(String[] langs) {
            mChannel.mSubtitleLangs = langs;
            return this;
        }

        public Builder setSubtitleTrackIndex(int index) {
            mChannel.mSubtitleTrackIndex = index;
            return this;
        }

        public Builder setDisplayNameMulti(String name) {
            mChannel.mDisplayNameMulti = name;
            return this;
        }

        public Builder setFreeCa(int free_ca) {
            mChannel.mFreeCa = free_ca;
            return this;
        }

        public Builder setScrambled(int scrambled) {
            mChannel.mScrambled = scrambled;
            return this;
        }

        public Builder setSdtVersion(int version) {
            mChannel.mSdtVersion = version;
            return this;
        }

        public Builder setLCN(int lcn) {
            mChannel.mLCN = lcn;
            return this;
        }

        public Builder setLCN1(int lcn) {
            mChannel.mLCN1 = lcn;
            return this;
        }

        public Builder setLCN2(int lcn) {
            mChannel.mLCN2 = lcn;
            return this;
        }

        public Builder setMajorChannelNumber(int number) {
            mChannel.mMajorChannelNumber = number;
            return this;
        }

        public Builder setMinorChannelNumber(int number) {
            mChannel.mMinorChannelNumber = number;
            return this;
        }

        public Builder setSourceId(int id) {
            mChannel.mSourceId = id;
            return this;
        }

        public Builder setAccessControled(int access) {
            mChannel.mAccessControlled = access;
            return this;
        }

        public Builder setHidden(int hidden) {
            mChannel.mHidden = hidden;
            return this;
        }

        public Builder setHideGuide(int hide) {
            mChannel.mHideGuide = hide;
            return this;
        }

        public Builder setVct(String vct) {
            mChannel.mVct = vct;
            return this;
        }

        public Builder setEitVersions(int[] versions) {
            mChannel.mEitVersions = versions;
            return this;
        }

        public Builder setProgramsInPat(int n) {
            mChannel.mProgramsInPat = n;
            return this;
        }

        public Builder setPatTsId(int tsid) {
            mChannel.mPatTsId = tsid;
            return this;
        }

        public ChannelInfo build() {
            return mChannel;
        }
    }

    private static int stringToInteger(String string) {
        if (TextUtils.isEmpty(string)) {
            return -1;
        }

        String s = string.replaceAll("\\D", "");
        if (!TextUtils.isEmpty(s))
            return Integer.valueOf(s);
        else
            return -1;
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

    public static boolean isSameChannel (ChannelInfo a, ChannelInfo b) {
        if (a == null || b== null )
            return false;

        if (a.getInputId().equals(b.getInputId()) && a.getId() == b.getId()) {
            return true;
        }

        return false;
    }

    public static boolean isVideoChannel(ChannelInfo channel) {
        if (channel != null)
            return TextUtils.equals(channel.getServiceType(), Channels.SERVICE_TYPE_AUDIO_VIDEO);

        return false;
    }

    public static boolean isRadioChannel(ChannelInfo channel) {
        if (channel != null)
            return TextUtils.equals(channel.getServiceType(), Channels.SERVICE_TYPE_AUDIO);

        return false;
    }

    public boolean isAnalogChannel() {
        return (mType.equals(TvContract.Channels.TYPE_PAL)
            || mType.equals(TvContract.Channels.TYPE_NTSC)
            || mType.equals(TvContract.Channels.TYPE_SECAM));
    }

    public boolean isDigitalChannel() {
        return (mType.equals(TvContract.Channels.TYPE_DTMB)
            || mType.equals(TvContract.Channels.TYPE_DVB_T)
            || mType.equals(TvContract.Channels.TYPE_DVB_C)
            || mType.equals(TvContract.Channels.TYPE_DVB_S)
            || mType.equals(TvContract.Channels.TYPE_ATSC_T)
            || mType.equals(TvContract.Channels.TYPE_ATSC_C)
            || mType.equals(TvContract.Channels.TYPE_ISDB_T));
    }

    public boolean isAtscChannel() {
        return (mType.equals(TvContract.Channels.TYPE_ATSC_C)
            || mType.equals(TvContract.Channels.TYPE_ATSC_T)
            || mType.equals(TvContract.Channels.TYPE_ATSC_M_H));
    }

    public boolean isNtscChannel() {
        return mType.equals(TvContract.Channels.TYPE_NTSC);
    }

    public boolean isOtherChannel() {
        return mType.equals(TvContract.Channels.TYPE_OTHER);
    }

    public boolean isAVChannel() {
        return (!mServiceType.equals(TvContract.Channels.SERVICE_TYPE_OTHER));
    }

    public boolean isSameChannel(ChannelInfo a) {
        if (a == null)
            return false;

        return a.getServiceId() == mServiceId
                && a.getOriginalNetworkId() == mOriginalNetworkId
                && a.getTransportStreamId() == mTransportStreamId
                && a.getFrequency() == mFrequency
                && TextUtils.equals(a.getDisplayName(), mDisplayName);
    }

    public void print () {
        Log.d(TAG, toString());
    }
    public String toString() {
        return "Id = " + mId +
                "\n InputId = " + mInputId +
                "\n Type = " + mType +
                "\n ServiceType = " + mServiceType +
                "\n ServiceId = " + mServiceId +
                "\n DisplayNumber = " + mDisplayNumber+
                "\n DisplayName = " + mDisplayName +
                "\n LogoUrl = " + mLogoUrl +
                "\n OriginalNetworkId = " + mOriginalNetworkId +
                "\n TransportStreamId = " + mTransportStreamId +
                "\n VideoStd = " + mVideoStd +
                "\n VideoFormat = " + mVideoFormat +
                "\n VideoWidth = " + mVideoWidth +
                "\n VideoHeight = " + mVideoHeight +
                "\n VideoPid = " + mVideoPid +
                "\n Vfmt = " + mVfmt +
                "\n AudioPids = " + Arrays.toString(mAudioPids) +
                "\n AudioFormats = " + Arrays.toString(mAudioFormats) +
                "\n AudioLangs = " + Arrays.toString(mAudioLangs) +
                "\n AudioExts = " + Arrays.toString(mAudioExts) +
                "\n AudioStd = " + mAudioStd +
                "\n IsAutoStd = " + mIsAutoStd +
                "\n AudioTrackIndex = " + mAudioTrackIndex +
                "\n mAudioOutPutMode = " + mAudioOutPutMode +
                "\n AudioCompensation = " + mAudioCompensation +
                "\n AudioChannel = " + mAudioChannel +
                "\n PcrPid = " + mPcrPid +
                "\n mFrequency = " + mFrequency +
                "\n mFinetune = " + mFineTune +
                "\n mFEPara = " + mFEParas +
                "\n Browsable = " + mBrowsable +
                "\n IsFavourite = " + mIsFavourite +
                "\n mFavInfo = " + mFavInfo +
                "\n mSatelliteInfo = " + mSatelliteInfo +
                "\n mTransponderInfo = " + mTransponderInfo +
                "\n mChannelSignalType = " + mChannelSignalType +
                "\n IsPassthrough = " + mIsPassthrough +
                "\n mLocked = " + mLocked +
                "\n SubtitlePids = " + Arrays.toString(mSubtitlePids) +
                "\n SubtitleTypes = " + Arrays.toString(mSubtitleTypes) +
                "\n SubtitleStypes = " + Arrays.toString(mSubtitleStypes) +
                "\n SubtitleId1s = " + Arrays.toString(mSubtitleId1s) +
                "\n SubtitleId2s = " + Arrays.toString(mSubtitleId2s) +
                "\n SubtitleLangs = " + Arrays.toString(mSubtitleLangs) +
                "\n SubtitleTrackIndex = " + mSubtitleTrackIndex +
                "\n FreeCa = " + mFreeCa +
                "\n Scrambled = " + mScrambled +
                "\n SdtVersion = " + mSdtVersion +
                "\n LCN = " + mLCN +
                "\n LCN1 = " + mLCN1 +
                "\n LCN2 = " + mLCN2 +
                "\n mSourceId = " + mSourceId +
                "\n AccessControled = " + mAccessControlled +
                "\n Hidden = " + mHidden +
                "\n HideGuide = " + mHideGuide +
                "\n Ratings = " + mContentRatings +
                "\n SignalType = " + mSignalType +
                "\n vct = " + mVct +
                "\n EitVers = " + Arrays.toString(mEitVersions) +
                "\n ProgramsInPat = " + mProgramsInPat +
                "\n PatTsId = " + mPatTsId;
    }

    public static class Subtitle {
        public static final int TYPE_DVB_SUBTITLE = 1;
        public static final int TYPE_DTV_TELETEXT = 2;
        public static final int TYPE_ATV_TELETEXT = 3;
        public static final int TYPE_DTV_CC = 4;
        public static final int TYPE_ATV_CC = 5;
        public static final int TYPE_DTV_TELETEXT_IMG = 6;
        public static final int TYPE_ISDB_SUB = 7;
        public static final int TYPE_SCTE27_SUB = 8;

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
        /*xds vchip services*/
        public static final int CC_CAPTION_VCHIP_ONLY = 15;

        public int mType;
        public int mPid;
        public int mStype;
        public int mId1;
        public int mId2;
        public String mLang;

        public int id;

        public Subtitle() {
            id = -1;
            mType = -1;
            mPid = -1;
            mStype = -1;
            mId1 = -1;
            mId2 = -1;
            mLang = "";
       }

        public Subtitle(int type, int pid, int stype, int id1, int id2, String language) {
            mType = type;
            mPid = pid;
            mStype = stype;
            mId1 = id1;
            mId2 = id2;
            mLang = language;
        }

        public Subtitle(int type, int pid, int stype, int id1, int id2, String language, int id) {
            this(type, pid, stype, id1, id2, language);
            setId(id);
        }

        public Subtitle(Subtitle subtitle) {
            mType = subtitle.mType;
            mPid = subtitle.mPid;
            mStype = subtitle.mStype;
            mId1 = subtitle.mId1;
            mId2 = subtitle.mId2;
            mLang = subtitle.mLang;
        }

        public void setId(int id) {
            this.id = id;
        }

        public static final class Builder {
            private final Subtitle mSubtitle = new Subtitle();

            public Builder() {
            }

            public Builder(Subtitle subtitle) {
                mSubtitle.id = subtitle.id;
                mSubtitle.mType = subtitle.mType;
                mSubtitle.mPid = subtitle.mPid;
                mSubtitle.mStype = subtitle.mStype;
                mSubtitle.mId1 = subtitle.mId1;
                mSubtitle.mId2 = subtitle.mId2;
                mSubtitle.mLang = new String(subtitle.mLang);
            }

            public Subtitle build() {
                return mSubtitle;
            }

            public Builder setId(int id) {
                mSubtitle.id = id;
                return this;
            }

            public Builder setType(int type) {
                mSubtitle.mType = type;
                return this;
            }

            public Builder setPid(int pid) {
                mSubtitle.mPid = pid;
                return this;
            }

            public Builder setStype(int stype) {
                mSubtitle.mStype = stype;
                return this;
            }

            public Builder setId1(int id1) {
                mSubtitle.mId1 = id1;
                return this;
            }

            public Builder setId2(int id2) {
                mSubtitle.mId2 = id2;
                return this;
            }

            public Builder setLang(String lang) {
                mSubtitle.mLang = lang;
                return this;
            }
        }
    }

    public static class Audio {

        public int mPid;
        public int mFormat;
        public int mExt;
        public String mLang;

        public int id;

        public Audio() {
            id = -1;
            mPid = -1;
            mFormat = -1;
            mExt = -1;
            mLang = "";
       }

        public Audio(int pid, int format, int ext, String language) {
            mPid = pid;
            mFormat = format;
            mExt = ext;
            mLang = language;
        }

        public Audio(int pid, int format, int ext, String language, int id) {
            this(pid, format, ext, language);
            setId(id);
        }

        public Audio(Audio audio) {
            mPid = audio.mPid;
            mFormat = audio.mFormat;
            mExt = audio.mExt;
            mLang = audio.mLang;
        }

        public void setId(int id) {
            this.id = id;
        }

        public static final class Builder {
            private final Audio mAudio = new Audio();

            public Builder() {
            }

            public Builder(Audio audio) {
                mAudio.id = audio.id;
                mAudio.mPid = audio.mPid;
                mAudio.mFormat = audio.mFormat;
                mAudio.mExt = audio.mExt;
                mAudio.mLang = new String(audio.mLang);
            }

            public Audio build() {
                return mAudio;
            }

            public Builder setId(int id) {
                mAudio.id = id;
                return this;
            }

            public Builder setPid(int pid) {
                mAudio.mPid = pid;
                return this;
            }

            public Builder setFormat(int format) {
                mAudio.mFormat = format;
                return this;
            }

            public Builder setExt(int ext) {
                mAudio.mExt = ext;
                return this;
            }

            public Builder setLang(String lang) {
                mAudio.mLang = lang;
                return this;
            }
        }
    }

}
