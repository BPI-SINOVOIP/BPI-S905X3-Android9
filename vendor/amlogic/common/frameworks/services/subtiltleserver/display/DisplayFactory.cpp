#include "ParserFactory.h"

#include "DemoParser.h"
#include "AssParser.h"

static std::shared_ptr<Parser> ParserFactory::create(
            enum SubtitleType,
            std::shared_ptr<DataSource> source) {
    switch (SubtitleType) {
        case TYPE_SUBTITLE_VOB:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_PGS:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_MKV_STR:
            return std::shared_ptr<Parser>(new AssParser(source));
        case TYPE_SUBTITLE_MKV_VOB:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_SSA:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_DVB:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_TMD_TXT:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_IDX_SUB:
            return std::shared_ptr<Parser>(new DemoParser(source));
        case TYPE_SUBTITLE_DVB_TELETEXT:
            return std::shared_ptr<Parser>(new DemoParser(source));
    }

    // TODO: change to stubParser
    return std::shared_ptr<Parser>(new DemoParser(source));
}

