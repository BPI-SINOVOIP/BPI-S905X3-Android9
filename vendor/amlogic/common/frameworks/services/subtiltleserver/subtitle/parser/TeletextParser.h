/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/

#ifndef __SUBTITLE_TELETEXT_PARSER_H__
#define __SUBTITLE_TELETEXT_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types.h"


#include <libzvbi.h>
#include "dvbCommon.h"

#include<stack>


#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)
#define AV_TIME_BASE            1000000
#define AV_TIME_BASE_Q          (AVRational){1, AV_TIME_BASE}
#define AVPALETTE_SIZE 1024
#define MAX_SLICES 64

struct TeletextPage {
    AVSubtitleRect *subRect;
    int pgno;
    int subno;
    int64_t pts;
};

/**\brief Error code of the Teletext module*/
enum TeletextFlgCode
{
    TT2_FAILURE = -1,   /**< Invalid parameter*/
    TT2_SUCCESS
};

/**\Teletext control state*/
enum TeletextCtrlState
{
    TT2_FULL_DISPLAY_STATE,
    TT2_DOUBLE_HEIGHT_STATE,
    TT2_DOUBLE_SCROLL_STATE,
};

/**\Teletext mix video state*/
enum TeletextMixVideoState
{
    TT2_MIX_BLACK = 0,
    TT2_MIX_TRANSPARENT,
    TT2_MIX_HALF_SCREEN,
};

/**\Teletext page state*/
enum TeletextPageState
{
    TT2_DISPLAY_STATE = 0,
    TT2_SEARCH_STATE,
    TT2_INPUT_STATE,
};

/**\Teletext subtitle mode*/
enum TeletextSubtitleMode
{
    TT2_SUBTITLE_MODE = 0,  /**< ttx subtitle*/
    TT2_GRAPHICS_MODE,      /**< ttx graphics*/
};



typedef struct TeletextCachedPageS
{
    int                   pageNo;
    int                   subPageNo;
    int                   pageType;
}TeletextCachedPageT;

typedef struct NavigatorPageS{
    int pageNo;
    int subPageNo;
}NavigatorPageT;

/**\brief Teletext color*/
typedef enum{
    TT2_COLOR_RED  = 1,           /**< red*/
    TT2_COLOR_GREEN,         /**< green*/
    TT2_COLOR_YELLOW,        /**< yellow*/
    TT2_COLOR_BLUE           /**< blue*/
}TeletextPageColorParam;

/**\brief Teletext double height state*/
typedef enum{
    DOUBLE_HEIGHT_NORMAL,
    DOUBLE_HEIGHT_TOP,
    DOUBLE_HEIGHT_BOTTOM,
}TeletextDoubleHeight;


struct TeletextContext {
    //AVClass        *class;
    char           *pgno;
    int             xOffset;
    int             yOffset;
    int             formatId; /* 0 = bitmap, 1 = text/ass */
    int             chopTop;
    int             subDuration; /* in msec */
    int             transparentBackground; //page backGround
    int             disPlayBackground;  //display backGround, own to page not full Green, need add prop define display backGround
    int             opacity;
    int             chopSpaces;
    int             lockSubpg;

    int             linesProcessed;
    TeletextPage    *pages;
    int             totalPages;
    int64_t         pts;
    int             handlerRet;
    TeletextPageState pageState;
    TeletextDoubleHeight doubleHeight;
    TeletextMixVideoState mixVideoState;
    TeletextSubtitleMode  subtitleMode;

    int             pageNum;               //add for teletext graphic
    int             subPageNum;

    int             acceptSubPage;
    int             dispUpdate;
    int             dispMode;  //1:whole gfx, 0:only page
    char            time[8];

    int             removewHeights;  //for double height and double sroll
    int             heightIndex;  //for double height and double sroll

    int             gotoPage;

    bool            isSubtitle;

    vbi_search      *search;
    vbi_decoder *   vbi;
#ifdef DEBUG
    vbi_export *    ex;
#endif
    vbi_sliced      sliced[MAX_SLICES];
    int             readorder;
    int             searchDir;//always used with pageState
};


class TeletextParser: public Parser {
public:
    TeletextParser(std::shared_ptr<DataSource> source);
    virtual ~TeletextParser();
    virtual int parse();
    virtual bool updateParameter(int type, void *data);
    virtual void dump(int fd, const char *prefix);
    static inline TeletextParser *getCurrentInstance();

    // TODO: move to private.
    int goHomeLocked();
    int nextPageLocked(int dir, bool fetch=true);
    int nextSubPageLocked(int dir);
    int gotoPageLocked(int pageNum, int subPageNum);
    int getSubPageInfoLocked();
    int fetchVbiPageLocked(int pageNum, int subPageNum);
    void notifyTeletextLoadState(int val);

    TeletextContext *mContext;
    unsigned char *mTextBack;
    unsigned char *mBarBack;
    std::vector<NavigatorPageT> mCurrentNavigatorPage;
    int mNavigatorPage = 0;
    int mNavigatorSubPage = 0;


private:
    int getSpu(std::shared_ptr<AML_SPUVAR> spu);
    int getInterSpu();

    void checkDebug();

    int getDvbTeletextSpu(std::shared_ptr<AML_SPUVAR> spu);
    int softDemuxParse(std::shared_ptr<AML_SPUVAR> spu);
    int hwDemuxParse(std::shared_ptr<AML_SPUVAR> spu);
    int teletextDecodeFrame(std::shared_ptr<AML_SPUVAR> spu, char *psrc, const int size);

    // event handler, need calling when hold the lock
    int fetchCountPageLocked(int dir, int count);
    int changeMixModeLocked();
    int setDisplayModeLocked();
    int setClockModeLocked();
    int setDoubleHeightStateLocked();
    int lockSubpgLocked();
    int doubleScrollLocked(int dir);
    void tt2AddCachedPageLocked(vbi_page *vp/*, int page_type*/);
    int gotoBackPageLocked();
    int gotoForwardPageLocked();
    void tt2AddBackCachePageLocked(int pgno, int subPgno);
    void tt2AddForwardCachePageLocked(int pgno, int subPgno);
    int convertPageDecimal2Hex(int magazine, int page);

    void notifyMixVideoState(int val);
    int saveTeletextGraphicsRect2Spu(std::shared_ptr<AML_SPUVAR> spu, AVSubtitleRect *subRect) ;

    int saveDisplayRect2Spu(std::shared_ptr<AML_SPUVAR> spu, AVSubtitleRect *subRect);
    bool getVbiNextValidPage(vbi_decoder *vbi, int dir, vbi_pgno *pgno, vbi_pgno *subno);

    int initContext();

    // control dump file or not
    int mDumpSub;
    int mIndex;
    int mGotoPageNum;
    static TeletextParser *sInstance;

    std::mutex mMutex;

    std::stack<TeletextCachedPageT> mBackPageStk;
    std::stack<TeletextCachedPageT> mForwardPageStk;
};


#endif
