/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __MPSUB_H_
#define __MPSUB_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class Mpsub: public ExtParser {
public:
    Mpsub(std::shared_ptr<DataSource> source);
    ~Mpsub();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    float mMpsubPosition;
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

