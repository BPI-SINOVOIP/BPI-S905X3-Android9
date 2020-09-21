/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CSqlite"

#include "include/CSqlite.h"
#include "include/CTvLog.h"

using namespace android;

CSqlite::CSqlite()
{
    mHandle = NULL;
}

CSqlite::~CSqlite()
{
#ifdef SUPPORT_ADTV
    if (mHandle != NULL) {
        sqlite3_close(mHandle);
        mHandle = NULL;
    }
#endif
}

bool CSqlite::integrityCheck()
{
#ifdef SUPPORT_ADTV
    char *err;
    int rval = sqlite3_exec(mHandle, "PRAGMA integrity_check;", sqlite3_exec_callback, NULL, &err);
    if (rval != SQLITE_OK) {
        LOGE(" val = %d, msg = %s!\n", rval, sqlite3_errmsg(mHandle));
        return false;
    }
#endif
    return true;
}

int CSqlite::sqlite3_exec_callback(void *data __unused, int nColumn, char **colValues __unused, char **colNames __unused)
{
    LOGD("sqlite3_exec_callback, nums = %d", nColumn);
    //for (int i = 0; i < nColumn; i++) {
    //    LOGD("%s\t", colValues[i]);
    //}
    return 0;
}

int CSqlite::openDb(const char *path)
{
#ifdef SUPPORT_ADTV
    if (sqlite3_open(path, &mHandle) != SQLITE_OK) {
        LOGD("open db(%s) error", path);
        mHandle = NULL;
        return -1;
    }
#endif
    return 0;
}

int CSqlite::closeDb()
{
    int rval = 0;
#ifdef SUPPORT_ADTV
    if (mHandle != NULL) {
        rval = sqlite3_close(mHandle);
        mHandle = NULL;
    }
#endif
    return rval;
}

void CSqlite::setHandle(sqlite3 *h)
{
    mHandle = h;
}

sqlite3 *CSqlite::getHandle()
{
    return mHandle;
}

int CSqlite::select(const char *sql, CSqlite::Cursor &c)
{
#ifdef SUPPORT_ADTV
    int col, row;
    char **pResult = NULL;
    char *errmsg;
    assert(mHandle && sql);

    if (strncmp(sql, "select", 6))
        return -1;
    //LOGD("sql=%s", sql);
    if (sqlite3_get_table(mHandle, sql, &pResult, &row, &col, &errmsg) != SQLITE_OK) {
        LOGD("errmsg=%s", errmsg);
        if (pResult != NULL)
            sqlite3_free_table(pResult);
        return -1;
    }

    //LOGD("row=%d, col=%d", row, col);
    c.Init(pResult, row, col);
#endif
    return 0;
}

void CSqlite::insert()
{
}

bool CSqlite::exeSql(const char *sql)
{
#ifdef SUPPORT_ADTV
    char *errmsg;
    if (sql == NULL) return false;
    if (sqlite3_exec(mHandle, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        //LOGE("exeSql=: %s error=%s", sql, errmsg ? errmsg : "Unknown");
        if (errmsg)
            sqlite3_free(errmsg);
        return false;
    }
#endif
    //LOGD("sql=%s", sql);
    return true;
}

bool CSqlite::beginTransaction()
{
    return exeSql("begin;");
}

bool CSqlite::commitTransaction()
{
    return exeSql("commit;");
}

bool CSqlite::rollbackTransaction()
{
    return exeSql("rollback;");
}

void CSqlite::del()
{
}

void CSqlite::update()
{
}

void CSqlite::xxtable()
{
}

