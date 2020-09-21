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

#define LOG_TAG "ReportPerformance"

#include <fstream>
#include <iostream>
#include <queue>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <sys/prctl.h>
#include <sys/time.h>
#include <utility>
#include <media/nblog/NBLog.h>
#include <media/nblog/PerformanceAnalysis.h>
#include <media/nblog/ReportPerformance.h>
#include <utils/Log.h>
#include <utils/String8.h>

namespace android {

namespace ReportPerformance {


// TODO: use a function like this to extract logic from writeToFile
// https://stackoverflow.com/a/9279620

// Writes outlier intervals, timestamps, and histograms spanning long time intervals to file.
// TODO: write data in binary format
void writeToFile(const std::deque<std::pair<timestamp, Histogram>> &hists,
                 const std::deque<std::pair<msInterval, timestamp>> &outlierData,
                 const std::deque<timestamp> &peakTimestamps,
                 const char * directory, bool append, int author, log_hash_t hash) {

    // TODO: remove old files, implement rotating files as in AudioFlinger.cpp

    if (outlierData.empty() && hists.empty() && peakTimestamps.empty()) {
        ALOGW("No data, returning.");
        return;
    }

    std::stringstream outlierName;
    std::stringstream histogramName;
    std::stringstream peakName;

    // get current time
    char currTime[16]; //YYYYMMDDHHMMSS + '\0' + one unused
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    strftime(currTime, sizeof(currTime), "%Y%m%d%H%M%S", &tm);

    // generate file names
    std::stringstream common;
    common << author << "_" << hash << "_" << currTime << ".csv";

    histogramName << directory << "histograms_" << common.str();
    outlierName << directory << "outliers_" << common.str();
    peakName << directory << "peaks_" << common.str();

    std::ofstream hfs;
    hfs.open(histogramName.str(), append ? std::ios::app : std::ios::trunc);
    if (!hfs.is_open()) {
        ALOGW("couldn't open file %s", histogramName.str().c_str());
        return;
    }
    // each histogram is written as a line where the first value is the timestamp and
    // subsequent values are pairs of buckets and counts. Each value is separated
    // by a comma, and each histogram is separated by a newline.
    for (auto hist = hists.begin(); hist != hists.end(); ++hist) {
        hfs << hist->first << ", ";
        for (auto bucket = hist->second.begin(); bucket != hist->second.end(); ++bucket) {
            hfs << bucket->first / static_cast<double>(kJiffyPerMs)
                << ", " << bucket->second;
            if (std::next(bucket) != end(hist->second)) {
                hfs << ", ";
            }
        }
        if (std::next(hist) != end(hists)) {
            hfs << "\n";
        }
    }
    hfs.close();

    std::ofstream ofs;
    ofs.open(outlierName.str(), append ? std::ios::app : std::ios::trunc);
    if (!ofs.is_open()) {
        ALOGW("couldn't open file %s", outlierName.str().c_str());
        return;
    }
    // outliers are written as pairs separated by newlines, where each
    // pair's values are separated by a comma
    for (const auto &outlier : outlierData) {
        ofs << outlier.first << ", " << outlier.second << "\n";
    }
    ofs.close();

    std::ofstream pfs;
    pfs.open(peakName.str(), append ? std::ios::app : std::ios::trunc);
    if (!pfs.is_open()) {
        ALOGW("couldn't open file %s", peakName.str().c_str());
        return;
    }
    // peaks are simply timestamps separated by commas
    for (auto peak = peakTimestamps.begin(); peak != peakTimestamps.end(); ++peak) {
        pfs << *peak;
        if (std::next(peak) != end(peakTimestamps)) {
            pfs << ", ";
        }
    }
    pfs.close();
}

} // namespace ReportPerformance

}   // namespace android
