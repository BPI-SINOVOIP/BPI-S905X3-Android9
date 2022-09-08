#ifndef _EXT_PARSER_FACTORY_H__
#define _EXT_PARSER_FACTORY_H__
#include "DataSource.h"

class ExtParserFactory {

public:
    static std::shared_ptr<Parser> CreateExtSubObject(std::shared_ptr<DataSource> source);
    static int getExtSubtitleformat(std::shared_ptr<DataSource> source);
private:
    static int getXmlExtentFormt(std::shared_ptr<DataSource> source);
};
#endif

