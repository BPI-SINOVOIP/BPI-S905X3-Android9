/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef __SUBTITLE_EXT_PARSER_H__
#define __SUBTITLE_EXT_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types.h"
#include "ExtParserUtils.h"

#define SUBAPI
#define SUB_MAX_TEXT                10

/* Maximal length of line of a subtitle */
#define ENABLE_PROBE_UTF8_UTF16
#define SUBTITLE_PROBE_SIZE     1024

typedef enum
{
    SUB_ALIGNMENT_BOTTOMLEFT = 1,
    SUB_ALIGNMENT_BOTTOMCENTER,
    SUB_ALIGNMENT_BOTTOMRIGHT,
    SUB_ALIGNMENT_MIDDLELEFT,
    SUB_ALIGNMENT_MIDDLECENTER,
    SUB_ALIGNMENT_MIDDLERIGHT,
    SUB_ALIGNMENT_TOPLEFT,
    SUB_ALIGNMENT_TOPCENTER,
    SUB_ALIGNMENT_TOPRIGHT
} sub_alignment_t;

/**
 * Subtitle struct unit
 */
typedef struct
{
    /// number of subtitle lines
    int lines;

    /// subtitle strings
    char *text[SUB_MAX_TEXT];

    /// alignment of subtitles
    sub_alignment_t alignment;
} subtext_t;

struct subdata_s
{
    list_t list;        /* head node of subtitle_t list */
    list_t list_temp;

    int sub_num;
    int sub_error;
    int sub_format;
};

typedef struct subdata_s subdata_t;

typedef struct subtitle_s subtitle_t;

struct subtitle_s
{
    list_t list;        /* linked list */
    int start;      /* start time */
    int end;        /* end time */
    subtext_t text;     /* subtitle text */
    unsigned char *subdata; /* data for divx bmp subtitle */
};

typedef struct
{
    subtitle_t *(*read)(subtitle_t *dest);
    void (*post)(subtitle_t *dest);
    const char *name;
} subreader_t;

typedef struct _DivXSubPictColor
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} DivXSubPictColor;

typedef struct _DivXSubPictHdr
{
    char duration[27];
    unsigned short width;
    unsigned short height;
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
    unsigned short field_offset;
    DivXSubPictColor background;
    DivXSubPictColor pattern1;
    DivXSubPictColor pattern2;
    DivXSubPictColor pattern3;
    unsigned char *rleData;
} DivXSubPictHdr;

typedef struct _DivXSubPictHdr_HD
{
    char duration[27];
    unsigned short width;
    unsigned short height;
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
    unsigned short field_offset;
    DivXSubPictColor background;
    DivXSubPictColor pattern1;
    DivXSubPictColor pattern2;
    DivXSubPictColor pattern3;
    unsigned char background_transparency;  //HD profile only
    unsigned char pattern1_transparency;    //HD profile only
    unsigned char pattern2_transparency;    //HD profile only
    unsigned char pattern3_transparency;    //HD profile only
    unsigned char *rleData;
} DivXSubPictHdr_HD;

#define sub_ms2pts(x) ((x) * 900)

class ExtParser: public Parser {
public:
    ExtParser(std::shared_ptr<DataSource> source);
    virtual ~ExtParser();
    virtual int parse();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);
    virtual void ExtSubtitlePost(subtitle_t *current);
    virtual void dump(int fd, const char *prefix) {};
    void resetForSeek();
    virtual std::shared_ptr<AML_SPUVAR> tryConsumeDecodedItem();


    // TODO: for ExtParser, need impl each. inherit from extParser
    //    Elimite below
    //init pts rate
    int mPtsRate = 24;
    bool mGotPtsRate = true;
    int mNoTextPostProcess = 0;  // 1 => do not apply text post-processing
    /* read one line of string from data source */
    int mSubIndex;
    // TODO: revise/rewrite above and the whole impl
private:

    void internal_sub_close(subdata_t *subdata);
    subdata_t *internal_sub_open(unsigned rate, char *charset);
    char *internal_sub_filenames(char *filename, unsigned perfect_match);
    subtitle_t *internal_sub_search(subdata_t *subdata, subtitle_t *ref, int pts);
    int internal_sub_get_starttime(subtitle_t *subt);
    int internal_sub_get_endtime(subtitle_t *subt);
    subtext_t *internal_sub_get_text(subtitle_t *subt);
    subtitle_t *internal_divx_sub_add(subdata_t *subdata, unsigned char *data);
    void internal_divx_sub_delete(subdata_t *subdata, int pts);
    void internal_divx_sub_flush(subdata_t *subdata);
    void reset_variate();
    int internal_sub_autodetect();
    int probe_utf8_utf16(char *probe_buffer, int size);
    int subtitle_uni_Utf16toUtf8(const UTF16 *in, int inLen, UTF8 *out, int outMax);
    int subtitle_utf16_utf8(char *s, int inLen);

    int getSpu();
    int getInterSpu();

    int subtitle_format;
};


/*The struct use fot notify the ui layer the audio information of the clip */
typedef struct _CLIP_AUDIO_INFO
{
    unsigned int audio_info;    /* audio codec info */
    unsigned int audio_cur_play_index;  /* audio play current index */
    unsigned int total_audio_stream;    /* total audio stream number */
} Clip_Audio_Info;

/*@}*/
#endif              /* SUB_API_H */
