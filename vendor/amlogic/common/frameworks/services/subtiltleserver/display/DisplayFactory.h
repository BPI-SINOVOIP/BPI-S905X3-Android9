#ifndef _SUBTITLE_DISPLAY_FACTORY_H__
#define _SUBTITLE_DISPLAY_FACTORY_H__

#include "DataSource.h"
#include "Parser.h"

enum DisplayType {
    SUBTITLE_BITMAP_DISPLAY       = 1,
    SUBTITLE_OPENGL_DISPLAY,
    SUBTITLE_IN_APP_DISPLAY,
    SUBTITLE_SERV_APP_DISPLAY,
};



class ParserFactory {

public:
    static std::shared_ptr<Parser> create(DisplayType t, std::shared_ptr<DataSource> source);

};


#endif
