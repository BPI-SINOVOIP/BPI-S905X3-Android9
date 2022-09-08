#ifndef _SUBTITLE_PARSER_FACTORY_H__
#define _SUBTITLE_PARSER_FACTORY_H__

#include "DataSource.h"
#include "Parser.h"

#define DEFAULT_CC_CHANNELD_ID 0



//before is disorder with amnuplayer send,now
//sync with amnuplayer client send definition
enum SubtitleType {
    TYPE_SUBTITLE_INVALID   = -1,
    TYPE_SUBTITLE_VOB       = 0,   //dvd subtitle
    TYPE_SUBTITLE_PGS,
    TYPE_SUBTITLE_MKV_STR,
    TYPE_SUBTITLE_SSA,
    TYPE_SUBTITLE_MKV_VOB,
    TYPE_SUBTITLE_DVB,
    TYPE_SUBTITLE_TMD_TXT   = 7,
    TYPE_SUBTITLE_IDX_SUB,  //now discard
    TYPE_SUBTITLE_DVB_TELETEXT,
    TYPE_SUBTITLE_CLOSED_CATPTION,
    TYPE_SUBTITLE_SCTE27,
    TYPE_SUBTITLE_DTVKIT_DVB, //12
    TYPE_SUBTITLE_DTVKIT_TELETEXT,
    TYPE_SUBTITLE_DTVKIT_SCTE27,
    TYPE_SUBTITLE_EXTERNAL,
    TYPE_SUBTITLE_MAX,
};


enum ExtSubtitleType {
    SUB_INVALID   = -1,
    SUB_MICRODVD,
    SUB_SUBRIP,
    SUB_SUBVIEWER,
    SUB_SAMI,
    SUB_VPLAYER,
    SUB_RT,
    SUB_SSA,
    SUB_PJS,
    SUB_MPSUB,
    SUB_AQTITLE,
    SUB_SUBVIEWER2,
    SUB_SUBVIEWER3,
    SUB_SUBRIP09,
    SUB_JACOSUB,
    SUB_MPL1,
    SUB_MPL2,
    SUB_XML,
    SUB_TTML,
    SUB_LRC,
    SUB_DIVX,
    SUB_EBUTTD,
};



//from ffmpeg avcodec.h
enum SubtitleCodecID {
    /* subtitle codecs */
    AV_CODEC_ID_DVD_SUBTITLE = 0x17000,
    AV_CODEC_ID_DVB_SUBTITLE,
    AV_CODEC_ID_TEXT,  ///< raw UTF-8 text
    AV_CODEC_ID_XSUB,
    AV_CODEC_ID_SSA,
    AV_CODEC_ID_MOV_TEXT,
    AV_CODEC_ID_HDMV_PGS_SUBTITLE,
    AV_CODEC_ID_DVB_TELETEXT,
    AV_CODEC_ID_SRT,

    AV_CODEC_ID_VOB_SUBTITLE = 0x1700a, //the same as AV_CODEC_ID_DVD_SUBTITLE

    AV_CODEC_ID_MICRODVD   = 0x17800,
    AV_CODEC_ID_EIA_608,
    AV_CODEC_ID_JACOSUB,
    AV_CODEC_ID_SAMI,
    AV_CODEC_ID_REALTEXT,
    AV_CODEC_ID_STL,
    AV_CODEC_ID_SUBVIEWER1,
    AV_CODEC_ID_SUBVIEWER,
    AV_CODEC_ID_SUBRIP,
    AV_CODEC_ID_WEBVTT,
    AV_CODEC_ID_MPL2,
    AV_CODEC_ID_VPLAYER,
    AV_CODEC_ID_PJS,
    AV_CODEC_ID_ASS,
    AV_CODEC_ID_HDMV_TEXT_SUBTITLE,

};



enum DisplayType {
    SUBTITLE_IMAGE_DISPLAY       = 1,
    SUBTITLE_TEXT_DISPLAY,
};

typedef enum {
    DTV_SUB_INVALID = -1,
    DTV_SUB_CC              = 2,
    DTV_SUB_SCTE27          = 3,
    DTV_SUB_DVB                     = 4,
    DTV_SUB_DTVKIT_DVB         =5,
    DTV_SUB_DTVKIT_TELETEXT = 6,
    DTV_SUB_DTVKIT_SCTE27    = 7,
} DtvSubtitleType;

enum VideoFormat {
    INVALID_CC_TYPE    = -1,
    MPEG_CC_TYPE       = 0,
    H264_CC_TYPE       = 2
};


typedef struct {
    int ChannelID;
    int vfmt;  //Video format
} CcParam;


typedef struct {
    int SCTE27_PID;
    int demuxId;
} Scte27Param;

typedef struct {
   int demuxId;
   int pid;
   int compositionId;
   int ancillaryId;
}DtvKitDvbParam;

typedef struct {
   int demuxId;
   int pid;
   int magazine;
   int page;
}DtvKitTeletextParam;
typedef enum {
    CMD_INVALID        = -1,
    CMD_GO_HOME        = 1,
    CMD_GO_TO_PAGE     = 2,
    CMD_NEXT_PAGE      = 3,
    CMD_NEXT_SUB_PAGE  = 4,
} TeletextCtrlCmd;


typedef enum{
   TT_EVENT_INVALID          = -1,
   // These are the four FastText shortcuts, usually represented by red, green,
   // yellow and blue keys on the handset.
   TT_EVENT_QUICK_NAVIGATE_1 = 0,
   TT_EVENT_QUICK_NAVIGATE_2,
   TT_EVENT_QUICK_NAVIGATE_3,
   TT_EVENT_QUICK_NAVIGATE_4,
   // The ten numeric keys used to input page indexes.
   TT_EVENT_0,
   TT_EVENT_1,
   TT_EVENT_2,
   TT_EVENT_3,
   TT_EVENT_4,
   TT_EVENT_5,
   TT_EVENT_6,
   TT_EVENT_7,
   TT_EVENT_8,
   TT_EVENT_9,
   // This is the home key, which returns to the nominated index page for this
   //   service.
   TT_EVENT_INDEXPAGE,
   // These are used to quickly increment/decrement the page index.
   TT_EVENT_NEXTPAGE,
   TT_EVENT_PREVIOUSPAGE,
   // These are used to navigate the sub-pages when in 'hold' mode.
   TT_EVENT_NEXTSUBPAGE,
   TT_EVENT_PREVIOUSSUBPAGE,
   // These are used to traverse the page history (if caching requested).
   TT_EVENT_BACKPAGE,
   TT_EVENT_FORWARDPAGE,
   // This is used to toggle hold on the current page.
   TT_EVENT_HOLD,
   // Reveal hidden page content (as defined in EBU specification)
   TT_EVENT_REVEAL,
   // This key toggles 'clear' mode (page hidden until updated)
   TT_EVENT_CLEAR,
   // This key toggles 'clock only' mode (page hidden until updated)
   TT_EVENT_CLOCK,
   // Used to toggle transparent background ('video mix' mode)
   TT_EVENT_MIX_VIDEO,
   // Used to toggle double height top / double-height bottom / normal height display.
   TT_EVENT_DOUBLE_HEIGHT,
   // Functional enhancement may offer finer scrolling of double-height display.
   TT_EVENT_DOUBLE_SCROLL_UP,
   TT_EVENT_DOUBLE_SCROLL_DOWN,
   // Used to initiate/cancel 'timer' mode (clear and re-display page at set time)
   TT_EVENT_TIMER,
   TT_EVENT_GO_TO_PAGE,
   TT_EVENT_GO_TO_SUBTITLE
} TeletextEvent;



typedef struct {
    int demuxId;
    int pid;
    int magazine;
    int page;
    int pageNo;
    int subPageNo;
    int pageDir;  //+1:next page, -1: last page
    int subPageDir;//+1:next sub page, -1: last sub page
    TeletextCtrlCmd ctrlCmd;
    TeletextEvent event;
} TeletextParam;

struct SubtitleParamType {
    SubtitleType subType;
    DtvSubtitleType dtvSubType;

    // TODO: maybe, use union for params
    Scte27Param scteParam;
    CcParam ccParam;
    TeletextParam ttParam;

    DtvKitDvbParam dtvkitDvbParam; //the pes pid for filter subtitle data from demux
    SubtitleParamType() {
        subType = TYPE_SUBTITLE_INVALID;
        ttParam.event = TT_EVENT_INVALID;
    }

    void update() {
        switch (dtvSubType) {
            case DTV_SUB_CC:
                subType = TYPE_SUBTITLE_CLOSED_CATPTION;
                break;
            case DTV_SUB_SCTE27:
                subType = TYPE_SUBTITLE_SCTE27;
                break;
		case DTV_SUB_DTVKIT_DVB:
		    subType = TYPE_SUBTITLE_DTVKIT_DVB;
		    break;
		case DTV_SUB_DTVKIT_TELETEXT:
		    subType = TYPE_SUBTITLE_DTVKIT_TELETEXT;
		    break;
		case DTV_SUB_DTVKIT_SCTE27:
		    subType = TYPE_SUBTITLE_DTVKIT_SCTE27;
		    break;
            default:
                break;
        }
    }

    bool isValidDtvParams () {
        return dtvSubType == DTV_SUB_CC || dtvSubType == DTV_SUB_SCTE27 ||dtvSubType == DTV_SUB_DTVKIT_SCTE27 ; //only cc or scte27 valid
    }

    void dump(int fd, const char * prefix) {
        dprintf(fd, "%s subType: %d\n", prefix, subType);
        dprintf(fd, "%s dtvSubType: %d\n", prefix, dtvSubType);
        dprintf(fd, "%s   SCTE27 (PID: %d)\n", prefix, scteParam.SCTE27_PID);
        dprintf(fd, "%s   CC     (ChannelID: %d vfmt: %d)\n", prefix, ccParam.ChannelID, ccParam.vfmt);
        dprintf(fd, "%s   TelTxt (PageNo: %d subPageNo: %d PageDir: %d subPageDir: %d ctlCmd: %d)\n",
            prefix, ttParam.pageNo, ttParam.subPageNo, ttParam.pageDir, ttParam.subPageDir, ttParam.ctrlCmd);
    }
};


class ParserFactory {

public:
    static std::shared_ptr<Parser> create(std::shared_ptr<SubtitleParamType>, std::shared_ptr<DataSource> source);
    static DisplayType getDisplayType(int type);

};


#endif
