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
#include "VtsCoverageProcessor.h"

#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <google/protobuf/text_format.h>
#include <test/vts/proto/VtsReportMessage.pb.h>

using namespace std;
using google::protobuf::TextFormat;

namespace android {
namespace vts {

void VtsCoverageProcessor::ParseCoverageData(const string& coverage_file,
                                             TestReportMessage* report_msg) {
  ifstream in(coverage_file, std::ios::in);
  string msg_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  if (!TextFormat::MergeFromString(msg_str, report_msg)) {
    cerr << "Can't parse a given coverage report: " << msg_str << endl;
    exit(-1);
  }
}

void VtsCoverageProcessor::UpdateCoverageData(
    const CoverageReportMessage& ref_msg,
    CoverageReportMessage* msg_to_be_updated) {
  if (ref_msg.file_path() == msg_to_be_updated->file_path()) {
    for (int line = 0; line < ref_msg.line_coverage_vector_size(); line++) {
      if (line < msg_to_be_updated->line_coverage_vector_size()) {
        if (ref_msg.line_coverage_vector(line) > 0 &&
            msg_to_be_updated->line_coverage_vector(line) > 0) {
          msg_to_be_updated->set_line_coverage_vector(line, 0);
          msg_to_be_updated->set_covered_line_count(
              msg_to_be_updated->covered_line_count() - 1);
        }
      } else {
        cout << "Reached the end of line_coverage_vector." << endl;
        break;
      }
    }
    // sanity check.
    if (msg_to_be_updated->covered_line_count() < 0) {
      cerr << __func__ << ": covered_line_count should not be negative."
           << endl;
      exit(-1);
    }
  }
}

void VtsCoverageProcessor::MergeCoverage(const string& coverage_file_dir,
                                         const string& merged_coverage_file) {
  DIR* coverage_dir = opendir(coverage_file_dir.c_str());
  if (coverage_dir == 0) {
    cerr << __func__ << ": " << coverage_file_dir << " does not exist." << endl;
    return;
  }
  TestReportMessage merged_coverage_report;

  struct dirent* file;
  while ((file = readdir(coverage_dir)) != NULL) {
    if (file->d_type == DT_REG) {
      string coverage_file = coverage_file_dir;
      if (coverage_file_dir.substr(coverage_file_dir.size() - 1) != "/") {
        coverage_file += "/";
      }
      string coverage_file_base_name = file->d_name;
      coverage_file += coverage_file_base_name;
      TestReportMessage coverage_report;
      ParseCoverageData(coverage_file, &coverage_report);

      for (const auto cov : coverage_report.coverage()) {
        bool seen_cov = false;
        for (int i = 0; i < merged_coverage_report.coverage_size(); i++) {
          if (merged_coverage_report.coverage(i).file_path() ==
              cov.file_path()) {
            MergeCoverageMsg(cov, merged_coverage_report.mutable_coverage(i));
            seen_cov = true;
            break;
          }
        }
        if (!seen_cov) {
          *merged_coverage_report.add_coverage() = cov;
        }
      }
    }
  }

  PrintCoverageSummary(merged_coverage_report);
  ofstream fout;
  fout.open(merged_coverage_file);
  fout << merged_coverage_report.DebugString();
  fout.close();
}

void VtsCoverageProcessor::MergeCoverageMsg(
    const CoverageReportMessage& ref_coverage_msg,
    CoverageReportMessage* merged_coverage_msg) {
  // sanity check.
  if (ref_coverage_msg.file_path() != merged_coverage_msg->file_path()) {
    cerr << "Trying to merge coverage data for different files." << endl;
    exit(-1);
  }
  if (ref_coverage_msg.line_coverage_vector_size() !=
      merged_coverage_msg->line_coverage_vector_size()) {
    cerr << "Trying to merge coverage data with different lines."
         << "ref_coverage_msg: " << ref_coverage_msg.DebugString()
         << "merged_coverage_msg: " << merged_coverage_msg->DebugString()
         << endl;
    exit(-1);
  }
  for (int i = 0; i < ref_coverage_msg.line_coverage_vector_size(); i++) {
    if (i > merged_coverage_msg->line_coverage_vector_size() - 1) {
      cerr << "Reach the end of coverage vector" << endl;
      break;
    }
    int ref_line_count = ref_coverage_msg.line_coverage_vector(i);
    int merged_line_count = merged_coverage_msg->line_coverage_vector(i);
    if (ref_line_count > 0) {
      if (merged_line_count == 0) {
        merged_coverage_msg->set_covered_line_count(
            merged_coverage_msg->covered_line_count() + 1);
      }
      merged_coverage_msg->set_line_coverage_vector(
          i, merged_line_count + ref_line_count);
    }
  }
}

void VtsCoverageProcessor::CompareCoverage(const string& ref_msg_file,
                                           const string& new_msg_file) {
  TestReportMessage ref_coverage_report;
  TestReportMessage new_coverage_report;
  ParseCoverageData(ref_msg_file, &ref_coverage_report);
  ParseCoverageData(new_msg_file, &new_coverage_report);
  map<string, vector<int>> new_coverage_map;

  for (const auto& new_coverage : new_coverage_report.coverage()) {
    bool seen_file = false;
    for (const auto& ref_coverage : ref_coverage_report.coverage()) {
      if (new_coverage.file_path() == ref_coverage.file_path()) {
        int line = 0;
        for (; line < new_coverage.line_coverage_vector_size(); line++) {
          if (new_coverage.line_coverage_vector(line) > 0 &&
              ref_coverage.line_coverage_vector(line) == 0) {
            if (new_coverage_map.find(new_coverage.file_path()) !=
                new_coverage_map.end()) {
              new_coverage_map[new_coverage.file_path()].push_back(line);
            } else {
              new_coverage_map.insert(std::pair<string, vector<int>>(
                  new_coverage.file_path(), vector<int>{line}));
            }
          }
        }
        seen_file = true;
        break;
      }
    }
    if (!seen_file) {
      vector<int> new_line;
      for (int line = 0; line < new_coverage.line_coverage_vector_size();
           line++) {
        if (new_coverage.line_coverage_vector(line) > 0) {
          new_line.push_back(line);
        }
      }
      new_coverage_map.insert(
          std::pair<string, vector<int>>(new_coverage.file_path(), new_line));
    }
  }
  for (auto it = new_coverage_map.begin(); it != new_coverage_map.end(); it++) {
    cout << it->first << ": " << endl;
    for (int covered_line : it->second) {
      cout << covered_line << endl;
    }
  }
}

void VtsCoverageProcessor::GetSubsetCoverage(const string& ref_msg_file,
                                             const string& full_msg_file,
                                             const string& result_msg_file) {
  TestReportMessage ref_coverage_report;
  TestReportMessage full_coverage_report;
  TestReportMessage result_coverage_report;
  ParseCoverageData(ref_msg_file, &ref_coverage_report);
  ParseCoverageData(full_msg_file, &full_coverage_report);

  for (const auto& ref_coverage : ref_coverage_report.coverage()) {
    bool seen_file = false;
    for (const auto& coverage : full_coverage_report.coverage()) {
      if (coverage.file_path() == ref_coverage.file_path()) {
        *result_coverage_report.add_coverage() = coverage;
        seen_file = true;
        break;
      }
    }
    if (!seen_file) {
      cout << ": missing coverage for file " << ref_coverage.file_path()
           << endl;
      CoverageReportMessage* empty_coverage =
          result_coverage_report.add_coverage();
      *empty_coverage = ref_coverage;
      for (int line = 0; line < empty_coverage->line_coverage_vector_size();
           line++) {
        if (empty_coverage->line_coverage_vector(line) > 0) {
          empty_coverage->set_line_coverage_vector(line, 0);
        }
      }
      empty_coverage->set_covered_line_count(0);
    }
  }
  PrintCoverageSummary(result_coverage_report);
  ofstream fout;
  fout.open(result_msg_file);
  fout << result_coverage_report.DebugString();
  fout.close();
}

void VtsCoverageProcessor::GetCoverageSummary(const string& coverage_msg_file) {
  TestReportMessage coverage_report;
  ParseCoverageData(coverage_msg_file, &coverage_report);
  PrintCoverageSummary(coverage_report);
}

void VtsCoverageProcessor::PrintCoverageSummary(
    const TestReportMessage& coverage_report) {
  long total_lines_covered = GetTotalCoverageLine(coverage_report);
  long total_code_lines = GetTotalCodeLine(coverage_report);
  double coverage_rate = (double)total_lines_covered / total_code_lines;
  cout << "total lines covered: " << total_lines_covered << endl;
  cout << "total lines: " << total_code_lines << endl;
  cout << "coverage rate: " << coverage_rate << endl;
}

long VtsCoverageProcessor::GetTotalCoverageLine(
    const TestReportMessage& msg) const {
  long total_coverage_line = 0;
  for (const auto coverage : msg.coverage()) {
    total_coverage_line += coverage.covered_line_count();
  }
  return total_coverage_line;
}

long VtsCoverageProcessor::GetTotalCodeLine(
    const TestReportMessage& msg) const {
  long total_line = 0;
  for (const auto coverage : msg.coverage()) {
    total_line += coverage.total_line_count();
  }
  return total_line;
}

}  // namespace vts
}  // namespace android
