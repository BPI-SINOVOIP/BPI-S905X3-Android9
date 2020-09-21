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

#include "GcdaParser.h"

#include <iostream>
#include <vector>

#include "GcdaFile.h"

using namespace std;

namespace android {
namespace vts {

bool GcdaRawCoverageParser::ParseMagic() {
  unsigned magic = gcda_file_->ReadUnsigned();
  unsigned version;
  const char* type = NULL;
  int endianness = 0;
  char m[4], v[4];

  if ((endianness = gcda_file_->Magic(magic, GCOV_DATA_MAGIC))) {
    type = "data";
  } else {
    cout << __func__ << ": not a GCOV file, " << filename_ << endl;
    gcda_file_->Close();
    return false;
  }
  version = gcda_file_->ReadUnsigned();
  GCOV_UNSIGNED2STRING(v, version);
  GCOV_UNSIGNED2STRING(m, magic);
  if (version != GCOV_VERSION) {
    char e[4];
    GCOV_UNSIGNED2STRING(e, GCOV_VERSION);
  }
  return true;
}

void GcdaRawCoverageParser::ParseBody() {
  unsigned tags[4];
  unsigned depth = 0;
  bool found;
  unsigned base;
  unsigned position;
  unsigned tag;
  unsigned length;
  unsigned tag_depth;
  int error;
  unsigned mask;

  gcda_file_->ReadUnsigned();  // stamp
  while (1) {
    position = gcda_file_->Position();

    tag = gcda_file_->ReadUnsigned();
    if (!tag) break;

    length = gcda_file_->ReadUnsigned();
    base = gcda_file_->Position();
    mask = GCOV_TAG_MASK(tag) >> 1;
    for (tag_depth = 4; mask; mask >>= 8) {
      if ((mask & 0xff) != 0xff) {
        cerr << __func__ << ": invalid tag, " << tag << endl;
        break;
      }
      tag_depth--;
    }
    found = false;

    if (tag) {
      if (depth && depth < tag_depth &&
          !GCOV_TAG_IS_SUBTAG(tags[depth - 1], tag)) {
        cerr << __func__ << ": incorrectly nested tag, " << tag << endl;
      }
      depth = tag_depth;
      tags[depth - 1] = tag;
    }

    switch(tag) {
      case GCOV_TAG_FUNCTION:
        TagFunction(tag, length);
        break;
      case GCOV_TAG_BLOCKS:
        TagBlocks(tag, length);
        break;
      case GCOV_TAG_ARCS:
        TagArcs(tag, length);
        break;
      case GCOV_TAG_LINES:
        TagLines(tag, length);
        break;
    }
    gcda_file_->Sync(base, length);

    if ((error = gcda_file_->IsError())) {
      cerr << __func__ << ": I/O error at "
           << gcda_file_->Position() << endl;
      break;
    }
  }
}

vector<unsigned> GcdaRawCoverageParser::Parse() {
  result.clear();
  if (!gcda_file_->Open()) {
    cerr << __func__ << " Cannot open a file, " << filename_ << endl;
    return result;
  }

  if (!ParseMagic()) return result;
  ParseBody();
  gcda_file_->Close();
  return result;
}

}  // namespace vts
}  // namespace android
