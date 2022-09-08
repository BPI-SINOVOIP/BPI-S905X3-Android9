#include <string>

#include "DataSource.h"
#include "FileSource.h"
#include "DeviceSource.h"
#include "SocketSource.h"
#include "ExternalDataSource.h"
#include "DemuxSource.h"

#include "DataSourceFactory.h"

std::shared_ptr<DataSource> DataSourceFactory::create(SubtitleIOType type) {
    switch (type) {
        case SubtitleIOType::E_SUBTITLE_FMQ:
            return std::shared_ptr<DataSource>(new ExternalDataSource());

        case SubtitleIOType::E_SUBTITLE_SOCK:
            return std::shared_ptr<DataSource>(new SocketSource());

        case SubtitleIOType::E_SUBTITLE_DEV:
            return std::shared_ptr<DataSource>(new DeviceSource());

        case SubtitleIOType::E_SUBTITLE_FILE:
            return std::shared_ptr<DataSource>(new FileSource());
        case SubtitleIOType::E_SUBTITLE_DEMUX:
            return std::shared_ptr<DataSource>(new DemuxSource());
        default:
            break;
    }
    return nullptr;
}

std::shared_ptr<DataSource> DataSourceFactory::create(int fd, SubtitleIOType type) {
    // TODO: check external subtitle
    // maybe we can impl a misc-IO impl for support both internal/ext subtitles
    switch (type) {
        case SubtitleIOType::E_SUBTITLE_FMQ:
            [[fallthrough]];
        case SubtitleIOType::E_SUBTITLE_SOCK:
            [[fallthrough]];
        case SubtitleIOType::E_SUBTITLE_DEMUX:
            [[fallthrough]];
        case SubtitleIOType::E_SUBTITLE_DEV:
            return create(type);
        case SubtitleIOType::E_SUBTITLE_FILE:
            return std::shared_ptr<DataSource>(new FileSource(fd));
        default:
            break;
    }
    return nullptr;
}
