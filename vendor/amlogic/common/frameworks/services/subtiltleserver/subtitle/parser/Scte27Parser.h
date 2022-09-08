#ifndef __SUBTITLE_SCTE27_PARSER_H__
#define __SUBTITLE_SCTE27_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types2.h"
#include "sub_types.h"

#define BUFFER_W 720
#define BUFFER_H 480
#define VIEDEO_W BUFFER_W
#define VIEDEO_H BUFFER_H

#define DEMUX_ID 2
#define BITMAP_PITCH BUFFER_W*4


class Scte27Parser: public Parser {
public:
    Scte27Parser(std::shared_ptr<DataSource> source);
    virtual ~Scte27Parser();
    virtual int parse();
    virtual bool updateParameter(int type, void *data);
    virtual void dump(int fd, const char *prefix);

    TVSubtitleData *getContexts() {
        return mScteContext;
    }
    void notifySubtitleDimension(int width, int height);
    static inline Scte27Parser *getCurrentInstance();
    void notifyCallerAvail(int avail);
    void notifyCallerLanguage(char *lang);
    TVSubtitleData *mScteContext;
    std::shared_ptr<uint8_t> mData;
private:
    int getSpu();
    int stopScte27();
    int startScte27(int dmxId, int pid);
    void checkDebug();

   // TVSubtitleData *mScteContext;
    int mPid;

    static Scte27Parser *sInstance;
};


#endif

