/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifdef SUPPORT_ADTV
#include <am_db.h>
#endif

#if !defined(_CTVDATABASE_H)
#define _CTVDATABASE_H
#include <unistd.h>
#include <stdlib.h>
#include <utils/String8.h>
#include <utils/Log.h>
#include <utils/Vector.h>
#include <utils/RefBase.h>
#include <CSqlite.h>

#include "CTvLog.h"


#define CTV_DATABASE_DEFAULT_XML        "/vendor/etc/tvconfig/tv_default.xml"

using namespace android;

class CTvDatabase: public CSqlite {
public:
    static const int DB_VERSION = 8;
    static const char *DB_VERSION_FIELD;

    static const char feTypes[][32];
    static const char srvTypes[][32];
    static const char vidFmts[][32];
    static const char audFmts[][32];
    static const char mods[][32];
    static const char bandwidths[][32];
    static const char lnbPowers[][32];
    static const char sig22K[][32];
    static const char tonebursts[][32];
    static const char diseqc10s[][32];
    static const char diseqc11s[][32];
    static const char motors[][32];
    static const char ofdmModes[][32];
    static const char atvVideoStds[][32];
    static const char atvAudioStds[][32];
    template<typename T>
    int StringToIndex(const T &t, const char *item)
    {
        if (item == NULL) return -1;
        int size = sizeof(t) / sizeof(t[0]);
        for (int i = 0; i < size; i++) {
            if (strcmp(t[i], item) == 0) return i;
        }
        return -1;
    }
public:
    CTvDatabase();
    //CTvDatabase(char* path, sqlite3 * h);
    static CTvDatabase *GetTvDb();
    static void deleteTvDb();
    ~CTvDatabase();
    int UnInitTvDb();
    int InitTvDb(const char *path);
    //showboz test
    class ChannelPara : public LightRefBase<ChannelPara> {
    public:
        int mode;
        int freq;
        int symbol_rate;
        int modulation;
        int bandwidth;
        int polar;
    };

    static int getChannelParaList(char *path, Vector<sp<ChannelPara> > &vcp);

    int importDbToXml();
    int importXmlToDB(const char *xmlPath);
    bool isAtv256ProgInsertForSkyworth();
    int insert256AtvProgForSkyworth();
    int ClearDbTable();
    int clearDbAllProgramInfoTable();
private:
    static CTvDatabase *mpDb;
    int isFreqListExist(void);
};

#endif  //_CTVDATABASE_H
