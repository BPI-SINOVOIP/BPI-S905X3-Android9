/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __TTML_H_
#define __TTML_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class Ttml: public ExtParser {
public:
    Ttml(std::shared_ptr<DataSource> source);
    ~Ttml();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    std::shared_ptr<DataSource> mDataSource;
    int parseTtml(char *buffer);

};

#endif

