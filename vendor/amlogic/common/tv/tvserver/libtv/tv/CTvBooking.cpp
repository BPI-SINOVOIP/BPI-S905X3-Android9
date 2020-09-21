/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvBooking"

#include "CTvBooking.h"
#include "CTvDatabase.h"

int CTvBooking::InitFromCursor(CTvDatabase::Cursor &c)
{
    int col;

    col = c.getColumnIndex("db_id");
    this->id = c.getInt(col);

    col = c.getColumnIndex("db_srv_id");
    this->programId = c.getInt(col);

    col = c.getColumnIndex("db_evt_id");
    this->eventId = c.getInt(col);

    col = c.getColumnIndex("flag");
    this->flag = c.getInt(col);

    col = c.getColumnIndex("status");
    this->status = c.getInt(col);

    col = c.getColumnIndex("repeat");
    this->repeat = c.getInt(col);

    col = c.getColumnIndex("start");
    this->start = (long)c.getInt(col);

    col = c.getColumnIndex("duration");
    this->duration = (long)c.getInt(col) ;

    col = c.getColumnIndex("srv_name");
    this->progName = c.getString(col);

    col = c.getColumnIndex("evt_name");
    this->evtName = c.getString(col);

    return 0;
}

int CTvBooking::selectByID(int id, CTvBooking &CtvBook)
{
    CTvDatabase::Cursor c;
    String8 sql;

    sql = String8("select * from booking_table where booking_table.db_evt_id = ") + String8::format("%d", id);
    CTvDatabase::GetTvDb()->select(sql.string(), c);
    if (c.moveToFirst()) {
        CtvBook.InitFromCursor(c);
    } else {
        c.close();
        return -1;
    }

    c.close();
    return 0;
}

int CTvBooking::getBookedEventList(Vector<sp<CTvBooking> > &vBv)
{
    String8 cmd;

    cmd = String8("select * from booking_table  order by start");

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        do {
            vBv.add(new CTvBooking(c));
        } while (c.moveToNext());


    } else {
        c.close();
        return -1;
    }

    c.close();
    return 0;

}

int CTvBooking::bookProgram(CTvProgram &prog, CTvEvent &evt)
{
    String8 cmd;
    String8 progName = String8(prog.getName());
    String8 evtName = String8(evt.getName());

    if (!prog.getAudio(0)) {
        return -1;
    }
    /*book this program*/
    cmd = String8("insert into booking_table(db_srv_id, db_evt_id, srv_name, evt_name,")
          + String8("start,duration,flag,status,file_name,vid_pid,vid_fmt,aud_pids,aud_fmts,aud_languages,")
          + String8("sub_pids,sub_types,sub_composition_page_ids,sub_ancillary_page_ids,sub_languages,")
          + String8("ttx_pids,ttx_types,ttx_magazine_numbers,ttx_page_numbers,ttx_languages, other_pids,from_storage,repeat)")
          + String8("values(") + String8::format("%d,", prog.getID()) + String8::format("%d,", evt.getEventId()) + progName.string() + String8(",") + evtName.string()
          + String8::format("%ld,", evt.getStartTime()) + String8::format("%ld,", evt.getEndTime() - evt.getStartTime()) + String8::format("%d,", flag)
          + String8::format("%d,", status) + String8(",") + String8::format("%d,", prog.getVideo()->getPID()) + String8::format("%d,", prog.getVideo()->getFormat())
          + String8::format("%d,", prog.getAudio(0)->getPID()) + String8::format("%d,", prog.getAudio(0)->getFormat()) + prog.getAudio(0)->getLang().string()
          + String8(" , , , , , , , , , , , , ,)");

    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    return 0;
}

int CTvBooking::deleteBook(int evtIdFlag)
{
    String8 cmd;

    cmd = String8("delete from booking_table where db_evt_id=")
          + String8::format("%d", evtIdFlag);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    return 0;
}

int CTvBooking::bookEvent(int evtId, bool bBookFlag)
{
    String8 cmd;

    cmd = String8("update evt_table set sub_flag=") + String8::format("%d", bBookFlag)
          + String8(" where db_id=") + String8::format("%d", evtId);

    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    if (true == bBookFlag) {
        CTvEvent evt;
        CTvEvent::selectByID(evtId, evt);

        CTvProgram prog;
        CTvProgram::selectByID(evt.getProgramId(), prog);

        bookProgram(prog, evt);
    } else {
        deleteBook(evtId);
    }

    return 0;

}

CTvBooking::CTvBooking(CTvDatabase::Cursor &c)
{
    InitFromCursor(c);
}

CTvBooking::CTvBooking()
{
    id = 0;
    programId = 0;
    eventId = 0;
    flag = 0;
    status = 0;
    repeat = 0;
    start = 0;
    duration = 0;
    progName = String8("");
    evtName = String8("");
}

CTvBooking::~CTvBooking()
{
}


