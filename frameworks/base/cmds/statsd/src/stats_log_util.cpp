/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "hash.h"
#include "stats_log_util.h"

#include <logd/LogEvent.h>
#include <private/android_filesystem_config.h>
#include <utils/Log.h>
#include <set>
#include <stack>
#include <utils/Log.h>
#include <utils/SystemClock.h>

using android::util::FIELD_COUNT_REPEATED;
using android::util::FIELD_TYPE_BOOL;
using android::util::FIELD_TYPE_FLOAT;
using android::util::FIELD_TYPE_INT32;
using android::util::FIELD_TYPE_INT64;
using android::util::FIELD_TYPE_UINT64;
using android::util::FIELD_TYPE_FIXED64;
using android::util::FIELD_TYPE_MESSAGE;
using android::util::FIELD_TYPE_STRING;
using android::util::ProtoOutputStream;

namespace android {
namespace os {
namespace statsd {

// for DimensionsValue Proto
const int DIMENSIONS_VALUE_FIELD = 1;
const int DIMENSIONS_VALUE_VALUE_STR = 2;
const int DIMENSIONS_VALUE_VALUE_INT = 3;
const int DIMENSIONS_VALUE_VALUE_LONG = 4;
// const int DIMENSIONS_VALUE_VALUE_BOOL = 5; // logd doesn't have bool data type.
const int DIMENSIONS_VALUE_VALUE_FLOAT = 6;
const int DIMENSIONS_VALUE_VALUE_TUPLE = 7;
const int DIMENSIONS_VALUE_VALUE_STR_HASH = 8;

const int DIMENSIONS_VALUE_TUPLE_VALUE = 1;

// for PulledAtomStats proto
const int FIELD_ID_PULLED_ATOM_STATS = 10;
const int FIELD_ID_PULL_ATOM_ID = 1;
const int FIELD_ID_TOTAL_PULL = 2;
const int FIELD_ID_TOTAL_PULL_FROM_CACHE = 3;
const int FIELD_ID_MIN_PULL_INTERVAL_SEC = 4;

namespace {

void writeDimensionToProtoHelper(const std::vector<FieldValue>& dims, size_t* index, int depth,
                                 int prefix, std::set<string> *str_set,
                                 ProtoOutputStream* protoOutput) {
    size_t count = dims.size();
    while (*index < count) {
        const auto& dim = dims[*index];
        const int valueDepth = dim.mField.getDepth();
        const int valuePrefix = dim.mField.getPrefix(depth);
        const int fieldNum = dim.mField.getPosAtDepth(depth);
        if (valueDepth > 2) {
            ALOGE("Depth > 2 not supported");
            return;
        }

        if (depth == valueDepth && valuePrefix == prefix) {
            uint64_t token = protoOutput->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                                 DIMENSIONS_VALUE_TUPLE_VALUE);
            protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_FIELD, fieldNum);
            switch (dim.mValue.getType()) {
                case INT:
                    protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_VALUE_INT,
                                       dim.mValue.int_value);
                    break;
                case LONG:
                    protoOutput->write(FIELD_TYPE_INT64 | DIMENSIONS_VALUE_VALUE_LONG,
                                       (long long)dim.mValue.long_value);
                    break;
                case FLOAT:
                    protoOutput->write(FIELD_TYPE_FLOAT | DIMENSIONS_VALUE_VALUE_FLOAT,
                                       dim.mValue.float_value);
                    break;
                case STRING:
                    if (str_set == nullptr) {
                        protoOutput->write(FIELD_TYPE_STRING | DIMENSIONS_VALUE_VALUE_STR,
                                           dim.mValue.str_value);
                    } else {
                        str_set->insert(dim.mValue.str_value);
                        protoOutput->write(
                                FIELD_TYPE_UINT64 | DIMENSIONS_VALUE_VALUE_STR_HASH,
                                (long long)Hash64(dim.mValue.str_value));
                    }
                    break;
                default:
                    break;
            }
            if (token != 0) {
                protoOutput->end(token);
            }
            (*index)++;
        } else if (valueDepth > depth && valuePrefix == prefix) {
            // Writing the sub tree
            uint64_t dimensionToken = protoOutput->start(
                    FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED | DIMENSIONS_VALUE_TUPLE_VALUE);
            protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_FIELD, fieldNum);
            uint64_t tupleToken =
                    protoOutput->start(FIELD_TYPE_MESSAGE | DIMENSIONS_VALUE_VALUE_TUPLE);
            writeDimensionToProtoHelper(dims, index, valueDepth, dim.mField.getPrefix(valueDepth),
                                        str_set, protoOutput);
            protoOutput->end(tupleToken);
            protoOutput->end(dimensionToken);
        } else {
            // Done with the prev sub tree
            return;
        }
    }
}

void writeDimensionLeafToProtoHelper(const std::vector<FieldValue>& dims,
                                     const int dimensionLeafField,
                                     size_t* index, int depth,
                                     int prefix, std::set<string> *str_set,
                                     ProtoOutputStream* protoOutput) {
    size_t count = dims.size();
    while (*index < count) {
        const auto& dim = dims[*index];
        const int valueDepth = dim.mField.getDepth();
        const int valuePrefix = dim.mField.getPrefix(depth);
        if (valueDepth > 2) {
            ALOGE("Depth > 2 not supported");
            return;
        }

        if (depth == valueDepth && valuePrefix == prefix) {
            uint64_t token = protoOutput->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                                dimensionLeafField);
            switch (dim.mValue.getType()) {
                case INT:
                    protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_VALUE_INT,
                                       dim.mValue.int_value);
                    break;
                case LONG:
                    protoOutput->write(FIELD_TYPE_INT64 | DIMENSIONS_VALUE_VALUE_LONG,
                                       (long long)dim.mValue.long_value);
                    break;
                case FLOAT:
                    protoOutput->write(FIELD_TYPE_FLOAT | DIMENSIONS_VALUE_VALUE_FLOAT,
                                       dim.mValue.float_value);
                    break;
                case STRING:
                    if (str_set == nullptr) {
                        protoOutput->write(FIELD_TYPE_STRING | DIMENSIONS_VALUE_VALUE_STR,
                                           dim.mValue.str_value);
                    } else {
                        str_set->insert(dim.mValue.str_value);
                        protoOutput->write(
                                FIELD_TYPE_UINT64 | DIMENSIONS_VALUE_VALUE_STR_HASH,
                                (long long)Hash64(dim.mValue.str_value));
                    }
                    break;
                default:
                    break;
            }
            if (token != 0) {
                protoOutput->end(token);
            }
            (*index)++;
        } else if (valueDepth > depth && valuePrefix == prefix) {
            writeDimensionLeafToProtoHelper(dims, dimensionLeafField,
                                            index, valueDepth, dim.mField.getPrefix(valueDepth),
                                            str_set, protoOutput);
        } else {
            // Done with the prev sub tree
            return;
        }
    }
}

void writeDimensionPathToProtoHelper(const std::vector<Matcher>& fieldMatchers,
                                     size_t* index, int depth, int prefix,
                                     ProtoOutputStream* protoOutput) {
    size_t count = fieldMatchers.size();
    while (*index < count) {
        const Field& field = fieldMatchers[*index].mMatcher;
        const int valueDepth = field.getDepth();
        const int valuePrefix = field.getPrefix(depth);
        const int fieldNum = field.getPosAtDepth(depth);
        if (valueDepth > 2) {
            ALOGE("Depth > 2 not supported");
            return;
        }

        if (depth == valueDepth && valuePrefix == prefix) {
            uint64_t token = protoOutput->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED |
                                                 DIMENSIONS_VALUE_TUPLE_VALUE);
            protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_FIELD, fieldNum);
            if (token != 0) {
                protoOutput->end(token);
            }
            (*index)++;
        } else if (valueDepth > depth && valuePrefix == prefix) {
            // Writing the sub tree
            uint64_t dimensionToken = protoOutput->start(
                    FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED | DIMENSIONS_VALUE_TUPLE_VALUE);
            protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_FIELD, fieldNum);
            uint64_t tupleToken =
                    protoOutput->start(FIELD_TYPE_MESSAGE | DIMENSIONS_VALUE_VALUE_TUPLE);
            writeDimensionPathToProtoHelper(fieldMatchers, index, valueDepth,
                                            field.getPrefix(valueDepth), protoOutput);
            protoOutput->end(tupleToken);
            protoOutput->end(dimensionToken);
        } else {
            // Done with the prev sub tree
            return;
        }
    }
}

}  // namespace

void writeDimensionToProto(const HashableDimensionKey& dimension, std::set<string> *str_set,
                           ProtoOutputStream* protoOutput) {
    if (dimension.getValues().size() == 0) {
        return;
    }
    protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_FIELD,
                       dimension.getValues()[0].mField.getTag());
    uint64_t topToken = protoOutput->start(FIELD_TYPE_MESSAGE | DIMENSIONS_VALUE_VALUE_TUPLE);
    size_t index = 0;
    writeDimensionToProtoHelper(dimension.getValues(), &index, 0, 0, str_set, protoOutput);
    protoOutput->end(topToken);
}

void writeDimensionLeafNodesToProto(const HashableDimensionKey& dimension,
                                    const int dimensionLeafFieldId,
                                    std::set<string> *str_set,
                                    ProtoOutputStream* protoOutput) {
    if (dimension.getValues().size() == 0) {
        return;
    }
    size_t index = 0;
    writeDimensionLeafToProtoHelper(dimension.getValues(), dimensionLeafFieldId,
                                    &index, 0, 0, str_set, protoOutput);
}

void writeDimensionPathToProto(const std::vector<Matcher>& fieldMatchers,
                               ProtoOutputStream* protoOutput) {
    if (fieldMatchers.size() == 0) {
        return;
    }
    protoOutput->write(FIELD_TYPE_INT32 | DIMENSIONS_VALUE_FIELD,
                       fieldMatchers[0].mMatcher.getTag());
    uint64_t topToken = protoOutput->start(FIELD_TYPE_MESSAGE | DIMENSIONS_VALUE_VALUE_TUPLE);
    size_t index = 0;
    writeDimensionPathToProtoHelper(fieldMatchers, &index, 0, 0, protoOutput);
    protoOutput->end(topToken);
}

// Supported Atoms format
// XYZ_Atom {
//     repeated SubMsg field_1 = 1;
//     SubMsg2 field_2 = 2;
//     int32/float/string/int63 field_3 = 3;
// }
// logd's msg format, doesn't allow us to distinguish between the 2 cases below
// Case (1):
// Atom {
//   SubMsg {
//     int i = 1;
//     int j = 2;
//   }
//   repeated SubMsg
// }
//
// and case (2):
// Atom {
//   SubMsg {
//     repeated int i = 1;
//     repeated int j = 2;
//   }
//   optional SubMsg = 1;
// }
//
//
void writeFieldValueTreeToStreamHelper(const std::vector<FieldValue>& dims, size_t* index,
                                       int depth, int prefix, ProtoOutputStream* protoOutput) {
    size_t count = dims.size();
    while (*index < count) {
        const auto& dim = dims[*index];
        const int valueDepth = dim.mField.getDepth();
        const int valuePrefix = dim.mField.getPrefix(depth);
        const int fieldNum = dim.mField.getPosAtDepth(depth);
        if (valueDepth > 2) {
            ALOGE("Depth > 2 not supported");
            return;
        }

        if (depth == valueDepth && valuePrefix == prefix) {
            switch (dim.mValue.getType()) {
                case INT:
                    protoOutput->write(FIELD_TYPE_INT32 | fieldNum, dim.mValue.int_value);
                    break;
                case LONG:
                    protoOutput->write(FIELD_TYPE_INT64 | fieldNum,
                                       (long long)dim.mValue.long_value);
                    break;
                case FLOAT:
                    protoOutput->write(FIELD_TYPE_FLOAT | fieldNum, dim.mValue.float_value);
                    break;
                case STRING:
                    protoOutput->write(FIELD_TYPE_STRING | fieldNum, dim.mValue.str_value);
                    break;
                default:
                    break;
            }
            (*index)++;
        } else if (valueDepth > depth && valuePrefix == prefix) {
            // Writing the sub tree
            uint64_t msg_token = 0ULL;
            if (valueDepth == depth + 2) {
                msg_token =
                        protoOutput->start(FIELD_TYPE_MESSAGE | FIELD_COUNT_REPEATED | fieldNum);
            } else if (valueDepth == depth + 1) {
                msg_token = protoOutput->start(FIELD_TYPE_MESSAGE | fieldNum);
            }
            // Directly jump to the leaf value because the repeated position field is implied
            // by the position of the sub msg in the parent field.
            writeFieldValueTreeToStreamHelper(dims, index, valueDepth,
                                              dim.mField.getPrefix(valueDepth), protoOutput);
            if (msg_token != 0) {
                protoOutput->end(msg_token);
            }
        } else {
            // Done with the prev sub tree
            return;
        }
    }
}

void writeFieldValueTreeToStream(int tagId, const std::vector<FieldValue>& values,
                                 util::ProtoOutputStream* protoOutput) {
    uint64_t atomToken = protoOutput->start(FIELD_TYPE_MESSAGE | tagId);

    size_t index = 0;
    writeFieldValueTreeToStreamHelper(values, &index, 0, 0, protoOutput);
    protoOutput->end(atomToken);
}

int64_t TimeUnitToBucketSizeInMillisGuardrailed(int uid, TimeUnit unit) {
    int64_t bucketSizeMillis = TimeUnitToBucketSizeInMillis(unit);
    if (bucketSizeMillis > 1000 && bucketSizeMillis < 5 * 60 * 1000LL && uid != AID_SHELL &&
        uid != AID_ROOT) {
        bucketSizeMillis = 5 * 60 * 1000LL;
    }
    return bucketSizeMillis;
}

int64_t TimeUnitToBucketSizeInMillis(TimeUnit unit) {
    switch (unit) {
        case ONE_MINUTE:
            return 60 * 1000LL;
        case FIVE_MINUTES:
            return 5 * 60 * 1000LL;
        case TEN_MINUTES:
            return 10 * 60 * 1000LL;
        case THIRTY_MINUTES:
            return 30 * 60 * 1000LL;
        case ONE_HOUR:
            return 60 * 60 * 1000LL;
        case THREE_HOURS:
            return 3 * 60 * 60 * 1000LL;
        case SIX_HOURS:
            return 6 * 60 * 60 * 1000LL;
        case TWELVE_HOURS:
            return 12 * 60 * 60 * 1000LL;
        case ONE_DAY:
            return 24 * 60 * 60 * 1000LL;
        case CTS:
            return 1000;
        case TIME_UNIT_UNSPECIFIED:
        default:
            return -1;
    }
}

void writePullerStatsToStream(const std::pair<int, StatsdStats::PulledAtomStats>& pair,
                              util::ProtoOutputStream* protoOutput) {
    uint64_t token = protoOutput->start(FIELD_TYPE_MESSAGE | FIELD_ID_PULLED_ATOM_STATS |
                                         FIELD_COUNT_REPEATED);
    protoOutput->write(FIELD_TYPE_INT32 | FIELD_ID_PULL_ATOM_ID, (int32_t)pair.first);
    protoOutput->write(FIELD_TYPE_INT64 | FIELD_ID_TOTAL_PULL, (long long)pair.second.totalPull);
    protoOutput->write(FIELD_TYPE_INT64 | FIELD_ID_TOTAL_PULL_FROM_CACHE,
                       (long long)pair.second.totalPullFromCache);
    protoOutput->write(FIELD_TYPE_INT64 | FIELD_ID_MIN_PULL_INTERVAL_SEC,
                       (long long)pair.second.minPullIntervalSec);
    protoOutput->end(token);
}

int64_t getElapsedRealtimeNs() {
    return ::android::elapsedRealtimeNano();
}

int64_t getElapsedRealtimeSec() {
    return ::android::elapsedRealtimeNano() / NS_PER_SEC;
}

int64_t getElapsedRealtimeMillis() {
    return ::android::elapsedRealtime();
}

int64_t getWallClockNs() {
    return time(nullptr) * NS_PER_SEC;
}

int64_t getWallClockSec() {
    return time(nullptr);
}

int64_t getWallClockMillis() {
    return time(nullptr) * MS_PER_SEC;
}

int64_t truncateTimestampNsToFiveMinutes(int64_t timestampNs) {
    return timestampNs / NS_PER_SEC / (5 * 60) * NS_PER_SEC * (5 * 60);
}

int64_t NanoToMillis(const int64_t nano) {
    return nano / 1000000;
}

int64_t MillisToNano(const int64_t millis) {
    return millis * 1000000;
}

}  // namespace statsd
}  // namespace os
}  // namespace android
