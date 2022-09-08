/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _EXT_PARSER_UTILS_H_
#define _EXT_PARSER_UTILS_H_

#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <string>
#include "list.h"
#include "DataSource.h"

#define  MALLOC(s)      malloc(s)
#define  FREE(d)        free(d)
#define  MEMCPY(d,s,l)  memcpy(d,s,l)
#define  MEMSET(d,s,l)  memset(d,s,l)
#define  MIN(x,y)       ((x)<(y)?(x):(y))
#define  UTF8           unsigned char
#define  UTF16          unsigned short
#define  UTF32          unsigned int
#define ERR ((void *) -1)
#define LINE_LEN                    1024*5

class ExtParserUtils {

public:
    ExtParserUtils(int charset, std::shared_ptr<DataSource> source);
    ~ExtParserUtils();
    int Ext_Subtitle_uni_Utf16toUtf8(int subformat, const UTF16 *in, int inLen, UTF8 *out, int outMax);
    int Ext_subtitle_utf16_utf8(int subformat, char *s, int inLen);
    int ExtSubtitleEol(char p);
    char *ExtSubtiltleReadtext(char *source, char **dest);
    char *ExtSubtitleStrdup(char *src);
    char *ExtSubtitleStristr(const char *haystack, const char *needle);
    void ExtSubtitleTrailspace(char *s) ;
    char *ExtSubtitlesfileGets(char *s/*, int fd*/);
private:
    char *subfile_buffer;
    int mBufferSize;
    unsigned mFileReaded;
    std::shared_ptr<DataSource> mDataSource;
    int subtitle_format;
};

#endif

