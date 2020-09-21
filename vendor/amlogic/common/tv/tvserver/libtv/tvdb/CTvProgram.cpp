/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */


#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvProgram"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#include "CTvProgram.h"
#include "CTvDatabase.h"
#include "CTvChannel.h"
#include "CTvEvent.h"

CTvProgram::CTvProgram(CTvDatabase::Cursor &c)
{
    CreateFromCursor(c);
}

CTvProgram::~CTvProgram()
{
    //free mem
    // video
    if (mpVideo != NULL) delete mpVideo;
    // audios
    int size = mvAudios.size();
    for (int i = 0; i < size; i++)
        delete mvAudios[i];
    // subtitles
    size = mvSubtitles.size();
    for (int i = 0; i < size; i++)
        delete mvSubtitles[i];
    // teletexts
    size = mvTeletexts.size();
    for (int i = 0; i < size; i++)
        delete mvTeletexts[i];
}

CTvProgram::CTvProgram(int channelID __unused, int type __unused)
{
    mpVideo = NULL;
    major = 0;
    minor = 0;
    minorCheck = 0;
    atscMode = false;
    id = 0;
    dvbServiceID = 0;
    type = 0;
    channelID = 0;
    skip = 0;
    favorite = 0;
    volume = 0;
    sourceID = 0;
    pmtPID = 0;
    src = 0;
    audioTrack = 0;
    chanOrderNum = 0;
    currAudTrackIndex = 0;
    lock = false;
    scrambled = false;
    pcrID = 0;
}

CTvProgram::CTvProgram()
{
    mpVideo = NULL;
    major = 0;
    minor = 0;
    minorCheck = 0;
    atscMode = false;
    id = 0;
    dvbServiceID = 0;
    type = 0;
    channelID = 0;
    skip = 0;
    favorite = 0;
    volume = 0;
    sourceID = 0;
    pmtPID = 0;
    src = 0;
    audioTrack = 0;
    chanOrderNum = 0;
    currAudTrackIndex = 0;
    lock = false;
    scrambled = false;
    pcrID = 0;
}

CTvProgram::CTvProgram(const CTvProgram& UNUSED(cTvProgram))
{
    major = 0;
    minor = 0;
    minorCheck = 0;
    atscMode = false;
    id = 0;
    dvbServiceID = 0;
    type = 0;
    channelID = 0;
    skip = 0;
    favorite = 0;
    volume = 0;
    sourceID = 0;
    pmtPID = 0;
    src = 0;
    audioTrack = 0;
    chanOrderNum = 0;
    currAudTrackIndex = 0;
    lock = false;
    scrambled = false;
    mpVideo = NULL;
    pcrID = 0;
}
//TODO
CTvProgram& CTvProgram::operator= (const CTvProgram& UNUSED(cTvProgram))
{
    return *this;
}

int CTvProgram::CreateFromCursor(CTvDatabase::Cursor &c)
{
    int i = 0;
    int col;
    int num;
    int major, minor;
    char tmp_buf[256];
    //LOGD("CTvProgram::CreateFromCursor");
    col = c.getColumnIndex("db_id");
    this->id = c.getInt(col);

    col = c.getColumnIndex("source_id");
    this->sourceID = c.getInt(col);

    col = c.getColumnIndex("src");
    this->src = c.getInt(col);

    col = c.getColumnIndex("service_id");
    this->dvbServiceID = c.getInt(col);

    col = c.getColumnIndex("db_ts_id");
    this->channelID = c.getInt(col);

    col = c.getColumnIndex("name");
    this->name = c.getString(col);

    col = c.getColumnIndex("chan_num");
    num = c.getInt(col);

    col = c.getColumnIndex("chan_order");
    this->chanOrderNum = c.getInt(col);

    col   = c.getColumnIndex("major_chan_num");
    major = c.getInt(col);

    col   = c.getColumnIndex("minor_chan_num");
    minor = c.getInt(col);

    col   = c.getColumnIndex("aud_track");
    this->audioTrack = c.getInt(col);

    if (src == CTvChannel::MODE_ATSC || (src == CTvChannel::MODE_ANALOG && major > 0)) {
        this->major = major;
        this->minor = minor;
        this->atscMode = true;
        this->minorCheck = MINOR_CHECK_NONE;
    } else {
        this->major = num;
        this->minor = 0;
        this->atscMode = false;
        this->minorCheck = MINOR_CHECK_NONE;
    }

    col = c.getColumnIndex("service_type");
    this->type = c.getInt(col);

    col = c.getColumnIndex("pmt_pid");
    pmtPID = c.getInt(col);

    //LOGD("CTvProgram::CreateFromCursor type = %d", this->type);
    col = c.getColumnIndex("skip");
    this->skip = c.getInt(col);

    col = c.getColumnIndex("lock");
    this->lock = (c.getInt(col) != 0);

    col = c.getColumnIndex("scrambled_flag");
    this->scrambled = (c.getInt(col) != 0);

    col = c.getColumnIndex("favor");
    this->favorite = (c.getInt(col) != 0);

    col = c.getColumnIndex("volume");
    this->volume =  c.getInt(col);

    //Video
    int pid, fmt;
    col = c.getColumnIndex("vid_pid");
    pid = c.getInt(col);

    col = c.getColumnIndex("vid_fmt");
    fmt = c.getInt(col);

    //LOGD("----------vpid = %d", pid);
    this->mpVideo = new Video(pid, fmt);
    //LOGD("----------vpid = %d", this->mpVideo->getPID());

    col = c.getColumnIndex("pcr_pid");
    this->pcrID = c.getInt(col);

    //Audio
    String8 strPids;
    String8 strFmts;
    String8 strLangs;
    //int count = 0;
    col = c.getColumnIndex("aud_pids");
    strPids = c.getString(col);

    col = c.getColumnIndex("aud_fmts");
    strFmts = c.getString(col);

    col = c.getColumnIndex("aud_langs");
    strLangs = c.getString(col);
    col = c.getColumnIndex("current_aud");
    this->currAudTrackIndex = c.getInt(col);

    char *tmp;
    Vector<String8> vpid ;
    Vector<String8> vfmt ;
    Vector<String8> vlang;

    char *pSave;
    tmp = strtok_r(strPids.lockBuffer(strPids.length()), " ", &pSave);
    while (tmp != NULL) {
        vpid.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strPids.unlockBuffer();

    tmp = strtok_r(strFmts.lockBuffer(strFmts.length()), " ", &pSave);
    while (tmp != NULL) {
        vfmt.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strFmts.unlockBuffer();

    tmp = strtok_r(strLangs.lockBuffer(strLangs.length()), " ", &pSave);

    while (tmp != NULL) {
        vlang.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strLangs.unlockBuffer();

    //check empty aud_langs
    for (i = vlang.size(); i < (int)vpid.size(); i++) {
        sprintf(tmp_buf, "Audio%d", i + 1);
        vlang.push_back(String8(tmp_buf));
        LOGE("%s, aud_langs is empty, add dummy data (%s).\n", "TV", tmp_buf);
    }

    //String8 l(vlang[i]);
    for (i = 0; i < (int)vpid.size(); i++) {
        int i_pid = atoi(vpid[i]);
        int i_fmt = atoi(vfmt[i]);
        mvAudios.push_back(new Audio(i_pid, vlang[i], i_fmt));
    }

    /* parse subtitles */
    vpid.clear();
    vlang.clear();
    Vector<String8> vcid;
    Vector<String8> vaid;
    String8 strCids;
    String8 strAids;

    col = c.getColumnIndex("sub_pids");
    strPids = c.getString(col);

    col = c.getColumnIndex("sub_composition_page_ids");
    strCids = c.getString(col);

    col = c.getColumnIndex("sub_ancillary_page_ids");
    strAids = c.getString(col);

    col = c.getColumnIndex("sub_langs");
    strLangs = c.getString(col);

    tmp = strtok_r(strPids.lockBuffer(strPids.length()), " ", &pSave);
    while (tmp != NULL) {
        vpid.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strPids.unlockBuffer();

    tmp = strtok_r(strCids.lockBuffer(strCids.length()), " ", &pSave);
    while (tmp != NULL) {
        vcid.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strCids.unlockBuffer();

    tmp = strtok_r(strAids.lockBuffer(strAids.length()), " ", &pSave);
    while (tmp != NULL) {
        vaid.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strAids.unlockBuffer();

    tmp = strtok_r(strLangs.lockBuffer(strLangs.length()), " ", &pSave);
    while (tmp != NULL) {
        vlang.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    strLangs.unlockBuffer();

    //Subtitle
    for (int i = 0; i < (int)vpid.size(); i++) {
        this->mvSubtitles.push_back(new Subtitle(
                                        atoi(vpid[i]),
                                        String8(vlang[i]), Subtitle::TYPE_DVB_SUBTITLE,
                                        atoi(vcid[i]),
                                        atoi(vaid[i])));
    }

    /*
    parse teletexts
    int ttx_count = 0, ttx_sub_count = 0;
    String8 str_ttx_pids, str_ttx_types, str_mag_nos, str_page_nos, str_ttx_langs;
    Vector<String8> v_ttx_pids, v_ttx_types, v_mag_nos, v_page_nos, v_ttx_langs;
    col = c.getColumnIndex("ttx_pids");
    str_ttx_pids = c.getString(col);

    col = c.getColumnIndex("ttx_types");
    str_ttx_types = c.getString(col);

    col = c.getColumnIndex("ttx_magazine_nos");
    str_mag_nos = c.getString(col);

    col = c.getColumnIndex("ttx_page_nos");
    str_page_nos = c.getString(col);

    col = c.getColumnIndex("ttx_langs");
    str_ttx_langs = c.getString(col);

    tmp = strtok_r(str_ttx_pids.lockBuffer(str_ttx_pids.length()), " ", &pSave);
    while (tmp != NULL) {
        v_ttx_pids.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    str_ttx_pids.unlockBuffer();

    tmp = strtok_r(str_ttx_types.lockBuffer(str_ttx_types.length()), " ", &pSave);
    while (tmp != NULL) {
        v_ttx_types.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    str_ttx_types.unlockBuffer();

    tmp = strtok_r(str_mag_nos.lockBuffer(str_mag_nos.length()), " ", &pSave);
    while (tmp != NULL) {
        v_mag_nos.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    str_mag_nos.unlockBuffer();

    tmp = strtok_r(str_page_nos.lockBuffer(str_page_nos.length()), " ", &pSave);
    while (tmp != NULL) {
        v_page_nos.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    str_page_nos.unlockBuffer();

    tmp = strtok_r(str_ttx_langs.lockBuffer(str_ttx_langs.length()), " ", &pSave);
    while (tmp != NULL) {
        v_ttx_langs.push_back(String8(tmp));
        tmp = strtok_r(NULL, " ", &pSave);
    }
    str_ttx_langs.unlockBuffer();

    for (int i = 0; i < v_ttx_pids.size(); i++) {
        int ttype = atoi(v_ttx_types[i]);
        if (ttype == 0x2 || ttype == 0x5) {
            this->mvSubtitles.push_back(new Subtitle(
                                            atoi(v_ttx_pids[i].string()),
                                            String8(v_ttx_langs[i]), Subtitle::TYPE_DTV_TELETEXT,
                                            atoi(v_mag_nos[i]),
                                            atoi(v_page_nos[i])));
        } else {
            this->mvTeletexts.push_back(new Teletext(
                                            atoi(v_ttx_pids[i]),
                                            String8(v_ttx_langs[i]),
                                            atoi(v_mag_nos[i]),
                                            atoi(v_page_nos[i])));
        }
    }
    */
    return 0;
}

int CTvProgram::selectProgramInChannelByNumber(int channelID, int num, CTvDatabase::Cursor &c)
{
    String8 cmd = String8("select * from srv_table where db_ts_id = ") + String8::format("%d", channelID) + String8(" and ");
    cmd += String8("chan_num = ") + String8::format("%d", num);
    return CTvDatabase::GetTvDb()->select(cmd, c);
}

int CTvProgram::selectProgramInChannelByNumber(int channelID, int major, int minor, CTvDatabase::Cursor &c)
{
    String8 cmd = String8("select * from srv_table where db_ts_id = ") + String8::format("%d", channelID) + String8(" and ");
    cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num = ") + String8::format("%d", minor);

    return CTvDatabase::GetTvDb()->select(cmd, c);
}

CTvProgram::CTvProgram(int channelID, int type, int num, int skipFlag)
{
    major = 0;
    minor = 0;
    minorCheck = 0;
    atscMode = false;

    dvbServiceID = 0;
    this->type = 0;
    this->channelID = 0;
    skip = 0;
    favorite = 0;
    volume = 0;
    sourceID = 0;
    pmtPID = 0;
    src = 0;
    audioTrack = 0;
    chanOrderNum = 0;
    currAudTrackIndex = 0;
    lock = false;
    scrambled = false;
    mpVideo = NULL;
    pcrID = 0;

    CTvChannel channel;
    int ret = CTvChannel::selectByID(channelID, channel);
    if (ret != 0) {
        //Log.d(TAG, "Cannot add new program, invalid channel id "+channelID);
        this->id = -1;
    } else {
        CTvDatabase::Cursor c ;
        selectProgramInChannelByNumber(channelID, num, c);

        if (c.moveToFirst()) {
            /*Construct*/
            CreateFromCursor(c);
        } else {
            String8 tpids = String8(""), ttypes = String8(""), tmagnums = String8(""), tpgnums = String8(""), tlangs = String8("");
            String8 spids = String8(""), stypes = String8(""), scpgids = String8(""), sapgids = String8(""), slangs = String8("");

            /*add a new atv program to database*/
            String8 cmd = String8("insert into srv_table(db_net_id,db_ts_id,service_id,src,name,service_type,");
            cmd += String8("eit_schedule_flag,eit_pf_flag,running_status,free_ca_mode,volume,aud_track,vid_pid,");
            cmd += String8("vid_fmt,aud_pids,aud_fmts,aud_langs,skip,lock,chan_num,major_chan_num,");
            cmd += String8("minor_chan_num,access_controlled,hidden,hide_guide,source_id,favor,current_aud,");
            cmd += String8("current_sub,sub_pids,sub_types,sub_composition_page_ids,sub_ancillary_page_ids,sub_langs,");
            cmd += String8("current_ttx,ttx_pids,ttx_types,ttx_magazine_nos,ttx_page_nos,ttx_langs,");
            cmd += String8("db_sat_para_id,scrambled_flag,lcn,hd_lcn,sd_lcn,default_chan_num,chan_order) ");
            cmd += String8("values(-1,") + String8::format("%d", channelID) + String8(",65535,") + String8::format("%d", channel.getMode()) + String8(",'',") + String8::format("%d", type) + String8(",");
            cmd += String8("0,0,0,0,0,0,8191,");
            int chanNum = num;
            int majorNum = 0;
            cmd += String8("-1,'','','',") + String8::format("%d", skipFlag) + String8(",0,") + String8::format("%d", chanNum) + String8(",") + String8::format("%d", majorNum) + String8(",");
            cmd += String8("") + /*num.getMinor()*/String8("0") + String8(",0,0,0,-1,0,-1,");
            cmd += String8("-1,'") + spids + String8("','") + stypes + String8("','") + scpgids + String8("','") + sapgids + String8("','") + slangs + String8("',");
            cmd += String8("-1,'") + tpids + String8("','") + ttypes + String8("','") + tmagnums + String8("','") + tpgnums + String8("','") + tlangs + String8("',");
            cmd += String8("-1,0,-1,-1,-1,-1,0)");
            CTvDatabase::GetTvDb()->exeSql(cmd.string());

            CTvDatabase::Cursor cr;
            selectProgramInChannelByNumber(channelID, num, cr);
            if (cr.moveToFirst()) {
                /*Construct*/
                CreateFromCursor(cr);
            } else {
                /*A critical error*/
                //Log.d(TAG, "Cannot add new program, sqlite error");
                this->id = -1;
            }
            cr.close();
        }
        c.close();
    }

}

CTvProgram::CTvProgram(int channelID, int type, int major, int minor, int skipFlag)
{
    this->major = 0;
    this->minor = 0;
    minorCheck = 0;
    atscMode = false;

    dvbServiceID = 0;
    this->type = 0;
    this->channelID = 0;
    skip = 0;
    favorite = 0;
    volume = 0;
    sourceID = 0;
    pmtPID = 0;
    src = 0;
    audioTrack = 0;
    chanOrderNum = 0;
    currAudTrackIndex = 0;
    lock = false;
    scrambled = false;
    mpVideo = NULL;
    pcrID = 0;

    CTvChannel channel;
    int ret = CTvChannel::selectByID(channelID, channel);
    if (ret != 0) {
        //Log.d(TAG, "Cannot add new program, invalid channel id "+channelID);
        this->id = -1;
    } else {
        CTvDatabase::Cursor c ;
        selectProgramInChannelByNumber(channelID, major, minor, c);

        if (c.moveToFirst()) {
            /*Construct*/
            CreateFromCursor(c);
        } else {
            String8 tpids = String8(""), ttypes = String8(""), tmagnums = String8(""), tpgnums = String8(""), tlangs = String8("");
            String8 spids = String8(""), stypes = String8(""), scpgids = String8(""), sapgids = String8(""), slangs = String8("");

            /*add a new atv program to database*/
            String8 cmd = String8("insert into srv_table(db_net_id,db_ts_id,service_id,src,name,service_type,");
            cmd += String8("eit_schedule_flag,eit_pf_flag,running_status,free_ca_mode,volume,aud_track,vid_pid,");
            cmd += String8("vid_fmt,aud_pids,aud_fmts,aud_langs,skip,lock,chan_num,major_chan_num,");
            cmd += String8("minor_chan_num,access_controlled,hidden,hide_guide,source_id,favor,current_aud,");
            cmd += String8("current_sub,sub_pids,sub_types,sub_composition_page_ids,sub_ancillary_page_ids,sub_langs,");
            cmd += String8("current_ttx,ttx_pids,ttx_types,ttx_magazine_nos,ttx_page_nos,ttx_langs,");
            cmd += String8("db_sat_para_id,scrambled_flag,lcn,hd_lcn,sd_lcn,default_chan_num,chan_order) ");
            cmd += String8("values(-1,") + String8::format("%d", channelID) + String8(",65535,") + String8::format("%d", channel.getMode()) + String8(",'',") + String8::format("%d", type) + String8(",");
            cmd += String8("0,0,0,0,0,0,8191,");
            int chanNum = major << 16 | minor;
            int majorNum = major;
            cmd += String8("-1,'','','',") + String8::format("%d", skipFlag) + String8(",0,") + String8::format("%d", chanNum) + String8(",") + String8::format("%d", majorNum) + String8(",");
            cmd += String8("") + String8::format("%d", minor) + String8(",0,0,0,-1,0,-1,");
            cmd += String8("-1,'") + spids + String8("','") + stypes + String8("','") + scpgids + String8("','") + sapgids + String8("','") + slangs + String8("',");
            cmd += String8("-1,'") + tpids + String8("','") + ttypes + String8("','") + tmagnums + String8("','") + tpgnums + String8("','") + tlangs + String8("',");
            cmd += String8("-1,0,-1,-1,-1,-1,0)");
            CTvDatabase::GetTvDb()->exeSql(cmd.string());

            CTvDatabase::Cursor cr;
            selectProgramInChannelByNumber(channelID, major, minor, cr);
            if (cr.moveToFirst()) {
                /*Construct*/
                CreateFromCursor(cr);
            } else {
                /*A critical error*/
                //Log.d(TAG, "Cannot add new program, sqlite error");
                this->id = -1;
            }
            cr.close();
        }
        c.close();
    }

}

int CTvProgram::selectByID(int id, CTvProgram &prog)
{
    CTvDatabase::Cursor c;
    String8 sql;
    sql = String8("select * from srv_table where srv_table.db_id = ") + String8::format("%d", id);
    CTvDatabase::GetTvDb()->select(sql.string(), c) ;
    if (c.moveToFirst()) {
        prog.CreateFromCursor(c);
    } else {
        c.close();
        return -1;
    }
    c.close();
    return 0;
}

int CTvProgram::updateVolComp(int progID, int volValue)
{
    String8 cmd = String8("update srv_table set volume = ") + String8::format("%d", volValue) +
                  String8(" where srv_table.db_id = ") + String8::format("%d", progID);
    LOGD("%s, cmd = %s\n", "TV", cmd.string());
    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    return 0;
}

int CTvProgram::getChannel(CTvChannel &c)
{
    return CTvChannel::selectByID(channelID, c);
}

int CTvProgram::upDateChannel(CTvChannel &c __unused, int std, int Freq, int fineFreq)
{
    return CTvChannel::updateByID(channelID, std, Freq, fineFreq);
}

int CTvProgram::deleteChannelsProgram(CTvChannel &c)
{
    String8 cmd;
    cmd = String8("delete  from srv_table where srv_table.db_ts_id = ") + String8::format("%d", c.getID());
    CTvDatabase::GetTvDb()->exeSql(cmd.string());
    return 0;
}

int CTvProgram::selectByNumber(int type, int num, CTvProgram &prog)
{
    String8 cmd;

    cmd = String8("select * from srv_table where ");
    if (type != TYPE_UNKNOWN) {
        if (type == TYPE_DTV) {
            cmd += String8("(service_type = ") + String8::format("%d", TYPE_DTV) + String8(" or service_type = ") + String8::format("%d", TYPE_RADIO) + String8(") and ");
        } else {
            cmd += String8("service_type = ") + String8::format("%d", type) + String8(" and ");
        }
    }

    cmd += String8("chan_num = ") + String8::format("%d", num);

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        prog.CreateFromCursor(c);
    } else {
        c.close();
        return -1;
    }
    c.close();

    return 0;
}

int CTvProgram::selectByNumber(int type, int major, int minor, CTvProgram &prog, int minor_check)
{
    String8 cmd;

    cmd = String8("select * from srv_table where ");
    if (type != TYPE_UNKNOWN) {
        if (type == TYPE_DTV) {
            cmd += String8("(service_type = ") + String8::format("%d", TYPE_TV) + String8(" or service_type = ") + String8::format("%d", TYPE_RADIO) + String8(") and ");
        } else {
            cmd += String8("service_type = ") + String8::format("%d", type) + String8(" and ");
        }
    }

    if (minor < 0) {
        /*recursive call*/
        /*select dtv program first*/
        //showbo
        int ret = selectByNumber(TYPE_DTV, major, 1, prog, MINOR_CHECK_NEAREST_UP);
        if (ret != 0) {
            /*then try atv program*/
            selectByNumber(TYPE_ATV, major, 0 , prog, MINOR_CHECK_NONE);
        }
        return 0;
    } else if (minor >= 1) {
        if (minor_check == MINOR_CHECK_UP) {
            cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num >= ") + String8::format("%d", minor) + String8(" ");
            cmd += String8("order by minor_chan_num DESC limit 1");
        } else if (minor_check == MINOR_CHECK_DOWN) {
            cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num <= ") + String8::format("%d", minor) + String8(" ");
            cmd += String8("order by minor_chan_num limit 1");
        } else if (minor_check == MINOR_CHECK_NEAREST_UP) {
            cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num >= ") + String8::format("%d", minor) + String8(" ");
            cmd += String8("order by minor_chan_num limit 1");
        } else if (minor_check == MINOR_CHECK_NEAREST_DOWN) {
            cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num <= ") + String8::format("%d", minor) + String8(" ");
            cmd += String8("order by minor_chan_num DESC limit 1");
        } else {
            cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num = ") + String8::format("%d", minor);
        }
    } else {
        cmd += String8("major_chan_num = ") + String8::format("%d", major) + String8(" and minor_chan_num = ") + String8::format("%d", minor);
    }


    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        prog.CreateFromCursor(c);
    } else {
        c.close();
        return -1;
    }
    c.close();

    return 0;
}

int CTvProgram::selectByChannel(int channelID, int type, Vector<sp<CTvProgram> > &out)
{
    //Vector<CTvProgram*> vp;
    String8 cmd = String8("select * from srv_table ");

    if (type == TYPE_DTV) {
        cmd += String8("where (service_type = ") + String8::format("%d", TYPE_TV) + String8(" or service_type = ") + String8::format("%d", TYPE_RADIO) + String8(") ");
    } else if (type != TYPE_UNKNOWN) {
        cmd += String8("where service_type = ") + String8::format("%d", type) + String8(" ");
    }

    cmd += String8(" and db_ts_id = ") + String8::format("%d", channelID) + String8(" order by chan_order");

    CTvDatabase::Cursor c;
    int ret = CTvDatabase::GetTvDb()->select(cmd, c);

    LOGD("selectByChannel select ret = %d", ret);

    if (c.moveToFirst()) {
        do {
            out.add(new CTvProgram(c));
        } while (c.moveToNext());
    }
    c.close();

    return 0;
}

void CTvProgram::deleteProgram(int progId)
{
    String8 cmd;

    cmd = String8("delete  from srv_table where srv_table.db_id = ") + String8::format("%d", progId);
    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

int CTvProgram::CleanAllProgramBySrvType(int srvType)
{
    String8 cmd = String8("delete  from srv_table where service_type = ") + String8::format("%d", srvType);
    CTvDatabase::GetTvDb()->exeSql(cmd.string());
    return 0;
}

int CTvProgram::selectByType(int type, int skip, Vector<sp<CTvProgram> > &out)
{
    String8 cmd = String8("select * from srv_table ");
    LOGD("%s, type= %d\n", "TV", (int)type);

    if (type == TYPE_UNKNOWN) {
        cmd += String8("where (service_type = ") + String8::format("%d", TYPE_ATV) +
               String8(" or service_type = ") + String8::format("%d", TYPE_DTV) +
               String8(" or service_type = ") + String8::format("%d", TYPE_RADIO) +
               String8(") ");
    } else {
        cmd += String8("where service_type = ") + String8::format("%d", type) + String8(" ");
    }

    if (skip == PROGRAM_SKIP_NO || skip == PROGRAM_SKIP_YES)
        cmd += String8(" and skip = ") + String8::format("%d", skip) + String8(" ");

    if (type == TYPE_DTV || type == TYPE_RADIO) {
        cmd += String8(" order by db_ts_id");
    } else {
        cmd += String8(" order by chan_order");
    }

    CTvDatabase::Cursor c;
    int ret = CTvDatabase::GetTvDb()->select(cmd, c);

    LOGD("selectByType select ret = %d", ret);

    if (c.moveToFirst()) {
        do {
            out.add(new CTvProgram(c));
        } while (c.moveToNext());
    }
    c.close();

    return 0;
}

int CTvProgram::selectByChanID(int type, int skip, Vector < sp < CTvProgram > > &out)
{
    String8 cmd = String8("select * from srv_table ");

    if (type == TYPE_UNKNOWN) {
        cmd += String8("where (service_type = ") + String8::format("%d", TYPE_ATV) +
               String8(" or service_type = ") + String8::format("%d", TYPE_DTV) +
               String8(") ");
    } else {
        cmd += String8("where service_type = ") + String8::format("%d", type) + String8(" ");
    }

    if (skip == PROGRAM_SKIP_NO || skip == PROGRAM_SKIP_YES)
        cmd += String8(" and skip = ") + String8::format("%d", PROGRAM_SKIP_NO) +
               String8(" or skip = ") + String8::format("%d", PROGRAM_SKIP_YES) + String8(" ");

    cmd += String8(" order by db_id");

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        do {
            out.add(new CTvProgram(c));
        } while (c.moveToNext());
    }
    c.close();

    return 0;
}

int CTvProgram::selectAll(bool no_skip, Vector<sp<CTvProgram> > &out)
{
    return selectByType(TYPE_UNKNOWN, no_skip, out);
}

Vector<CTvProgram> CTvProgram::selectByChannel(int channelID __unused)
{
    Vector<CTvProgram> vProg;
    return vProg;
}

Vector<CTvProgram> CTvProgram::selectByName(int name __unused)
{
    Vector<CTvProgram> vProg;
    return vProg;
}

void CTvProgram::tvProgramDelByChannelID(int UNUSED(channelID))
{
    //TODO
}

String8 CTvProgram::getName()
{
    return name;
}

int CTvProgram::getProgSkipFlag()
{
    return skip;
}

int CTvProgram::getCurrentAudio(String8 defaultLang)
{
    CTvDatabase::Cursor c;
    String8 cmd;
    cmd = String8("select current_aud from srv_table where db_id = ") + String8::format("%d", this->id);
    CTvDatabase::GetTvDb()->select(cmd, c);
    LOGD("getCurrentAudio a size = %d", mvAudios.size());
    int id = 0;
    if (c.moveToFirst()) {
        id = c.getInt(0);
        LOGD("getCurrentAudio a id = %d", id);
        if (id < 0 && mvAudios.size() > 0) {
            LOGD("getCurrentAudio defaultLang.isEmpty() =%s= %d", defaultLang.string(), defaultLang.isEmpty());
            if (!defaultLang.isEmpty()) {
                for (int i = 0; i < (int)mvAudios.size(); i++) {
                    LOGD("getCurrentAudio a mvAudios[i] = %x", (long)mvAudios[i]);
                    if (mvAudios[i]->getLang() == defaultLang) {
                        id = i;
                        break;
                    }
                }
            }

            if (id < 0) {
                /* still not found, using the first */
                id = 0;
            }
        }
    }
    LOGD("getCurrentAudio a idsss = %d", id);
    c.close();

    return id;
}

int CTvProgram::getAudioChannel()
{
    return audioTrack;
}

int CTvProgram::updateAudioChannel(int progId, int ch)
{
    String8 cmd;

    cmd = String8("update srv_table set aud_track =") + "\'" + String8::format("%d", ch) + "\'"
          + String8(" where srv_table.db_id = ") + String8::format("%d", progId);

    return CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

int CTvProgram::getSubtitleIndex(int progId)
{
    CTvDatabase::Cursor c;
    String8 cmd = String8("select current_sub from srv_table where db_id = ") + String8::format("%d", progId);
    CTvDatabase::GetTvDb()->select(cmd, c);
    if (c.moveToFirst()) {
        return c.getInt(0);
    } else {
        return -1;
    }
}

int CTvProgram::setSubtitleIndex(int progId, int index)
{
    String8 cmd = String8("update srv_table set current_sub = ") + String8::format("%d", index) + String8(" where db_id = ") + String8::format("%d", progId);
    return CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

int CTvProgram::getCurrAudioTrackIndex()
{
    int audTrackIdx = -1;

    if (-1 == currAudTrackIndex) { // no set field "current_aud"
        audTrackIdx = getCurrentAudio(String8("eng"));
        setCurrAudioTrackIndex(this->id, audTrackIdx);
    } else {
        audTrackIdx = currAudTrackIndex;
    }

    return audTrackIdx;
}

void CTvProgram::setCurrAudioTrackIndex(int programId, int audioIndex)
{
    String8 cmd;

    cmd = String8("update srv_table set current_aud = ")
          + String8::format("%d", audioIndex) + String8(" where srv_table.db_id = ") + String8::format("%d", programId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

void CTvProgram::setFavoriteFlag(int progId, bool bFavor)
{
    String8 cmd;

    cmd = String8("update srv_table set favor = ")
          + String8::format("%d", bFavor ? 1 : 0) + String8(" where srv_table.db_id = ") + String8::format("%d", progId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

void CTvProgram::setSkipFlag(int progId, bool bSkipFlag)
{
    String8 cmd;

    cmd = String8("update srv_table set skip = ") + String8::format("%d", bSkipFlag ? 1 : 0)
          + String8(" where srv_table.db_id = ") + String8::format("%d", progId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

void CTvProgram::updateProgramName(int progId, String8 strName)
{
    String8 newName = String8("xxx") + strName;
    String8 cmd;

    cmd = String8("update srv_table set name =") + "\'" + String8::format("%s", newName.string()) + "\'"
          + String8(" where srv_table.db_id = ") + String8::format("%d", progId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

void CTvProgram::swapChanOrder(int ProgId1, int chanOrderNum1, int ProgId2, int chanOrderNum2)
{
    String8 cmd;

    cmd = String8("update srv_table set chan_order = ") + String8::format("%d", chanOrderNum2)
          + String8(" where db_id = ") + String8::format("%d", ProgId1);
    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    String8 cmd2;
    cmd2 = String8("update srv_table set chan_order = ") + String8::format("%d", chanOrderNum1)
           + String8(" where db_id = ") + String8::format("%d", ProgId2);
    CTvDatabase::GetTvDb()->exeSql(cmd2.string());
}

void CTvProgram::setLockFlag(int progId, bool bLockFlag)
{
    String8 cmd;

    cmd = String8("update srv_table set lock = ") + String8::format("%d", bLockFlag ? 1 : 0)
          + String8(" where srv_table.db_id = ") + String8::format("%d", progId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}

bool CTvProgram::getLockFlag()
{
    return lock;
}

