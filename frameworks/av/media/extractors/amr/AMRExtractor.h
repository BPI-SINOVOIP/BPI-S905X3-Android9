/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AMR_EXTRACTOR_H_

#define AMR_EXTRACTOR_H_

#include <utils/Errors.h>
#include <media/MediaExtractor.h>
#include <media/stagefright/MetaDataBase.h>

namespace android {

struct AMessage;
class String8;
#define OFFSET_TABLE_LEN    300

class AMRExtractor : public MediaExtractor {
public:
    explicit AMRExtractor(DataSourceBase *source);

    virtual size_t countTracks();
    virtual MediaTrack *getTrack(size_t index);
    virtual status_t getTrackMetaData(MetaDataBase& meta, size_t index, uint32_t flags);

    virtual status_t getMetaData(MetaDataBase& meta);
    virtual const char * name() { return "AMRExtractor"; }

protected:
    virtual ~AMRExtractor();

private:
    DataSourceBase *mDataSource;
    MetaDataBase mMeta;
    status_t mInitCheck;
    bool mIsWide;

    off64_t mOffsetTable[OFFSET_TABLE_LEN]; //5 min
    size_t mOffsetTableLength;

    AMRExtractor(const AMRExtractor &);
    AMRExtractor &operator=(const AMRExtractor &);
};

}  // namespace android

#endif  // AMR_EXTRACTOR_H_
