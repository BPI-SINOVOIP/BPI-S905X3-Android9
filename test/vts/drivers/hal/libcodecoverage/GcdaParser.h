/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef __VTS_SYSFUZZER_LIBMEASUREMENT_GCDA_PARSER_H__
#define __VTS_SYSFUZZER_LIBMEASUREMENT_GCDA_PARSER_H__

#include <iostream>
#include <string>
#include <vector>

#include "GcdaFile.h"

using namespace std;

namespace android {
namespace vts {

// Parses a GCDA file and extracts raw coverage info.
class GcdaRawCoverageParser {
 public:
  GcdaRawCoverageParser(const char* filename)
    : filename_(filename),
      gcda_file_(new GcdaFile(filename_)) {}

  virtual ~GcdaRawCoverageParser() {}

  // Parses a given file and returns a vector which contains IDs of raw
  // coverage measurement units (e.g., basic blocks).
  vector<unsigned> Parse();

 protected:
  // Parses the GCOV magic number.
  bool ParseMagic();

  // Parses the GCOV file body.
  void ParseBody();

  // Processes tag for functions.
  void TagFunction(unsigned /*tag*/, unsigned length) {
    /* unsigned long pos = */ gcda_file_->Position();

    if (length) {
      gcda_file_->ReadUnsigned();  // ident %u
      unsigned lineno_checksum = gcda_file_->ReadUnsigned();
      result.push_back(lineno_checksum);
      gcda_file_->ReadUnsigned();  // cfg_checksum 0x%08x
    }
  }

  // Processes tag for blocks.
  void TagBlocks(unsigned /*tag*/, unsigned length) {
    unsigned n_blocks = GCOV_TAG_BLOCKS_NUM(length);
    cout << __func__ << ": " << n_blocks << " blocks" << endl;
  }

  // Processes tag for arcs.
  void TagArcs(unsigned /*tag*/, unsigned length) {
    unsigned n_arcs = GCOV_TAG_ARCS_NUM(length);
    cout << __func__ << ": " << n_arcs << " arcs" << endl;
  }

  // Processes tag for lines.
  void TagLines(unsigned /*tag*/, unsigned /*length*/) {
    cout << __func__ << endl;
  }

 private:
  // the name of a target file.
  const string filename_;

  // global GcovFile data structure.
  GcdaFile* gcda_file_;

  // vector containing the parsed, raw coverage data.
  vector<unsigned> result;
};

}  // namespace vts
}  // namespace android

#endif
