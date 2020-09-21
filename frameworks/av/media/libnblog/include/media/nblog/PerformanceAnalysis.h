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

#ifndef ANDROID_MEDIA_PERFORMANCEANALYSIS_H
#define ANDROID_MEDIA_PERFORMANCEANALYSIS_H

#include <deque>
#include <map>
#include <vector>

#include <media/nblog/ReportPerformance.h>

namespace android {

namespace ReportPerformance {

class PerformanceAnalysis;

// a map of PerformanceAnalysis instances
// The outer key is for the thread, the inner key for the source file location.
using PerformanceAnalysisMap = std::map<int, std::map<log_hash_t, PerformanceAnalysis>>;

class PerformanceAnalysis {
    // This class stores and analyzes audio processing wakeup timestamps from NBLog
    // FIXME: currently, all performance data is stored in deques. Turn these into circular
    // buffers.
    // TODO: add a mutex.
public:

    PerformanceAnalysis() {};

    friend void dump(int fd, int indent,
                     PerformanceAnalysisMap &threadPerformanceAnalysis);

    // Called in the case of an audio on/off event, e.g., EVENT_AUDIO_STATE.
    // Used to discard idle time intervals
    void handleStateChange();

    // Writes wakeup timestamp entry to log and runs analysis
    void logTsEntry(timestamp ts);

    // FIXME: make peakdetector and storeOutlierData a single function
    // Input: mOutlierData. Looks at time elapsed between outliers
    // finds significant changes in the distribution
    // writes timestamps of significant changes to mPeakTimestamps
    bool detectAndStorePeak(msInterval delta, timestamp ts);

    // stores timestamps of intervals above a threshold: these are assumed outliers.
    // writes to mOutlierData <time elapsed since previous outlier, outlier timestamp>
    bool detectAndStoreOutlier(const msInterval diffMs);

    // Generates a string of analysis of the buffer periods and prints to console
    // FIXME: move this data visualization to a separate class. Model/view/controller
    void reportPerformance(String8 *body, int author, log_hash_t hash,
                           int maxHeight = 10);

private:

    // TODO use a circular buffer for the deques and vectors below

    // stores outlier analysis:
    // <elapsed time between outliers in ms, outlier beginning timestamp>
    std::deque<std::pair<msInterval, timestamp>> mOutlierData;

    // stores each timestamp at which a peak was detected
    // a peak is a moment at which the average outlier interval changed significantly
    std::deque<timestamp> mPeakTimestamps;

    // stores buffer period histograms with timestamp of first sample
    std::deque<std::pair<timestamp, Histogram>> mHists;

    // Parameters used when detecting outliers
    struct BufferPeriod {
        double    mMean = -1;          // average time between audio processing wakeups
        double    mOutlierFactor = -1; // values > mMean * mOutlierFactor are outliers
        double    mOutlier = -1;       // this is set to mMean * mOutlierFactor
        timestamp mPrevTs = -1;        // previous timestamp
    } mBufferPeriod;

    // capacity allocated to data structures
    struct MaxLength {
        size_t Hists; // number of histograms stored in memory
        size_t Outliers; // number of values stored in outlier array
        size_t Peaks; // number of values stored in peak array
        int HistTimespanMs; // maximum histogram timespan
    };
    // These values allow for 10 hours of data allowing for a glitch and a peak
    // as often as every 3 seconds
    static constexpr MaxLength kMaxLength = {.Hists = 60, .Outliers = 12000,
            .Peaks = 12000, .HistTimespanMs = 10 * kSecPerMin * kMsPerSec };

    // these variables ensure continuity while analyzing the timestamp
    // series one sample at a time.
    // TODO: change this to a running variance/mean class
    struct OutlierDistribution {
        msInterval mMean = 0;         // sample mean since previous peak
        msInterval mSd = 0;           // sample sd since previous peak
        msInterval mElapsed = 0;      // time since previous detected outlier
        const int  kMaxDeviation = 5; // standard deviations from the mean threshold
        msInterval mTypicalDiff = 0;  // global mean of outliers
        double     mN = 0;            // length of sequence since the last peak
        double     mM2 = 0;           // used to calculate sd
    } mOutlierDistribution;
};

void dump(int fd, int indent, PerformanceAnalysisMap &threadPerformanceAnalysis);
void dumpLine(int fd, int indent, const String8 &body);

} // namespace ReportPerformance

}   // namespace android

#endif  // ANDROID_MEDIA_PERFORMANCEANALYSIS_H
