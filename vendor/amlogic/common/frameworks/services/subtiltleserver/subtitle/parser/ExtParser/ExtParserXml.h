/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __EXTPARSER_XML_H_
#define __EXTPARSER_XML_H_

#include "ExtParser.h"
#include "ExtParserUtils.h"

class ExtParserXml: public ExtParser {
public:
    ExtParserXml(std::shared_ptr<DataSource> source);
    ~ExtParserXml();
    virtual subtitle_t * ExtSubtitleParser(subtitle_t *current);
protected:
    std::list<std::shared_ptr<AML_SPUVAR>> mDecodedSpu;

private:
    std::shared_ptr<DataSource> mDataSource;
    ExtParserUtils *mExtParserUtils = NULL;
    int mSubIndex;
    int parseXml(char *buffer);

};

#endif

