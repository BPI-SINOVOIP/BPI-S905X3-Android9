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

#ifndef TOOLS_TRACE_PROCESSOR_VTSTRACEPROCESSOR_H_
#define TOOLS_TRACE_PROCESSOR_VTSTRACEPROCESSOR_H_

#include <android-base/macros.h>
#include <test/vts/proto/VtsProfilingMessage.pb.h>
#include <test/vts/proto/VtsReportMessage.pb.h>
#include "VtsCoverageProcessor.h"

namespace android {
namespace vts {

class VtsTraceProcessor {
 public:
  VtsTraceProcessor(VtsCoverageProcessor* coverage_processor)
      : coverage_processor_(coverage_processor){};
  virtual ~VtsTraceProcessor(){};

  enum TraceSelectionMetric {
    MAX_COVERAGE,
    MAX_COVERAGE_SIZE_RATIO,
  };
  // Cleanups the given trace file/all trace files under the given directory to
  // be used for replaying. Current cleanup depends on the trace type:
  //   1. For sever side trace, remove client side and passthrough records.
  //   2. For client side trace, remove server side and passthrough records.
  //   3. For passthrough trace, remove server and client side records.
  void CleanupTraces(const std::string& path);
  // Parses the given trace file and outputs the latency for each API call.
  void ProcessTraceForLatencyProfiling(const std::string& trace_file);
  // Parses all trace files under the the given trace directory and remove
  // duplicate trace file.
  void DedupTraces(const std::string& trace_dir);
  // Selects a subset of trace files from a give trace set based on their
  // corresponding coverage data that maximize the total coverage.
  // coverage_file_dir: directory that stores all the coverage data files.
  // trace_file_dir: directory that stores the corresponding trace files.
  // metric: metric used to select traces, currently support two metrics:
  //   1. MAX_COVERAGE: select trace that leads to the maximum coverage lines.
  //   2. MAX_COVERAGE_SIZE_RATIO: select trace that has the maximum coverage
  //      lines/trace size.
  void SelectTraces(
      const std::string& coverage_file_dir, const std::string& trace_file_dir,
      TraceSelectionMetric metric = TraceSelectionMetric::MAX_COVERAGE);
  // Reads a binary trace file, parse each trace event and print the proto.
  void ParseTrace(const std::string& trace_file);
  // Reads a text trace file, parse each trace event and convert it into a
  // binary trace file.
  void ConvertTrace(const std::string& trace_file);
  // Parse all trace files under test_trace_dir and create a list of test
  // modules for each hal@version that access all apis covered by the whole test
  // set. (i.e. such list should be a subset of the whole test list that access
  // the corresponding hal@version)
  void GetTestListForHal(const std::string& test_trace_dir,
                         const std::string& output_file,
                         bool verbose_output = false);

 private:
  // Reads a binary trace file and parse each trace event into
  // VtsProfilingRecord.
  bool ParseBinaryTrace(const std::string& trace_file, bool ignore_timestamp,
                        bool entry_only, bool summary_only,
                        VtsProfilingMessage* profiling_msg);

  // Reads a text trace file and parse each trace event into
  // VtsProfilingRecord.
  bool ParseTextTrace(const std::string& trace_file,
                      VtsProfilingMessage* profiling_msg);

  // Writes the given VtsProfilingMessage into an output file.
  bool WriteProfilingMsg(const std::string& output_file,
                         const VtsProfilingMessage& profiling_msg);

  // Internal method to cleanup a trace file.
  void CleanupTraceFile(const std::string& trace_file);
  // Reads a test report file that contains the coverage data and parse it into
  // TestReportMessage.
  bool ParseCoverageData(const std::string& coverage_file,
                         TestReportMessage* report_msg);
  // Updates msg_to_be_updated by removing all the covered lines in ref_msg
  // and recalculates the count of covered lines accordingly.
  void UpdateCoverageData(const CoverageReportMessage& ref_msg,
                          CoverageReportMessage* msg_to_be_updated);
  // Helper method to calculate total coverage line in the given report message.
  long GetTotalCoverageLine(const TestReportMessage& msg);
  // Helper method to calculate total code line in the given report message.
  long GetTotalLine(const TestReportMessage& msg);
  // Helper method to extract the trace file name from the given file name.
  std::string GetTraceFileName(const std::string& coverage_file_name);
  // Helper method to check whether the given event is an entry event.
  bool isEntryEvent(const InstrumentationEventType& event);
  // Helper method to check whether the given two records are paired records.
  // Paired records means the two records are for the same hal interface, and
  // have corresponding entry/exit events.
  bool isPairedRecord(const VtsProfilingRecord& entry_record,
                      const VtsProfilingRecord& exit_record);

  // Struct to store the coverage data.
  struct CoverageInfo {
    TestReportMessage coverage_msg;
    std::string trace_file_name;
    long trace_file_size;
  };

  // Struct to store the trace summary data.
  struct TraceSummary {
    // Name of test module that generates the trace. e.g. CtsUsbTests.
    std::string test_name;
    // Hal package name. e.g. android.hardware.light
    std::string package;
    // Hal version e.g. 1.0
    float version;
    // Total number of API calls recorded in the trace.
    long total_api_count;
    // Total number of different APIs recorded in the trace.
    long unique_api_count;
    // Call statistics for each API: <API_name, number_called>
    std::map<std::string, long> api_stats;

    TraceSummary(std::string test_name, std::string package, float version,
                 long total_api_count, long unique_api_count,
                 std::map<std::string, long> api_stats)
        : test_name(test_name),
          package(package),
          version(version),
          total_api_count(total_api_count),
          unique_api_count(unique_api_count),
          api_stats(api_stats){};
  };

  // Internal method to parse all trace files under test_trace_dir and create
  // the mapping from each hal@version to the list of test that access it.
  void GetHalTraceMapping(
      const std::string& test_trace_dir,
      std::map<std::string, std::vector<TraceSummary>>* hal_trace_mapping);

  // Internal method to parse a trace file and create the corresponding
  // TraceSummary from it.
  void GetHalTraceSummary(const std::string& trace_file,
                          const std::string& test_name,
                          std::vector<TraceSummary>* trace_summaries);

  // A class to process coverage reports. Not owned.
  VtsCoverageProcessor* coverage_processor_;

  DISALLOW_COPY_AND_ASSIGN(VtsTraceProcessor);
};

}  // namespace vts
}  // namespace android
#endif  // TOOLS_TRACE_PROCESSOR_VTSTRACEPROCESSOR_H_
