/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvEvent"

#include "CTvEvent.h"
#include "CTvDatabase.h"
#include "CTvProgram.h"
#include <stdlib.h>

void CTvEvent::InitFromCursor(CTvDatabase::Cursor &c)
{
    int col;

    col = c.getColumnIndex("db_id");
    this->id = c.getInt(col);

    col = c.getColumnIndex("event_id");
    this->dvbEventID = c.getInt(col);

    col = c.getColumnIndex("name");
    this->name = c.getString(col);

    col = c.getColumnIndex("start");
    this->start = (long)c.getInt(col);

    col = c.getColumnIndex("end");
    this->end = (long)c.getInt(col) ;

    col = c.getColumnIndex("nibble_level");
    this->dvbContent = c.getInt(col);

    col = c.getColumnIndex("parental_rating");
    this->dvbViewAge = c.getInt(col);

    col = c.getColumnIndex("sub_flag");
    this->sub_flag = c.getInt(col);

    col = c.getColumnIndex("db_srv_id");
    this->programID = c.getInt(col);

    col = c.getColumnIndex("rrt_ratings");
    String8 rrtRatings = c.getString(col);
    char *tmp;
    Vector<String8> ratings;
    char *pSave;
    tmp = strtok_r(rrtRatings.lockBuffer(rrtRatings.size()), ",", &pSave);
    LOGD("TV, %d, %s", __LINE__, tmp);
    while (tmp != NULL) {
        ratings.push_back(String8(tmp));
        tmp = strtok_r(NULL, ",", &pSave);
    }
    rrtRatings.unlockBuffer();
    rating_len = ratings.size();
    if (!ratings.isEmpty()) {
        for (int i = 0; i < (int)ratings.size(); i++) {
            Vector<String8> rating;
            tmp = strtok_r(ratings.editItemAt(i).lockBuffer(ratings.editItemAt(i).length()), " ", &pSave);
            while (tmp != NULL) {
                rating.push_back(String8(tmp));
                tmp = strtok_r(NULL, " ", &pSave);
            }
            ratings.editItemAt(i).unlockBuffer();
            if (rating.size() >= 3) {
                int re = atoi(rating[0]);
                int dm = atoi(rating[1]);
                int vl = atoi(rating[2]);
                vchipRatings.add( new CTvDimension::VChipRating(re, dm, vl));
            } else
                vchipRatings.add(NULL);
        }
    }

    col = c.getColumnIndex("descr");
    this->description = c.getString(col);

    col = c.getColumnIndex("ext_descr");
    this->extDescription = c.getString(col);
}

//id; CTvChannel.MODE_ATSC sourceid   , other   id
int CTvEvent::getProgPresentEvent(int progSrc, int progID, long nowTime, CTvEvent &ev)
{
    String8 cmd;
    CTvDatabase::Cursor c;

    cmd = String8("select * from evt_table where evt_table.");

    if (progSrc == CTvChannel::MODE_ATSC) {
        cmd += String8("source_id = ") + String8::format("%d", progID);
    } else {
        cmd += String8("db_srv_id = ") + String8::format("%d", progID);
    }

    cmd += String8(" and evt_table.start <= ") + String8::format("%ld", nowTime) + String8(" and evt_table.end > ") + String8::format("%ld", nowTime);

    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        ev.InitFromCursor(c);
    } else {
        c.close();
        return -1;
    }

    c.close();

    return 0;
}

int CTvEvent::getProgScheduleEvents(int progSrc, int progID, long start, long duration, Vector<sp<CTvEvent> > &vEv)
{
    String8 cmd;
    long begin = start;
    long end   = start + duration;

    cmd = String8("select * from evt_table where evt_table.");
    if (progSrc == CTvChannel::MODE_ATSC) {
        cmd += String8("source_id = ") + String8::format("%d", progID);
    } else {
        cmd += String8("db_srv_id = ") + String8::format("%d", progID);
    }
    cmd += String8(" and ");
    cmd += String8(" ((start < ") + String8::format("%ld", begin) + String8(" and end > ") + String8::format("%ld", begin) + String8(") ||");
    cmd += String8(" (start >= ") + String8::format("%ld", begin) + String8(" and start < ") + String8::format("%ld", end) + String8("))");
    cmd += String8(" order by evt_table.start");

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        do {
            vEv.add(new CTvEvent(c));
        } while (c.moveToNext());

    } else {
        c.close();
        return -1;
    }

    c.close();
    return 0;
}
int CTvEvent::getATVProgEvent(int progSrc, int progID, CTvEvent &ev)
{
    String8 cmd;
    CTvDatabase::Cursor c;

    cmd = String8("select * from evt_table where evt_table.");

    if (progSrc == CTvChannel::MODE_ATSC) {
        LOGD("%s, %d  MODE_ATSC", "TV", __LINE__);
        cmd += String8("source_id = ") + String8::format("%d", progID);
    } else {
        LOGD("%s, %d  MODE_ANALOG", "TV", __LINE__);
        cmd += String8("db_srv_id = ") + String8::format("%d", progID);
    }

    //cmd += String8(" and evt_table.start <= ") + String8::format("%ld", nowTime) + String8(" and evt_table.end > ") + String8::format("%ld", nowTime);

    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        ev.InitFromCursor(c);
    } else {
        c.close();
        return -1;
    }

    c.close();

    return 0;
}

int CTvEvent::CleanAllEvent()
{
    CTvDatabase::GetTvDb()->exeSql("delete from evt_table");
    return 0;
}

int CTvEvent::selectByID(int id, CTvEvent &evt)
{
    CTvDatabase::Cursor c;
    String8 sql;

    sql = String8("select * from evt_table where evt_table.db_id = ") + String8::format("%d", id);
    CTvDatabase::GetTvDb()->select(sql.string(), c);
    if (c.moveToFirst()) {
        evt.InitFromCursor(c);
    } else {
        c.close();
        return -1;
    }

    c.close();
    return 0;
}

int CTvEvent::bookEvent(int evtId, bool bBookFlag)
{
    String8 cmd;

    cmd = String8("update evt_table set sub_flag=") + String8::format("%d", bBookFlag)
          + String8(" where event_id=") + String8::format("%d", evtId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    return 0;
}

CTvEvent::CTvEvent(CTvDatabase::Cursor &c)
{
    InitFromCursor(c);
}

CTvEvent::CTvEvent()
{
    id = 0;
    dvbEventID = 0;
    programID = 0;
    start = false;
    end = false;
    dvbContent = 0;
    dvbViewAge = 0;
    sub_flag = 0;
    rating_len = 0;
}

CTvEvent::~CTvEvent()
{
    int size = vchipRatings.size();
    for (int i = 0; i < size; i++)
        delete vchipRatings[i];
}
Vector<CTvDimension::VChipRating *> CTvEvent::getVChipRatings()
{
    return vchipRatings;
}

