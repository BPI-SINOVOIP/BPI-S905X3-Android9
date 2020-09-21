/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import android.util.Log;
import java.util.ArrayList;
import java.util.Arrays;

import com.droidlogic.app.tv.TvChannelParams;
import com.droidlogic.app.tv.ChannelInfo;

abstract public class DTVEpgScanner {

    public static final String TAG = "DTVEpgScanner";

    public static final int MODE_ADD    = 0;
    public static final int MODE_REMOVE = 1;
    public static final int MODE_SET    = 2;

    public static final int SCAN_PAT = 0x01;
    public static final int SCAN_PMT = 0x02;
    public static final int SCAN_CAT = 0x04;
    public static final int SCAN_SDT = 0x08;
    public static final int SCAN_NIT = 0x10;
    public static final int SCAN_TDT = 0x20;
    public static final int SCAN_EIT_PF_ACT = 0x40;
    public static final int SCAN_EIT_PF_OTH = 0x80;
    public static final int SCAN_EIT_SCHE_ACT = 0x100;
    public static final int SCAN_EIT_SCHE_OTH = 0x200;
    public static final int SCAN_MGT = 0x400;
    public static final int SCAN_VCT = 0x800;
    public static final int SCAN_STT = 0x1000;
    public static final int SCAN_RRT = 0x2000;
    public static final int SCAN_PSIP_EIT = 0x4000;
    public static final int SCAN_PSIP_ETT = 0x8000;
    public static final int SCAN_PSIP_EIT_VERSION_CHANGE = 0x20000;
    public static final int SCAN_EIT_PF_ALL = SCAN_EIT_PF_ACT | SCAN_EIT_PF_OTH;
    public static final int SCAN_EIT_SCHE_ALL = SCAN_EIT_SCHE_ACT | SCAN_EIT_SCHE_OTH;
    public static final int SCAN_EIT_ALL = SCAN_EIT_PF_ALL | SCAN_EIT_SCHE_ALL;
    public static final int SCAN_PSIP_EIT_ALL = SCAN_PSIP_EIT | SCAN_PSIP_ETT;
    public static final int SCAN_ALL = SCAN_PAT | SCAN_PMT | SCAN_CAT | SCAN_SDT | SCAN_NIT | SCAN_TDT | SCAN_EIT_ALL |
                                       SCAN_MGT | SCAN_VCT | SCAN_STT | SCAN_RRT | SCAN_PSIP_EIT_ALL;

    public class Event {
        public static final int EVENT_PF_EIT_END            = 1;
        public static final int EVENT_SCH_EIT_END           = 2;
        public static final int EVENT_PMT_END               = 3;
        public static final int EVENT_SDT_END               = 4;
        public static final int EVENT_TDT_END               = 5;
        public static final int EVENT_NIT_END               = 6;
        public static final int EVENT_PROGRAM_AV_UPDATE     = 7;
        public static final int EVENT_PROGRAM_NAME_UPDATE   = 8;
        public static final int EVENT_PROGRAM_EVENTS_UPDATE = 9;
        public static final int EVENT_CHANNEL_UPDATE        = 10;
        public static final int EVENT_EIT_CHANGED           = 11;
        public static final int EVENT_PMT_RATING            = 12;

        public int type;
        public int channelID;
        public int programID;
        public int dvbOrigNetID;
        public int dvbTSID;
        public int dvbServiceID;
        public int dvbServiceType;
        public long time;
        public int[] dvbVersion;
        public Evt[] evts;
        public ChannelInfo channel;
        public ServiceInfosFromSDT services;
        public int eitNumber;//atsc:eit-(k),dvb:0
        public byte[] pmt_rrt_ratings;
        public class Evt {
            int src;
            int srv_id;
            int ts_id;
            int net_id;
            int evt_id;
            long start;
            long end;
            int nibble;
            int parental_rating;
            byte[] name;
            byte[] desc;
            byte[] ext_item;
            byte[] ext_descr;
            int sub_flag;
            int sub_status;
            int source_id;
            byte[] rrt_ratings;

            Evt() {}
        }

        public class ServiceInfosFromSDT {

            public int mNetworkId;
            public int mTSId;
            public int mVersion;
            public ArrayList<ServiceInfoFromSDT> mServices;

            public class ServiceInfoFromSDT {
                public int mId;
                public int mType;
                public String mName;
                public int mRunning;
                public int mFreeCA;

                ServiceInfoFromSDT(){}
            }
            ServiceInfosFromSDT(){}
        }

        public Event(int type) {
            this.type = type;
        }
    }

    private long native_handle;
    private native void native_epg_create(int fend_id, int dmx_id, int src, String textLangs);
    private native void native_epg_destroy();
    private native void native_epg_change_mode(int op, int mode);
    private native void native_epg_monitor_service(ChannelInfo channel);
    private native void native_epg_set_dvb_text_coding(String coding);

    /** Load native library*/
    static {
        System.loadLibrary("jnidtvepgscanner");
    }

    private int fend_dev_id = -1;
    private int dmx_dev_id  = -1;
    private boolean created = false;

    private int fend_type   = -1;
    private ChannelInfo mChannel;

    private int mode;

    /*Start scan the sections.*/
    public void startScan(int mode) {
        if (!created)
            return;

        native_epg_change_mode(MODE_ADD, mode);
    }

    /*Stop scan the sections.*/
    public void stopScan(int mode) {
        if (!created)
            return;

        native_epg_change_mode(MODE_REMOVE, mode);
    }

    public DTVEpgScanner(int mode) {
        this.mode = mode;
    }

    public void setSource(int fend, int dmx, int src, String textLanguages) {
        if (created)
            destroy();

        fend_dev_id = fend;
        dmx_dev_id  = dmx;
        fend_type   = src;

        native_epg_create(fend, dmx, src, textLanguages);
        created = true;
    }

    public void destroy() {
        if (created) {
            native_epg_destroy();
            created = false;
        }
    }

    /*Enter a channel.*/
    public void enterChannel() {
        if (!created)
            return;

        leaveChannel();

        Log.d(TAG, "enter Channel");

        startScan(mode);
/*
        if (fend_type == TvChannelParams.MODE_ATSC) {
            startScan(SCAN_PSIP_EIT | SCAN_MGT | SCAN_VCT | SCAN_RRT | SCAN_STT);
        } else {
            startScan(SCAN_EIT_ALL | SCAN_TDT | SCAN_SDT);//SCAN_NIT|SCAN_CAT
        }
*/
    }

    /*Leave the channel.*/
    public void leaveChannel() {
        if (!created)
            return;

        Log.d(TAG, "leave Channel");

        stopScan(SCAN_ALL);
    }

    /*Enter the program.*/
    public void enterProgram(ChannelInfo channel) {
        /*do not check this, we need show all channel info to lower level.
        if (mChannel != null
            && channel.getServiceId() == mChannel.getServiceId()
            && channel.getTransportStreamId() == mChannel.getTransportStreamId()
            && channel.getOriginalNetworkId() == mChannel.getOriginalNetworkId())
            return;
        */

        if (!created)
            return;

        if (mChannel != null) {
            leaveProgram();
        }

        Log.d(TAG, "enter Program sid["+channel.getServiceId()+"] name["+channel.getDisplayName()+"]");
        Log.d(TAG, "\tapid:["+Arrays.toString(channel.getAudioPids())+"] spid:["+Arrays.toString(channel.getSubtitlePids())+"]");

        native_epg_monitor_service(channel);
        startScan(SCAN_PAT | SCAN_PMT);
        //startScan(SCAN_PAT);
        mChannel = channel;
    }

    /*Leave the program.*/
    public void leaveProgram() {
        if (mChannel == null)
            return;

        if (!created)
            return;

        Log.d(TAG, "leave Program");

        stopScan(SCAN_PAT|SCAN_PMT);
        native_epg_monitor_service(null);
        mChannel = null;
    }

    /*Set the default dvb text coding, 'standard' indicates using DVB text coding standard*/
    public void setDvbTextCoding(String coding) {
        native_epg_set_dvb_text_coding(coding);
    }

    public void rescanTDT() {
        if (!created)
            return;

        Log.d(TAG, "rescan TDT");

        stopScan(SCAN_TDT);
        startScan(SCAN_TDT);
    }

    abstract void onEvent(Event event);
}

