#ifndef __SUBTITLE_DATASOURCE_FACTORY_H__
#define __SUBTITLE_DATASOURCE_FACTORY_H__

#include "DataSource.h"

class DataSourceFactory {

public:
    static std::shared_ptr<DataSource> create(SubtitleIOType type);
    // TODO: use std::string &url
    static std::shared_ptr<DataSource> create(int fd, SubtitleIOType type);

};

#endif
