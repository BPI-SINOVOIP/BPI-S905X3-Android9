/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __AQTILTLE_H_
#define __AQTILTLE_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class Aqtitle: public ExtParser {
public:
    Aqtitle(std::shared_ptr<DataSource> source);
    ~Aqtitle();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    int mPtsRate;
    std::shared_ptr<DataSource> mDataSource;
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

