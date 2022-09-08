#ifndef __SUBTITLE_PGS_PARSER_H__
#define __SUBTITLE_PGS_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types.h"

#define OSD_HALF_SIZE 1920*1280/8

struct PgsSubtitleEpgs;

class PgsParser: public Parser {
public:
    PgsParser(std::shared_ptr<DataSource> source);
    virtual ~PgsParser();
    virtual int parse();
    virtual void dump(int fd, const char *prefix);

private:
    int getSpu(std::shared_ptr<AML_SPUVAR> spu);
    int getInterSpu();
    void checkDebug();
    int decode(std::shared_ptr<AML_SPUVAR> spu, unsigned char *psrc);
    int parserOnePgs(std::shared_ptr<AML_SPUVAR> spu);

    int softDemuxParse(std::shared_ptr<AML_SPUVAR> spu);
    int hwDemuxParse(std::shared_ptr<AML_SPUVAR> spu);
    int mDumpSub;

    PgsSubtitleEpgs *mPgsEpgs;
};


#endif

