#ifndef __SUBTITLE_DVD_PARSER_H__
#define __SUBTITLE_DVD_PARSER_H__
#include "Parser.h"
#include "DataSource.h"
#include "sub_types.h"

class DvdParser: public Parser {
public:
    DvdParser(std::shared_ptr<DataSource> source);
    virtual ~DvdParser();
    virtual int parse();
    virtual void dump(int fd, const char *prefix);

private:

    int getSpu(std::shared_ptr<AML_SPUVAR> spu);
    int getInterSpu();
    void checkDebug();

    int getVobSpu(char *spuBuffer, int *bufsize, unsigned length, std::shared_ptr<AML_SPUVAR> spu);
    void fillResize(std::shared_ptr<AML_SPUVAR> spu);
    unsigned char spuFillPixel(unsigned short *pixelIn, char *pixelOut,
                               std::shared_ptr<AML_SPUVAR> subFrame, int n);

    void parsePalette(char *p);
    int parseExtradata(char *extradata, int extradataSize);
    unsigned int *parseInterSpu(int *buffer, std::shared_ptr<AML_SPUVAR> spu);

    // for parser left chars
    int mRestLen;
    char *mRestbuf;

    // dvb Platte
    uint32_t mPalette[16];
    bool mHasPalette;


    bool mDumpSub;
};


#endif

