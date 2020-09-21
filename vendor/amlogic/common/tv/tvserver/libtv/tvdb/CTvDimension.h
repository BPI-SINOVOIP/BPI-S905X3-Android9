/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVDIMENSION_H)
#define _CTVDIMENSION_H
#include <utils/Vector.h>
#include "CTvDatabase.h"
#include <utils/String8.h>
#include <stdlib.h>

#include "CTvLog.h"

#include <vector>

// TV ATSC rating dimension
class CTvDimension {
public:
    class VChipRating {
    private:
        int region;
        int dimension;
        int value;

    public:
        VChipRating(int region, int dimension, int value);
        VChipRating();
        ~VChipRating();
        int getRegion();
        int getDimension();
        int getValue()const;
    };
    /* 'All' is a very special case, it links to dimension0 & dimension5 */
    void createFromCursor(CTvDatabase::Cursor &c);
    static int getUSPGAllLockStatus(String8 abbrev);
    static void setUSPGAllLockStatus(String8 abbrev, int lock);
    static void selectByID(CTvDimension &dm, int id);
    static int selectByIndex(CTvDimension &dm, int ratingRegionID, int index);
    static void selectByName(CTvDimension &dm, int ratingRegionID, String8 dimensionName);
    static void selectByRatingRegion(CTvDimension &dm, int ratingRegionID);
    bool isBlocked(CTvDimension &dm, VChipRating *definedRating);
    int getID();
    int getRatingRegion();
    String8 getRatingRegionName();
    String8 getName();
    int getGraduatedScale();
    int getDefinedValue();
    //int* getLockStatus();
    int getLockStatus(int valueIndex);
    void getLockStatus(String8 abbrevs[], int lock[], int *array_len);
    int getAbbrev(std::vector<String8> abb);
    String8 getAbbrev(int valueIndex);
    int getText(String8 tx[]);
    String8 getText(int valueIndex);
    void setLockStatus(int valueIndex, int status);
    void setLockStatus(int status[]);
    void setLockStatus(String8 abbrevs[], int locks[], int abb_size);
    /**Rating regions*/
public:
    static const int REGION_US = 0x1;
    static const int REGION_CANADA = 0x2;
    static const int REGION_TAIWAN = 0x3;
    static const int REGION_SOUTHKOREA = 0x4;

    CTvDimension(CTvDatabase::Cursor &c);
    CTvDimension();
    ~CTvDimension();

    static void insertNewDimension(const int region, String8 regionName, String8 name,
                                   int indexj, int *lock, const char **abbrev, const char **text, int size);
    static  void builtinAtscDimensions();
    static  int isDimensionTblExist();
    String8 getCurdimension();
    String8 getCurAbbr();
    String8 getCurText();
private:
    int id;
    int indexj;
    int ratingRegion;
    int graduatedScale;
    int valuesDefined;
    int *lockValues;
    String8 name;
    String8 ratingRegionName;
    String8 *abbrevValues;
    String8 *textValues;
    bool isPGAll;
    String8 CurvchipDimension;
    String8 CurvchipAbbrev;
    String8 CurvchipText;
};
#endif  //_CTVDIMENSION_H
