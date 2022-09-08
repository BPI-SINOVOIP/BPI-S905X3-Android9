/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SUBVIEWER2_H_
#define __SUBVIEWER2_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class SubViewer2: public ExtParser {
public:
    SubViewer2(std::shared_ptr<DataSource> source);
    ~SubViewer2();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    ExtParserUtils *mExtParserUtils = NULL;
};

#endif

