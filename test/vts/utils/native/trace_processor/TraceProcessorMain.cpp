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
#include <getopt.h>

#include "VtsCoverageProcessor.h"
#include "VtsTraceProcessor.h"

static constexpr const char* kDefaultMode = "noop";
static constexpr const char* kDefaultOutputFile =
    "/temp/vts_trace_processor_output";

using namespace std;

enum mode_code {
  // Trace related operations.
  CLEANUP_TRACE,
  CONVERT_TRACE,
  DEDUPE_TRACE,
  GET_TEST_LIST_FROM_TRACE,
  PARSE_TRACE,
  PROFILING_TRACE,
  SELECT_TRACE,
  // Coverage related operations.
  COMPARE_COVERAGE,
  GET_COVERGAGE_SUMMARY,
  GET_SUBSET_COVERAGE,
  MERGE_COVERAGE,
};

mode_code getModeCode(const std::string& str) {
  if (str == "cleanup_trace") return mode_code::CLEANUP_TRACE;
  if (str == "convert_trace") return mode_code::CONVERT_TRACE;
  if (str == "dedup_trace") return mode_code::DEDUPE_TRACE;
  if (str == "get_test_list_from_trace")
    return mode_code::GET_TEST_LIST_FROM_TRACE;
  if (str == "parse_trace") return mode_code::PARSE_TRACE;
  if (str == "profiling_trace") return mode_code::PROFILING_TRACE;
  if (str == "select_trace") return mode_code::SELECT_TRACE;
  if (str == "compare_coverage") return mode_code::COMPARE_COVERAGE;
  if (str == "get_coverage_summary") return mode_code::GET_COVERGAGE_SUMMARY;
  if (str == "get_subset_coverage") return mode_code::GET_SUBSET_COVERAGE;
  if (str == "merge_coverage") return mode_code::MERGE_COVERAGE;
  printf("Unknown operation mode: %s\n", str.c_str());
  exit(-1);
}

void ShowUsage() {
  printf(
      "Usage:   trace_processor [options] <input>\n"
      "--mode:  The operation applied to the trace file.\n"
      "\t cleanup_trace: cleanup trace for replay (remove duplicate events "
      "etc.).\n"
      "\t convert_trace: convert a text format trace file into a binary format "
      "trace.\n"
      "\t dedup_trace: remove duplicate trace file in the given directory. A "
      "trace is considered duplicated if there exists a trace that contains "
      "the "
      "same API call sequence as the given trace and the input parameters for "
      "each API call are all the same.\n"
      "\t get_test_list_from_trace: parse all trace files under the given "
      "directory and create a list of test modules for each hal@version that "
      "access all apis covered by the whole test set. (i.e. such list should "
      "be a subset of the whole test list that access the corresponding "
      "hal@version)\n"
      "\t parse_trace: parse the binary format trace file and print the text "
      "format trace. \n"
      "\t profiling_trace: parse the trace file to get the latency of each api "
      "call.\n"
      "\t select_trace: select a subset of trace files from a give trace set "
      "based on their corresponding coverage data, the goal is to pick up the "
      "minimal num of trace files that to maximize the total coverage.\n"
      "\t compare_coverage: compare a coverage report with a reference "
      "coverage report and print the additional file/lines covered.\n"
      "\t get_coverage_summary: print the summary of the coverage file (e.g. "
      "covered lines, total lines, coverage rate.) \n"
      "\t get_subset_coverage: extract coverage measurement from coverage "
      "report for files covered in the reference coverage report. It is used "
      "in cases when we have an aggregated coverage report for all files but "
      "are only interested in the coverage measurement of a subset of files in "
      "that report.\n"
      "\t merge_coverage: merge all coverage reports under the given directory "
      "and generate a merged report.\n"
      "--output: The file path to store the output results.\n"
      "--help:   Show help\n");
  exit(-1);
}

int main(int argc, char** argv) {
  string mode = kDefaultMode;
  string output = kDefaultOutputFile;
  bool verbose_output = false;

  android::vts::VtsCoverageProcessor coverage_processor;
  android::vts::VtsTraceProcessor trace_processor(&coverage_processor);

  const char* const short_opts = "hm:o:v:";
  const option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"mode", required_argument, nullptr, 'm'},
      {"output", required_argument, nullptr, 'o'},
      {"verbose", no_argument, nullptr, 'v'},
      {nullptr, 0, nullptr, 0},
  };

  while (true) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (opt == -1) {
      break;
    }
    switch (opt) {
      case 'h':
      case '?':
        ShowUsage();
        return 0;
      case 'm': {
        mode = string(optarg);
        break;
      }
      case 'o': {
        output = string(optarg);
        break;
      }
      case 'v': {
        verbose_output = true;
        break;
      }
      default:
        printf("getopt_long returned unexpected value: %d\n", opt);
        return -1;
    }
  }

  if (optind == argc - 1) {
    string trace_path = argv[optind];
    switch (getModeCode(mode)) {
      case mode_code::CLEANUP_TRACE:
        trace_processor.CleanupTraces(trace_path);
        break;
      case mode_code::CONVERT_TRACE:
        trace_processor.ConvertTrace(trace_path);
        break;
      case mode_code::DEDUPE_TRACE:
        trace_processor.DedupTraces(trace_path);
        break;
      case mode_code::GET_TEST_LIST_FROM_TRACE:
        trace_processor.GetTestListForHal(trace_path, output, verbose_output);
        break;
      case mode_code::PARSE_TRACE:
        trace_processor.ParseTrace(trace_path);
        break;
      case mode_code::PROFILING_TRACE:
        trace_processor.ProcessTraceForLatencyProfiling(trace_path);
        break;
      case mode_code::GET_COVERGAGE_SUMMARY:
        coverage_processor.GetCoverageSummary(trace_path);
        break;
      case mode_code::MERGE_COVERAGE:
        coverage_processor.MergeCoverage(trace_path, output);
        break;
      default:
        printf("Invalid argument.");
        return -1;
    }
  } else if (optind == argc - 2) {
    switch (getModeCode(mode)) {
      case mode_code::SELECT_TRACE: {
        string coverage_dir = argv[optind];
        string trace_dir = argv[optind + 1];
        trace_processor.SelectTraces(coverage_dir, trace_dir);
        break;
      }
      case mode_code::COMPARE_COVERAGE: {
        string ref_coverage_path = argv[optind];
        string coverage_path = argv[optind + 1];
        coverage_processor.CompareCoverage(ref_coverage_path, coverage_path);
        break;
      }
      case mode_code::GET_SUBSET_COVERAGE: {
        string ref_coverage_path = argv[optind];
        string coverage_path = argv[optind + 1];
        coverage_processor.GetSubsetCoverage(ref_coverage_path, coverage_path,
                                             output);
        break;
      }
      default:
        printf("Invalid argument.");
        return -1;
    }
  }
  return 0;
}
