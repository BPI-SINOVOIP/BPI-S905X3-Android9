/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __EXTPARSER_SAMI_H_
#define __EXTPARSER_SAMI_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class ExtParserSami: public ExtParser {
public:
    ExtParserSami(std::shared_ptr<DataSource> source);
    ~ExtParserSami();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    int mSubSlackTime;
    int mNoTextPostProcess;
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

