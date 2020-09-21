/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef TOOLS_TRACE_PROCESSOR_VTSCOVERAGEPROCESSOR_H_
#define TOOLS_TRACE_PROCESSOR_VTSCOVERAGEPROCESSOR_H_

#include <android-base/macros.h>
#include <test/vts/proto/VtsReportMessage.pb.h>

namespace android {
namespace vts {
// A class used for processing coverage report data, such as parse the
// coverage report file, merge multiple coverage reports, and compare
// two coverage reports.
class VtsCoverageProcessor {
 public:
  VtsCoverageProcessor(){};
  virtual ~VtsCoverageProcessor(){};

  // Merge the coverage files under coverage_file_dir and output the merged
  // coverage data to merged_coverage_file.
  void MergeCoverage(const std::string& coverage_file_dir,
                     const std::string& merged_coverage_file);

  // Compare coverage data contained in new_msg_file with ref_msg_file and
  // print the additional file/lines covered by the new_msg_file.
  void CompareCoverage(const std::string& ref_msg_file,
                       const std::string& new_msg_file);

  // Parse the given coverage_file into a coverage report.
  void ParseCoverageData(const std::string& coverage_file,
                         TestReportMessage* coverage_report);

  // Updates msg_to_be_updated by removing all the covered lines in ref_msg
  // and recalculates the count of covered lines accordingly.
  void UpdateCoverageData(const CoverageReportMessage& ref_msg,
                          CoverageReportMessage* msg_to_be_updated);

  // Extract the files covered in ref_msg_file from full_msg_file and store
  // the result in result_msg_file.
  void GetSubsetCoverage(const std::string& ref_msg_file,
                         const std::string& full_msg_file,
                         const std::string& result_msg_file);

  // Parse the coverage report and print the coverage summary.
  void GetCoverageSummary(const std::string& coverage_msg_file);

  // Calculate total coverage line in the given report message.
  long GetTotalCoverageLine(const TestReportMessage& msg) const;
  // Calculate total code line in the given report message.
  long GetTotalCodeLine(const TestReportMessage& msg) const;

 private:
  // Internal method to merge the ref_coverage_msg into merged_covergae_msg.
  void MergeCoverageMsg(const CoverageReportMessage& ref_coverage_msg,
                        CoverageReportMessage* merged_covergae_msg);

  // Help method to print the coverage summary.
  void PrintCoverageSummary(const TestReportMessage& coverage_report);

  DISALLOW_COPY_AND_ASSIGN(VtsCoverageProcessor);
};

}  // namespace vts
}  // namespace android
#endif  // TOOLS_TRACE_PROCESSOR_VTSCOVERAGEPROCESSOR_H_
