/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CSqlite"

#include "CSqlite.h"
#include <utils/Log.h>
#include "CPQLog.h"

CSqlite::CSqlite()
{
    mHandle = NULL;
}

CSqlite::~CSqlite()
{
    AutoMutex _l( mLock );
    if (mHandle != NULL) {
        sqlite3_close(mHandle);
        mHandle = NULL;
    }
}

int CSqlite::sqlite3_exec_callback(void *data __unused, int nColumn, char **colValues __unused, char **colNames __unused)
{
    SYS_LOGD("sqlite3_exec_callback, nums = %d", nColumn);
    return 0;
}

int CSqlite::openDb(const char *path)
{
    AutoMutex _l( mLock );
    if (sqlite3_open(path, &mHandle) != SQLITE_OK) {
        SYS_LOGD("open db(%s) error", path);
        mHandle = NULL;
        return -1;
    }
    return 0;
}

int CSqlite::closeDb()
{
    AutoMutex _l( mLock );
    int rval = 0;
    if (mHandle != NULL) {
        rval = sqlite3_close(mHandle);
        mHandle = NULL;
    }
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
    AutoMutex _l( mLock );
    int col, row;
    char **pResult = NULL;
    char *errmsg;
    assert(mHandle && sql);

    if (strncmp(sql, "select", 6))
        return -1;
    if (sqlite3_get_table(mHandle, sql, &pResult, &row, &col, &errmsg) != SQLITE_OK) {
        SYS_LOGE("errmsg=%s", errmsg);
        if (pResult != NULL)
            sqlite3_free_table(pResult);
        return -1;
    }

    c.Init(pResult, row, col);
    return 0;
}

void CSqlite::insert()
{
}

bool CSqlite::exeSql(const char *sql)
{
    AutoMutex _l( mLock );
    char *errmsg;
    if (sql == NULL) return false;
    if (sqlite3_exec(mHandle, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        SYS_LOGE("exeSql=: %s error=%s", sql, errmsg ? errmsg : "Unknown");
        if (errmsg)
            sqlite3_free(errmsg);
        return false;
    }
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

