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


#define LOG_TAG "PerformanceAnalysis"
// #define LOG_NDEBUG 0

#include <algorithm>
#include <climits>
#include <deque>
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <time.h>
#include <new>
#include <audio_utils/roundup.h>
#include <media/nblog/NBLog.h>
#include <media/nblog/PerformanceAnalysis.h>
#include <media/nblog/ReportPerformance.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include <queue>
#include <utility>

namespace android {

namespace ReportPerformance {

// Given an audio processing wakeup timestamp, buckets the time interval
// since the previous timestamp into a histogram, searches for
// outliers, analyzes the outlier series for unexpectedly
// small or large values and stores these as peaks
void PerformanceAnalysis::logTsEntry(timestamp ts) {
    // after a state change, start a new series and do not
    // record time intervals in-between
    if (mBufferPeriod.mPrevTs == 0) {
        mBufferPeriod.mPrevTs = ts;
        return;
    }

    // calculate time interval between current and previous timestamp
    const msInterval diffMs = static_cast<msInterval>(
        deltaMs(mBufferPeriod.mPrevTs, ts));

    const int diffJiffy = deltaJiffy(mBufferPeriod.mPrevTs, ts);

    // old versus new weight ratio when updating the buffer period mean
    static constexpr double exponentialWeight = 0.999;
    // update buffer period mean with exponential weighting
    mBufferPeriod.mMean = (mBufferPeriod.mMean < 0) ? diffMs :
            exponentialWeight * mBufferPeriod.mMean + (1.0 - exponentialWeight) * diffMs;
    // set mOutlierFactor to a smaller value for the fastmixer thread
    const int kFastMixerMax = 10;
    // NormalMixer times vary much more than FastMixer times.
    // TODO: mOutlierFactor values are set empirically based on what appears to be
    // an outlier. Learn these values from the data.
    mBufferPeriod.mOutlierFactor = mBufferPeriod.mMean < kFastMixerMax ? 1.8 : 2.0;
    // set outlier threshold
    mBufferPeriod.mOutlier = mBufferPeriod.mMean * mBufferPeriod.mOutlierFactor;

    // Check whether the time interval between the current timestamp
    // and the previous one is long enough to count as an outlier
    const bool isOutlier = detectAndStoreOutlier(diffMs);
    // If an outlier was found, check whether it was a peak
    if (isOutlier) {
        /*bool isPeak =*/ detectAndStorePeak(
            mOutlierData[0].first, mOutlierData[0].second);
        // TODO: decide whether to insert a new empty histogram if a peak
        // TODO: remove isPeak if unused to avoid "unused variable" error
        // occurred at the current timestamp
    }

    // Insert a histogram to mHists if it is empty, or
    // close the current histogram and insert a new empty one if
    // if the current histogram has spanned its maximum time interval.
    if (mHists.empty() ||
        deltaMs(mHists[0].first, ts) >= kMaxLength.HistTimespanMs) {
        mHists.emplace_front(ts, std::map<int, int>());
        // When memory is full, delete oldest histogram
        // TODO: use a circular buffer
        if (mHists.size() >= kMaxLength.Hists) {
            mHists.resize(kMaxLength.Hists);
        }
    }
    // add current time intervals to histogram
    ++mHists[0].second[diffJiffy];
    // update previous timestamp
    mBufferPeriod.mPrevTs = ts;
}


// forces short-term histogram storage to avoid adding idle audio time interval
// to buffer period data
void PerformanceAnalysis::handleStateChange() {
    mBufferPeriod.mPrevTs = 0;
    return;
}


// Checks whether the time interval between two outliers is far enough from
// a typical delta to be considered a peak.
// looks for changes in distribution (peaks), which can be either positive or negative.
// The function sets the mean to the starting value and sigma to 0, and updates
// them as long as no peak is detected. When a value is more than 'threshold'
// standard deviations from the mean, a peak is detected and the mean and sigma
// are set to the peak value and 0.
bool PerformanceAnalysis::detectAndStorePeak(msInterval diff, timestamp ts) {
    bool isPeak = false;
    if (mOutlierData.empty()) {
        return false;
    }
    // Update mean of the distribution
    // TypicalDiff is used to check whether a value is unusually large
    // when we cannot use standard deviations from the mean because the sd is set to 0.
    mOutlierDistribution.mTypicalDiff = (mOutlierDistribution.mTypicalDiff *
            (mOutlierData.size() - 1) + diff) / mOutlierData.size();

    // Initialize short-term mean at start of program
    if (mOutlierDistribution.mMean == 0) {
        mOutlierDistribution.mMean = diff;
    }
    // Update length of current sequence of outliers
    mOutlierDistribution.mN++;

    // Check whether a large deviation from the mean occurred.
    // If the standard deviation has been reset to zero, the comparison is
    // instead to the mean of the full mOutlierInterval sequence.
    if ((fabs(diff - mOutlierDistribution.mMean) <
            mOutlierDistribution.kMaxDeviation * mOutlierDistribution.mSd) ||
            (mOutlierDistribution.mSd == 0 &&
            fabs(diff - mOutlierDistribution.mMean) <
            mOutlierDistribution.mTypicalDiff)) {
        // update the mean and sd using online algorithm
        // https://en.wikipedia.org/wiki/
        // Algorithms_for_calculating_variance#Online_algorithm
        mOutlierDistribution.mN++;
        const double kDelta = diff - mOutlierDistribution.mMean;
        mOutlierDistribution.mMean += kDelta / mOutlierDistribution.mN;
        const double kDelta2 = diff - mOutlierDistribution.mMean;
        mOutlierDistribution.mM2 += kDelta * kDelta2;
        mOutlierDistribution.mSd = (mOutlierDistribution.mN < 2) ? 0 :
                sqrt(mOutlierDistribution.mM2 / (mOutlierDistribution.mN - 1));
    } else {
        // new value is far from the mean:
        // store peak timestamp and reset mean, sd, and short-term sequence
        isPeak = true;
        mPeakTimestamps.emplace_front(ts);
        // if mPeaks has reached capacity, delete oldest data
        // Note: this means that mOutlierDistribution values do not exactly
        // match the data we have in mPeakTimestamps, but this is not an issue
        // in practice for estimating future peaks.
        // TODO: turn this into a circular buffer
        if (mPeakTimestamps.size() >= kMaxLength.Peaks) {
            mPeakTimestamps.resize(kMaxLength.Peaks);
        }
        mOutlierDistribution.mMean = 0;
        mOutlierDistribution.mSd = 0;
        mOutlierDistribution.mN = 0;
        mOutlierDistribution.mM2 = 0;
    }
    return isPeak;
}


// Determines whether the difference between a timestamp and the previous
// one is beyond a threshold. If yes, stores the timestamp as an outlier
// and writes to mOutlierdata in the following format:
// Time elapsed since previous outlier: Timestamp of start of outlier
// e.g. timestamps (ms) 1, 4, 5, 16, 18, 28 will produce pairs (4, 5), (13, 18).
// TODO: learn what timestamp sequences correlate with glitches instead of
// manually designing a heuristic.
bool PerformanceAnalysis::detectAndStoreOutlier(const msInterval diffMs) {
    bool isOutlier = false;
    if (diffMs >= mBufferPeriod.mOutlier) {
        isOutlier = true;
        mOutlierData.emplace_front(
                mOutlierDistribution.mElapsed, mBufferPeriod.mPrevTs);
        // Remove oldest value if the vector is full
        // TODO: turn this into a circular buffer
        // TODO: make sure kShortHistSize is large enough that that data will never be lost
        // before being written to file or to a FIFO
        if (mOutlierData.size() >= kMaxLength.Outliers) {
            mOutlierData.resize(kMaxLength.Outliers);
        }
        mOutlierDistribution.mElapsed = 0;
    }
    mOutlierDistribution.mElapsed += diffMs;
    return isOutlier;
}

static int widthOf(int x) {
    int width = 0;
    if (x < 0) {
        width++;
        x = x == INT_MIN ? INT_MAX : -x;
    }
    // assert (x >= 0)
    do {
        ++width;
        x /= 10;
    } while (x > 0);
    return width;
}

// computes the column width required for a specific histogram value
inline int numberWidth(double number, int leftPadding) {
    // Added values account for whitespaces needed around numbers, and for the
    // dot and decimal digit not accounted for by widthOf
    return std::max(std::max(widthOf(static_cast<int>(number)) + 3, 2), leftPadding + 1);
}

// rounds value to precision based on log-distance from mean
__attribute__((no_sanitize("signed-integer-overflow")))
inline double logRound(double x, double mean) {
    // Larger values decrease range of high resolution and prevent overflow
    // of a histogram on the console.
    // The following formula adjusts kBase based on the buffer period length.
    // Different threads have buffer periods ranging from 2 to 40. The
    // formula below maps buffer period 2 to kBase = ~1, 4 to ~2, 20 to ~3, 40 to ~4.
    // TODO: tighten this for higher means, the data still overflows
    const double kBase = log(mean) / log(2.2);
    const double power = floor(
        log(abs(x - mean) / mean) / log(kBase)) + 2;
    // do not round values close to the mean
    if (power < 1) {
        return x;
    }
    const int factor = static_cast<int>(pow(10, power));
    return (static_cast<int>(x) * factor) / factor;
}

// TODO Make it return a std::string instead of modifying body
// TODO: move this to ReportPerformance, probably make it a friend function
// of PerformanceAnalysis
void PerformanceAnalysis::reportPerformance(String8 *body, int author, log_hash_t hash,
                                            int maxHeight) {
    if (mHists.empty()) {
        return;
    }

    // ms of active audio in displayed histogram
    double elapsedMs = 0;
    // starting timestamp of histogram
    timestamp startingTs = mHists[0].first;

    // histogram which stores .1 precision ms counts instead of Jiffy multiple counts
    std::map<double, int> buckets;
    for (const auto &shortHist: mHists) {
        for (const auto &countPair : shortHist.second) {
            const double ms = static_cast<double>(countPair.first) / kJiffyPerMs;
            buckets[logRound(ms, mBufferPeriod.mMean)] += countPair.second;
            elapsedMs += ms * countPair.second;
        }
    }

    // underscores and spaces length corresponds to maximum width of histogram
    static const int kLen = 200;
    std::string underscores(kLen, '_');
    std::string spaces(kLen, ' ');

    auto it = buckets.begin();
    double maxDelta = it->first;
    int maxCount = it->second;
    // Compute maximum values
    while (++it != buckets.end()) {
        if (it->first > maxDelta) {
            maxDelta = it->first;
        }
        if (it->second > maxCount) {
            maxCount = it->second;
        }
    }
    int height = log2(maxCount) + 1; // maxCount > 0, safe to call log2
    const int leftPadding = widthOf(1 << height);
    const int bucketWidth = numberWidth(maxDelta, leftPadding);
    int scalingFactor = 1;
    // scale data if it exceeds maximum height
    if (height > maxHeight) {
        scalingFactor = (height + maxHeight) / maxHeight;
        height /= scalingFactor;
    }
    body->appendFormat("\n%*s %3.2f %s", leftPadding + 11,
            "Occurrences in", (elapsedMs / kMsPerSec), "seconds of audio:");
    body->appendFormat("\n%*s%d, %lld, %lld\n", leftPadding + 11,
            "Thread, hash, starting timestamp: ", author,
            static_cast<long long int>(hash), static_cast<long long int>(startingTs));
    // write histogram label line with bucket values
    body->appendFormat("\n%s", " ");
    body->appendFormat("%*s", leftPadding, " ");
    for (auto const &x : buckets) {
        const int colWidth = numberWidth(x.first, leftPadding);
        body->appendFormat("%*d", colWidth, x.second);
    }
    // write histogram ascii art
    body->appendFormat("\n%s", " ");
    for (int row = height * scalingFactor; row >= 0; row -= scalingFactor) {
        const int value = 1 << row;
        body->appendFormat("%.*s", leftPadding, spaces.c_str());
        for (auto const &x : buckets) {
            const int colWidth = numberWidth(x.first, leftPadding);
            body->appendFormat("%.*s%s", colWidth - 1,
                               spaces.c_str(), x.second < value ? " " : "|");
        }
        body->appendFormat("\n%s", " ");
    }
    // print x-axis
    const int columns = static_cast<int>(buckets.size());
    body->appendFormat("%*c", leftPadding, ' ');
    body->appendFormat("%.*s", (columns + 1) * bucketWidth, underscores.c_str());
    body->appendFormat("\n%s", " ");

    // write footer with bucket labels
    body->appendFormat("%*s", leftPadding, " ");
    for (auto const &x : buckets) {
        const int colWidth = numberWidth(x.first, leftPadding);
        body->appendFormat("%*.*f", colWidth, 1, x.first);
    }
    body->appendFormat("%.*s%s", bucketWidth, spaces.c_str(), "ms\n");

    // Now report glitches
    body->appendFormat("\ntime elapsed between glitches and glitch timestamps:\n");
    for (const auto &outlier: mOutlierData) {
        body->appendFormat("%lld: %lld\n", static_cast<long long>(outlier.first),
                           static_cast<long long>(outlier.second));
    }
}

//------------------------------------------------------------------------------

// writes summary of performance into specified file descriptor
void dump(int fd, int indent, PerformanceAnalysisMap &threadPerformanceAnalysis) {
    String8 body;
    const char* const kDirectory = "/data/misc/audioserver/";
    for (auto & thread : threadPerformanceAnalysis) {
        for (auto & hash: thread.second) {
            PerformanceAnalysis& curr = hash.second;
            // write performance data to console
            curr.reportPerformance(&body, thread.first, hash.first);
            if (!body.isEmpty()) {
                dumpLine(fd, indent, body);
                body.clear();
            }
            // write to file
            writeToFile(curr.mHists, curr.mOutlierData, curr.mPeakTimestamps,
                        kDirectory, false, thread.first, hash.first);
        }
    }
}


// Writes a string into specified file descriptor
void dumpLine(int fd, int indent, const String8 &body) {
    dprintf(fd, "%.*s%s \n", indent, "", body.string());
}

} // namespace ReportPerformance

}   // namespace android
