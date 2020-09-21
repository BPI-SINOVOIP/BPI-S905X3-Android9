/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */


#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvDimension"

#include "CTvDimension.h"
#include "CTvProgram.h"
#include "CTvTime.h"
#include "CTvEvent.h"
#include <tvconfig.h>

#include <vector>

/**
 *TV ATSC rating dimension
 */

void CTvDimension::createFromCursor(CTvDatabase::Cursor &c)
{
    int col;

    col = c.getColumnIndex("db_id");
    this->id = c.getInt(col);

    col = c.getColumnIndex("index_j");
    this->indexj = c.getInt(col);

    col = c.getColumnIndex("rating_region");
    this->ratingRegion = c.getInt(col);

    col = c.getColumnIndex("graduated_scale");
    this->graduatedScale = c.getInt(col);

    col = c.getColumnIndex("name");
    this->name = c.getString(col);

    col = c.getColumnIndex("rating_region_name");
    this->ratingRegionName = c.getString(col);

    col = c.getColumnIndex("values_defined");
    this->valuesDefined = c.getInt(col);
    this->lockValues = new int[valuesDefined];
    this->abbrevValues = new String8[valuesDefined];
    this->textValues = new String8[valuesDefined];
    char temp[256];
    for (int i = 0; i < valuesDefined; i++) {
        sprintf(temp, "abbrev%d", i);
        col = c.getColumnIndex(temp);
        this->abbrevValues[i] = c.getString(col);
        sprintf(temp, "text%d", i);
        col = c.getColumnIndex(temp);
        this->textValues[i] = c.getString(col);
        sprintf(temp, "locked%d", i);
        col = c.getColumnIndex(temp);
        this->lockValues[i] = c.getInt(col);
    }

    if (ratingRegion == REGION_US && !strcmp(name, "All")) {
        isPGAll = true;
    } else {
        isPGAll = false;
    }
}

CTvDimension::CTvDimension(CTvDatabase::Cursor &c)
{
    createFromCursor(c);
}
CTvDimension::CTvDimension()
{
    id = 0;
    indexj = 0;
    ratingRegion = 0;
    graduatedScale = 0;
    valuesDefined = 0;
    lockValues = NULL;
    abbrevValues = NULL;
    textValues = NULL;
    isPGAll = false;
}
CTvDimension::~CTvDimension()
{
    if (lockValues != NULL) {
        delete []lockValues;
    }
    if (textValues != NULL) {
        delete []textValues;
    }
    if (abbrevValues != NULL) {
        delete []abbrevValues;
    }
}

/* 'All' is a very special case, it links to dimension0 & dimension5 */
int CTvDimension::getUSPGAllLockStatus(String8 abbrev)
{
//TODO
    int len = 0;
    CTvDimension dm5;
    int j = 0;
    selectByIndex(dm5, CTvDimension::REGION_US, 5);
    len = dm5.getDefinedValue();

    std::vector<String8> dm5Abbrev(len - 1);
    dm5.getAbbrev(dm5Abbrev);
    for (j = 0; j < len - 1; j++) {
        if (dm5Abbrev[j] == abbrev) {
            return dm5.getLockStatus(j + 1);
        }
    }
    CTvDimension dm0;
    selectByIndex(dm0, CTvDimension::REGION_US, 0);
    len = dm0.getDefinedValue();
    std::vector<String8> dm0Abbrev(len - 1);
    dm0.getAbbrev(dm0Abbrev);
    for (j = 0; j < len - 1; j++) {
        if (dm0Abbrev[j] == abbrev) {
            return dm0.getLockStatus(j + 1);
        }
    }
    return -1;
}

void CTvDimension::setUSPGAllLockStatus(String8 abbrev, int lock)
{

    int len = 0;
    int j = 0;

    CTvDimension dm5;

    selectByIndex(dm5, REGION_US, 5);
    len = dm5.getDefinedValue();
    std::vector<String8> dm5Abbrev(len - 1);
    dm5.getAbbrev(dm5Abbrev);

    for (j = 0; j < len - 1; j++) {
        if (abbrev == dm5Abbrev[j]) {
            dm5.setLockStatus(j + 1, lock);
            return;
        }
    }

    CTvDimension dm0;
    selectByIndex(dm0, REGION_US, 0);
    len = dm0.getDefinedValue();
    std::vector<String8> dm0Abbrev(len - 1);
    dm0.getAbbrev(dm0Abbrev);

    for (j = 0; j < len - 1; j++) {
        if (abbrev == dm0Abbrev[j]) {
            dm0.setLockStatus(j + 1, lock);
            return;
        }
    }

    return;
}


void CTvDimension::selectByID(CTvDimension &dm, int id)
{
    String8 cmd = String8("select * from dimension_table where evt_table.db_id = ") + String8::format("%d", id);
    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        dm.createFromCursor(c);
        LOGD("%s, %d  success", "TV", __LINE__);
    } else {
        LOGD("%s, %d  fail", "TV", __LINE__);
        c.close();
        return;
    }
    c.close();

}

void CTvDimension::selectByRatingRegion(CTvDimension &dm, int ratingRegionID)
{
    String8 cmd = String8("select * from dimension_table where rating_region = ") + String8::format("%d", ratingRegionID);
    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        dm.createFromCursor(c);
        LOGD("%s, %d  success", "TV", __LINE__);
    } else {
        LOGD("%s, %d  fail", "TV", __LINE__);
        c.close();
        return;
    }
    c.close();
}

int CTvDimension::selectByIndex(CTvDimension &dm, int ratingRegionID, int index)
{
    String8 cmd = String8("select * from dimension_table where rating_region = ") + String8::format("%d", ratingRegionID);
    cmd += String8(" and index_j=") + String8::format("%d", index);
    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        dm.createFromCursor(c);
        LOGD("%s, %d  success", "TV", __LINE__);
    } else {
        LOGD("%s, %d  fail", "TV", __LINE__);
        c.close();
        return -1;
    }
    c.close();

    return 0;
}

void CTvDimension::selectByName(CTvDimension &dm, int ratingRegionID, String8 dimensionName)
{
    String8 cmd = String8("select * from dimension_table where rating_region = ") + String8::format("%d", ratingRegionID);
    cmd += String8(" and name='") + dimensionName + String8("'");
    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        LOGD("%s, %d  success", "TV", __LINE__);
        dm.createFromCursor(c);
    } else {
        LOGD("%s, %d  fail", "TV", __LINE__);
        c.close();
        return;
    }
    c.close();

    return;
}


bool CTvDimension::isBlocked(CTvDimension &dm, VChipRating *definedRating)
{
    int ret = 0;
    ret = selectByIndex(dm, definedRating->getRegion(), definedRating->getDimension());
    if (ret != -1) {
        LOGD("%s, %d, index=%d", "TV", __LINE__, definedRating->getValue());
        return (dm.getLockStatus(definedRating->getValue()) == 1);
    }

    return false;
}

int CTvDimension::getID()
{
    return id;
}

int CTvDimension::getRatingRegion()
{
    return ratingRegion;
}

int CTvDimension::getDefinedValue()
{
    return valuesDefined;
}

String8 CTvDimension::getRatingRegionName()
{
    return ratingRegionName;
}

String8 CTvDimension::getName()
{
    return name;
}

int CTvDimension::getGraduatedScale()
{
    return graduatedScale;
}

#if 0
int *CTvDimension::getLockStatus()
{
    int len = getDefinedValue();
    if (len > 1) {
        if (isPGAll) {
            return getUSPGAllLockStatus(abbrevValues);
        } else {
            int *lock = new int[len - 1];
            //System.arraycopy(lockValues, 1, l, 0, l.length);
            for (int i = 0; i < len - 1; i++)
                lock[i] = lockValues[1 + i];
            return lock;
        }
    } else {
        return NULL;
    }
}
#endif

int CTvDimension::getLockStatus(int valueIndex)
{
    int len = getDefinedValue();
    if (valueIndex >= len) {
        return -1;
    } else {
        return lockValues[valueIndex];
    }
}

void CTvDimension::getLockStatus(String8 abbrevs[], int lock[], int *array_len)
{
    int len = getDefinedValue();

    if (abbrevs != NULL && lock != NULL) {
        for (int i = 0; i < *array_len; i++) {
            *(lock + i) = -1;
            for (int j = 0; j < len; j++) {
                if (abbrevs[i] == abbrevValues[j]) {
                    *(lock + i) = lockValues[j];
                    break;
                }
            }
        }
    }
    *array_len = len;
}

int CTvDimension::getAbbrev(std::vector<String8> abb)
{
    /* the first rating_value must be not visible to user */
    int len = getDefinedValue();
    if (len > 1) {
        for (int i = 0; i < len - 1; i++)
            abb[i] = abbrevValues[i + 1];
        return 0;
    } else {
        return -1;
    }
}

String8 CTvDimension::getAbbrev(int valueIndex)
{
    int len = getDefinedValue();
    if (valueIndex >= len)
        return String8("");
    else
        return abbrevValues[valueIndex];
}

int CTvDimension::getText(String8 tx[])
{
    int len = getDefinedValue();
    if (len > 1) {
        for (int i = 0; i < len - 1; i++)
            tx[i] = textValues[i + 1];
        return 0;
    } else {
        return -1;
    }
}

String8 CTvDimension::getText(int valueIndex)
{
    int len = getDefinedValue();
    if (valueIndex >= len)
        return String8("");
    else
        return textValues[valueIndex];
}

void CTvDimension::setLockStatus(int valueIndex, int status)
{
    int len = getDefinedValue();
    if (valueIndex >= len)
        return;

    if (lockValues[valueIndex] != -1 && lockValues[valueIndex] != status) {
        lockValues[valueIndex] = status;
        String8 cmd = String8("update dimension_table set locked") + String8::format("%d", valueIndex);
        cmd += String8("=") + String8::format("%d", status) + String8(" where db_id = ") + String8::format("%d", id);

        CTvDatabase::GetTvDb()->exeSql(cmd.string());
    }
}

void CTvDimension::setLockStatus(int status[])
{
    int len = getDefinedValue();
    if (status == NULL) {
        LOGD("Cannot set lock status, invalid param");
        return;
    }
    for (int i = 0; i < len; i++) {
        setLockStatus(i + 1, status[i]);
    }
}

void CTvDimension::setLockStatus(String8 abbrevs[], int locks[], int abb_size)
{
    int len = getDefinedValue();
    if (abbrevs == NULL || locks == NULL)
        return;

    for (int i = 0; i < abb_size; i++) {
        for (int j = 0; j < len; j++) {
            if (abbrevs[i] == abbrevValues[j]) {
                setLockStatus(j, locks[i]);
                break;
            }
        }
    }
}

CTvDimension::VChipRating::VChipRating(int region, int dimension, int value)
{
    this->region = region;
    this->dimension = dimension;
    this->value = value;
}
CTvDimension::VChipRating::VChipRating()
{
    region = 0;
    dimension = 0;
    value = 0;
}

CTvDimension::VChipRating::~VChipRating()
{
}
int CTvDimension::VChipRating::getRegion()
{
    return region;
}

int CTvDimension::VChipRating::getDimension()
{
    return dimension;
}

int CTvDimension::VChipRating::getValue()const
{
    return value;
}

String8 CTvDimension::getCurdimension()
{
    return CurvchipDimension;
}
String8 CTvDimension::getCurAbbr()
{
    return CurvchipAbbrev;

}
String8 CTvDimension::getCurText()
{
    return CurvchipText;
}

void CTvDimension::insertNewDimension(const int region, String8 regionName, String8 name,
                                      int indexj, int *lock, const char **abbrev, const char **text, int size)
{
    String8 cmd = String8("insert into dimension_table(rating_region,rating_region_name,name,graduated_scale,");
    cmd += String8("values_defined,index_j,version,abbrev0,text0,locked0,abbrev1,text1,locked1,abbrev2,text2,locked2,");
    cmd += String8("abbrev3,text3,locked3,abbrev4,text4,locked4,abbrev5,text5,locked5,abbrev6,text6,locked6,");
    cmd += String8("abbrev7,text7,locked7,abbrev8,text8,locked8,abbrev9,text9,locked9,abbrev10,text10,locked10,");
    cmd += String8("abbrev11,text11,locked11,abbrev12,text12,locked12,abbrev13,text13,locked13,abbrev14,text14,locked14,");
    cmd += String8("abbrev15,text15,locked15) values(") + String8::format("%d", region) + String8(",'") + regionName.string();
    cmd += String8("','") + name.string() + String8("',0,") + String8::format("%d", size) + String8(",") + String8::format("%d", indexj) + String8(",0");
    for (int i = 0; i < 16; i++) {
        if (i < size) {
            cmd += String8(",'") + String8::format("%s", abbrev[i]) + String8("'");
            cmd += String8(",'") + String8::format("%s", text[i])  + String8("'");
            cmd += String8(",'") + String8::format("%d", lock[i])  + String8("'");
        } else {
            cmd += String8(",''");
            cmd += String8(",''");
            cmd += String8(",-1");
        }
    }

    cmd += String8(")");
    CTvDatabase::GetTvDb()->exeSql(cmd.string());
}
/**
 * ??????Standard ATSC V-Chip Dimensions
 */
void CTvDimension::builtinAtscDimensions()
{
    CTvDatabase::GetTvDb()->exeSql("delete from dimension_table");

    /* Add U.S. Rating region 0x1 */
    const char *abbrev0[] = {"", "None", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    const char *text0[]  = {"", "None", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    int  lock0[]     = { -1,   -1,    0,       0,      0,     0};
    const char *abbrev1[] = {"", "D", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    const char *text1[]  = {"", "D", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    int  lock1[]     = { -1, -1,    -1,     0,      0,     -1};
    const char *abbrev2[] = {"", "L", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    const char *text2[]  = {"", "L", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    int  lock2[]     = { -1, -1,    -1,     0,      0,      0};
    const char *abbrev3[] = {"", "S", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    const char *text3[]  = {"", "S", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    int  lock3[]     = { -1, -1,    -1,     0,      0,      0};
    const char *abbrev4[] = {"", "V", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    const char *text4[]  = {"", "V", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    int  lock4[]     = { -1, -1,    -1,     0,      0,      0};
    const char *abbrev5[] = {"", "TV-Y", "TV-Y7"};
    const char *text5[]  = {"", "TV-Y", "TV-Y7"};
    int  lock5[]     = { -1,  0,      0};
    const char *abbrev6[] = {"", "FV", "TV-Y7"};
    const char *text6[]  = {"", "FV", "TV-Y7"};
    int  lock6[]     = { -1, -1,      0};
    const char *abbrev7[] = {"", "N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "NR"};
    const char *text7[]  = {"", "MPAA Rating Not Applicable", "Suitable for AllAges",
                            "Parental GuidanceSuggested", "Parents Strongly Cautioned",
                            "Restricted, under 17 must be accompanied by adult",
                            "No One 17 and Under Admitted", "No One 17 and Under Admitted",
                            "no Rated by MPAA"
                           };
    int  lock7[]     = { -1, -1, 0, 0, 0, 0, 0, 0, 0};
    /*Extra for 'All' */
    const char *abbrevall[] = {"TV-Y", "TV-Y7", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    const char *textall[]   = {"TV-Y", "TV-Y7", "TV-G", "TV-PG", "TV-14", "TV-MA"};
    int  lockall[]   = {0,   0,      0,      0,      0,     0};

    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Entire Audience"),  0, lock0, abbrev0, text0, sizeof(lock0) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Dialogue"),         1, lock1, abbrev1, text1, sizeof(lock1) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Language"),         2, lock2, abbrev2, text2, sizeof(lock2) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Sex"),              3, lock3, abbrev3, text3, sizeof(lock3) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Violence"),         4, lock4, abbrev4, text4, sizeof(lock4) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Children"),         5, lock5, abbrev5, text5, sizeof(lock5) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("Fantasy violence"), 6, lock6, abbrev6, text6, sizeof(lock6) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("MPAA"),             7, lock7, abbrev7, text7, sizeof(lock7) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_US, String8("US (50 states + possessions)"),
                       String8("All"),             -1, lockall, abbrevall, textall, sizeof(lockall) / sizeof(int));
    /* Add Canadian Rating region 0x2 */
    const char *cabbrev0[] = {"E",    "C",       "C8+", "G",     "PG", "14+", "18+"};
    const char *ctext0[]   = {"Exempt", "Children", "8+", "General", "PG", "14+", "18+"};
    int  clock0[]   = {0,     0,         0,    0,        0,   0,    0};
    const char *cabbrev1[] = {"E",       "G",        "8 ans+", "13 ans+", "16 ans+", "18 ans+"};
    const char *ctext1[]   = {"Exempt??es", "Pour tous", "8+",    "13+",     "16+",    "18+"};
    int  clock1[]   = {0,        0,          0,       0,        0,        0};

    insertNewDimension(CTvDimension::REGION_CANADA, String8("Canada"),
                       String8("Canadian English Language Rating"), 0, clock0, cabbrev0, ctext0, sizeof(clock0) / sizeof(int));
    insertNewDimension(CTvDimension::REGION_CANADA, String8("Canada"),
                       String8("Codes francais du Canada"),      1, clock1, cabbrev1, ctext1, sizeof(clock1) / sizeof(int));
}

int CTvDimension::isDimensionTblExist()
{
    String8 cmd = String8("select * from dimension_table");
    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    return c.moveToFirst();
}
