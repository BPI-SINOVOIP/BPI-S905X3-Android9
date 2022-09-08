/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __MIRCODVD_H_
#define __MIRCODVD_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class Mircodvd: public ExtParser {
public:
    Mircodvd(std::shared_ptr<DataSource> source);
    ~Mircodvd();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    int mPtsRate = 24;
    bool mGotPtsRate = true;
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

