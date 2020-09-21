/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVREGION_H)
#define _CTVREGION_H

#include "CTvDatabase.h"
#include "CTvChannel.h"
#include <utils/String8.h>
#include <utils/Vector.h>

#define COUNTRY_NAME_LEN        3
using namespace android;
class CTvRegion {
public:
    int id;
    String8 name;
    String8 country;
    CTvRegion(CTvDatabase db);
    CTvRegion();
    ~CTvRegion();
    static CTvRegion selectByID();
    static int getChannelListByName(char *name, Vector<sp<CTvChannel> > &vcp);
    static int getChannelListByNameAndFreqRange(char *name, int beginFreq, int endFreq, Vector<sp<CTvChannel> > &vcp);
    static int getLogicNumByNameAndfreq(char *name, int freq);
    static char* getTvCountry();
    static void  setTvCountry(const char *country);
    Vector<String8> getAllCountry();
    //CTvChannel getChannels();

    private:
        static char stCountry[COUNTRY_NAME_LEN];
};

#endif  //_CTVREGION_H
