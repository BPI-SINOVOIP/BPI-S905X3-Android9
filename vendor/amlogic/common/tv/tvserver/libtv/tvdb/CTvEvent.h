/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVEVENT_H)
#define _CTVEVENT_H

#include <utils/Vector.h>
#include "CTvProgram.h"
#include "CTvDatabase.h"
#include "CTvDimension.h"

class CTvEvent : public LightRefBase<CTvEvent> {
public:
    CTvEvent(CTvDatabase::Cursor &c);
    CTvEvent();
    ~CTvEvent();

    int getProgPresentEvent(int progSrc, int progID, long nowTime, CTvEvent &ev);
    int getProgScheduleEvents(int progSrc, int progID, long start, long duration, Vector<sp<CTvEvent> > &vEv);
    int getATVProgEvent(int progSrc, int progID, CTvEvent &ev);
    int bookEvent(int evtId, bool bBookFlag);
    static int selectByID(int id, CTvEvent &p);
    static int CleanAllEvent();
    String8 &getName()
    {
        return name;
    };
    String8 &getDescription()
    {
        return description;
    };
    String8 &getExtDescription()
    {
        return extDescription;
    };
    long getStartTime()
    {
        return start;
    };
    long getEndTime()
    {
        return end;
    };
    int getSubFlag()
    {
        return sub_flag;
    };
    int getProgramId()
    {
        return programID;
    };
    int getEventId()
    {
        return dvbEventID;
    };
    Vector<CTvDimension::VChipRating *> getVChipRatings();

private:
    void InitFromCursor(CTvDatabase::Cursor &c);

    int id;
    int dvbEventID;
    String8 name;
    String8 description;
    String8 extDescription;
    int programID;
    long start;
    long end;
    int dvbContent;
    int dvbViewAge;
    int sub_flag;
    int rating_len;
    Vector<CTvDimension::VChipRating *> vchipRatings;
};

#endif  //_CTVEVENT_H
