#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// DATA Type: All first char is upper Case letter.
// API functions: symbols start with "amlsub_"

typedef void *AmlSubtitleHnd;

typedef enum {
    E_SUBTITLE_FMQ = 0,   /* for soft support subtitle data, use hidl FastMessageQueue */
    E_SUBTITLE_DEV,       /* use /dev/amstream_sub_read as the IO source */
    E_SUBTITLE_FILE,      /* for external subtitle file */
    E_SUBTITLE_SOCK,      /* deprecated, android not permmit to use anymore */
    E_SUBTITLE_DEMUX,     /* use aml hwdemux as the data source */
} AmlSubtitleIOType;


typedef enum {
    SUB_DATA_TYPE_STRING = 0,
    SUB_DATA_TYPE_CC_JSON = 1,
    SUB_DATA_TYPE_BITMAP = 2,
    SUB_DATA_TYPE_POSITON_BITMAP = 4,
} AmlSubDataType;

typedef enum {
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
    TYPE_SUBTITLE_EXTERNAL,
    TYPE_SUBTITLE_MAX,
} AmlSubtitletype;


typedef enum {
    /*invalid calling, wrong parameter */
    SUB_STAT_INV = -1,
    /* calling failed */
    SUB_STAT_FAIL = 0,
    SUB_STAT_OK,
} AmlSubtitleStatus;

typedef enum {
    SUBCMD_CC_VIDEO_FORMAT = 1,
    SUBCMD_CC_CHANNEL_ID = 2,
    SUBCMD_DVB_ANCILLARY_PAGE_ID = 3,
    SUBCMD_DVB_COMPOSITION_PAGE_ID = 4,
    SUBCMD_PID = 5,
} AmlSubtitleParamCmd;



typedef enum {
   TT_EVENT_INVALID          = -1,

   // These are the four FastText shortcuts, usually represented by red, green,
   // yellow and blue keys on the handset.
   TT_EVENT_QUICK_NAVIGATE_RED = 0,
   TT_EVENT_QUICK_NAVIGATE_GREEN,
   TT_EVENT_QUICK_NAVIGATE_YELLOW,
   TT_EVENT_QUICK_NAVIGATE_BLUE,

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

   // This is the home key, which returns to the nominated index page for this service.
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
} AmlTeletextEvent;

typedef struct {
    const char *extSubPath;
    AmlSubtitleIOType ioSource;
    int subtitleType;
    int pid;
    int videoFormat;             //cc
    int channelId;               //cc
    int ancillaryPageId;         //dvb
    int compositionPageId;       //dvb
} AmlSubtitleParam;


typedef struct {
    int magazine;
    int page;
    AmlTeletextEvent event;
} AmlTeletextCtrlParam;


// Callback functions.
typedef void (*AmlSubtitleDataCb)(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight,
                int showing);

typedef void (*AmlAfdEventCb)(int event);

typedef void (*AmlChannelUpdateCb)(int event, int id);

typedef void (*AmlSubtitleAvailCb)(int avail);

typedef void (*AmlSubtitleDimensionCb)(int width, int height);

typedef void (*AmlSubtitleLanguageCb)(char *lang);

typedef void (*AmlSubtitleInfoCb)(int what, int extra);



////////////////////////////////////////////////////////////
///////////////////// Session manager //////////////////////
////////////////////////////////////////////////////////////
AmlSubtitleHnd amlsub_Create();
AmlSubtitleStatus amlsub_Destroy(AmlSubtitleHnd handle);


////////////////////////////////////////////////////////////
///////////////////// generic control //////////////////////
////////////////////////////////////////////////////////////
/**
 * open, start to play subtitle.
 * Param:
 *   handle: current subtitle handle.
 *   path: the subtitle external file path.
 */
AmlSubtitleStatus amlsub_Open(AmlSubtitleHnd handle, AmlSubtitleParam *param);
AmlSubtitleStatus amlsub_Close(AmlSubtitleHnd handle);

/**
 * reset current play, mostly, used for seek.
 */
AmlSubtitleStatus amlsub_Reset(AmlSubtitleHnd handle);

////////////////////////////////////////////////////////////
//////////////// DTV operation/param related ///////////////
////////////////////////////////////////////////////////////
AmlSubtitleStatus amlsub_SetParameter(AmlSubtitleHnd handle, AmlSubtitleParamCmd cmd, void *value, int paramSize);
int amlsub_GetParameter(AmlSubtitleHnd handle, AmlSubtitleParamCmd cmd, void *value);

AmlSubtitleStatus amlsub_TeletextControl(AmlSubtitleHnd handle, AmlTeletextCtrlParam *param);

AmlSubtitleStatus amlsub_SelectCcChannel(AmlSubtitleHnd handle, int ch);


////////////////////////////////////////////////////////////
////// Regist callbacks for subtitle Event and data ////////
////////////////////////////////////////////////////////////
AmlSubtitleStatus amlsub_RegistOnDataCB(AmlSubtitleHnd handle, AmlSubtitleDataCb listener);
AmlSubtitleStatus amlsub_RegistAfdEventCB(AmlSubtitleHnd handle, AmlAfdEventCb listener);
AmlSubtitleStatus amlsub_RegistOnChannelUpdateCb(AmlSubtitleHnd handle, AmlChannelUpdateCb listener);
AmlSubtitleStatus amlsub_RegistOnSubtitleAvailCb(AmlSubtitleHnd handle, AmlSubtitleAvailCb listener);
AmlSubtitleStatus amlsub_RegistGetDimesionCb(AmlSubtitleHnd handle, AmlSubtitleDimensionCb listener);
AmlSubtitleStatus amlsub_RegistOnSubtitleLanguageCb(AmlSubtitleHnd handle, AmlSubtitleLanguageCb listener);
AmlSubtitleStatus amlsub_RegistOnSubtitleInfo(AmlSubtitleHnd handle, AmlSubtitleInfoCb listener);


////////////////////////////////////////////////////////////
//////////////////// Standalone UI related /////////////////
////////////////////////////////////////////////////////////

/* Bellow APIs only available for control standalone subtitle display */
/* !!!!DEPRECATED!!!! suggest not use this. */
/* Suggest:
           1. Java App use SubtitleManager java API to display subtitle.
           2. use above callback functions receive decoded SPU and render locally.
*/

AmlSubtitleStatus amlsub_UiShow(AmlSubtitleHnd handle);
AmlSubtitleStatus amlsub_UiHide(AmlSubtitleHnd handle);

/* Only available for text subtitle */
AmlSubtitleStatus amlsub_UiSetTextColor(AmlSubtitleHnd handle, int color);
AmlSubtitleStatus amlsub_UiSetTextSize(AmlSubtitleHnd handle, int sp);
AmlSubtitleStatus amlsub_UiSetGravity(AmlSubtitleHnd handle, int gravity);

AmlSubtitleStatus amlsub_UiSetPosHeight(AmlSubtitleHnd handle, int yOffset);
AmlSubtitleStatus amlsub_UiSetImgRatio(AmlSubtitleHnd handle, float ratioW, float ratioH, int maxW, int maxH);
AmlSubtitleStatus amlsub_UiSetSurfaceViewRect(AmlSubtitleHnd handle, int x, int y, int width, int height);


#ifdef __cplusplus
}
#endif
