/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SUBRIP09_H_
#define __SUBRIP09_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class Subrip09: public ExtParser {
public:
    Subrip09(std::shared_ptr<DataSource> source);
    ~Subrip09();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

