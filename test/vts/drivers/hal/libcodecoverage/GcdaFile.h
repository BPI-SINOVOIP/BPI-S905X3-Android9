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

#ifndef __VTS_SYSFUZZER_LIBMEASUREMENT_GCDA_FILE_H__
#define __VTS_SYSFUZZER_LIBMEASUREMENT_GCDA_FILE_H__

#include <string>

#include "gcov_basic_io.h"

using namespace std;

namespace android {
namespace vts {

// Basic I/O methods for a GCOV file.
class GcdaFile {
 public:
  GcdaFile(const string& filename) :
    filename_(filename) {}
  virtual ~GcdaFile() {};

  // Opens a file.
  bool Open();

  // Closes a file and returns any existing error code.
  int Close();

  // Synchronizes to the given base and length.
  void Sync(unsigned base, unsigned length);

  // Reads a string array where the maximum number of strings is also specified.
  unsigned ReadStringArray(char** string_array, unsigned num_strings);

  // Reads a string.
  const char* ReadString();

  // Reads an unsigned integer.
  unsigned ReadUnsigned();

  // Reads 'words' number of words.
  const unsigned* ReadWords(unsigned words);

  // Reads a counter.
  gcov_type ReadCounter();

  // Write a block using 'size'.
  void WriteBlock(unsigned size);

  // Allocates memory for length.
  void Allocate(unsigned length);

  // Processes the magic tag.
  int Magic(unsigned magic, unsigned expected);

  // Returns the current position in the file.
  inline unsigned Position() const {
    return gcov_var_.start + gcov_var_.offset;
  }

  // Returns non-zero error code if there's an error.
  inline int IsError() const {
    return gcov_var_.file ? gcov_var_.error : 1;
  }

 protected:
  inline unsigned FromFile(unsigned value) {
    if (gcov_var_.endian) {
      value = (value >> 16) | (value << 16);
      value = ((value & 0xff00ff) << 8) | ((value >> 8) & 0xff00ff);
    }
    return value;
  }

 private:
  // The GCOV var data structure for an opened file.
  struct gcov_var_t gcov_var_;
  const string& filename_;
};

}  // namespace vts
}  // namespace android

#endif
