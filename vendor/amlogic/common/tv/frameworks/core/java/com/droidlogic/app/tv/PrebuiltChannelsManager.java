/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.content.Intent;
import android.media.tv.TvContract;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;

import java.io.*;
import java.util.*;

public class PrebuiltChannelsManager {
    private static final String TAG = "PrebuiltChannelsManager";
    private static final boolean DEBUG = true;

    protected TvDataBaseManager mTvDataBaseManager;
    private static final String FILE_PATH_TV_PRESET_CHANNEL_TABLE = Environment.getExternalStorageDirectory()
            + "/tv_preset_channel_table.cfg";
    public static final int RETURN_VALUE_SUCCESS                        = 0;
    public static final int RETURN_VALUE_FAILED                         = -1;

    public static final String CHECKOUT_HEAD_STRING                     = "+++++";
    public static final String CHECKOUT_TAIL_STRING                     = "-----";

    public static final int DISPATCH_CMD_BACKUP_CHANNEL_INFO_TO_FILE    = 0x0;
    public static final int DISPATCH_CMD_CFG_CHANNEL_INFO_TO_DB         = 0x1;

    private final String TV_INPUT_ID = "com.droidlogic.tvinput/.services.ADTVInputService/HW16";

    private Context mContext;
    public FileWriter mFileWritter;
    public BufferedWriter mBufferWriter;

    public FileReader mFileReader;
    public BufferedReader mBufferedReader;

    private Map<String, String> mMapChannelInfoKeyToValue = new LinkedHashMap<String, String>();

    private final String[] mChannelInfoKey = {
            TvContract.Channels._ID,
            TvContract.Channels.COLUMN_INPUT_ID,
            TvContract.Channels.COLUMN_TYPE,
            TvContract.Channels.COLUMN_SERVICE_TYPE,
            TvContract.Channels.COLUMN_SERVICE_ID,
            TvContract.Channels.COLUMN_DISPLAY_NUMBER,
            TvContract.Channels.COLUMN_DISPLAY_NAME,
            TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID,
            TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID,
            TvContract.Channels.COLUMN_VIDEO_FORMAT,
            ChannelInfo.KEY_AUDIO_PIDS,
            ChannelInfo.KEY_AUDIO_FORMATS,
            ChannelInfo.KEY_AUDIO_EXTS,
            ChannelInfo.KEY_AUDIO_LANGS,
            ChannelInfo.KEY_VFMT,
            ChannelInfo.KEY_FREQUENCY,
            ChannelInfo.KEY_BAND_WIDTH,
            ChannelInfo.KEY_SYMBOL_RATE,
            ChannelInfo.KEY_MODULATION,
            ChannelInfo.KEY_FE_PARAS,
            ChannelInfo.KEY_VIDEO_PID,
            ChannelInfo.KEY_PCR_ID,
            ChannelInfo.KEY_CONTENT_RATINGS,
            ChannelInfo.KEY_SIGNAL_TYPE,
            ChannelInfo.KEY_AUDIO_TRACK_INDEX,
            ChannelInfo.KEY_AUDIO_OUTPUT_MODE,
            ChannelInfo.KEY_AUDIO_COMPENSATION,
            ChannelInfo.KEY_AUDIO_CHANNEL,
            ChannelInfo.KEY_IS_FAVOURITE,
            ChannelInfo.KEY_VIDEO_STD,
            ChannelInfo.KEY_AUDIO_STD,
            ChannelInfo.KEY_IS_AUTO_STD,
            ChannelInfo.KEY_FINE_TUNE,
            ChannelInfo.KEY_SUBT_PIDS,
            ChannelInfo.KEY_SUBT_TYPES,
            ChannelInfo.KEY_SUBT_STYPES,
            ChannelInfo.KEY_SUBT_ID1S,
            ChannelInfo.KEY_SUBT_ID2S,
            ChannelInfo.KEY_SUBT_LANGS,
            ChannelInfo.KEY_SUBT_TRACK_INDEX,
            ChannelInfo.KEY_MULTI_NAME,
            ChannelInfo.KEY_FREE_CA,
            ChannelInfo.KEY_SCRAMBLED,
            ChannelInfo.KEY_SDT_VERSION,
            ChannelInfo.KEY_MAJOR_NUM,
            ChannelInfo.KEY_MINOR_NUM,
            ChannelInfo.KEY_SOURCE_ID,
            ChannelInfo.KEY_ACCESS_CONTROL,
            ChannelInfo.KEY_HIDDEN,
            ChannelInfo.KEY_HIDE_GUIDE,
            ChannelInfo.KEY_VCT,
            ChannelInfo.KEY_EITV,
            ChannelInfo.KEY_PROGRAMS_IN_PAT,
            ChannelInfo.KEY_PAT_TS_ID,
            TvContract.Channels.COLUMN_BROWSABLE,
            TvContract.Channels.COLUMN_LOCKED,
            ChannelInfo.COLUMN_LCN,
            ChannelInfo.COLUMN_LCN1,
            ChannelInfo.COLUMN_LCN2,
    };

    public PrebuiltChannelsManager(Context context) {
        mContext = context;
    }

    private int writeChannelInfoToFile(ChannelInfo channel, int number) {
        mMapChannelInfoKeyToValue.put(TvContract.Channels._ID, String.valueOf(channel.getId()));
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_INPUT_ID, channel.getInputId());
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_TYPE, channel.getType());
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_SERVICE_TYPE, channel.getServiceType());
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_SERVICE_ID, String.valueOf(channel.getServiceId()));
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_DISPLAY_NUMBER, channel.getDisplayNumber());
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_DISPLAY_NAME, channel.getDisplayName());
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID, channel.getOriginalNetworkId() + "");
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID, channel.getTransportStreamId() + "");
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_VIDEO_FORMAT, channel.getVfmt() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_PIDS, intArrayToString(channel.getAudioPids()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_FORMATS, intArrayToString(channel.getAudioFormats()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_EXTS, intArrayToString(channel.getAudioExts()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_LANGS, stringArrayToString(channel.getAudioLangs()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_VFMT, channel.getVideoFormat());
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_FREQUENCY, channel.getFrequency() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_BAND_WIDTH, channel.getBandwidth() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SYMBOL_RATE, channel.getSymbolRate() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_MODULATION, channel.getModulation() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_FE_PARAS, channel.getFEParas());
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_VIDEO_PID, channel.getVideoPid() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_PCR_ID, channel.getPcrPid() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_CONTENT_RATINGS, channel.getContentRatings());
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SIGNAL_TYPE, channel.getSignalType());
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_TRACK_INDEX, channel.getAudioTrackIndex() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_OUTPUT_MODE, channel.getAudioOutPutMode() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_COMPENSATION, channel.getAudioCompensation() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_CHANNEL, channel.getAudioChannel() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_IS_FAVOURITE, channel.isFavourite() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_VIDEO_STD, channel.getVideoStd() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_AUDIO_STD, channel.getAudioStd() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_IS_AUTO_STD, channel.getIsAutoStd() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_FINE_TUNE, channel.getFineTune() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_PIDS, intArrayToString(channel.getSubtitlePids()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_TYPES, intArrayToString(channel.getSubtitleTypes()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_STYPES, intArrayToString(channel.getSubtitleStypes()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_ID1S, intArrayToString(channel.getSubtitleId1s()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_ID2S, intArrayToString(channel.getSubtitleId2s()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_LANGS, stringArrayToString(channel.getSubtitleLangs()) + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SUBT_TRACK_INDEX, channel.getSubtitleTrackIndex() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_MULTI_NAME, channel.getDisplayNameMulti());
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_FREE_CA, channel.getFreeCa() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SCRAMBLED, channel.getScrambled() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SDT_VERSION, channel.getSdtVersion() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_MAJOR_NUM, channel.getMajorChannelNumber() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_MINOR_NUM, channel.getMinorChannelNumber() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_SOURCE_ID, channel.getSourceId() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_ACCESS_CONTROL, channel.getAccessControled() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_HIDDEN, channel.getHidden() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_HIDE_GUIDE, channel.getHideGuide() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_VCT, channel.getVct());
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_EITV, intArrayToString(channel.getEitVersions()));
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_PROGRAMS_IN_PAT, channel.getProgramsInPat() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.KEY_PAT_TS_ID, channel.getPatTsId() + "");
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_BROWSABLE, channel.isBrowsable() + "");
        mMapChannelInfoKeyToValue.put(TvContract.Channels.COLUMN_LOCKED, channel.isLocked() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.COLUMN_LCN, channel.getLCN() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.COLUMN_LCN1, channel.getLCN1() + "");
        mMapChannelInfoKeyToValue.put(ChannelInfo.COLUMN_LCN2, channel.getLCN2() + "");

        try {
            mBufferWriter.write(CHECKOUT_HEAD_STRING + "+++++++++++++++++[channel_number:" + number + "]++++++++++++++++++++++\n");
            Iterator iter = mMapChannelInfoKeyToValue.entrySet().iterator();
            while (iter.hasNext()) {
                Map.Entry entry = (Map.Entry) iter.next();
                // write Channel Info to File
                mBufferWriter.write(entry.getKey() + ": " + entry.getValue() + "\n");
            }
            mBufferWriter.write(CHECKOUT_TAIL_STRING + "----------------------------------------------------------\n\n");
        } catch (Exception e) {
            Log.e(TAG,"write Channel Info failed!");
            e.printStackTrace();
            return RETURN_VALUE_FAILED;
        }
        return RETURN_VALUE_SUCCESS;
    }

    private int buildChannelInfo(String oneLineString, ChannelInfo.Builder builder) {
        int index = 0;
        for (index=0; index< mChannelInfoKey.length; index++) {
            if (oneLineString.startsWith(mChannelInfoKey[index])) {
                break;
            }
        }
        oneLineString = oneLineString.replaceAll(mChannelInfoKey[index] + ": ", "");
        switch (mChannelInfoKey[index]) {
            case TvContract.Channels._ID:
                builder.setId(stringToint(oneLineString));
                break;
            case TvContract.Channels.COLUMN_INPUT_ID:
                builder.setInputId(oneLineString);
                break;
            case TvContract.Channels.COLUMN_TYPE:
                builder.setType(oneLineString);
                break;
            case TvContract.Channels.COLUMN_SERVICE_TYPE:
                builder.setServiceType(oneLineString);
                break;
            case TvContract.Channels.COLUMN_SERVICE_ID:
                builder.setServiceId(stringToint(oneLineString));
                break;
            case TvContract.Channels.COLUMN_DISPLAY_NUMBER:
                builder.setDisplayNumber(oneLineString);
                break;
            case TvContract.Channels.COLUMN_DISPLAY_NAME:
                builder.setDisplayName(oneLineString);
                break;
            case TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID:
                builder.setOriginalNetworkId(stringToint(oneLineString));
                break;
            case TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID:
                builder.setTransportStreamId(stringToint(oneLineString));
                break;
            case TvContract.Channels.COLUMN_VIDEO_FORMAT:
                builder.setVfmt(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_PIDS:
                builder.setAudioPids(stringToIntArray(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_FORMATS:
                builder.setAudioFormats(stringToIntArray(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_EXTS:
                builder.setAudioExts(stringToIntArray(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_LANGS:
                builder.setAudioLangs(oneLineString.split(","));
                break;
            case ChannelInfo.KEY_VFMT:
                builder.setVideoFormat(oneLineString);
                break;
            case ChannelInfo.KEY_FREQUENCY:
                builder.setFrequency(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_BAND_WIDTH:
                builder.setBandwidth(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_SYMBOL_RATE:
                builder.setSymbolRate(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_MODULATION:
                builder.setModulation(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_FE_PARAS:
                builder.setFEParas(oneLineString);
                break;
            case ChannelInfo.KEY_VIDEO_PID:
                builder.setVideoPid(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_PCR_ID:
                builder.setPcrPid(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_CONTENT_RATINGS:
                builder.setContentRatings(oneLineString);
                break;
            case ChannelInfo.KEY_SIGNAL_TYPE:
                builder.setSignalType(oneLineString);
                break;
            case ChannelInfo.KEY_AUDIO_TRACK_INDEX:
                builder.setAudioTrackIndex(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_OUTPUT_MODE:
                builder.setAudioOutPutMode(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_COMPENSATION:
                builder.setAudioCompensation(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_CHANNEL:
                builder.setAudioChannel(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_IS_FAVOURITE:
                builder.setIsFavourite(oneLineString.equals("true"));
                break;
            case ChannelInfo.KEY_VIDEO_STD:
                builder.setVideoStd(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_AUDIO_STD:
                builder.setAudioStd(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_IS_AUTO_STD:
                builder.setIsAutoStd(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_FINE_TUNE:
                builder.setFineTune(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_SUBT_PIDS:
                builder.setSubtitlePids(stringToIntArray((oneLineString)));
                break;
            case ChannelInfo.KEY_SUBT_TYPES:
                builder.setSubtitleTypes(stringToIntArray((oneLineString)));
                break;
            case ChannelInfo.KEY_SUBT_STYPES:
                builder.setSubtitleStypes(stringToIntArray((oneLineString)));
                break;
            case ChannelInfo.KEY_SUBT_ID1S:
                builder.setSubtitleId1s(stringToIntArray((oneLineString)));
                break;
            case ChannelInfo.KEY_SUBT_ID2S:
                builder.setSubtitleId2s(stringToIntArray((oneLineString)));
                break;
            case ChannelInfo.KEY_SUBT_LANGS:
                builder.setSubtitleLangs(oneLineString.split(","));
                                break;
            case ChannelInfo.KEY_SUBT_TRACK_INDEX:
                builder.setSubtitleTrackIndex(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_MULTI_NAME:
                builder.setDisplayNameMulti(oneLineString);
                break;
            case ChannelInfo.KEY_FREE_CA:
                builder.setFreeCa(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_SCRAMBLED:
                builder.setScrambled(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_SDT_VERSION:
                builder.setSdtVersion(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_MAJOR_NUM:
                builder.setMajorChannelNumber(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_MINOR_NUM:
                builder.setMinorChannelNumber(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_SOURCE_ID:
                builder.setSourceId(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_ACCESS_CONTROL:
                builder.setAccessControled(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_HIDDEN:
                builder.setHidden(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_HIDE_GUIDE:
                builder.setHideGuide(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_VCT:
                builder.setVct(oneLineString);
                break;
            case ChannelInfo.KEY_EITV:
                builder.setEitVersions(stringToIntArray((oneLineString)));
                break;
            case ChannelInfo.KEY_PROGRAMS_IN_PAT:
                builder.setProgramsInPat(stringToint(oneLineString));
                break;
            case ChannelInfo.KEY_PAT_TS_ID:
                builder.setPatTsId(stringToint(oneLineString));
                break;
            case TvContract.Channels.COLUMN_BROWSABLE:
                builder.setBrowsable(oneLineString.equals("true"));
                break;
            case TvContract.Channels.COLUMN_LOCKED:
                builder.setLocked(oneLineString.equals("true"));
                break;
            case ChannelInfo.COLUMN_LCN:
                builder.setLCN(stringToint(oneLineString));
                break;
            case ChannelInfo.COLUMN_LCN1:
                builder.setLCN1(stringToint(oneLineString));
                break;
            case ChannelInfo.COLUMN_LCN2:
                builder.setLCN2(stringToint(oneLineString));
                break;
            default:
                break;
        }
        return 0;
    }

    private int stringToint(String stringValue) {
        if (null == stringValue) {
            return 0;
        }
        int tempValue = 0;
        try {
            tempValue = Integer.parseInt(stringValue);
        } catch (NumberFormatException e) {
            Log.e(TAG,"string covert to int failed, stringValue:" + stringValue);
            e.printStackTrace();
        }
        return tempValue;
    }

    private String intArrayToString(int[] array) {
        if (null == array || 0 == array.length) {
            return "null";
        }
        return Arrays.toString(array);
    }

    private String stringArrayToString(String[] stringArray) {
        if (null == stringArray) {
            return "";
        }
        String tempString = "";
        for (int i = 0; i < stringArray.length; i++) {
            // first element not need "," character
            if (0 != i) {
                tempString += ",";
            }
            tempString += stringArray[i];
        }
        return tempString;
    }

    // only support, eg: string:"9232,45,78,174" to IntArray:{9232,45,78,174}
    private int[] stringToIntArray(String stringValue) {
        if (0 == stringValue.length()) {
            return null;
        }
        String[] stringArray = stringValue.replace("[", "").replace("]", "").replace(" ", "").split(",");
        if (stringArray[0].equals("null")) {
            return null;
        }
        int[] intArray = new int[stringArray.length];

        for (int i=0; i<stringArray.length; i++) {
            intArray[i] = Integer.parseInt(stringArray[i]);
        }
        return intArray;
    }

    public int configFileReadToDbChannelInfo() {
        Log.i(TAG,"configFileReadToDbChannelInfo enter");
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        ArrayList<ChannelInfo> channelListAll = mTvDataBaseManager.getChannelList(TV_INPUT_ID, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO);

        int ret;
        ret = fileReadInit();
        if (RETURN_VALUE_SUCCESS != ret || 0 != channelListAll.size()) {
            Log.i(TAG, "not need load channel info file, db channel count:" + channelListAll.size());
            return RETURN_VALUE_FAILED;
        }
        ArrayList<ChannelInfo> configFileChannelList = readChannelInfoFromFlile();
        ret = saveChannelInfoListToDb(configFileChannelList);
        fileReadDeInit();
        return ret;
    }

    public int dbChannelInfoWriteToConfigFile() {
        Log.i(TAG,"dbChannelInfoWriteToConfigFile enter");
        mTvDataBaseManager = new TvDataBaseManager(mContext);
        ArrayList<ChannelInfo> channelListAll = mTvDataBaseManager.getChannelList(TV_INPUT_ID, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO);
        if (0 == channelListAll.size()) {
            Log.i(TAG,"Db channel info count is 0!");
            return RETURN_VALUE_SUCCESS;
        }
        int ret = RETURN_VALUE_SUCCESS;
        ret = fileWriteInit();
        if (ret != RETURN_VALUE_SUCCESS) {
            Log.w(TAG, "file write init faild");
            return ret;
        }
        for (int i = 0; i <= channelListAll.size() - 1; i++) {
            writeChannelInfoToFile(channelListAll.get(i), i);
        }
        ret = fileWriteDeInit();
        return ret;
    }

    private int fileReadInit(){
        try {
            File tvChannelInfoFile = new File(FILE_PATH_TV_PRESET_CHANNEL_TABLE);
            if (!tvChannelInfoFile.exists()) {
                Log.i(TAG, "file:" + FILE_PATH_TV_PRESET_CHANNEL_TABLE + " not exist, cannot load channel info");
                return RETURN_VALUE_FAILED;
            }
            mFileReader = new FileReader(tvChannelInfoFile);
            mBufferedReader = new BufferedReader(mFileReader);
        } catch (Exception e) {
            Log.e(TAG,"file read operation failed!");
            e.printStackTrace();
            return RETURN_VALUE_FAILED;
        }
        return RETURN_VALUE_SUCCESS;
    }

    private int fileWriteInit(){
        try {
            File tvChannelInfoFile = new File(FILE_PATH_TV_PRESET_CHANNEL_TABLE);
            if (tvChannelInfoFile.exists()) {
                Log.i(TAG, "file already exist, delete " + FILE_PATH_TV_PRESET_CHANNEL_TABLE + " file!!!");
                tvChannelInfoFile.delete();
            }
            Log.i(TAG, "create " + FILE_PATH_TV_PRESET_CHANNEL_TABLE + " file!!!");
            tvChannelInfoFile.createNewFile();
            mFileWritter = new FileWriter(tvChannelInfoFile);
            mBufferWriter = new BufferedWriter(mFileWritter);
        } catch (Exception e) {
            Log.e(TAG,"file write operation failed!");
            e.printStackTrace();
            return RETURN_VALUE_FAILED;
        }
        return RETURN_VALUE_SUCCESS;
    }

    private int fileWriteDeInit() {
        try {
            mFileWritter.flush();
            mBufferWriter.close();
            mFileWritter.close();
        } catch (IOException e) {
            Log.e(TAG,"file close failed!");
            e.printStackTrace();
            return RETURN_VALUE_FAILED;
        }
        return RETURN_VALUE_SUCCESS;
    }

    private int fileReadDeInit() {
        try {
            mFileReader.close();
            mBufferedReader.close();
        } catch (IOException e) {
            Log.e(TAG,"file close failed!");
            e.printStackTrace();
            return RETURN_VALUE_FAILED;
        }
        return RETURN_VALUE_SUCCESS;
    }

    private ArrayList<ChannelInfo> readChannelInfoFromFlile() {
        ArrayList<ChannelInfo> channelInfoList = new ArrayList<ChannelInfo>();
        try {
            String oneLineString ="";
            int channelCnt = 0;

            while ((oneLineString = mBufferedReader.readLine()) != null) {
                // read one channel info start
                if (oneLineString.contains(CHECKOUT_HEAD_STRING)) {
                    ChannelInfo.Builder builder = new ChannelInfo.Builder();
                    while ((oneLineString = mBufferedReader.readLine()) != null) {
                        // read one channel info end
                        if (oneLineString.startsWith(CHECKOUT_TAIL_STRING)) {
                            channelInfoList.add(builder.build());
                            channelCnt++;
                            Log.d(TAG,"current channel info build complete! channelCnt: " + channelCnt);
                            //Log.d(TAG, builder.build().toString());
                            break;
                        }
                        // read every member variable in ChannelInfo from config file
                        buildChannelInfo(oneLineString, builder);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG,"write Channel Info failed!");
            e.printStackTrace();
            return null;
        }

        return channelInfoList;
    }

    private int saveChannelInfoListToDb(ArrayList<ChannelInfo> channelInfoList) {
        if (null == channelInfoList) {
            Log.w(TAG,"not find channel info in config file!");
            return RETURN_VALUE_FAILED;
        }
        ArrayList<ChannelInfo> atvChannels = new ArrayList<ChannelInfo>();
        ArrayList<ChannelInfo> dtvChannels = new ArrayList<ChannelInfo>();
        for (ChannelInfo c : channelInfoList) {
            if (c.isAnalogChannel()) {
                atvChannels.add(c);
            } else {
                dtvChannels.add(c);
            }
        }
        if (atvChannels.size() > 0) {
            mTvDataBaseManager.updateOrinsertChannelInList(null, atvChannels, false);
        }
        if (dtvChannels.size() > 0) {
            mTvDataBaseManager.updateOrinsertChannelInList(null, dtvChannels, true);
        }
        return RETURN_VALUE_SUCCESS;
    }
}
