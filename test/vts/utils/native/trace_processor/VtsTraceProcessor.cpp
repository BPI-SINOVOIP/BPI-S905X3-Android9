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
#include "VtsTraceProcessor.h"

#include <dirent.h>
#include <fcntl.h>
#include <json/json.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <sys/stat.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include <test/vts/proto/VtsReportMessage.pb.h>

#include "VtsProfilingUtil.h"

using namespace std;
using google::protobuf::TextFormat;

namespace android {
namespace vts {

bool VtsTraceProcessor::ParseBinaryTrace(const string& trace_file,
                                         bool ignore_timestamp, bool entry_only,
                                         bool ignore_func_params,
                                         VtsProfilingMessage* profiling_msg) {
  int fd =
      open(trace_file.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    cerr << "Can not open trace file: " << trace_file
         << "error: " << std::strerror(errno);
    return false;
  }
  google::protobuf::io::FileInputStream input(fd);
  VtsProfilingRecord record;
  while (readOneDelimited(&record, &input)) {
    if (ignore_timestamp) {
      record.clear_timestamp();
    }
    if (ignore_func_params) {
      record.mutable_func_msg()->clear_arg();
      record.mutable_func_msg()->clear_return_type_hidl();
    }
    if (entry_only) {
      if (isEntryEvent(record.event())) {
        *profiling_msg->add_records() = record;
      }
    } else {
      *profiling_msg->add_records() = record;
    }
    record.Clear();
  }
  input.Close();
  return true;
}

bool VtsTraceProcessor::ParseTextTrace(const string& trace_file,
                                       VtsProfilingMessage* profiling_msg) {
  ifstream in(trace_file, std::ios::in);
  bool new_record = true;
  string record_str, line;

  while (getline(in, line)) {
    // Assume records are separated by '\n'.
    if (line.empty()) {
      new_record = false;
    }
    if (new_record) {
      record_str += line + "\n";
    } else {
      VtsProfilingRecord record;
      if (!TextFormat::MergeFromString(record_str, &record)) {
        cerr << "Can't parse a given record: " << record_str << endl;
        return false;
      }
      *profiling_msg->add_records() = record;
      new_record = true;
      record_str.clear();
    }
  }
  in.close();
  return true;
}

void VtsTraceProcessor::ParseTrace(const string& trace_file) {
  VtsProfilingMessage profiling_msg;
  if (!ParseBinaryTrace(trace_file, false, false, false, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  for (auto record : profiling_msg.records()) {
    cout << record.DebugString() << endl;
  }
}

bool VtsTraceProcessor::WriteProfilingMsg(
    const string& output_file, const VtsProfilingMessage& profiling_msg) {
  int fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_EXCL,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    cerr << "Can not open trace file: " << output_file
         << "error: " << std::strerror(errno);
    return false;
  }
  google::protobuf::io::FileOutputStream output(fd);
  for (const auto& record : profiling_msg.records()) {
    if (!writeOneDelimited(record, &output)) {
      cerr << "Failed to write record";
    }
  }
  output.Close();
  return true;
}

void VtsTraceProcessor::ConvertTrace(const string& trace_file) {
  VtsProfilingMessage profiling_msg;
  if (!ParseTextTrace(trace_file, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  string tmp_file = trace_file + "_binary";
  if (!WriteProfilingMsg(tmp_file, profiling_msg)) {
    cerr << __func__ << ": Failed to write new trace file: " << tmp_file
         << endl;
    return;
  }
}

void VtsTraceProcessor::CleanupTraceFile(const string& trace_file) {
  VtsProfilingMessage profiling_msg;
  if (!ParseBinaryTrace(trace_file, false, false, true, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  VtsProfilingMessage clean_profiling_msg;
  bool first_record = true;
  enum TRACE_TYPE { server_trace, client_trace, passthrough_trace };
  string package;
  float version;
  TRACE_TYPE trace_type;
  for (const auto& record : profiling_msg.records()) {
    if (first_record) {
      package = record.package();
      version = record.version();
      // determine trace type based on the event of the first record.
      switch (record.event()) {
        case InstrumentationEventType::SERVER_API_ENTRY:
          trace_type = TRACE_TYPE::server_trace;
          break;
        case InstrumentationEventType::CLIENT_API_ENTRY:
          trace_type = TRACE_TYPE::client_trace;
          break;
        case InstrumentationEventType::PASSTHROUGH_ENTRY:
          trace_type = TRACE_TYPE::passthrough_trace;
          break;
        default:
          cerr << "Unexpected record: " << record.DebugString() << endl;
          return;
      }
      first_record = false;
    }
    // If trace contains records for a different hal, remove it.
    if (record.package() != package || record.version() != version) {
      cerr << "Unexpected record: " << record.DebugString() << endl;
      continue;
    }
    switch (trace_type) {
      case TRACE_TYPE::server_trace: {
        if (record.event() == InstrumentationEventType::SERVER_API_ENTRY ||
            record.event() == InstrumentationEventType::SERVER_API_EXIT) {
          *clean_profiling_msg.add_records() = record;
        }
        break;
      }
      case TRACE_TYPE::client_trace: {
        if (record.event() == InstrumentationEventType::CLIENT_API_ENTRY ||
            record.event() == InstrumentationEventType::CLIENT_API_EXIT) {
          *clean_profiling_msg.add_records() = record;
        }
        break;
      }
      case TRACE_TYPE::passthrough_trace: {
        if (record.event() == InstrumentationEventType::PASSTHROUGH_ENTRY ||
            record.event() == InstrumentationEventType::PASSTHROUGH_EXIT) {
          *clean_profiling_msg.add_records() = record;
        }
        break;
      }
      default:
        cerr << "Unknow trace type: " << trace_type << endl;
        return;
    }
  }
  string tmp_file = trace_file + "_tmp";
  if (!WriteProfilingMsg(tmp_file, clean_profiling_msg)) {
    cerr << __func__ << ": Failed to write new trace file: " << tmp_file
         << endl;
    return;
  }
  if (rename(tmp_file.c_str(), trace_file.c_str())) {
    cerr << __func__ << ": Failed to replace old trace file: " << trace_file
         << endl;
    return;
  }
}

void VtsTraceProcessor::CleanupTraces(const string& path) {
  struct stat path_stat;
  stat(path.c_str(), &path_stat);
  if (S_ISREG(path_stat.st_mode)) {
    CleanupTraceFile(path);
  } else if (S_ISDIR(path_stat.st_mode)) {
    DIR* dir = opendir(path.c_str());
    struct dirent* file;
    while ((file = readdir(dir)) != NULL) {
      if (file->d_type == DT_REG) {
        string trace_file = path;
        if (path.substr(path.size() - 1) != "/") {
          trace_file += "/";
        }
        trace_file += file->d_name;
        CleanupTraceFile(trace_file);
      }
    }
  }
}

void VtsTraceProcessor::ProcessTraceForLatencyProfiling(
    const string& trace_file) {
  VtsProfilingMessage profiling_msg;
  if (!ParseBinaryTrace(trace_file, false, false, true, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  if (!profiling_msg.records_size()) return;
  if (profiling_msg.records(0).event() ==
          InstrumentationEventType::PASSTHROUGH_ENTRY ||
      profiling_msg.records(0).event() ==
          InstrumentationEventType::PASSTHROUGH_EXIT) {
    cout << "hidl_hal_mode:passthrough" << endl;
  } else {
    cout << "hidl_hal_mode:binder" << endl;
  }

  // stack to store all seen records.
  vector<VtsProfilingRecord> seen_records;
  // stack to store temp records that not processed.
  vector<VtsProfilingRecord> pending_records;
  for (auto record : profiling_msg.records()) {
    if (isEntryEvent(record.event())) {
      seen_records.emplace_back(record);
    } else {
      while (!seen_records.empty() &&
             !isPairedRecord(seen_records.back(), record)) {
        pending_records.emplace_back(seen_records.back());
        seen_records.pop_back();
      }
      if (seen_records.empty()) {
        cerr << "Could not found entry record for record: "
             << record.DebugString() << endl;
        continue;
      } else {
        // Found the paired entry record, calculate the latency.
        VtsProfilingRecord entry_record = seen_records.back();
        seen_records.pop_back();
        string api = record.func_msg().name();
        int64_t start_timestamp = entry_record.timestamp();
        int64_t end_timestamp = record.timestamp();
        int64_t latency = end_timestamp - start_timestamp;
        // sanity check.
        if (latency < 0) {
          cerr << __func__ << ": got negative latency for " << api << endl;
          exit(-1);
        }
        cout << api << ":" << latency << endl;
        while (!pending_records.empty()) {
          seen_records.emplace_back(pending_records.back());
          pending_records.pop_back();
        }
      }
    }
  }
}

void VtsTraceProcessor::DedupTraces(const string& trace_dir) {
  DIR* dir = opendir(trace_dir.c_str());
  if (dir == 0) {
    cerr << trace_dir << "does not exist." << endl;
    return;
  }
  vector<VtsProfilingMessage> seen_msgs;
  vector<string> duplicate_trace_files;
  struct dirent* file;
  long total_trace_num = 0;
  long duplicat_trace_num = 0;
  while ((file = readdir(dir)) != NULL) {
    if (file->d_type == DT_REG) {
      total_trace_num++;
      string trace_file = trace_dir;
      if (trace_dir.substr(trace_dir.size() - 1) != "/") {
        trace_file += "/";
      }
      trace_file += file->d_name;
      VtsProfilingMessage profiling_msg;
      if (!ParseBinaryTrace(trace_file, true, true, false, &profiling_msg)) {
        cerr << "Failed to parse trace file: " << trace_file << endl;
        return;
      }
      if (!profiling_msg.records_size()) {  // empty trace file
        duplicate_trace_files.push_back(trace_file);
        duplicat_trace_num++;
        continue;
      }
      auto found =
          find_if(seen_msgs.begin(), seen_msgs.end(),
                  [&profiling_msg](const VtsProfilingMessage& seen_msg) {
                    std::string str_profiling_msg;
                    std::string str_seen_msg;
                    profiling_msg.SerializeToString(&str_profiling_msg);
                    seen_msg.SerializeToString(&str_seen_msg);
                    return (str_profiling_msg == str_seen_msg);
                  });
      if (found == seen_msgs.end()) {
        seen_msgs.push_back(profiling_msg);
      } else {
        duplicate_trace_files.push_back(trace_file);
        duplicat_trace_num++;
      }
    }
  }
  for (const string& duplicate_trace : duplicate_trace_files) {
    cout << "deleting duplicate trace file: " << duplicate_trace << endl;
    remove(duplicate_trace.c_str());
  }
  cout << "Num of traces processed: " << total_trace_num << endl;
  cout << "Num of duplicate trace deleted: " << duplicat_trace_num << endl;
  cout << "Duplicate percentage: "
       << float(duplicat_trace_num) / total_trace_num << endl;
}

void VtsTraceProcessor::SelectTraces(const string& coverage_file_dir,
                                     const string& trace_file_dir,
                                     TraceSelectionMetric metric) {
  DIR* coverage_dir = opendir(coverage_file_dir.c_str());
  if (coverage_dir == 0) {
    cerr << __func__ << ": " << coverage_file_dir << " does not exist." << endl;
    return;
  }
  DIR* trace_dir = opendir(trace_file_dir.c_str());
  if (trace_dir == 0) {
    cerr << __func__ << ": " << trace_file_dir << " does not exist." << endl;
    return;
  }
  map<string, CoverageInfo> original_coverages;
  map<string, CoverageInfo> selected_coverages;

  // Parse all the coverage files and store them into original_coverage_msgs.
  struct dirent* file;
  while ((file = readdir(coverage_dir)) != NULL) {
    if (file->d_type == DT_REG) {
      string coverage_file = coverage_file_dir;
      if (coverage_file_dir.substr(coverage_file_dir.size() - 1) != "/") {
        coverage_file += "/";
      }
      string coverage_file_base_name = file->d_name;
      coverage_file += coverage_file_base_name;
      TestReportMessage coverage_msg;
      coverage_processor_->ParseCoverageData(coverage_file, &coverage_msg);

      string trace_file = trace_file_dir;
      if (trace_file_dir.substr(trace_file_dir.size() - 1) != "/") {
        trace_file += "/";
      }
      string trace_file_base_name = GetTraceFileName(coverage_file_base_name);
      trace_file += trace_file_base_name;
      ifstream in(trace_file, ifstream::binary | ifstream::ate);
      if (!in.good()) {
        cerr << "trace file: " << trace_file << " does not exists." << endl;
        continue;
      }
      long trace_file_size = in.tellg();

      CoverageInfo coverage_info;
      coverage_info.coverage_msg = coverage_msg;
      coverage_info.trace_file_name = trace_file;
      coverage_info.trace_file_size = trace_file_size;

      original_coverages[coverage_file] = coverage_info;
    }
  }
  // Greedy algorithm that selects coverage files with the maximal code
  // coverage delta at each iteration. Note: Not guaranteed to generate the
  // optimal set. Example (*: covered, -: not_covered) line#\coverage_file
  // cov1 cov2 cov3
  //          1              *   -    -
  //          2              *   *    -
  //          3              -   *    *
  //          4              -   *    *
  //          5              -   -    *
  // This algorithm will select cov2, cov1, cov3 while optimal solution is:
  // cov1, cov3.
  // double max_coverage_size_ratio = 0.0;
  TestReportMessage selected_coverage_msg;
  while (true) {
    double max_selection_metric = 0.0;
    string selected_coverage_file = "";
    // Update the remaining coverage file in original_coverage_msgs.
    for (auto it = original_coverages.begin(); it != original_coverages.end();
         ++it) {
      TestReportMessage cur_coverage_msg = it->second.coverage_msg;
      for (const auto ref_coverage : selected_coverage_msg.coverage()) {
        for (int i = 0; i < cur_coverage_msg.coverage_size(); i++) {
          CoverageReportMessage* coverage_to_be_updated =
              cur_coverage_msg.mutable_coverage(i);
          coverage_processor_->UpdateCoverageData(ref_coverage,
                                                  coverage_to_be_updated);
        }
      }
      it->second.coverage_msg = cur_coverage_msg;
      long total_coverage_line =
          coverage_processor_->GetTotalCoverageLine(cur_coverage_msg);
      long trace_file_size = it->second.trace_file_size;
      double coverage_size_ratio =
          (double)total_coverage_line / trace_file_size;
      if (metric == TraceSelectionMetric::MAX_COVERAGE) {
        if (coverage_size_ratio > max_selection_metric) {
          max_selection_metric = coverage_size_ratio;
          selected_coverage_file = it->first;
        }
      } else if (metric == TraceSelectionMetric::MAX_COVERAGE_SIZE_RATIO) {
        if (total_coverage_line > max_selection_metric) {
          max_selection_metric = total_coverage_line;
          selected_coverage_file = it->first;
        }
      }
    }
    if (!max_selection_metric) {
      break;
    } else {
      CoverageInfo selected_coverage =
          original_coverages[selected_coverage_file];
      selected_coverages[selected_coverage_file] = selected_coverage;
      // Remove the coverage file from original_coverage_msgs.
      original_coverages.erase(selected_coverage_file);
      selected_coverage_msg = selected_coverage.coverage_msg;
    }
  }
  // Calculate the total code lines and total line covered.
  long total_lines = 0;
  long total_lines_covered = 0;
  for (auto it = selected_coverages.begin(); it != selected_coverages.end();
       ++it) {
    cout << "select trace file: " << it->second.trace_file_name << endl;
    TestReportMessage coverage_msg = it->second.coverage_msg;
    total_lines_covered +=
        coverage_processor_->GetTotalCoverageLine(coverage_msg);
    if (coverage_processor_->GetTotalCodeLine(coverage_msg) > total_lines) {
      total_lines = coverage_processor_->GetTotalCodeLine(coverage_msg);
    }
  }
  double coverage_rate = (double)total_lines_covered / total_lines;
  cout << "total lines covered: " << total_lines_covered << endl;
  cout << "total lines: " << total_lines << endl;
  cout << "coverage rate: " << coverage_rate << endl;
}

string VtsTraceProcessor::GetTraceFileName(const string& coverage_file_name) {
  std::size_t start = coverage_file_name.find("android.hardware");
  std::size_t end = coverage_file_name.find("vts.trace") + sizeof("vts.trace");
  return coverage_file_name.substr(start, end - start - 1);
}

bool VtsTraceProcessor::isEntryEvent(const InstrumentationEventType& event) {
  if (event == InstrumentationEventType::SERVER_API_ENTRY ||
      event == InstrumentationEventType::CLIENT_API_ENTRY ||
      event == InstrumentationEventType::PASSTHROUGH_ENTRY) {
    return true;
  }
  return false;
}

bool VtsTraceProcessor::isPairedRecord(const VtsProfilingRecord& entry_record,
                                       const VtsProfilingRecord& exit_record) {
  if (entry_record.package() != exit_record.package() ||
      entry_record.version() != exit_record.version() ||
      entry_record.interface() != exit_record.interface() ||
      entry_record.func_msg().name() != exit_record.func_msg().name()) {
    return false;
  }
  switch (entry_record.event()) {
    case InstrumentationEventType::SERVER_API_ENTRY: {
      if (exit_record.event() == InstrumentationEventType::SERVER_API_EXIT) {
        return true;
      }
      break;
    }
    case InstrumentationEventType::CLIENT_API_ENTRY: {
      if (exit_record.event() == InstrumentationEventType::CLIENT_API_EXIT)
        return true;
      break;
    }
    case InstrumentationEventType::PASSTHROUGH_ENTRY: {
      if (exit_record.event() == InstrumentationEventType::PASSTHROUGH_EXIT)
        return true;
      break;
    }
    default:
      cout << "Unsupported event: " << entry_record.event() << endl;
      return false;
  }
  return false;
}

void VtsTraceProcessor::GetTestListForHal(const string& test_trace_dir,
                                          const string& output_file,
                                          bool verbose_output) {
  // Mapping from hal name to the list of test that access that hal.
  map<string, vector<TraceSummary>> hal_trace_mapping;
  GetHalTraceMapping(test_trace_dir, &hal_trace_mapping);

  map<string, set<string>> test_list;
  for (auto it = hal_trace_mapping.begin(); it != hal_trace_mapping.end();
       it++) {
    test_list[it->first] = set<string>();
    vector<TraceSummary> trace_summaries = it->second;
    vector<string> covered_apis;
    for (auto summary : trace_summaries) {
      for (auto const& api_stat_it : summary.api_stats) {
        if (std::find(covered_apis.begin(), covered_apis.end(),
                      api_stat_it.first) == covered_apis.end()) {
          covered_apis.push_back(api_stat_it.first);
          test_list[it->first].insert(summary.test_name);
        }
      }
    }
    for (auto api : covered_apis) {
      cout << "covered api: " << api << endl;
    }
  }

  ofstream fout;
  fout.open(output_file);
  for (auto it = hal_trace_mapping.begin(); it != hal_trace_mapping.end();
       it++) {
    if (verbose_output) {
      Json::Value root(Json::objectValue);
      root["Hal_name"] = Json::Value(it->first);
      Json::Value arr(Json::arrayValue);
      for (const TraceSummary& summary : it->second) {
        Json::Value obj;
        obj["Test_name"] = summary.test_name;
        obj["Unique_Api_Count"] = std::to_string(summary.unique_api_count);
        obj["Total_Api_Count"] = std::to_string(summary.total_api_count);
        arr.append(obj);
      }
      root["Test_list"] = arr;
      fout << root.toStyledString();
    } else {
      fout << it->first << ",";
      for (auto test : test_list[it->first]) {
        auto found = find_if(it->second.begin(), it->second.end(),
                             [&](const TraceSummary& trace_summary) {
                               return (trace_summary.test_name == test);
                             });
        if (found != it->second.end()) {
          fout << found->test_name << "(" << found->unique_api_count << "/"
               << found->total_api_count << "),";
        }
      }
      fout << endl;
    }
  }
  fout.close();
}

void VtsTraceProcessor::GetHalTraceMapping(
    const string& test_trace_dir,
    map<string, vector<TraceSummary>>* hal_trace_mapping) {
  DIR* trace_dir = opendir(test_trace_dir.c_str());
  if (trace_dir == 0) {
    cerr << __func__ << ": " << trace_dir << " does not exist." << endl;
    return;
  }
  vector<TraceSummary> trace_summaries;
  struct dirent* test_dir;
  while ((test_dir = readdir(trace_dir)) != NULL) {
    if (test_dir->d_type == DT_DIR) {
      string test_name = test_dir->d_name;
      cout << "Processing test: " << test_name << endl;
      string trace_file_dir_name = test_trace_dir;
      if (test_trace_dir.substr(test_trace_dir.size() - 1) != "/") {
        trace_file_dir_name += "/";
      }
      trace_file_dir_name += test_name;
      DIR* trace_file_dir = opendir(trace_file_dir_name.c_str());
      struct dirent* trace_file;
      while ((trace_file = readdir(trace_file_dir)) != NULL) {
        if (trace_file->d_type == DT_REG) {
          string trace_file_name =
              trace_file_dir_name + "/" + trace_file->d_name;
          GetHalTraceSummary(trace_file_name, test_name, &trace_summaries);
        }
      }
    }
  }

  // Generate hal_trace_mapping mappings.
  for (const TraceSummary& trace_summary : trace_summaries) {
    string test_name = trace_summary.test_name;
    stringstream stream;
    stream << fixed << setprecision(1) << trace_summary.version;
    string hal_name = trace_summary.package + "@" + stream.str();
    if (hal_trace_mapping->find(hal_name) != hal_trace_mapping->end()) {
      (*hal_trace_mapping)[hal_name].push_back(trace_summary);
    } else {
      (*hal_trace_mapping)[hal_name] = vector<TraceSummary>{trace_summary};
    }
  }
  for (auto it = hal_trace_mapping->begin(); it != hal_trace_mapping->end();
       it++) {
    // Sort the tests according to unique_api_count and break tie with
    // total_api_count.
    std::sort(it->second.begin(), it->second.end(),
              [](const TraceSummary& lhs, const TraceSummary& rhs) {
                return (lhs.unique_api_count > rhs.unique_api_count) ||
                       (lhs.unique_api_count == rhs.unique_api_count &&
                        lhs.total_api_count > rhs.total_api_count);
              });
  }
}

void VtsTraceProcessor::GetHalTraceSummary(
    const string& trace_file, const string& test_name,
    vector<TraceSummary>* trace_summaries) {
  VtsProfilingMessage profiling_msg;
  if (!ParseBinaryTrace(trace_file, true, true, true, &profiling_msg)) {
    cerr << __func__ << ": Failed to parse trace file: " << trace_file << endl;
    return;
  }
  for (const auto& record : profiling_msg.records()) {
    string package = record.package();
    float version = record.version();
    string func_name = record.func_msg().name();
    auto found = find_if(trace_summaries->begin(), trace_summaries->end(),
                         [&](const TraceSummary& trace_summary) {
                           return (test_name == trace_summary.test_name &&
                                   package == trace_summary.package &&
                                   version == trace_summary.version);
                         });
    if (found != trace_summaries->end()) {
      found->total_api_count++;
      if (found->api_stats.find(func_name) != found->api_stats.end()) {
        found->api_stats[func_name]++;
      } else {
        found->unique_api_count++;
        found->api_stats[func_name] = 1;
      }
    } else {
      map<string, long> api_stats;
      api_stats[func_name] = 1;
      TraceSummary trace_summary(test_name, package, version, 1, 1, api_stats);
      trace_summaries->push_back(trace_summary);
    }
  }
}

}  // namespace vts
}  // namespace android
