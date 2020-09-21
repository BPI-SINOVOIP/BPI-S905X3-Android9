/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CIniFile"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <include/CTvLog.h>

#include "include/CIniFile.h"

CIniFile::CIniFile()
{
    mpFirstSection = NULL;
    mpFileName[0] = '\0';
    m_pIniFile = NULL;
    mpFirstLine = NULL;
}

CIniFile::~CIniFile()
{
    LOGD("CIniFile::~CIniFile()");
    FreeAllMem();
}

int CIniFile::LoadFromFile(const char *filename)
{
    char   lineStr[MAX_INI_FILE_LINE_LEN];
    LINE *pCurLINE = NULL;
    SECTION *pCurSection = NULL;

    FreeAllMem();

    if (filename == NULL) {
        return -1;
    }

    LOGD("LoadFromFile name = %s", filename);
    strncpy(mpFileName, filename, sizeof(mpFileName)-1);
    if ((m_pIniFile = fopen (mpFileName, "r")) == NULL) {
        LOGE("open %s fail: %s", mpFileName, strerror(errno));
        return -1;
    }

    while (fgets (lineStr, MAX_INI_FILE_LINE_LEN, m_pIniFile) != NULL) {
        allTrim(lineStr);

        LINE *pLINE = new LINE();
        pLINE->pKeyStart = pLINE->Text;
        pLINE->pKeyEnd = pLINE->Text;
        pLINE->pValueStart = pLINE->Text;
        pLINE->pValueEnd = pLINE->Text;
        pLINE->pNext = NULL;
        pLINE->type = getLineType(lineStr);
        //LOGD("getline=%s len=%d type=%d", lineStr, strlen(lineStr), pLINE->type);
        strcpy(pLINE->Text, lineStr);
        pLINE->LineLen = strlen(pLINE->Text);

        //head
        if (mpFirstLine == NULL) {
            mpFirstLine = pLINE;
        } else {
            pCurLINE->pNext = pLINE;
        }

        pCurLINE = pLINE;

        switch (pCurLINE->type) {
        case LINE_TYPE_SECTION: {
            SECTION *pSec = new SECTION();
            pSec->pLine = pLINE;
            pSec->pNext = NULL;
            if (mpFirstSection == NULL) { //first section
                mpFirstSection = pSec;
            } else {
                pCurSection->pNext = pSec;
            }
            pCurSection = pSec;
            break;
        }
        case LINE_TYPE_KEY: {
            char *pM = strchr(pCurLINE->Text, '=');
            pCurLINE->pKeyStart = pCurLINE->Text;
            pCurLINE->pKeyEnd = pM - 1;
            pCurLINE->pValueStart = pM + 1;
            pCurLINE->pValueEnd = pCurLINE->Text + pCurLINE->LineLen - 1;
            break;
        }
        case LINE_TYPE_COMMENT:
        default:
            break;
        }
    }

    fclose (m_pIniFile);
    m_pIniFile = NULL;
    return 0;
}

int CIniFile::SaveToFile(const char *filename)
{
    const char *filepath = NULL;
    FILE *pFile = NULL;

    if (filename == NULL) {
        if (strlen(mpFileName) == 0) {
            LOGD("error save file is null");
            return -1;
        } else {
            filepath = mpFileName;
        }
    } else {
        filepath = filename;
    }

    if ((pFile = fopen (filepath, "wb")) == NULL) {
        LOGD("%s: open %s error(%s)", __FUNCTION__, filepath, strerror(errno));
        return -1;
    }

    LINE *pCurLine = NULL;
    for (pCurLine = mpFirstLine; pCurLine != NULL; pCurLine = pCurLine->pNext) {
        fprintf (pFile, "%s\r\n", pCurLine->Text);
    }

    fflush(pFile);
    fsync(fileno(pFile));
    fclose(pFile);
    return 0;
}

int CIniFile::SetString(const char *section, const char *key, const char *value)
{
    SECTION *pNewSec = NULL;
    LINE *pNewSecLine = NULL;
    LINE *pNewKeyLine = NULL;

    SECTION *pSec = getSection(section);
    if (pSec == NULL) {
        pNewSec = new SECTION();
        pNewSecLine = new LINE();
        pNewKeyLine = new LINE();

        pNewKeyLine->type = LINE_TYPE_KEY;
        pNewSecLine->type = LINE_TYPE_SECTION;


        sprintf(pNewSecLine->Text, "[%s]", section);
        pNewSec->pLine = pNewSecLine;

        InsertSection(pNewSec);

        int keylen = strlen(key);
        sprintf(pNewKeyLine->Text, "%s=%s", key, value);
        pNewKeyLine->LineLen = strlen(pNewKeyLine->Text);
        pNewKeyLine->pKeyStart = pNewKeyLine->Text;
        pNewKeyLine->pKeyEnd = pNewKeyLine->pKeyStart + keylen - 1;
        pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen + 1;
        pNewKeyLine->pValueEnd = pNewKeyLine->Text + pNewKeyLine->LineLen - 1;

        InsertKeyLine(pNewSec, pNewKeyLine);

    } else { //find section
        LINE *pLine = getKeyLineAtSec(pSec, key);
        if (pLine == NULL) { //, not find key
            pNewKeyLine = new LINE();
            pNewKeyLine->type = LINE_TYPE_KEY;

            int keylen = strlen(key);
            sprintf(pNewKeyLine->Text, "%s=%s", key, value);
            pNewKeyLine->LineLen = strlen(pNewKeyLine->Text);
            pNewKeyLine->pKeyStart = pNewKeyLine->Text;
            pNewKeyLine->pKeyEnd = pNewKeyLine->pKeyStart + keylen - 1;
            pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen + 1;
            pNewKeyLine->pValueEnd = pNewKeyLine->Text + pNewKeyLine->LineLen - 1;

            InsertKeyLine(pSec, pNewKeyLine);
        } else { //all find, change it
            sprintf(pLine->Text, "%s=%s", key, value);
            pLine->LineLen = strlen(pLine->Text);
            pLine->pValueEnd = pLine->Text + pLine->LineLen - 1;
        }
    }

    //save
    SaveToFile(NULL);
    return 0;
}

int CIniFile::SetInt(const char *section, const char *key, int value)
{
    char tmp[64];
    sprintf(tmp, "%d", value);
    SetString(section, key, tmp);
    return 0;
}

const char *CIniFile::GetString(const char *section, const char *key, const char *def_value)
{
    SECTION *pSec = getSection(section);
    if (pSec == NULL) return def_value;
    LINE *pLine = getKeyLineAtSec(pSec, key);
    if (pLine == NULL) return def_value;

    return pLine->pValueStart;
}

int CIniFile::GetInt(const char *section, const char *key, int def_value)
{
    const char *num = GetString(section, key, NULL);
    if (num != NULL) {
        return atoi(num);
    }
    return def_value;
}

int CIniFile::SetFloat(const char *section, const char *key, float value)
{
    char tmp[64];
    sprintf(tmp, "%.2f", value);
    SetString(section, key, tmp);
    return 0;
}

float CIniFile::GetFloat(const char *section, const char *key, float def_value)
{
    const char *num = GetString(section, key, NULL);
    if (num != NULL) {
        return atof(num);
    }
    return def_value;
}

LINE_TYPE CIniFile::getLineType(char *Str)
{
    LINE_TYPE type = LINE_TYPE_COMMENT;
    if (strchr(Str, '#') != NULL) {
        type = LINE_TYPE_COMMENT;
    } else if ( (strstr (Str, "[") != NULL) && (strstr (Str, "]") != NULL) ) { /* Is Section */
        type = LINE_TYPE_SECTION;
    } else {
        if (strstr (Str, "=") != NULL) {
            type = LINE_TYPE_KEY;
        } else {
            type = LINE_TYPE_COMMENT;
        }
    }
    return type;
}

void CIniFile::FreeAllMem()
{
    //line
    LINE *pCurLine = NULL;
    LINE *pNextLine = NULL;
    for (pCurLine = mpFirstLine; pCurLine != NULL;) {
        pNextLine = pCurLine->pNext;
        delete pCurLine;
        pCurLine = pNextLine;
    }
    mpFirstLine = NULL;
    //section
    SECTION *pCurSec = NULL;
    SECTION *pNextSec = NULL;
    for (pCurSec = mpFirstSection; pCurSec != NULL;) {
        pNextSec = pCurSec->pNext;
        delete pCurSec;
        pCurSec = pNextSec;
    }
    mpFirstSection = NULL;
}

int CIniFile::InsertSection(SECTION *pSec)
{
    //insert it to sections list ,as first section
    pSec->pNext = mpFirstSection;
    mpFirstSection = pSec;
    //insert it to lines list, at first
    pSec->pLine->pNext = mpFirstLine;
    mpFirstLine = pSec->pLine;
    return 0;
}

int CIniFile::InsertKeyLine(SECTION *pSec, LINE *line)
{
    LINE *line1 = pSec->pLine;
    LINE *line2 = line1->pNext;
    line1->pNext = line;
    line->pNext = line2;
    return 0;
}

SECTION *CIniFile::getSection(const char *section)
{
    //section
    for (SECTION *psec = mpFirstSection; psec != NULL; psec = psec->pNext) {
        if (strncmp((psec->pLine->Text) + 1, section, strlen(section)) == 0)
            return psec;
    }
    LOGE("not find section: %s", section);
    return NULL;
}

LINE *CIniFile::getKeyLineAtSec(SECTION *pSec, const char *key)
{
    //line
    for (LINE *pline = pSec->pLine->pNext; (pline != NULL && pline->type != LINE_TYPE_SECTION); pline = pline->pNext) {
        if (pline->type == LINE_TYPE_KEY) {
            if (strncmp(pline->Text, key, strlen(key)) == 0)
                return pline;
        }
    }
    return NULL;
}

void CIniFile::allTrim(char *Str)
{
    char *pStr;
    pStr = strchr (Str, '\n');
    if (pStr != NULL) {
        *pStr = 0;
    }
    int Len = strlen(Str);
    if ( Len > 0 ) {
        if ( Str[Len - 1] == '\r' ) {
            Str[Len - 1] = '\0';
        }
    }
    pStr = Str;
    while (*pStr != '\0') {
        if (*pStr == ' ') {
            char *pTmp = pStr;
            while (*pTmp != '\0') {
                *pTmp = *(pTmp + 1);
                pTmp++;
            }
        } else {
            pStr++;
        }
    }
    return;
}

