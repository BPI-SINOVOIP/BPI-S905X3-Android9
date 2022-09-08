#ifndef __SUBTITLE_ASS_PARSER_H__
#define __SUBTITLE_ASS_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types.h"

class AssParser: public Parser {
public:
    AssParser(std::shared_ptr<DataSource> source);
    virtual ~AssParser() {
        stopParse();
    }
    virtual int parse();
    virtual void dump(int fd, const char *prefix);

private:

    int getSpu(std::shared_ptr<AML_SPUVAR> spu);
    int getInterSpu();

    // for parser left chars
    int mRestLen;
    char *mRestbuf;
};


#endif

