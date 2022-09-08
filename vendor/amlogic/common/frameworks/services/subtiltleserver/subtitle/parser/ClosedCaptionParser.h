#ifndef __SUBTITLE_CLOSED_CAPTION_PARSER_H__
#define __SUBTITLE_CLOSED_CAPTION_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types2.h"
#include "sub_types.h"


class ClosedCaptionParser: public Parser {
public:
    ClosedCaptionParser(std::shared_ptr<DataSource> source);
    virtual ~ClosedCaptionParser();
    virtual int parse();
    virtual bool updateParameter(int type, void *data);
    virtual void dump(int fd, const char *prefix);


    static inline char sJsonStr[CC_JSON_BUFFER_SIZE];
    static inline ClosedCaptionParser *getCurrentInstance();
    void notifyChannelState(int stat, int chnnlId);
    void notifyAvailable(int err);

private:
    static void saveJsonStr(char * str);
    void setDvbDebugLogLevel();
    //void afd_evt_callback(long dev_no, int event_type, void *param, void *user_data);
    //void json_update_cb(AM_CC_Handle_t handle);
    int startAtscCc(int source, int vfmt, int caption, int fgColor,int fgOpacity,
            int bgColor, int bgOpacity, int fontStyle, int fontSize);

    int startAmlCC();
    int stopAmlCC();

    int mChannelId = 0;
    int mDevNo = 0; // userdata device number
    int mVfmt = -1;

    TVSubtitleData *mCcContext;


    static ClosedCaptionParser *sInstance;
};


#endif

