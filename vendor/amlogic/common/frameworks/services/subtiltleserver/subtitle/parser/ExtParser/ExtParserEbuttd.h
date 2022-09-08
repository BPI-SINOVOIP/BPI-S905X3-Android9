/*
 * Copyright (c) 2020-2024 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __EXT_EBUTT_D_H_
#define __EXT_EBUTT_D_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class ExtParserEbuttd: public ExtParser {
public:
    ExtParserEbuttd(std::shared_ptr<DataSource> source);
    ~ExtParserEbuttd();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);

private:
    std::shared_ptr<DataSource> mDataSource;
    int parseEbuttd(char *buffer);

};

#endif


