/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SUBRIP_H_
#define __SUBRIP_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class Subrip: public ExtParser {
public:
    Subrip(std::shared_ptr<DataSource> source);
    ~Subrip();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

