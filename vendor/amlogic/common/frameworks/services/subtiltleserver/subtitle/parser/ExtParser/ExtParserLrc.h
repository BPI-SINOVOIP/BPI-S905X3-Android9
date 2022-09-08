/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __EXTPARSER_LRC_H_
#define __EXTPARSER_LRC_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class ExtParserLrc: public ExtParser {
public:
    ExtParserLrc(std::shared_ptr<DataSource> source);
    ~ExtParserLrc();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

