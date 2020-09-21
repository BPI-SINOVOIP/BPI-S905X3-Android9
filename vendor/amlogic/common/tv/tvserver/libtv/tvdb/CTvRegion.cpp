/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */


#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvRegion"

#include "CTvRegion.h"
#include "CTvDatabase.h"

char CTvRegion::stCountry[COUNTRY_NAME_LEN] = "US";
CTvRegion::CTvRegion(CTvDatabase db __unused)
{
    id = 0;
    name = String8("");
    country = String8("");
}

CTvRegion::CTvRegion()
{
    id = 0;
    name = String8("");
    country = String8("");
}

CTvRegion::~CTvRegion()
{
}

CTvRegion CTvRegion::selectByID()
{
    CTvRegion r;
    return r;
}

int CTvRegion::getChannelListByName(char *name, Vector<sp<CTvChannel> > &vcp)
{
    if (name == NULL)
        return -1;

    String8 cmd;
    cmd = String8("select * from region_table where name = ") + String8("\'") + name + String8("\'");

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    int col, size = 0;
    int id;
    int mode;
    int frequency = 0;
    int bandwidth;
    int modulation;
    int symbolRate;
    int ofdmMode;
    int channelNum = 0;
    //add dtmb extral channel frequency
    String8 physicalNumDisplayName;

    if (c.moveToFirst()) {
        do {
            col = c.getColumnIndex("db_id");
            id = c.getInt(col);
            col = c.getColumnIndex("fe_type");
            mode = c.getInt(col);
            col = c.getColumnIndex("frequency");
            frequency = c.getInt(col);
            col = c.getColumnIndex("modulation");
            modulation = c.getInt(col);
            col = c.getColumnIndex("bandwidth");
            bandwidth = c.getInt(col);
            col = c.getColumnIndex("symbol_rate");
            symbolRate = c.getInt(col);
            col = c.getColumnIndex("ofdm_mode");
            ofdmMode = c.getInt(col);
            col = c.getColumnIndex("logical_channel_num");
            channelNum = c.getInt(col);
            col = c.getColumnIndex("display");
            physicalNumDisplayName = c.getString(col);//add dtmb extral channel displayname
            CTvChannel *tempCTvChannel = new CTvChannel(id, mode, frequency, bandwidth, modulation, symbolRate, ofdmMode, channelNum);
            if (tempCTvChannel) {
              tempCTvChannel->setPhysicalNumDisplayName(physicalNumDisplayName);
            }
            vcp.add(tempCTvChannel);
            size++;
        } while (c.moveToNext());
    }
    c.close();

    return size;
}

int CTvRegion::getChannelListByNameAndFreqRange(char *name, int beginFreq, int endFreq, Vector<sp<CTvChannel> > &vcp)
{
    if (name == NULL)
        return -1;
    int ret = 0;
    String8 cmd;
    cmd = String8("select * from region_table where name = ") + String8("\'") + name + String8("\'")
          + String8(" and frequency >= ") + String8::format("%d", beginFreq) + String8(" and frequency <= ")
          + String8::format("%d", endFreq);

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    int col, size = 0;
    int id;
    int mode;
    int frequency = 0;
    int bandwidth;
    int modulation;
    int symbolRate;
    int ofdmMode;
    int channelNum = 0;

    do {
        if (c.moveToFirst()) {
            do {
                col = c.getColumnIndex("db_id");
                id = c.getInt(col);
                col = c.getColumnIndex("fe_type");
                mode = c.getInt(col);
                col = c.getColumnIndex("frequency");
                frequency = c.getInt(col);
                col = c.getColumnIndex("modulation");
                modulation = c.getInt(col);
                col = c.getColumnIndex("bandwidth");
                bandwidth = c.getInt(col);
                col = c.getColumnIndex("symbol_rate");
                symbolRate = c.getInt(col);
                col = c.getColumnIndex("ofdm_mode");
                ofdmMode = c.getInt(col);
                col = c.getColumnIndex("logical_channel_num");
                channelNum = c.getInt(col);
                vcp.add(new CTvChannel(id, mode, frequency, bandwidth, modulation, symbolRate, ofdmMode, channelNum));
                size++;
            } while (c.moveToNext());
        } else {
            ret = -1;
            break;
        }
    } while (false);

    c.close();
    return ret;
}

char* CTvRegion::getTvCountry()
{
    return stCountry;
}

void CTvRegion::setTvCountry(const char* country)
{
    if (NULL != country) {
        strncpy(stCountry, country, COUNTRY_NAME_LEN-1);
    }
}

Vector<String8> CTvRegion::getAllCountry()
{
    Vector<String8> vStr;
    return vStr;
}

//CTvChannel CTvRegion::getChannels()
//{
//    CTvChannel p;
//    return p;
//}

int CTvRegion::getLogicNumByNameAndfreq(char *name, int freq)
{
    int ret = 0;
    int col = 0;

    if (name == NULL)
        return -1;

    String8 cmd;
    cmd = String8("select * from region_table where name = ") + String8("\'") + name + String8("\'");
    cmd += String8(" and frequency = ") + String8::format("%d", freq);


    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    if (c.moveToFirst()) {
        col = c.getColumnIndex("logical_channel_num");
        ret = c.getInt(col);
    }
    c.close();

    return ret;
}

