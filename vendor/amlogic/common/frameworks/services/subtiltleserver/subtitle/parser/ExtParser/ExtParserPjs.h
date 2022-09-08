/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __EXTPARSER_PJS_H_
#define __EXTPARSER_PJS_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class ExtParserPjs: public ExtParser {
public:
    ExtParserPjs(std::shared_ptr<DataSource> source);
    ~ExtParserPjs();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    int mPtsRate;
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

