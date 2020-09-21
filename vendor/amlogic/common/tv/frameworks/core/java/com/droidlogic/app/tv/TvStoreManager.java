/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;
import java.util.Comparator;

import android.util.Log;
import android.os.Bundle;
import android.content.Context;
import android.media.tv.TvContract;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.os.SystemProperties;
import android.os.HandlerThread;

import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvControlManager.FreqList;
import com.droidlogic.app.tv.TvControlManager.TvMode;

public abstract class TvStoreManager {
    public static final String TAG = "TvStoreManager";

    private Context mContext;
    private TvDataBaseManager mTvDataBaseManager;

    private String mInputId;
    private int mInitialDisplayNumber;
    private int mInitialLcnNumber;

    private int mDisplayNumber;
    private Integer mDisplayNumber2 = null;

    private TvControlManager.ScanMode mScanMode = null;
    private TvControlManager.SortMode mSortMode = null;
    private TvControlManager mTvControlManager = null;
    private ArrayList<TvControlManager.ScannerLcnInfo> mLcnInfo = null;

    /*for store in search*/
    private boolean isFinalStoreStage = false;
    private boolean isRealtimeStore = true;
    private HandlerThread mhandlerThread;
    private Handler mChildHandler;
    private int mCurrentStoreFrequency = 0;

    private ArrayList<ChannelInfo> mChannelsOld = null;
    private ArrayList<ChannelInfo> mChannelsNew = null;
    private ArrayList<ChannelInfo> mChannelsExist = null;
    private ArrayList<ChannelInfo> mChannelsAll = null;

    private ArrayList<FreqList> mATSC_T = null;
    private ArrayList<FreqList> mATSC_C_STD = null;
    private ArrayList<FreqList> mATSC_C_LRC = null;
    private ArrayList<FreqList> mATSC_HRC = null;

    private boolean on_channel_store_tschanged = true;

    private int lcn_overflow_start;
    private int display_number_start;

    public TvStoreManager(Context context, String inputId, int initialDisplayNumber) {
        Log.d(TAG, "inputId["+inputId+"] initialDisplayNumber["+initialDisplayNumber+"]");

        mContext = context;
        mInputId = inputId;
        mInitialDisplayNumber = initialDisplayNumber;
        mInitialLcnNumber = mInitialDisplayNumber;

        mTvDataBaseManager = new TvDataBaseManager(mContext);
        mTvControlManager = TvControlManager.getInstance();
        display_number_start = mInitialDisplayNumber;
        lcn_overflow_start = mInitialLcnNumber;
        mhandlerThread = new HandlerThread("dealevent");
        mhandlerThread.start();
        mChildHandler = new Handler(mhandlerThread.getLooper(), new ChildCallback());
    }

    public void releaseHandlerThread() {
       if (mhandlerThread != null) {
           mhandlerThread.quit();
           mhandlerThread = null;
       }
       if (mChildHandler != null)
           mChildHandler = null;
    }


    public void setInitialLcnNumber(int initialLcnNumber) {
        Log.d(TAG, "initalLcnNumber["+initialLcnNumber+"]");
        mInitialLcnNumber = initialLcnNumber;
    }

    public void onEvent(String eventType, Bundle eventArgs) {}

    public void onUpdateCurrent(ChannelInfo channel, boolean store) {}

    public void onDtvNumberMode(String mode) {}

    public abstract void onScanEnd();

    public void onScanExit(int freg) {}

    public void onStoreEnd(int freg) {}

    public void onScanEndBeforeStore(int freg) {}

    private Bundle getScanEventBundle(TvControlManager.ScannerEvent mEvent) {
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_TYPE, mEvent.type);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_PRECENT, mEvent.precent);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_TOTALCOUNT, mEvent.totalcount);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_LOCK, mEvent.lock);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_CNUM, mEvent.cnum);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_FREQ, mEvent.freq);
        bundle.putString(DroidLogicTvUtils.SIG_INFO_C_PROGRAMNAME, mEvent.programName);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_SRVTYPE, mEvent.srvType);
        bundle.putString(DroidLogicTvUtils.SIG_INFO_C_MSG, "");
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_STRENGTH, mEvent.strength);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_QUALITY, mEvent.quality);
        // ATV
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_VIDEOSTD, mEvent.videoStd);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_AUDIOSTD, mEvent.audioStd);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_ISAUTOSTD, mEvent.isAutoStd);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_FINETUNE, mEvent.fineTune);
        // DTV
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_MODE, mEvent.mode);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_SR, mEvent.sr);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_MOD, mEvent.mod);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_BANDWIDTH, mEvent.bandwidth);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_OFM_MODE, TvControlManager.TvMode.fromMode(mEvent.mode).getGen());
        bundle.putString(DroidLogicTvUtils.SIG_INFO_C_PARAS, mEvent.paras);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_TS_ID, mEvent.ts_id);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_ORIG_NET_ID, mEvent.orig_net_id);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_SERVICEiD, mEvent.serviceID);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_VID, mEvent.vid);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_VFMT, mEvent.vfmt);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_AIDS, mEvent.aids);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_AFMTS, mEvent.afmts);
        bundle.putStringArray(DroidLogicTvUtils.SIG_INFO_C_ALANGS, mEvent.alangs);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_ATYPES, mEvent.atypes);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_AEXTS, mEvent.aexts);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_PCR, mEvent.pcr);

        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_STYPES, mEvent.stypes);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SIDS, mEvent.sids);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SSTYPES, mEvent.sstypes);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SID1S, mEvent.sid1s);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SID2S, mEvent.sid2s);
        bundle.putStringArray(DroidLogicTvUtils.SIG_INFO_C_SLANGS, mEvent.slangs);

        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM, -1);

        return bundle;
    }

    private Bundle getDisplayNumBunlde(int displayNum) {
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM, displayNum);
        return bundle;
    }


    private void prepareStore(TvControlManager.ScannerEvent event) {
        mScanMode = new TvControlManager.ScanMode(event.scan_mode);
        mSortMode = new TvControlManager.SortMode(event.sort_mode);
        Log.d(TAG, "scan mode:"+event.scan_mode);
        Log.d(TAG, "sort mode:"+event.sort_mode);

        mDisplayNumber = mInitialDisplayNumber;
        mDisplayNumber2 = new Integer(mInitialDisplayNumber);
        isFinalStoreStage = false;
        isRealtimeStore = true;

        Bundle bundle = null;
        bundle = getScanEventBundle(event);
        onEvent(DroidLogicTvUtils.SIG_INFO_C_SCAN_BEGIN_EVENT, bundle);
    }

    private void checkOrPatchBeginLost(TvControlManager.ScannerEvent event) {
        if (mScanMode == null) {
            Log.d(TAG, "!Lost EVENT_SCAN_BEGIN, assume began.");
            prepareStore(event);
        }
    }

    private void initChannelsExist() {
        //get all old channles exist.
        //init display number count.
        if (mChannelsOld == null) {
            mChannelsOld = mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO);
            mChannelsOld.addAll(mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO));
            //do not count service_other.
            mDisplayNumber = mChannelsOld.size() + 1;
            mChannelsOld.addAll(mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_OTHER));
            Log.d(TAG, "Store> channel next:" + mDisplayNumber);
        }
        if (mChannelsAll == null) {
            mChannelsAll = new ArrayList<ChannelInfo>();
        }
    }
    private void reinitChannels() {
        if (mChannelsExist == null) {
            mChannelsExist = mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO);
            mChannelsExist.addAll(mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO));
            mChannelsExist.addAll(mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_OTHER));
        }
        if (mChannelsAll == null) {
            mChannelsAll = new ArrayList<ChannelInfo>();
        }
    }
    private ChannelInfo createDtvChannelInfo(TvControlManager.ScannerEvent event) {
        String name = null;
        String serviceType;

        try {
            name = TvMultilingualText.getText(event.programName);
        } catch (Exception e) {
            e.printStackTrace();
            name = "????";
        }

        if (event.srvType == 1) {
            serviceType = TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO;
        } else if (event.srvType == 2) {
            serviceType = TvContract.Channels.SERVICE_TYPE_AUDIO;
        } else {
            serviceType = TvContract.Channels.SERVICE_TYPE_OTHER;
        }

        TvControlManager.FEParas fep =
            new TvControlManager.FEParas(DroidLogicTvUtils.getObjectString(event.paras, "fe"));

        return new ChannelInfo.Builder()
               .setInputId(mInputId)
               .setType(fep.getMode().toType())
               .setServiceType(serviceType)
               .setServiceId(event.serviceID)
               .setDisplayNumber(Integer.toString(mDisplayNumber))
               .setDisplayName(name)
               .setLogoUrl(null)
               .setOriginalNetworkId(event.orig_net_id)
               .setTransportStreamId(event.ts_id)
               .setVideoPid(event.vid)
               .setVideoStd(0)
               .setVfmt(event.vfmt)
               .setVideoWidth(0)
               .setVideoHeight(0)
               .setAudioPids(event.aids)
               .setAudioFormats(event.afmts)
               .setAudioLangs(event.alangs)
               .setAudioExts(event.aexts)
               .setAudioStd(0)
               .setIsAutoStd(event.isAutoStd)
               //.setAudioTrackIndex(getidxByDefLan(event.alangs))
               .setAudioCompensation(0)
               .setPcrPid(event.pcr)
               .setFrequency(event.freq)
               .setBandwidth(event.bandwidth)
               .setSymbolRate(event.sr)
               .setModulation(event.mod)
               .setFEParas(fep.toString())
               .setFineTune(0)
               .setBrowsable(true)
               .setIsFavourite(false)
               .setPassthrough(false)
               .setLocked(false)
               .setSubtitleTypes(event.stypes)
               .setSubtitlePids(event.sids)
               .setSubtitleStypes(event.sstypes)
               .setSubtitleId1s(event.sid1s)
               .setSubtitleId2s(event.sid2s)
               .setSubtitleLangs(event.slangs)
               //.setSubtitleTrackIndex(getidxByDefLan(event.slangs))
               .setDisplayNameMulti(event.programName)
               .setFreeCa(event.free_ca)
               .setScrambled(event.scrambled)
               .setSdtVersion(event.sdtVersion)
               .setMajorChannelNumber(event.majorChannelNumber)
               .setMinorChannelNumber(event.minorChannelNumber)
               .setSourceId(event.sourceId)
               .setAccessControled(event.accessControlled)
               .setHidden(event.hidden)
               .setHideGuide(event.hideGuide)
               .setVct(event.vct)
               .setProgramsInPat(event.programs_in_pat)
               .setPatTsId(event.pat_ts_id)
               .setSignalType(DroidLogicTvUtils.getCurrentSignalType(mContext) == DroidLogicTvUtils.SIGNAL_TYPE_ERROR
                ? TvContract.Channels.TYPE_ATSC_T : DroidLogicTvUtils.getCurrentSignalType(mContext))
               .build();
    }

    private ChannelInfo createAtvChannelInfo(TvControlManager.ScannerEvent event) {
        String ATVName = event.programName;
        String type = TvContract.Channels.TYPE_PAL;
        switch (event.videoStd) {
            case TvControlManager.ATV_VIDEO_STD_PAL:
                type = TvContract.Channels.TYPE_PAL;
                break;
            case TvControlManager.ATV_VIDEO_STD_NTSC:
                type = TvContract.Channels.TYPE_NTSC;
                break;
            case TvControlManager.ATV_VIDEO_STD_SECAM:
                type = TvContract.Channels.TYPE_SECAM;
                break;
            default:
                type = TvContract.Channels.TYPE_PAL;
                break;
        }

        TvControlManager.FEParas fep =
            new TvControlManager.FEParas(DroidLogicTvUtils.getObjectString(event.paras, "fe"));

        if (ATVName.length() == 0)
            ATVName = "xxxATV Program";
        if (ATVName.startsWith("xxxATV Program"))
            ATVName = ATVName;

        return new ChannelInfo.Builder()
               .setInputId(mInputId == null ? "NULL" : mInputId)
               .setType(type)
               .setServiceType(TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO)//default is SERVICE_TYPE_AUDIO_VIDEO
               .setServiceId(0)
               .setDisplayNumber(Integer.toString(mDisplayNumber))
               .setDisplayName(TvMultilingualText.getText(ATVName))
               .setLogoUrl(null)
               .setOriginalNetworkId(0)
               .setTransportStreamId(0)
               .setVideoPid(0)
               .setVideoStd(event.videoStd)
               .setVfmt(event.vfmt)
               .setVideoWidth(0)
               .setVideoHeight(0)
               .setAudioPids(null)
               .setAudioFormats(null)
               .setAudioLangs(null)
               .setAudioExts(null)
               .setAudioStd(event.audioStd)
               .setIsAutoStd(event.isAutoStd)
               .setAudioTrackIndex(0)
               .setAudioCompensation(0)
               .setPcrPid(0)
               .setFrequency(event.freq)
               .setBandwidth(0)
               .setSymbolRate(0)
               .setModulation(0)
               .setFEParas(fep.toString())
               .setFineTune(0)
               .setBrowsable(true)
               .setIsFavourite(false)
               .setPassthrough(false)
               .setLocked(false)
               .setSubtitleTypes(event.stypes)
               .setSubtitlePids(event.sids)
               .setSubtitleStypes(event.sstypes)
               .setSubtitleId1s(event.sid1s)
               .setSubtitleId2s(event.sid2s)
               .setSubtitleLangs(event.slangs)
               .setDisplayNameMulti(ATVName)
               .setMajorChannelNumber(event.majorChannelNumber)
               .setMinorChannelNumber(event.minorChannelNumber)
               .setSourceId(event.sourceId)
               .setAccessControled(event.accessControlled)
               .setHidden(event.hidden)
               .setHideGuide(event.hideGuide)
               .setContentRatings(null)
               .setSignalType(DroidLogicTvUtils.getCurrentSignalType(mContext) == DroidLogicTvUtils.SIGNAL_TYPE_ERROR
                ? TvContract.Channels.TYPE_ATSC_T : DroidLogicTvUtils.getCurrentSignalType(mContext))
               .build();
    }

    private void filterChannels(TvControlManager.ScannerEvent event, ChannelInfo channel) {
        if (mChannelsOld != null) {
            Log.d(TAG, "remove channels with freq!="+channel.getFrequency());
            //remove channles with diff freq from old channles
            Iterator<ChannelInfo> iter = mChannelsOld.iterator();
            while (iter.hasNext()) {
                ChannelInfo c = iter.next();
                if (c.getFrequency() != channel.getFrequency())
                    iter.remove();
            }
        }
    }

    private void cacheChannel(TvControlManager.ScannerEvent event, ChannelInfo channel) {

        if (mScanMode == null) {
            Log.d(TAG, "mScanMode is null, store return.");
            return;
        }

        if (mChannelsNew == null)
            mChannelsNew = new ArrayList();

        //not add the same channel
        int newindex = dealRepeatCacheChannels(channel);
        if (newindex > -1) {
            channel = mChannelsNew.get(newindex);
            Log.d(TAG, "cacheChannel update id = " + channel.getId());
        }

        Log.d(TAG, "store save [" + channel.getDisplayNumber() + "][" + channel.getFrequency() + "][" + channel.getServiceType() + "][" + channel.getDisplayName() + "][" + channel.getId() + "]");

        if (mScanMode.isDTVManulScan() || mScanMode.isATVManualScan()) {
            if (on_channel_store_tschanged) {
                on_channel_store_tschanged = false;
                filterChannels(event, channel);
            }
        }
    }

    private int dealRepeatCacheChannels(ChannelInfo ch) {
        if (ch == null) {
            return -1;
        }
        boolean hasaddtoall = false;
        for (int j = 0; j < mChannelsAll.size(); j++) {
            ChannelInfo tempch = mChannelsAll.get(j);
            if (tempch != null && tempch.isSameChannel(ch)) {
                if (tempch.getId() < 0) {
                    long id = mTvDataBaseManager.queryChannelIdInDb(tempch);
                    if (id > -1) {
                        ch.setId(id);
                    }
                } else {
                    ch.setId(tempch.getId());
                }

                mChannelsAll.remove(j);
                mChannelsAll.add(j, ch);
                hasaddtoall = true;
                Log.d(TAG, "dealRepeatCacheChannels exist in all  = " + mChannelsAll.get(j).getDisplayNumber() + ", id = " + mChannelsAll.get(j).getId() + ", type = " + mChannelsAll.get(j).getType());
                if (ch.isAnalogChannel() || ch.isAtscChannel()) {
                    return -1;//atsc and analog channel don't need to cache the same channel as dtmb channel DisplayNumber may change
                }
            }
        }
        if (!hasaddtoall) {
            mChannelsAll.add(ch);
            Log.d(TAG, "dealRepeatCacheChannels not exist in all  = " + ch.getDisplayNumber());
        }
        boolean hasaddtonew = false;
        for (int i = 0; i < mChannelsNew.size(); i++) {
            ChannelInfo tempch = mChannelsNew.get(i);
            if (tempch != null && tempch.isSameChannel(ch)) {
                Log.d(TAG, "dealRepeatCacheChannels exist in new = " + tempch.getDisplayNumber());
                mChannelsNew.remove(i);
                mChannelsNew.add(i, ch);
                hasaddtonew = true;
                return i;
            }
        }
        if (!hasaddtonew) {
            mChannelsNew.add(ch);
            Log.d(TAG, "dealRepeatCacheChannels not exist in new = " + ch.getDisplayNumber());
        }
        return mChannelsNew.size() -1;
    }

    private boolean isChannelInListbyId(ChannelInfo channel, ArrayList<ChannelInfo> list) {
        if (list == null)
            return false;

        for (ChannelInfo c : list)
            if (c.getId() == channel.getId())
                return true;

        return false;
    }

    private void updateChannelNumber(ChannelInfo channel) {
        updateChannelNumber(channel, null);
    }

    private void updateChannelNumber(ChannelInfo channel, ArrayList<ChannelInfo> channels) {
        boolean ignoreDBCheck = false;//mScanMode.isDTVAutoScan();
        boolean findDisNum = true;
        int number = -1;
        ArrayList<ChannelInfo> chs = null;

        if (!ignoreDBCheck) {
            if (channels != null)
                chs = channels;
            else
                chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                        null, null);
            for (ChannelInfo c : chs) {
                if ((c.getNumber() >= display_number_start) && !isChannelInListbyId(c, mChannelsOld)) {
                    display_number_start = c.getNumber() + 1;
                }
            }
        }
        Log.d(TAG, "display number start from:" + display_number_start);

        Log.d(TAG, "Service["+channel.getOriginalNetworkId()+":"+channel.getTransportStreamId()+":"+channel.getServiceId()+"]");

        if (channel.getServiceType() == TvContract.Channels.SERVICE_TYPE_OTHER) {
            Log.d(TAG, "Service["+channel.getServiceId()+"] is Type OTHER, ignore NUMBER update and set to unbrowsable");
            channel.setBrowsable(false);
            return;
        }

        if (mChannelsOld != null) {//may only in manual search
            for (ChannelInfo c : mChannelsOld) {
                if ((c.getOriginalNetworkId() == channel.getOriginalNetworkId())
                    && (c.getTransportStreamId() == channel.getTransportStreamId())
                    && (c.getServiceId() == channel.getServiceId())) {
                    //same freq, reuse number if old channel is identical
                    Log.d(TAG, "found num:" + c.getDisplayNumber() + " by same old service["+c.getOriginalNetworkId()+":"+c.getTransportStreamId()+":"+c.getServiceId()+"]");
                    number = c.getNumber();
                }
            }
        }

        //service totally new
        if (number < 0) {
            if (mScanMode.isDTVManulScan()) {
                while (findDisNum && mChannelsExist != null) {
                    findDisNum = false;
                    for (ChannelInfo c : mChannelsExist) {
                        if (display_number_start == c.getNumber()) {
                            findDisNum = true;
                            break;
                        }
                    }
                    if (findDisNum)
                        display_number_start++;
                }
            }
            number = display_number_start++;
        }
        Log.d(TAG, "update displayer number["+number+"]..");

        channel.setDisplayNumber(String.valueOf(number));

        if (channels != null)
            channels.add(channel);
    }

    private void updateChannelLCN(ChannelInfo channel) {
        updateChannelLCN(channel, null);
    }

    private void updateChannelLCN(ChannelInfo channel, ArrayList<ChannelInfo> channels) {
        boolean ignoreDBCheck = false;//mScanMode.isDTVAutoScan();

        int lcn = -1;
        int lcn_1 = -1;
        int lcn_2 = -1;
        boolean visible = true;
        boolean swapped = false;
        boolean findDisNum = true;

        ArrayList<ChannelInfo> chs = null;

        if (!ignoreDBCheck) {
            if (channels != null)
                chs = channels;
            else
                chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                        null, null);
            for (ChannelInfo c : chs) {
                if ((c.getLCN() >= lcn_overflow_start) && !isChannelInListbyId(c, mChannelsOld)) {
                    lcn_overflow_start = c.getLCN() + 1;
                }
            }
        }
        Log.d(TAG, "lcn overflow start from:"+lcn_overflow_start);

        Log.d(TAG, "Service["+channel.getOriginalNetworkId()+":"+channel.getTransportStreamId()+":"+channel.getServiceId()+"]");

        if (channel.getServiceType() == TvContract.Channels.SERVICE_TYPE_OTHER) {
            Log.d(TAG, "Service["+channel.getServiceId()+"] is Type OTHER, ignore LCN update and set to unbrowsable");
            channel.setBrowsable(false);
            return;
        }

        if (mLcnInfo != null) {
            for (TvControlManager.ScannerLcnInfo l : mLcnInfo) {
                if ((l.netId == channel.getOriginalNetworkId())
                    && (l.tsId == channel.getTransportStreamId())
                    && (l.serviceId == channel.getServiceId())) {

                    Log.d(TAG, "lcn found:");
                    Log.d(TAG, "\tlcn[0:"+l.lcn[0]+":"+l.visible[0]+":"+l.valid[0]+"]");
                    Log.d(TAG, "\tlcn[1:"+l.lcn[1]+":"+l.visible[1]+":"+l.valid[1]+"]");

                    // lcn found, use lcn[0] by default.
                    lcn_1 = l.valid[0] == 0 ? -1 : l.lcn[0];
                    lcn_2 = l.valid[1] == 0 ? -1 : l.lcn[1];
                    lcn = lcn_1;
                    visible = l.visible[0] == 0 ? false : true;

                    if ((lcn_1 != -1) && (lcn_2 != -1) && !ignoreDBCheck) {
                        // check for lcn already exist just on Maunual Scan
                        // look for service with sdlcn equal to l's hdlcn, if found, change the service's lcn to it's hdlcn
                        ChannelInfo ch = null;
                        if (channels != null) {
                            for (ChannelInfo c : channels) {
                                if (c.getLCN() == lcn_2)
                                    ch = c;
                            }
                        } else {
                            chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                                    ChannelInfo.COLUMN_LCN+"=?",
                                    new String[]{String.valueOf(lcn_2)});
                            if (chs.size() > 0) {
                                if (chs.size() > 1)
                                    Log.d(TAG, "Warning: found " + chs.size() + "channels with lcn="+lcn_2);
                                ch = chs.get(0);
                            }
                        }
                        if ((ch != null) && !isChannelInListbyId(ch, mChannelsOld)) {// do not check those will be deleted.
                            Log.d(TAG, "swap exist lcn["+ch.getLCN()+"] -> ["+ch.getLCN2()+"]");
                            Log.d(TAG, "\t for Service["+ch.getOriginalNetworkId()+":"+ch.getTransportStreamId()+":"+ch.getServiceId()+"]");

                            ch.setLCN(ch.getLCN2());
                            ch.setLCN1(ch.getLCN2());
                            ch.setLCN2(lcn_2);
                            if (channels == null)
                                mTvDataBaseManager.updateChannelInfo(ch);

                            swapped = true;
                        }
                    } else if (lcn_1 == -1) {
                        lcn = lcn_2;
                        visible = l.visible[1] == 0 ? false : true;
                        Log.d(TAG, "lcn[0] invalid, use lcn[1]");
                    }
                }
            }
        }

        Log.d(TAG, "Service visible = "+visible);
        channel.setBrowsable(visible);

        if (!swapped) {
            if (lcn >= 0) {
                ChannelInfo ch = null;
                if (channels != null) {
                    for (ChannelInfo c : channels) {
                        if (c.getLCN() == lcn)
                            ch = c;
                    }
                } else {
                    chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                            ChannelInfo.COLUMN_LCN+"=?",
                            new String[]{String.valueOf(lcn)});
                    if (chs.size() > 0) {
                        if (chs.size() > 1)
                            Log.d(TAG, "Warning: found " + chs.size() + "channels with lcn="+lcn);
                        ch = chs.get(0);
                    }
                }
                if (ch != null) {
                    if (!isChannelInListbyId(ch, mChannelsOld)) {//do not check those will be deleted.
                        Log.d(TAG, "found lcn conflct:" + lcn + " by service["+ch.getOriginalNetworkId()+":"+ch.getTransportStreamId()+":"+ch.getServiceId()+"]");
                        lcn = lcn_overflow_start++;
                    }
                }
            } else {
                Log.d(TAG, "no LCN info found for service");
                if (mChannelsOld != null) {//may only in manual search
                    for (ChannelInfo c : mChannelsOld) {
                        if ((c.getOriginalNetworkId() == channel.getOriginalNetworkId())
                            && (c.getTransportStreamId() == channel.getTransportStreamId())
                            && (c.getServiceId() == channel.getServiceId())) {
                            //same freq, reuse lcn if old channel is identical
                            Log.d(TAG, "found lcn:" + c.getLCN() + " by same old service["+c.getOriginalNetworkId()+":"+c.getTransportStreamId()+":"+c.getServiceId()+"]");
                            lcn = c.getLCN();
                        }
                    }
                }
                //service totally new
                if (lcn < 0) {
                    if (mScanMode.isDTVManulScan()) {
                        while (findDisNum && mChannelsExist != null) {
                            findDisNum = false;
                            for (ChannelInfo c : mChannelsExist) {
                                if (lcn_overflow_start == c.getLCN()) {
                                    findDisNum = true;
                                    break;
                                }
                            }
                            if (findDisNum)
                                lcn_overflow_start++;
                        }
                    }
                    lcn = lcn_overflow_start++;
                }
            }
        }

        Log.d(TAG, "update LCN[0:"+lcn+" 1:"+lcn_1+" 2:"+lcn_2+"]..");

        channel.setLCN(lcn);
        channel.setLCN1(lcn_1);
        channel.setLCN2(lcn_2);

        if (channels != null)
            channels.add(channel);
    }

    private void storeTvChannel(boolean isRealtimeStore, boolean isFinalStore) {
        Bundle bundle = null;
        ArrayList<ChannelInfo> dtvchannelsnew= new ArrayList<ChannelInfo>();
        ArrayList<ChannelInfo> atvchannelsnew= new ArrayList<ChannelInfo>();
        ArrayList<ChannelInfo> dtvchannelsinsert;
        ArrayList<ChannelInfo> dtvchannelsupdate;
        ArrayList<ChannelInfo> atvchannelsinsert;
        ArrayList<ChannelInfo> atvchannelsupdate;

        Log.d(TAG, "isRealtimeStore:" + isRealtimeStore + " isFinalStore:"+ isFinalStore);

        if (mChannelsNew != null) {

            /*sort channels by serviceId*/
            Collections.sort(mChannelsNew, new Comparator<ChannelInfo> () {
                @Override
                public int compare(ChannelInfo a, ChannelInfo b) {
                    /*sort: frequency 1st, serviceId 2nd*/
                    int A = a.getFrequency();
                    int B = b.getFrequency();
                    return (A > B) ? 1 : ((A == B) ? (a.getServiceId() - b.getServiceId()) : -1);
                }
            });

            ArrayList<ChannelInfo> mChannels = new ArrayList();

            for (ChannelInfo c : mChannelsNew) {

                //Calc display number / LCN
                if (mSortMode.isLCNSort()) {
                    if (isRealtimeStore)
                        updateChannelLCN(c, mChannels);
                    else
                        updateChannelLCN(c);
                    c.setDisplayNumber(String.valueOf(c.getLCN()));
                    Log.d(TAG, "LCN DisplayNumber:"+ c.getDisplayNumber());
                    onDtvNumberMode("lcn");
                } else if (mSortMode.isATSCStandard()) {
                    Log.d(TAG, "ATSC DisplayNumber:"+c.getDisplayNumber());
                    /*nothing to do*/
                } else {
                    if (isRealtimeStore)
                        updateChannelNumber(c, mChannels);
                    else
                        updateChannelNumber(c);
                    Log.d(TAG, "NUM DisplayNumber:"+ c.getDisplayNumber());
                }

                if (isRealtimeStore) {
                    if (c.isAnalogChannel()) {
                        //mTvDataBaseManager.updateOrinsertAtvChannelWithNumber(c);
                        atvchannelsnew.add(c);
                    } else {
                        //mTvDataBaseManager.updateOrinsertDtvChannelWithNumber(c);
                        dtvchannelsnew.add(c);
                    }
                } else {
                    if (c.isAnalogChannel())
                        mTvDataBaseManager.insertAtvChannel(c, c.getDisplayNumber());
                    else
                        mTvDataBaseManager.insertDtvChannel(c, c.getDisplayNumber());
                }

                /*Log.d(TAG, ((isRealtimeStore) ? "update/insert [" : "insert [") + c.getDisplayNumber()
                    + "][" + c.getFrequency() + "][" + c.getServiceType() + "][" + c.getDisplayName() + "]");*/

                if (isFinalStore) {
                    bundle = getDisplayNumBunlde(c.getNumber());
                    onEvent(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM_EVENT, bundle);
                }
            }
        }
        if (atvchannelsnew != null && atvchannelsnew.size() > 0) {
            atvchannelsupdate = getNeedUpdateChannel(atvchannelsnew);
            atvchannelsinsert= getNeedInsertChannel(atvchannelsnew);
            mTvDataBaseManager.updateOrinsertChannelInList(atvchannelsupdate, atvchannelsinsert, false);
        }
        if (dtvchannelsnew != null && dtvchannelsnew.size() > 0) {
            dtvchannelsupdate = getNeedUpdateChannel(dtvchannelsnew);
            dtvchannelsinsert= getNeedInsertChannel(dtvchannelsnew);
            mTvDataBaseManager.updateOrinsertChannelInList(dtvchannelsupdate, dtvchannelsinsert, true);
        }

        lcn_overflow_start = mInitialLcnNumber;
        display_number_start = mInitialDisplayNumber;
        on_channel_store_tschanged = true;
        mChannelsOld = null;
        mChannelsNew = null;
        mChannelsExist = null;
    }

    private ArrayList<ChannelInfo> getNeedUpdateChannel(ArrayList<ChannelInfo> list) {
        ArrayList<ChannelInfo> needupdate = new ArrayList<ChannelInfo>();
        for (int i = 0; i < list.size(); i++) {
            for (int j = 0; j < mChannelsAll.size(); j++) {
                ChannelInfo tempch = mChannelsAll.get(j);
                if (tempch != null && TextUtils.equals(tempch.getDisplayNumber(), list.get(i).getDisplayNumber()) && tempch.getFrequency() == list.get(i).getFrequency()) {
                    ChannelInfo updateinfo = list.get(i);
                    if (tempch.getId() > -1) {
                        updateinfo.setId(tempch.getId());
                    }
                    needupdate.add(updateinfo);
                }
            }
        }
        Log.d(TAG, "getNeedUpdateChannel size = " + needupdate.size());
        return needupdate;
    }

    private ArrayList<ChannelInfo> getNeedInsertChannel(ArrayList<ChannelInfo> list) {
        ArrayList<ChannelInfo> needinsert = new ArrayList<ChannelInfo>();
        boolean exist = false;
        for (int i = 0; i < list.size(); i++) {
            for (int j = 0; j < mChannelsAll.size(); j++) {
                ChannelInfo tempch = mChannelsAll.get(j);
                if (tempch != null && TextUtils.equals(tempch.getDisplayNumber(), list.get(i).getDisplayNumber()) && tempch.getFrequency() == list.get(i).getFrequency()) {
                    exist = true;
                    break;
                }
            }
            if (!exist) {
                needinsert.add(list.get(i));
            } else {
                exist = false;
            }
        }
        Log.d(TAG, "getNeedInsertChannel size = " + needinsert.size());
        return needinsert;
    }

   private boolean getIsSameDisplayNumber(ChannelInfo currentChannel) {
          int size = mChannelsExist.size();
          for (int i = 0; i < size; i++) {
            ChannelInfo channel = mChannelsExist.get(i);
            if (TextUtils.equals(currentChannel.getDisplayNumber(), channel.getDisplayNumber())
                && (currentChannel.getFrequency() != channel.getFrequency())
                && TextUtils.equals(currentChannel.getSignalType(), channel.getSignalType())) {
               return true;
           }
         }
       return false;
    }

   private int getDvbPhysicalNumFromListByFre(ArrayList<FreqList> mlist, int freq, int diff) {
        int size = mlist.size();
        for (int i = 0; i < size; i++) {
            if (freq == mlist.get(i).freq) {
                return i + 2;
            }
        }
        for (int i = 0; i < size; i++) {
            if (mlist.get(i).freq - diff < freq && freq < mlist.get(i).freq + diff) {
                return i + 2;
            }
        }
       return -1;
    }

    private int getDvbPhysicalNumByFre(TvMode tvMode, int freq) {

        int diff = 3000000;
        String type = tvMode.toType();
        int physicalNum = -1;
        Log.d(TAG, "type:" + type + " freq :" + freq);
        if (type.equals(TvContract.Channels.TYPE_ATSC_T)) {
          tvMode.setList(0);
          if (mATSC_T == null) {
              mATSC_T = mTvControlManager.DTVGetScanFreqList(tvMode.getMode());
          }
          physicalNum = getDvbPhysicalNumFromListByFre(mATSC_T, freq, diff);
          return physicalNum;
        }
        if (type.equals(TvContract.Channels.TYPE_ATSC_C)) {
          tvMode.setList(1);
          if (mATSC_C_STD == null) {
            mATSC_C_STD = mTvControlManager.DTVGetScanFreqList(tvMode.getMode());
          }
          physicalNum = getDvbPhysicalNumFromListByFre(mATSC_C_STD, freq, diff);
          if (physicalNum != -1) {
            //error
            return physicalNum;
          }
          tvMode.setList(2);
          if (mATSC_C_LRC == null) {
            mATSC_C_LRC = mTvControlManager.DTVGetScanFreqList(tvMode.getMode());
          }
          physicalNum = getDvbPhysicalNumFromListByFre(mATSC_C_LRC, freq, diff);
          if (physicalNum != -1) {
            //error
            return physicalNum;
          }
          tvMode.setList(3);
          if (mATSC_HRC == null) {
            mATSC_HRC = mTvControlManager.DTVGetScanFreqList(tvMode.getMode());
          }
          physicalNum = getDvbPhysicalNumFromListByFre(mATSC_HRC, freq, diff);
          if (physicalNum != -1) {
            //error
            return physicalNum;
          }
        }
        return physicalNum;
    }
    public void onStoreEvent(TvControlManager.ScannerEvent event) {
        sendStoreEvent(event);
    }

    private void sendStoreEvent(TvControlManager.ScannerEvent event) {
        Message msg = new Message();
        msg.arg1 = event.type;
        msg.obj = event;
        mChildHandler.sendMessage(msg);
    }

    public void dealStoreEvent(TvControlManager.ScannerEvent event) {
        ChannelInfo channel = null;
        String name = null;
        Bundle bundle = null;
        TvMode mode = null;
        int freq = 0;
        int physicalNum = -1;
        Log.d(TAG, "onEvent:" + event.type + " :" + mDisplayNumber);

        switch (event.type) {
        case TvControlManager.EVENT_SCAN_BEGIN:
            Log.d(TAG, "Scan begin");
            prepareStore(event);
            break;

        case TvControlManager.EVENT_LCN_INFO_DATA:

            checkOrPatchBeginLost(event);

            if (mLcnInfo == null)
                mLcnInfo = new ArrayList<TvControlManager.ScannerLcnInfo>();
            mLcnInfo.add(event.lcnInfo);
            Log.d(TAG, "Lcn["+event.lcnInfo.netId+":"+event.lcnInfo.tsId+":"+event.lcnInfo.serviceId+"]");
            Log.d(TAG, "\t[0:"+event.lcnInfo.lcn[0]+":"+event.lcnInfo.visible[0]+":"+event.lcnInfo.valid[0]+"]");
            Log.d(TAG, "\t[1:"+event.lcnInfo.lcn[1]+":"+event.lcnInfo.visible[1]+":"+event.lcnInfo.valid[1]+"]");
            break;

        case TvControlManager.EVENT_DTV_PROG_DATA:
            Log.d(TAG, "dtv prog data");

            checkOrPatchBeginLost(event);

            if (!isFinalStoreStage)
                isRealtimeStore = true;

            if (mScanMode == null) {
                Log.d(TAG, "mScanMode is null, store return.");
                return;
            }

            if (mScanMode.isDTVManulScan())
               initChannelsExist();
             /*get exist channel info list*/
             reinitChannels();
            TvControlManager.FEParas fep =
                new TvControlManager.FEParas(DroidLogicTvUtils.getObjectString(event.paras, "fe"));
            channel = createDtvChannelInfo(event);

            if (mDisplayNumber2 != null)
                channel.setDisplayNumber(Integer.toString(mDisplayNumber2));
            else
                channel.setDisplayNumber(String.valueOf(mDisplayNumber));

            if (event.minorChannelNumber != -1) {
                channel.setDisplayNumber(""+event.majorChannelNumber+"-"+event.minorChannelNumber);
                int vct = DroidLogicTvUtils.getObjectValueInt(event.paras, "srv", "vct", 0);
                Log.d(TAG, "srv.vct:"+vct);
                if (vct == 1 && ((event.majorChannelNumber >> 4) == 0x3f)) {
                  //one-part channnel numbers for digital cable system.
                  //see atsc a/65 page 35, if major_channel_number hi 6 bit is 11 1111
                  //one_part_number = (major_channel_number & 0x00f) << 10 + mino_channel_number
                  int one_part_number = ((event.majorChannelNumber & 0x00f) << 10) + event.minorChannelNumber;
                  Log.d(TAG, "set one_part_number:"+ one_part_number + " maj:" + event.majorChannelNumber + " min:" +event.minorChannelNumber);
                  if (one_part_number == 0) {
                      mode = fep.getMode();
                      freq = fep.getFrequency();
                      physicalNum = getDvbPhysicalNumByFre(mode, freq);
                      if (physicalNum > 0)
                         channel.setDisplayNumber(""+physicalNum+"-"+channel.getServiceId());
                       else
                        channel.setDisplayNumber("0"+"-"+channel.getServiceId());
                  } else {
                      channel.setDisplayNumber(""+one_part_number);
                  }
                }
            }

            if (getIsSameDisplayNumber(channel) == true) {
              //is same
               mode = fep.getMode();
               freq = fep.getFrequency();
               physicalNum = getDvbPhysicalNumByFre(mode, freq);
               if (physicalNum > 0)
                 channel.setDisplayNumber(""+physicalNum+"-"+channel.getServiceId());
              Log.d(TAG, "----Channels physicalNum set DisplayName:" + physicalNum + " getDisplayNumber:" + channel.getDisplayNumber());
            }
            channel.print();
            /*add seach channel*/
            mChannelsExist.add(channel);
            cacheChannel(event, channel);

            if (mDisplayNumber2 != null) {
                Log.d(TAG, "mid store, num:"+mDisplayNumber2);
                mDisplayNumber2++;//count for realtime stage
            } else {
                Log.d(TAG, "final store, num: " + mDisplayNumber);
                bundle = getDisplayNumBunlde(mDisplayNumber);
                onEvent(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM_EVENT, bundle);
                mDisplayNumber++;//count for store stage
            }
            break;

        case TvControlManager.EVENT_ATV_PROG_DATA:
            Log.d(TAG, "atv prog data");
            checkOrPatchBeginLost(event);

            if (isFinalStoreStage && !mScanMode.isATVManualScan())
                break;

            initChannelsExist();

            channel = createAtvChannelInfo(event);
            if (event.majorChannelNumber != -1)
                channel.setDisplayNumber(""+event.majorChannelNumber+"-"+event.minorChannelNumber);

            Log.d(TAG, "reset number to " + channel.getDisplayNumber());

            channel.print();

            if (mSortMode.isATSCStandard()) {
                cacheChannel(event, channel);
            } else {
                mDisplayNumber = mTvDataBaseManager.insertAtvChannelWithFreOrder(channel);
                mDisplayNumber++;
            }

            Log.d(TAG, "onEvent,displayNum:" + mDisplayNumber);

            if (isFinalStoreStage) {
                bundle = getDisplayNumBunlde(mDisplayNumber);
                onEvent(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM_EVENT, bundle);
                mDisplayNumber++;//count for storestage
            }
            break;

        case TvControlManager.EVENT_SCAN_PROGRESS:
            Log.d(TAG, event.precent + "%\tfreq[" + event.freq + "] lock[" + event.lock + "] strength[" + event.strength + "] quality[" + event.quality + "] mode[" + event.mode + "]");

            checkOrPatchBeginLost(event);

            //take evt:progress as a store-loop end.
            if (!isFinalStoreStage && !mScanMode.isDTVManulScan()) {
                if (mCurrentStoreFrequency != event.freq) {
                    // one frequency store channel once time
                    mCurrentStoreFrequency = event.freq;
                    storeTvChannel(isRealtimeStore, isFinalStoreStage);
                }
                mDisplayNumber2 = mInitialDisplayNumber;//dtv pop all channels scanned every store-loop
            }

            bundle = getScanEventBundle(event);
            if (((event.mode & 0xff)  == TvChannelParams.MODE_ANALOG) && (event.lock == 0x11)) {
                bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM, mDisplayNumber);
                mDisplayNumber++;//count for progress stage
            }
            onEvent(DroidLogicTvUtils.SIG_INFO_C_PROCESS_EVENT, bundle);
            break;

        case TvControlManager.EVENT_STORE_BEGIN:
            Log.d(TAG, "Store begin");

            //reset for store stage
            isFinalStoreStage = true;
            mDisplayNumber = mInitialDisplayNumber;
            mDisplayNumber2 = null;
            if (mLcnInfo != null)
                mLcnInfo.clear();

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_STORE_BEGIN_EVENT, bundle);
            break;

        case TvControlManager.EVENT_STORE_END:
            Log.d(TAG, "Store end");
            if (null != mChannelsNew && 0 != mChannelsNew.size()) {
                Log.d(TAG, "Store end mChannelsNew.size=" + mChannelsNew.size());
                onScanEndBeforeStore(event.freq);
            }
            mCurrentStoreFrequency = 0;
            storeTvChannel(isRealtimeStore, isFinalStoreStage);

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_STORE_END_EVENT, bundle);

            onStoreEnd(event.freq);
            break;

        case TvControlManager.EVENT_SCAN_END:
            Log.d(TAG, "Scan end");

            onScanEnd();

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_SCAN_END_EVENT, bundle);
            break;

        case TvControlManager.EVENT_SCAN_EXIT:
            Log.d(TAG, "Scan exit.");

            isFinalStoreStage = false;
            isRealtimeStore = false;
            mDisplayNumber = mInitialDisplayNumber;
            mDisplayNumber2 = null;
            if (mLcnInfo != null) {
                mLcnInfo.clear();
                mLcnInfo = null;
            }

            mScanMode = null;
            mChannelsAll = null;

            //onScanExit(event.freq);

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_SCAN_EXIT_EVENT, bundle);
            break;

        default:
            break;
        }
    }

    private class ChildCallback implements Handler.Callback {
        @Override
        public boolean handleMessage(Message msg) {
            dealStoreEvent((TvControlManager.ScannerEvent)msg.obj);
            return false;
        }
    }
}

